# Benchmark environment
Measurements were collected using the following system:

## Hardware
| Component | Specification |
|---|---|
| Processor class | 12th-generation Intel x86-64 mobile processor |
| CPU design | Hybrid performance/efficiency-core architecture |
| Benchmark core | Process pinned to one performance-core hardware thread |
| System page size | 4 KiB |
| Execution environment | Native hardware |

## Software
| Component | Version or configuration |
|---|---|
| Operating system | 64-bit Linux |
| Compiler | GCC 13 |
| C library | glibc 2.39 |
| Performance counters | Linux `perf` |

## Benchmark configuration
| Setting | Configuration |
|---|---|
| Execution model | Single-threaded |
| CPU affinity | Pinned to the same performance core for every run |
| Optimization | `-O3` |
| Throughput workload | Steady-state allocation/free churn |
| Throughput operations | 5 million iterations, 10 million allocator operations |
| Latency samples | 100,000 allocations and 100,000 frees |
| Latency clock | `CLOCK_MONOTONIC` |
| Cache-locality collection | `perf stat`, 30 repetitions |
| Result aggregation | Median across repeated runs |

**NOTE**: The unhardened free-list implementation contained in the results is preserved in the repository's Git history at commit `9f0ace4` and was checked out in a seperate Git worktree for benchmarking.

