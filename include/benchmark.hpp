#pragma once
#include <string>
#include <vector>

struct BenchmarkRow {
    int workers;
    double median_ms;
    double speedup;
    double efficiency;
    double max_abs_difference_C;
    int cases;
    int elements;
    int repetitions;
    unsigned seed;
    unsigned logical_cpus;
};

/// Run the fixed-seed parallel benchmark and write results to csv_path.
/// Returns the benchmark rows for API use.
std::vector<BenchmarkRow> run_benchmark(const std::string& csv_path,
                                         int cases = 2000,
                                         int elements = 200,
                                         unsigned seed = 2026);
