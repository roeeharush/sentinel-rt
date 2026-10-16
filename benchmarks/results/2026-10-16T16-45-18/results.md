# Benchmark results

Generated: 2026-10-16T16:45:18

## Experiment

| setting | value |
|---|---|
| mode | rate |
| devices | 10 |
| target rate | 100000 |
| duration per run (ms) | 5000 |
| warm-up per point (ms) | 1000 |
| runs per point | 3 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T16:45:18 |
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
| 1 | 99991 | 99991 | 99992 | 16 | 34 | 48 | 358 | 1 | 3 | 1 | 26 | 100 |
| 2 | 99985 | 99983 | 99989 | 20 | 65 | 101 | 804 | 1 | 27 | 4 | 132 | 286 |
| 4 | 99978 | 99978 | 99981 | 32 | 107 | 147 | 744 | 1 | 40 | 4 | 171 | 308 |
| 6 | 99957 | 99956 | 99971 | 45 | 120 | 164 | 1869 | 2 | 44 | 8 | 191 | 508 |
| 8 | 99965 | 99951 | 99973 | 43 | 121 | 169 | 1537 | 2 | 52 | 7 | 154 | 576 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
