# Benchmark results

Generated: 2026-10-16T12:57:27

## Experiment

| setting | value |
|---|---|
| mode | max |
| devices | 10 |
| target rate | NA |
| duration per run (ms) | 5000 |
| warm-up per point (ms) | 1000 |
| runs per point | 3 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T12:57:27 |
| cpu | AMD Ryzen 5 3500X 6-Core Processor |
| logical_cores | 6 |
| os | Linux 6.18.33.2-microsoft-standard-WSL2 |
| python | 3.14.4 |
| compiler | g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| cmake | cmake version 4.2.3 |
| binary | /home/roee/projects/sentinel-rt/cmake-build-release-wsl/src/apps/benchmark |

## Results (median of 3 runs per point; min and max are the throughput range)

| workers | throughput/s (median) | min | max | total p50 us | total p95 us | total p99 us | total max us | service p50 us | service p99 us | queue avg | queue max | max lag |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 2232080 | 1980512 | 2244995 | 333 | 476 | 642 | 1854 | 1 | 1 | 729 | 1024 | 0 |
| 2 | 2373245 | 2371802 | 2396158 | 300 | 530 | 744 | 2447 | 1 | 2 | 748 | 1024 | 0 |
| 4 | 1066124 | 1025548 | 1154665 | 646 | 945 | 1095 | 2935 | 1 | 52 | 706 | 1024 | 0 |
| 6 | 1051249 | 974330 | 1057165 | 676 | 964 | 1103 | 3237 | 1 | 65 | 700 | 1024 | 0 |
| 8 | 923645 | 917990 | 981806 | 617 | 997 | 1217 | 7491 | 1 | 109 | 572 | 1024 | 0 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
