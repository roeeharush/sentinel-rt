# Benchmark results

Generated: 2026-10-16T12:34:36

## Experiment

| setting | value |
|---|---|
| mode | calibrate |
| devices | 10 |
| target rate | NA |
| duration per run (ms) | 5000 |
| warm-up per point (ms) | 1000 |
| runs per point | 3 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T12:34:36 |
| cpu | AMD Ryzen 5 3500X 6-Core Processor |
| logical_cores | 6 |
| os | Linux 6.18.33.2-microsoft-standard-WSL2 |
| python | 3.14.4 |
| compiler | g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| cmake | cmake version 3.30.5 |
| binary | /home/roee/projects/sentinel-rt/cmake-build-release-wsl/src/apps/benchmark |

## Results (median of 3 runs per point; min and max are the throughput range)

| workers | throughput/s (median) | min | max | total p50 us | total p95 us | total p99 us | total max us | service p50 us | service p99 us | queue avg | queue max | max lag |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 3046618 | 3017751 | 3567475 | NA | NA | NA | NA | NA | NA | 47 | 1024 | 0 |
| 2 | 225485 | 205832 | 227230 | NA | NA | NA | NA | NA | NA | 36 | 1024 | 0 |
| 4 | 116966 | 113528 | 118011 | NA | NA | NA | NA | NA | NA | 5 | 357 | 0 |
| 6 | 159168 | 155280 | 160364 | NA | NA | NA | NA | NA | NA | 4 | 195 | 0 |
| 8 | 114277 | 108445 | 114481 | NA | NA | NA | NA | NA | NA | 3 | 205 | 0 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
