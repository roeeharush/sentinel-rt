# Benchmark results

Generated: 2026-10-16T12:45:17

## Experiment

| setting | value |
|---|---|
| mode | max |
| devices | 10 |
| target rate | NA |
| duration per run (ms) | 10000 |
| warm-up per point (ms) | 1000 |
| runs per point | 5 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T12:45:17 |
| cpu | AMD Ryzen 5 3500X 6-Core Processor |
| logical_cores | 6 |
| os | Linux 6.18.33.2-microsoft-standard-WSL2 |
| python | 3.14.4 |
| compiler | g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| cmake | cmake version 3.30.5 |
| binary | /home/roee/projects/sentinel-rt/cmake-build-release-wsl/src/apps/benchmark |

## Results (median of 5 runs per point; min and max are the throughput range)

| workers | throughput/s (median) | min | max | total p50 us | total p95 us | total p99 us | total max us | service p50 us | service p99 us | queue avg | queue max | max lag |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2 | 148320 | 133228 | 162679 | 7071 | 12411 | 14426 | 19702 | 1 | 6 | 1006 | 1024 | 0 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
