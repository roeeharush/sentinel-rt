# Benchmark results

Generated: 2026-10-16T16:46:48

## Experiment

| setting | value |
|---|---|
| mode | rate |
| devices | 10 |
| target rate | 500000 |
| duration per run (ms) | 5000 |
| warm-up per point (ms) | 1000 |
| runs per point | 3 |

## Environment

| item | value |
|---|---|
| date | 2026-10-16T16:46:48 |
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
| 1 | 499954 | 499952 | 499956 | 29 | 73 | 209 | 4178 | 1 | 2 | 21 | 1024 | 1904 |
| 2 | 499941 | 499874 | 499947 | 63 | 439 | 743 | 10021 | 1 | 12 | 76 | 1024 | 14617 |
| 4 | 499836 | 499748 | 499886 | 186 | 750 | 975 | 7751 | 1 | 56 | 136 | 1024 | 5803 |
| 6 | 499891 | 499865 | 499894 | 288 | 860 | 1066 | 7880 | 1 | 73 | 177 | 1024 | 8835 |
| 8 | 499697 | 499658 | 499822 | 271 | 811 | 1043 | 18290 | 1 | 87 | 165 | 1024 | 10794 |

## Limitations

- Measured on WSL2 (a virtualised Linux on Windows): scheduling noise from the host is possible.
- The CPU frequency during the run, the Windows power plan and the background load are unknown.
- The compiler and CMake versions are those installed when this script ran, not necessarily those that built the binary.
- Latency is stamped when a measurement is pushed: when the feeder was blocked (a large max lag) the latency looks better than the real wait.
- Percentiles are the upper edge of a 1 microsecond bucket; values beyond 100 ms are counted as overflow and shown as NA.
- Only the release build is accepted; debug results are refused.
