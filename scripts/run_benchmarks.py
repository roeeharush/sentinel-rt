import subprocess
import statistics
import argparse
from pathlib import Path
import sys
import csv
import datetime
import os
import platform

EXPECTED_FIELDS = 22
CONFIG_FIELDS = ["mode", "build", "workers", "devices", "target_rate"]
DEFAULT_BINARY = Path(__file__).resolve().parent.parent / "cmake-build-release-wsl" / "src" / "apps" / "benchmark"
DEFAULT_OUTPUT_DIR = Path(__file__).resolve().parent.parent / "benchmarks" / "results"
RESULT_COLUMNS = [
    ("workers", "workers"),
    ("throughput_per_s", "throughput/s (median)"),
    ("throughput_min", "min"),
    ("throughput_max", "max"),
    ("total_p50_us", "total p50 us"),
    ("total_p95_us", "total p95 us"),
    ("total_p99_us", "total p99 us"),
    ("total_max_us", "total max us"),
    ("service_p50_us", "service p50 us"),
    ("service_p99_us", "service p99 us"),
    ("queue_avg", "queue avg"),
    ("queue_max", "queue max"),
    ("max_lag", "max lag"),
]

LIMITATIONS = [
    "Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.",
    "The CPU frequency during the run, the Windows power plan and the background load are unknown.",
    "The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.",
    "Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.",
    "Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.",
    "Only the release build is accepted; debug results are refused.",
]


def run_once(binary, args):
    command = [binary] + args
    return subprocess.run(command, capture_output=True, text=True)

def parse_value(text):
    if text == "NA":
        return None
    try:
        return int(text)
    except ValueError:
        return text


def parse_line(line):
    row = {}
    for token in line.split():
        key, value = token.split("=", 1)
        row[key] = parse_value(value)
    return row


def check_result(result):
    if result.returncode != 0:
        raise ValueError(f"benchmark failed with exit code {result.returncode}: {result.stderr.strip()}")

    lines = result.stdout.strip().splitlines()
    if len(lines) != 1:
        raise ValueError(f"expected exactly one output line, got {len(lines)}")

    row = parse_line(lines[0])
    if len(row) != EXPECTED_FIELDS:
        raise ValueError(f"expected {EXPECTED_FIELDS} fields, got {len(row)}")

    if row["build"] != "release":
        raise ValueError(f"refusing a non-release result (build={row['build']!r})")

    return row


def median_or_none(values):
    if not values:
        return None

    for value in values:
        if value is None:
            return None

    return statistics.median(values)


def collect_runs(binary, args, repeats):
    rows = []
    for _ in range(repeats):
        rows.append(check_result(run_once(binary, args)))
    return rows


def summarize_runs(rows):
    summary = {}

    for key in rows[0]:
        values = [row[key] for row in rows]
        if key in CONFIG_FIELDS:
            if len(set(values)) != 1:
                raise ValueError(f"field {key!r} differs between runs: {values}")
            summary[key] = values[0]
        else:
            summary[key] = median_or_none(values)

    throughputs = [row["throughput_per_s"] for row in rows]
    summary["runs"] = len(rows)
    summary["throughput_min"] = min(throughputs)
    summary["throughput_max"] = max(throughputs)
    return summary


def command_output(command):
    try:
        result = subprocess.run(command, capture_output=True, text=True)
    except OSError:
        return "unknown"

    if result.returncode != 0:
        return "unknown"

    lines = result.stdout.strip().splitlines()
    return lines[0] if lines else "unknown"


def cpu_model():
    try:
        with open("/proc/cpuinfo", encoding="utf-8") as handle:
            for line in handle:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass

    return "unknown"


def environment_info(binary):
    return {
        "date": datetime.datetime.now().isoformat(timespec="seconds"),
        "cpu": cpu_model(),
        "logical_cores": os.cpu_count() or "unknown",
        "os": f"{platform.system()} {platform.release()}",
        "python": platform.python_version(),
        "compiler": command_output(["g++", "--version"]),
        "cmake": command_output(["cmake", "--version"]),
        "binary": str(binary),
    }


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Run the benchmark program several times per point and summarise the medians.")
    parser.add_argument("--binary", default=str(DEFAULT_BINARY))
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR))
    parser.add_argument("--mode", choices=["max", "rate", "calibrate"], default="max")
    parser.add_argument("--workers", type=int, nargs="+", default=[1, 2, 4, 6])
    parser.add_argument("--devices", type=int, default=10)
    parser.add_argument("--rate", type=int, default=None)
    parser.add_argument("--duration-ms", type=int, default=5000)
    parser.add_argument("--warmup-ms", type=int, default=1000)
    parser.add_argument("--repeats", type=int, default=3)

    options = parser.parse_args(argv)
    if options.repeats < 1:
        parser.error("--repeats must be at least 1")
    return options


def build_args(options, workers):
    args = [
        "--mode", options.mode,
        "--workers", str(workers),
        "--devices", str(options.devices),
        "--duration-ms", str(options.duration_ms),
        "--warmup-ms", str(options.warmup_ms),
    ]
    if options.rate is not None:
        args += ["--rate", str(options.rate)]
    return args


def run_matrix(options):
    raw_rows = []
    summaries = []

    for workers in options.workers:
        rows = collect_runs(options.binary, build_args(options, workers), options.repeats)
        raw_rows.extend(rows)
        summaries.append(summarize_runs(rows))

    return raw_rows, summaries


def write_csv(path, rows):
    fieldnames = list(rows[0])

    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: ("NA" if value is None else value) for key, value in row.items()})


def timestamp_dir_name(moment):
    return moment.strftime("%Y-%m-%dT%H-%M-%S")


def format_cell(value):
    return "NA" if value is None else str(value)


def markdown_table(headers, rows):
    lines = ["| " + " | ".join(headers) + " |",
             "|" + "|".join("---" for _ in headers) + "|"]
    for row in rows:
        lines.append("| " + " | ".join(format_cell(cell) for cell in row) + " |")
    return "\n".join(lines)


def render_markdown(summaries, env, options):
    experiment_rows = [
        ["mode", options.mode],
        ["devices", options.devices],
        ["target rate", options.rate],
        ["duration per run (ms)", options.duration_ms],
        ["warm-up per point (ms)", options.warmup_ms],
        ["runs per point", options.repeats],
    ]
    environment_rows = [[key, value] for key, value in env.items()]
    headers = [title for _, title in RESULT_COLUMNS]
    result_rows = [[summary[key] for key, _ in RESULT_COLUMNS] for summary in summaries]

    parts = [
        "# Benchmark results",
        f"Generated: {env['date']}",
        "## Experiment",
        markdown_table(["setting", "value"], experiment_rows),
        "## Environment",
        markdown_table(["item", "value"], environment_rows),
        f"## Results (median of {options.repeats} runs per point; min and max are the throughput range)",
        markdown_table(headers, result_rows),
        "## Limitations",
        "\n".join(f"- {line}" for line in LIMITATIONS),
    ]
    return "\n\n".join(parts) + "\n"


def save_results(output_dir, raw_rows, summaries, env, options, moment):
    directory = Path(output_dir) / timestamp_dir_name(moment)
    directory.mkdir(parents=True, exist_ok=False)

    write_csv(directory / "results.csv", raw_rows)
    (directory / "results.md").write_text(render_markdown(summaries, env, options), encoding="utf-8")

    return directory


def main(argv):
    options = parse_args(argv)

    try:
        raw_rows, summaries = run_matrix(options)
        env = environment_info(options.binary)
        directory = save_results(options.output_dir, raw_rows, summaries, env, options,datetime.datetime.now())
    except (ValueError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    for summary in summaries:
        print(f"workers={summary['workers']} throughput_per_s={summary['throughput_per_s']} "
              f"(min {summary['throughput_min']}, max {summary['throughput_max']})")
    print(f"results saved to {directory}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))