# Benchmark results

Generated: 2026-10-16T16:48:18

## Experiment

| setting | value |
|---|---|
| mode | rate |
| devices | 10 |
| target rate | 800000 |
| duration per run (ms) | 5000 |
| warm-up per point (ms) | 1000 |
| runs per point | 3 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T16:48:18 |
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
| 1 | 799925 | 799923 | 799928 | 44 | 247 | 587 | 9570 | 1 | 1 | 72 | 1024 | 7545 |
| 2 | 799873 | 799671 | 799881 | 94 | 553 | 955 | 17259 | 1 | 4 | 147 | 1024 | 23740 |
| 4 | 799773 | 799681 | 799860 | 353 | 817 | 992 | 7858 | 1 | 56 | 308 | 1024 | 7237 |
| 6 | 799646 | 799477 | 799856 | 482 | 891 | 1084 | 22229 | 1 | 69 | 382 | 1024 | 21177 |
| 8 | 799498 | 799425 | 799528 | 559 | 965 | 1104 | 4701 | 1 | 95 | 438 | 1024 | 16972 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
