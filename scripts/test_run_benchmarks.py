import csv
import datetime
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import run_benchmarks
from run_benchmarks import (
    LIMITATIONS,
    RESULT_COLUMNS,
    build_args,
    check_result,
    collect_runs,
    command_output,
    cpu_model,
    environment_info,
    format_cell,
    main,
    markdown_table,
    median_or_none,
    parse_args,
    parse_line,
    parse_value,
    render_markdown,
    run_matrix,
    summarize_runs,
    timestamp_dir_name,
    write_csv,
)
from run_benchmarks import save_results as run_benchmarks_save

FIELD_NAMES = [
    "mode", "build", "workers", "devices", "target_rate", "duration_ms",
    "sent", "processed", "throughput_per_s", "max_lag", "queue_avg", "queue_max",
    "total_p50_us", "total_p95_us", "total_p99_us", "total_max_us", "total_overflow",
    "service_p50_us", "service_p95_us", "service_p99_us", "service_max_us", "service_overflow",
]


def make_line(build="release", **overrides):
    values = {name: "1" for name in FIELD_NAMES}
    values["mode"] = "max"
    values["build"] = build
    values.update(overrides)
    return " ".join(f"{name}={values[name]}" for name in FIELD_NAMES)


def fake(returncode=0, stdout="", stderr=""):
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


class ParseValueTests(unittest.TestCase):
    def test_a_number_becomes_an_int(self):
        self.assertEqual(parse_value("41058"), 41058)
        self.assertIsInstance(parse_value("41058"), int)

    def test_zero_is_a_number_not_a_missing_value(self):
        self.assertEqual(parse_value("0"), 0)
        self.assertIsNotNone(parse_value("0"))

    def test_text_stays_text(self):
        self.assertEqual(parse_value("debug"), "debug")

    def test_NA_becomes_None(self):
        self.assertIsNone(parse_value("NA"))

    def test_a_negative_number_is_parsed(self):
        self.assertEqual(parse_value("-3"), -3)

    def test_an_empty_string_stays_a_string(self):
        self.assertEqual(parse_value(""), "")


class ParseLineTests(unittest.TestCase):
    def test_numbers_text_and_NA_in_one_line(self):
        row = parse_line("workers=2 build=debug total_p99_us=NA")
        self.assertEqual(row, {"workers": 2, "build": "debug", "total_p99_us": None})

    def test_a_trailing_newline_is_ignored(self):
        self.assertEqual(parse_line("workers=2\n"), {"workers": 2})

    def test_only_the_first_equals_sign_splits(self):
        self.assertEqual(parse_line("path=a=b"), {"path": "a=b"})

    def test_field_order_does_not_matter_to_the_reader(self):
        self.assertEqual(parse_line("a=1 b=2"), parse_line("b=2 a=1"))


class CheckResultTests(unittest.TestCase):
    def test_a_good_release_result_is_accepted(self):
        row = check_result(fake(stdout=make_line() + "\n"))
        self.assertEqual(len(row), 22)
        self.assertEqual(row["build"], "release")

    def test_a_debug_result_is_refused(self):
        with self.assertRaisesRegex(ValueError, "non-release.*debug"):
            check_result(fake(stdout=make_line(build="debug") + "\n"))

    def test_an_unknown_build_is_refused(self):
        with self.assertRaisesRegex(ValueError, "non-release.*unknown"):
            check_result(fake(stdout=make_line(build="unknown") + "\n"))

    def test_a_failing_exit_code_is_refused_and_the_message_names_the_code_and_stderr(self):
        with self.assertRaisesRegex(ValueError, "exit code 2.*Usage: benchmark"):
            check_result(fake(returncode=2, stderr="Usage: benchmark ...\n"))

    def test_a_failing_exit_code_is_refused_even_when_stdout_looks_fine(self):
        with self.assertRaisesRegex(ValueError, "exit code 1"):
            check_result(fake(returncode=1, stdout=make_line() + "\n"))

    def test_no_output_is_refused(self):
        with self.assertRaisesRegex(ValueError, "exactly one output line, got 0"):
            check_result(fake(stdout=""))

    def test_two_lines_are_refused(self):
        with self.assertRaisesRegex(ValueError, "exactly one output line, got 2"):
            check_result(fake(stdout=make_line() + "\n" + make_line() + "\n"))

    def test_too_few_fields_are_refused(self):
        line = make_line().rsplit(" ", 1)[0]
        with self.assertRaisesRegex(ValueError, "expected 22 fields, got 21"):
            check_result(fake(stdout=line + "\n"))

    def test_too_many_fields_are_refused(self):
        with self.assertRaisesRegex(ValueError, "expected 22 fields, got 23"):
            check_result(fake(stdout=make_line() + " extra=1\n"))

    def test_NA_in_a_field_is_kept_as_None_not_refused(self):
        row = check_result(fake(stdout=make_line(total_p99_us="NA") + "\n"))
        self.assertIsNone(row["total_p99_us"])

    def test_the_expected_field_count_matches_the_contract(self):
        self.assertEqual(run_benchmarks.EXPECTED_FIELDS, len(FIELD_NAMES))


class MedianTests(unittest.TestCase):
    def test_the_middle_value_after_sorting(self):
        self.assertEqual(median_or_none([100, 104, 70]), 100)

    def test_one_polluted_run_does_not_drag_the_result(self):
        self.assertEqual(median_or_none([100, 104, 7]), 100)

    def test_a_single_value(self):
        self.assertEqual(median_or_none([42]), 42)

    def test_one_missing_value_means_no_median(self):
        self.assertIsNone(median_or_none([400, None, 450]))

    def test_all_missing_means_no_median(self):
        self.assertIsNone(median_or_none([None, None, None]))

    def test_an_empty_list_means_no_median(self):
        self.assertIsNone(median_or_none([]))

    def test_an_even_count_averages_the_two_middle_values(self):
        self.assertEqual(median_or_none([1, 2, 3, 4]), 2.5)

    def test_zero_is_a_real_value(self):
        self.assertEqual(median_or_none([0, 0, 5]), 0)


def good_run(**overrides):
    return fake(stdout=make_line(**overrides) + "\n")


class CollectRunsTests(unittest.TestCase):
    def test_returns_one_parsed_row_per_run(self):
        with mock.patch("run_benchmarks.run_once", side_effect=[good_run(), good_run(), good_run()]):
            rows = collect_runs("bin", ["--workers", "4"], 3)

        self.assertEqual(len(rows), 3)
        self.assertEqual(rows[0]["mode"], "max")
        self.assertEqual(len(rows[0]), 22)

    def test_runs_exactly_the_requested_number_of_times(self):
        with mock.patch("run_benchmarks.run_once", return_value=good_run()) as run_once:
            collect_runs("bin", [], 5)

        self.assertEqual(run_once.call_count, 5)

    def test_passes_the_binary_and_the_arguments_on(self):
        with mock.patch("run_benchmarks.run_once", return_value=good_run()) as run_once:
            collect_runs("/path/benchmark", ["--workers", "4"], 1)

        run_once.assert_called_once_with("/path/benchmark", ["--workers", "4"])

    def test_a_failing_run_stops_everything_and_returns_no_partial_list(self):
        runs = [good_run(), fake(returncode=1, stderr="crashed"), good_run()]
        with mock.patch("run_benchmarks.run_once", side_effect=runs) as run_once:
            with self.assertRaisesRegex(ValueError, "exit code 1.*crashed"):
                collect_runs("bin", [], 3)

        self.assertEqual(run_once.call_count, 2)

    def test_a_debug_run_in_the_middle_is_refused(self):
        runs = [good_run(), good_run(build="debug"), good_run()]
        with mock.patch("run_benchmarks.run_once", side_effect=runs):
            with self.assertRaisesRegex(ValueError, "non-release"):
                collect_runs("bin", [], 3)


def make_row(**overrides):
    return parse_line(make_line(**overrides))


class SummarizeRunsTests(unittest.TestCase):
    def test_a_measured_field_becomes_the_median(self):
        rows = [make_row(throughput_per_s="70"), make_row(throughput_per_s="104"), make_row(throughput_per_s="100")]

        self.assertEqual(summarize_runs(rows)["throughput_per_s"], 100)

    def test_configuration_fields_are_copied_not_computed(self):
        rows = [make_row(workers="4"), make_row(workers="4"), make_row(workers="4")]
        summary = summarize_runs(rows)

        self.assertEqual(summary["mode"], "max")
        self.assertEqual(summary["build"], "release")
        self.assertEqual(summary["workers"], 4)

    def test_differing_configuration_is_refused_and_names_the_field(self):
        rows = [make_row(workers="4"), make_row(workers="4"), make_row(workers="8")]

        with self.assertRaisesRegex(ValueError, "workers.*differs"):
            summarize_runs(rows)

    def test_mixed_builds_are_refused(self):
        rows = [make_row(), make_row(build="debug"), make_row()]

        with self.assertRaisesRegex(ValueError, "build.*differs"):
            summarize_runs(rows)

    def test_the_range_of_the_throughput_is_reported(self):
        rows = [make_row(throughput_per_s="100"), make_row(throughput_per_s="104"), make_row(throughput_per_s="70")]
        summary = summarize_runs(rows)

        self.assertEqual(summary["throughput_min"], 70)
        self.assertEqual(summary["throughput_max"], 104)

    def test_the_number_of_runs_is_recorded(self):
        self.assertEqual(summarize_runs([make_row(), make_row()])["runs"], 2)
        self.assertEqual(summarize_runs([make_row(), make_row(), make_row(), make_row(), make_row()])["runs"], 5)

    def test_a_missing_value_in_one_run_gives_no_median(self):
        rows = [make_row(total_p99_us="400"), make_row(total_p99_us="NA"), make_row(total_p99_us="450")]

        self.assertIsNone(summarize_runs(rows)["total_p99_us"])

    def test_one_polluted_run_does_not_drag_the_summary(self):
        rows = [make_row(throughput_per_s="7"), make_row(throughput_per_s="104"), make_row(throughput_per_s="100")]

        self.assertEqual(summarize_runs(rows)["throughput_per_s"], 100)

    def test_the_summary_has_the_original_fields_plus_three(self):
        summary = summarize_runs([make_row(), make_row(), make_row()])

        self.assertEqual(len(summary), 25)
        for extra in ("runs", "throughput_min", "throughput_max"):
            self.assertIn(extra, summary)


class ParseArgsTests(unittest.TestCase):
    def test_no_arguments_give_the_defaults(self):
        options = parse_args([])

        self.assertEqual(options.mode, "max")
        self.assertEqual(options.workers, [1, 2, 4, 6])
        self.assertEqual(options.devices, 10)
        self.assertIsNone(options.rate)
        self.assertEqual(options.duration_ms, 5000)
        self.assertEqual(options.warmup_ms, 1000)
        self.assertEqual(options.repeats, 3)
        self.assertTrue(str(options.binary).endswith("cmake-build-release-wsl/src/apps/benchmark"))

    def test_every_option_is_parsed(self):
        options = parse_args(["--binary", "/x/bench", "--mode", "rate", "--workers", "3", "5",
                              "--devices", "20", "--rate", "2000", "--duration-ms", "700",
                              "--warmup-ms", "0", "--repeats", "5"])

        self.assertEqual(options.binary, "/x/bench")
        self.assertEqual(options.mode, "rate")
        self.assertEqual(options.workers, [3, 5])
        self.assertEqual(options.devices, 20)
        self.assertEqual(options.rate, 2000)
        self.assertEqual(options.duration_ms, 700)
        self.assertEqual(options.warmup_ms, 0)
        self.assertEqual(options.repeats, 5)

    def refused(self, argv):
        with mock.patch("sys.stderr"):
            with self.assertRaises(SystemExit) as context:
                parse_args(argv)
        return context.exception.code

    def test_zero_repeats_are_refused_with_exit_code_two(self):
        self.assertEqual(parse_args(["--repeats", "1"]).repeats, 1)
        self.assertEqual(self.refused(["--repeats", "0"]), 2)

    def test_an_unknown_mode_is_refused(self):
        self.assertEqual(parse_args(["--mode", "max"]).mode, "max")
        self.assertEqual(self.refused(["--mode", "fast"]), 2)

    def test_a_non_number_is_refused(self):
        self.assertEqual(parse_args(["--workers", "4"]).workers, [4])
        self.assertEqual(self.refused(["--workers", "abc"]), 2)

    def test_an_unknown_option_is_refused(self):
        self.assertEqual(parse_args(["--devices", "5"]).devices, 5)
        self.assertEqual(self.refused(["--bogus", "1"]), 2)


class BuildArgsTests(unittest.TestCase):
    def options(self, *argv):
        return parse_args(list(argv))

    def test_the_arguments_are_strings_in_the_order_the_benchmark_expects(self):
        args = build_args(self.options("--devices", "10", "--duration-ms", "5000", "--warmup-ms", "1000"), 4)

        self.assertEqual(args, ["--mode", "max", "--workers", "4", "--devices", "10",
                                "--duration-ms", "5000", "--warmup-ms", "1000"])
        for item in args:
            self.assertIsInstance(item, str)

    def test_the_rate_is_added_only_when_it_was_given(self):
        without = build_args(self.options(), 2)
        with_rate = build_args(self.options("--mode", "rate", "--rate", "2000"), 2)

        self.assertNotIn("--rate", without)
        self.assertEqual(with_rate[-2:], ["--rate", "2000"])

    def test_the_worker_count_comes_from_the_argument_not_the_options(self):
        options = self.options("--workers", "1", "2", "4")

        self.assertEqual(build_args(options, 6)[3], "6")

    def test_the_mode_is_passed_on(self):
        self.assertEqual(build_args(self.options("--mode", "calibrate"), 1)[:2], ["--mode", "calibrate"])


class RunMatrixTests(unittest.TestCase):
    def test_one_summary_per_worker_count_and_all_raw_rows_kept(self):
        options = parse_args(["--workers", "1", "2", "4", "--repeats", "3"])
        outputs = [good_run(workers=str(w), throughput_per_s=str(100 + i))
                   for w in (1, 2, 4) for i in range(3)]

        with mock.patch("run_benchmarks.run_once", side_effect=outputs):
            raw_rows, summaries = run_matrix(options)

        self.assertEqual(len(raw_rows), 9)
        self.assertEqual([s["workers"] for s in summaries], [1, 2, 4])
        self.assertTrue(all(s["runs"] == 3 for s in summaries))

    def test_each_point_is_run_with_its_own_worker_count(self):
        options = parse_args(["--workers", "2", "6", "--repeats", "1"])

        with mock.patch("run_benchmarks.run_once", side_effect=[good_run(workers="2"), good_run(workers="6")]) as run_once:
            run_matrix(options)

        calls = [call.args[1] for call in run_once.call_args_list]
        self.assertEqual([args[3] for args in calls], ["2", "6"])

    def test_a_failure_in_a_later_point_stops_everything(self):
        options = parse_args(["--workers", "1", "2", "--repeats", "1"])

        with mock.patch("run_benchmarks.run_once", side_effect=[good_run(workers="1"), fake(returncode=1, stderr="boom")]):
            with self.assertRaisesRegex(ValueError, "exit code 1"):
                run_matrix(options)


class WriteCsvTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.path = Path(self.directory.name) / "results.csv"

    def tearDown(self):
        self.directory.cleanup()

    def read_lines(self):
        return self.path.read_text(encoding="utf-8").splitlines()

    def read_rows(self):
        with open(self.path, newline="", encoding="utf-8") as handle:
            return list(csv.DictReader(handle))

    def test_the_header_is_the_field_names_in_the_contract_order(self):
        write_csv(self.path, [make_row()])

        self.assertEqual(self.read_lines()[0].split(","), FIELD_NAMES)

    def test_one_data_row_per_run_and_no_blank_lines(self):
        write_csv(self.path, [make_row(), make_row(), make_row()])

        lines = self.read_lines()
        self.assertEqual(len(lines), 4)
        self.assertTrue(all(line.strip() for line in lines))

    def test_a_missing_value_is_written_as_NA(self):
        write_csv(self.path, [make_row(total_p99_us="NA")])

        self.assertEqual(self.read_rows()[0]["total_p99_us"], "NA")

    def test_zero_is_written_as_0_not_as_NA(self):
        write_csv(self.path, [make_row(max_lag="0")])

        self.assertEqual(self.read_rows()[0]["max_lag"], "0")

    def test_reading_the_file_back_gives_the_original_values(self):
        originals = [make_row(throughput_per_s="100", total_p99_us="NA"),
                     make_row(throughput_per_s="104", total_p99_us="450")]
        write_csv(self.path, originals)

        read_back = [{key: parse_value(value) for key, value in row.items()} for row in self.read_rows()]
        self.assertEqual(read_back, originals)

    def test_writing_twice_replaces_the_file(self):
        write_csv(self.path, [make_row(), make_row()])
        write_csv(self.path, [make_row()])

        self.assertEqual(len(self.read_lines()), 2)

    def test_a_plain_string_path_works_too(self):
        write_csv(str(self.path), [make_row()])

        self.assertEqual(len(self.read_lines()), 2)


class CommandOutputTests(unittest.TestCase):
    def test_returns_the_first_line_of_the_output(self):
        with mock.patch("run_benchmarks.subprocess.run", return_value=fake(stdout="g++ 15.2.0\nCopyright\nmore\n")):
            self.assertEqual(command_output(["g++", "--version"]), "g++ 15.2.0")

    def test_a_missing_program_gives_unknown(self):
        with mock.patch("run_benchmarks.subprocess.run", side_effect=FileNotFoundError(2, "no such program")):
            self.assertEqual(command_output(["nope"]), "unknown")

    def test_a_failing_program_gives_unknown(self):
        with mock.patch("run_benchmarks.subprocess.run", return_value=fake(returncode=1, stdout="something\n")):
            self.assertEqual(command_output(["x"]), "unknown")

    def test_an_empty_output_gives_unknown(self):
        with mock.patch("run_benchmarks.subprocess.run", return_value=fake(stdout="")):
            self.assertEqual(command_output(["x"]), "unknown")

    def test_the_command_is_passed_on_unchanged(self):
        with mock.patch("run_benchmarks.subprocess.run", return_value=fake(stdout="v1\n")) as run:
            command_output(["cmake", "--version"])

        self.assertEqual(run.call_args.args[0], ["cmake", "--version"])


CPUINFO = ("processor\t: 0\nvendor_id\t: Fake\nmodel name\t: Fake CPU 9000 6-Core\ncpu MHz\t\t: 3600.0\n"
           "processor\t: 1\nmodel name\t: Second Core Name\n")


class CpuModelTests(unittest.TestCase):
    def test_returns_the_model_name_of_the_first_core(self):
        with mock.patch("builtins.open", mock.mock_open(read_data=CPUINFO)):
            self.assertEqual(cpu_model(), "Fake CPU 9000 6-Core")

    def test_a_missing_file_gives_unknown(self):
        with mock.patch("builtins.open", side_effect=FileNotFoundError(2, "no file")):
            self.assertEqual(cpu_model(), "unknown")

    def test_a_file_without_a_model_name_gives_unknown(self):
        with mock.patch("builtins.open", mock.mock_open(read_data="processor\t: 0\nvendor_id\t: Fake\n")):
            self.assertEqual(cpu_model(), "unknown")


class EnvironmentInfoTests(unittest.TestCase):
    def info(self, binary="/x/bench", compiler="g++ 15", cmake="cmake 4"):
        def fake_command_output(command):
            return compiler if command[0] == "g++" else cmake

        with mock.patch("run_benchmarks.command_output", side_effect=fake_command_output), \
                mock.patch("run_benchmarks.cpu_model", return_value="Fake CPU"):
            return environment_info(binary)

    def test_has_exactly_the_documented_fields(self):
        self.assertEqual(set(self.info()), {"date", "cpu", "logical_cores", "os", "python",
                                            "compiler", "cmake", "binary"})

    def test_the_date_is_iso_to_the_second(self):
        self.assertRegex(self.info()["date"], r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}$")

    def test_the_cpu_and_the_tools_come_from_the_helpers(self):
        info = self.info(compiler="g++ 15.2.0", cmake="cmake 4.2.3")

        self.assertEqual(info["cpu"], "Fake CPU")
        self.assertEqual(info["compiler"], "g++ 15.2.0")
        self.assertEqual(info["cmake"], "cmake 4.2.3")

    def test_the_binary_is_recorded_as_text(self):
        self.assertEqual(self.info(binary=Path("/a/b/benchmark"))["binary"], "/a/b/benchmark")

    def test_the_core_count_is_recorded(self):
        with mock.patch("run_benchmarks.os.cpu_count", return_value=6):
            self.assertEqual(self.info()["logical_cores"], 6)

    def test_an_unknown_core_count_is_written_as_unknown(self):
        with mock.patch("run_benchmarks.os.cpu_count", return_value=None):
            self.assertEqual(self.info()["logical_cores"], "unknown")

    def test_the_os_has_the_system_and_the_release(self):
        with mock.patch("run_benchmarks.platform.system", return_value="Linux"), \
                mock.patch("run_benchmarks.platform.release", return_value="6.1-test"):
            self.assertEqual(self.info()["os"], "Linux 6.1-test")

    def test_missing_tools_do_not_stop_it(self):
        info = self.info(compiler="unknown", cmake="unknown")

        self.assertEqual(info["compiler"], "unknown")
        self.assertEqual(info["cmake"], "unknown")


class TimestampDirNameTests(unittest.TestCase):
    def test_the_format_is_year_month_day_T_hour_minute_second(self):
        moment = datetime.datetime(2026, 10, 16, 14, 30, 5)

        self.assertEqual(timestamp_dir_name(moment), "2026-10-16T14-30-05")

    def test_single_digits_are_padded_with_zeros(self):
        self.assertEqual(timestamp_dir_name(datetime.datetime(2026, 1, 2, 3, 4, 5)), "2026-01-02T03-04-05")

    def test_sorting_the_names_sorts_chronologically(self):
        moments = [datetime.datetime(2025, 12, 31, 23, 59, 59),
                   datetime.datetime(2026, 1, 1, 0, 0, 0),
                   datetime.datetime(2026, 9, 30, 12, 0, 0),
                   datetime.datetime(2026, 10, 1, 8, 0, 0),
                   datetime.datetime(2026, 10, 1, 8, 0, 1),
                   datetime.datetime(2026, 10, 16, 14, 30, 5)]
        names = [timestamp_dir_name(moment) for moment in moments]

        self.assertEqual(sorted(names), names)
        self.assertEqual(sorted(reversed(names)), names)

    def test_the_name_is_safe_for_file_systems(self):
        name = timestamp_dir_name(datetime.datetime(2026, 10, 16, 14, 30, 5))

        for forbidden in (":", " ", "/", "\\"):
            self.assertNotIn(forbidden, name)


class OutputDirOptionTests(unittest.TestCase):
    def test_the_default_is_benchmarks_results_inside_the_project(self):
        self.assertTrue(str(parse_args([]).output_dir).endswith("benchmarks/results"))

    def test_an_explicit_directory_is_used(self):
        self.assertEqual(parse_args(["--output-dir", "/tmp/smoke"]).output_dir, "/tmp/smoke")


class FormatCellTests(unittest.TestCase):
    def test_a_missing_value_is_NA(self):
        self.assertEqual(format_cell(None), "NA")

    def test_zero_is_a_number_not_NA(self):
        self.assertEqual(format_cell(0), "0")

    def test_numbers_and_text_become_text(self):
        self.assertEqual(format_cell(12), "12")
        self.assertEqual(format_cell("debug"), "debug")


class MarkdownTableTests(unittest.TestCase):
    def test_the_exact_table(self):
        table = markdown_table(["workers", "throughput"], [[1, 100], [2, 190]])

        self.assertEqual(table, "| workers | throughput |\n|---|---|\n| 1 | 100 |\n| 2 | 190 |")

    def test_a_missing_cell_shows_NA(self):
        table = markdown_table(["a", "b"], [[1, None]])

        self.assertEqual(table.splitlines()[2], "| 1 | NA |")

    def test_no_rows_gives_only_the_header_and_the_separator(self):
        self.assertEqual(markdown_table(["a", "b", "c"], []).splitlines(), ["| a | b | c |", "|---|---|---|"])

    def test_one_separator_cell_per_header(self):
        separator = markdown_table(["a", "b", "c", "d"], []).splitlines()[1]

        self.assertEqual(separator.count("---"), 4)

    def test_there_is_one_line_per_row_plus_two(self):
        table = markdown_table(["a"], [[1], [2], [3]])

        self.assertEqual(len(table.splitlines()), 5)


SUMMARY_2 = {"workers": 2, "throughput_per_s": 200, "throughput_min": 190, "throughput_max": 210,
             "total_p50_us": 11, "total_p95_us": 12, "total_p99_us": 13, "total_max_us": 14,
             "service_p50_us": 21, "service_p99_us": 22, "queue_avg": 31, "queue_max": 32, "max_lag": 41}
SUMMARY_4 = dict(SUMMARY_2, workers=4, throughput_per_s=380, total_p99_us=None)
ENV = {"date": "2026-10-16T14:30:05", "cpu": "Fake CPU", "logical_cores": 6, "os": "Linux 6.1",
       "python": "3.14", "compiler": "g++ 15", "cmake": "cmake 4", "binary": "/x/bench"}


def report(argv=None, summaries=None):
    options = parse_args(argv or ["--mode", "max", "--devices", "10", "--duration-ms", "5000",
                                  "--warmup-ms", "1000", "--repeats", "3"])
    return render_markdown(summaries or [SUMMARY_2, SUMMARY_4], ENV, options)


class RenderMarkdownTests(unittest.TestCase):
    def test_the_sections_appear_in_order(self):
        text = report()
        positions = [text.index(heading) for heading in
                     ("# Benchmark results", "## Experiment", "## Environment", "## Results", "## Limitations")]

        self.assertEqual(positions, sorted(positions))

    def test_the_date_is_shown(self):
        self.assertIn("Generated: 2026-10-16T14:30:05", report())

    def test_the_experiment_table_has_the_settings(self):
        text = report()

        for row in ("| mode | max |", "| devices | 10 |", "| duration per run (ms) | 5000 |",
                    "| warm-up per point (ms) | 1000 |", "| runs per point | 3 |"):
            self.assertIn(row, text)

    def test_a_missing_target_rate_is_NA(self):
        self.assertIn("| target rate | NA |", report())

    def test_a_given_target_rate_is_shown(self):
        text = report(["--mode", "rate", "--rate", "2000", "--repeats", "3"])

        self.assertIn("| target rate | 2000 |", text)

    def test_every_environment_item_is_listed(self):
        text = report()

        for key, value in ENV.items():
            self.assertIn(f"| {key} | {value} |", text)

    def test_the_results_row_has_every_column_in_order(self):
        text = report()

        self.assertIn("| 2 | 200 | 190 | 210 | 11 | 12 | 13 | 14 | 21 | 22 | 31 | 32 | 41 |", text)

    def test_a_missing_percentile_is_NA_in_the_results(self):
        text = report()

        self.assertIn("| 4 | 380 | 190 | 210 | 11 | 12 | NA | 14 | 21 | 22 | 31 | 32 | 41 |", text)

    def test_the_results_header_uses_the_column_titles(self):
        header = [line for line in report().splitlines() if line.startswith("| workers | throughput/s")][0]

        self.assertEqual(header, "| " + " | ".join(title for _, title in RESULT_COLUMNS) + " |")

    def test_one_results_row_per_summary(self):
        text = report(summaries=[SUMMARY_2, SUMMARY_4, dict(SUMMARY_2, workers=6)])

        for workers in (2, 4, 6):
            self.assertEqual(len([line for line in text.splitlines() if line.startswith(f"| {workers} | ")]), 1)

    def test_the_number_of_runs_is_in_the_results_title(self):
        text = report(["--repeats", "5"])

        self.assertIn("## Results (median of 5 runs per point", text)

    def test_every_limitation_is_listed_as_a_bullet(self):
        text = report()

        for limitation in LIMITATIONS:
            self.assertIn(f"- {limitation}", text)

    def test_the_sections_are_separated_by_blank_lines(self):
        text = report()

        self.assertIn("\n\n## Experiment\n\n", text)
        self.assertIn("\n\n## Limitations\n\n", text)

    def test_the_file_ends_with_exactly_one_newline(self):
        text = report()

        self.assertTrue(text.endswith("\n"))
        self.assertFalse(text.endswith("\n\n"))

    def test_the_limitations_name_the_important_unknowns(self):
        joined = " ".join(LIMITATIONS)

        for phrase in ("WSL2", "frequency", "compiler", "release"):
            self.assertIn(phrase, joined)


class SaveResultsTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.options = parse_args(["--repeats", "2"])
        self.moment = datetime.datetime(2026, 10, 16, 14, 30, 5)

    def tearDown(self):
        self.directory.cleanup()

    def save(self, output_dir=None, moment=None, rows=None):
        return run_benchmarks_save(output_dir or self.root, rows or [make_row(), make_row()],
                                   [SUMMARY_2], ENV, self.options, moment or self.moment)

    def test_it_creates_a_folder_named_after_the_moment_and_returns_it(self):
        directory = self.save()

        self.assertEqual(directory, self.root / "2026-10-16T14-30-05")
        self.assertTrue(directory.is_dir())

    def test_it_writes_both_files_into_that_folder(self):
        directory = self.save()

        self.assertEqual(sorted(path.name for path in directory.iterdir()), ["results.csv", "results.md"])

    def test_the_csv_has_every_raw_run(self):
        directory = self.save(rows=[make_row(), make_row(), make_row()])

        self.assertEqual(len((directory / "results.csv").read_text(encoding="utf-8").splitlines()), 4)

    def test_the_markdown_is_the_rendered_report(self):
        directory = self.save()

        self.assertEqual((directory / "results.md").read_text(encoding="utf-8"),
                         render_markdown([SUMMARY_2], ENV, self.options))

    def test_missing_parent_folders_are_created(self):
        directory = self.save(output_dir=self.root / "a" / "b")

        self.assertTrue((directory / "results.csv").is_file())

    def test_an_existing_run_folder_is_never_overwritten(self):
        first = self.save()
        original = (first / "results.md").read_text(encoding="utf-8")

        with self.assertRaises(FileExistsError):
            self.save(rows=[make_row()])

        self.assertEqual((first / "results.md").read_text(encoding="utf-8"), original)

    def test_two_different_moments_keep_both_runs(self):
        first = self.save()
        second = self.save(moment=datetime.datetime(2026, 10, 16, 14, 30, 6))

        self.assertNotEqual(first, second)
        self.assertTrue((first / "results.csv").is_file())
        self.assertTrue((second / "results.csv").is_file())


class MainTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.output = Path(self.directory.name) / "results"

    def tearDown(self):
        self.directory.cleanup()

    def run_main(self, argv, side_effect):
        with mock.patch("run_benchmarks.run_once", side_effect=side_effect):
            with mock.patch("sys.stdout") as out, mock.patch("sys.stderr") as err:
                code = main(argv + ["--output-dir", str(self.output)])
        printed = "".join(call.args[0] for call in out.write.call_args_list)
        errors = "".join(call.args[0] for call in err.write.call_args_list)
        return code, printed, errors

    def run_folders(self):
        return sorted(self.output.iterdir()) if self.output.exists() else []

    def test_success_returns_zero_and_prints_one_line_per_point(self):
        outputs = [good_run(workers="1"), good_run(workers="2")]

        code, printed, errors = self.run_main(["--workers", "1", "2", "--repeats", "1"], outputs)

        self.assertEqual(code, 0)
        self.assertEqual(errors, "")
        self.assertIn("workers=1", printed)
        self.assertIn("workers=2", printed)

    def test_success_saves_both_files_and_says_where(self):
        outputs = [good_run(workers="1"), good_run(workers="2")]

        code, printed, errors = self.run_main(["--workers", "1", "2", "--repeats", "1"], outputs)

        folders = self.run_folders()
        self.assertEqual(len(folders), 1)
        self.assertIn(f"results saved to {folders[0]}", printed)
        self.assertTrue((folders[0] / "results.csv").is_file())
        self.assertIn("# Benchmark results", (folders[0] / "results.md").read_text(encoding="utf-8"))

    def test_the_csv_keeps_every_raw_run_not_only_the_summaries(self):
        outputs = [good_run(workers=str(w)) for w in (1, 1, 1, 2, 2, 2)]

        self.run_main(["--workers", "1", "2", "--repeats", "3"], outputs)

        csv_text = (self.run_folders()[0] / "results.csv").read_text(encoding="utf-8")
        self.assertEqual(len(csv_text.splitlines()), 7)

    def test_a_refused_result_returns_one_and_writes_the_reason_to_stderr(self):
        code, printed, errors = self.run_main(["--workers", "1", "--repeats", "1"], [good_run(build="debug")])

        self.assertEqual(code, 1)
        self.assertEqual(printed, "")
        self.assertIn("non-release", errors)

    def test_a_failed_run_leaves_no_results_folder_behind(self):
        self.run_main(["--workers", "1", "2", "--repeats", "1"], [good_run(workers="1"), fake(returncode=1)])

        self.assertEqual(self.run_folders(), [])

    def test_a_missing_binary_returns_one_without_a_traceback(self):
        code, printed, errors = self.run_main(["--workers", "1", "--repeats", "1"], FileNotFoundError(2, "No such file"))

        self.assertEqual(code, 1)
        self.assertIn("error:", errors)

    def test_an_existing_run_folder_gives_an_error_and_keeps_the_first_run(self):
        with mock.patch("run_benchmarks.timestamp_dir_name", return_value="same-second"):
            first, _, _ = self.run_main(["--workers", "1", "--repeats", "1"], [good_run(workers="1")])
            original = (self.output / "same-second" / "results.md").read_text(encoding="utf-8")
            second, printed, errors = self.run_main(["--workers", "1", "--repeats", "1"], [good_run(workers="1")])

        self.assertEqual(first, 0)
        self.assertEqual(second, 1)
        self.assertIn("error:", errors)
        self.assertEqual((self.output / "same-second" / "results.md").read_text(encoding="utf-8"), original)


if __name__ == "__main__":
    unittest.main()
