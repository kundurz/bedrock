import subprocess
from pathlib import Path


RUNS = 30
CPU = 2
OPERATIONS = 10_000_000

EVENTS = [
    "cycles",
    "instructions",
    "cache-references",
    "cache-misses",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
    "branches",
    "branch-misses",
]

REPOSITORY = Path(__file__).resolve().parents[2]
BUILD_DIRECTORY = REPOSITORY / "build"


def parse_perf_output(output):
    counters = {}

    for line in output.splitlines():
        fields = line.split(",")
        if len(fields) < 3 or fields[0].startswith("<"):
            continue

        event = fields[2]

        # On this hybrid CPU, use the performance-core counters. Ignore the
        # negligible cpu_atom counts produced while starting the process.
        if event.startswith("cpu_atom/"):
            continue
        if event.startswith("cpu_core/"):
            event = event.removeprefix("cpu_core/").removesuffix("/u")
        else:
            event = event.removesuffix(":u")

        if event in EVENTS:
            counters[event] = float(fields[0])

    missing = [event for event in EVENTS if event not in counters]
    if missing:
        raise RuntimeError("perf did not report: " + ", ".join(missing))

    return counters


def run_benchmark(executable, arguments):
    command = [
        "perf",
        "stat",
        "--no-big-num",
        "-x,",
        "-r",
        str(RUNS),
        "-e",
        ",".join(EVENTS),
        "taskset",
        "-c",
        str(CPU),
        str(BUILD_DIRECTORY / executable),
        *arguments,
    ]

    process = subprocess.run(command, capture_output=True, text=True)
    if process.returncode != 0:
        raise RuntimeError(process.stderr)

    # perf stat writes its measurements to stderr. The benchmark's own result
    # is written to stdout.
    return parse_perf_output(process.stderr)


def derive_measurements(counters):
    return {
        "cycles_per_operation": counters["cycles"] / OPERATIONS,
        "instructions_per_operation": counters["instructions"] / OPERATIONS,
        "instructions_per_cycle": counters["instructions"] / counters["cycles"],
        "cache_references_per_operation": counters["cache-references"] / OPERATIONS,
        "cache_misses_per_operation": counters["cache-misses"] / OPERATIONS,
        "cache_miss_rate": counters["cache-misses"] / counters["cache-references"] * 100,
        "l1_loads_per_operation": counters["L1-dcache-loads"] / OPERATIONS,
        "l1_misses_per_operation": counters["L1-dcache-load-misses"] / OPERATIONS,
        "l1_miss_rate": counters["L1-dcache-load-misses"] / counters["L1-dcache-loads"] * 100,
        "dtlb_misses_per_operation": counters["dTLB-load-misses"] / OPERATIONS,
        "dtlb_miss_rate": counters["dTLB-load-misses"] / counters["dTLB-loads"] * 100,
        "branches_per_operation": counters["branches"] / OPERATIONS,
        "branch_misses_per_operation": counters["branch-misses"] / OPERATIONS,
        "branch_miss_rate": counters["branch-misses"] / counters["branches"] * 100,
    }


def print_measurements(allocator, measurements):
    for name, value in measurements.items():
        unit = "%" if name.endswith("rate") else ""
        print(f"{allocator}_{name}: {value:.2f} {unit}".rstrip())


subprocess.run(
    ["make", "throughput", "malloc_throughput"],
    cwd=BUILD_DIRECTORY,
    check=True,
)

hardened_counters = run_benchmark("throughput", ["s"])
glibc_counters = run_benchmark("malloc_throughput", [])

print_measurements("hardened", derive_measurements(hardened_counters))
print_measurements("glibc", derive_measurements(glibc_counters))
