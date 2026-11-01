#include "benchmark.hpp"
#include "thermal.hpp"
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <string>

static std::vector<ThermalConfig> make_cases(int count, unsigned seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> kd(5, 30), qd(1000, 20000);
    std::vector<ThermalConfig> v(count);
    for (auto& c : v) { c.conductivity = kd(gen); c.generation = qd(gen); }
    return v;
}

static std::vector<double> run_serial(const std::vector<ThermalConfig>& cs, int n) {
    const std::size_t stride = static_cast<std::size_t>(n) + 1;
    std::vector<double> out(cs.size() * stride);
    for (std::size_t i = 0; i < cs.size(); ++i) {
        const auto result = solve_thermal(cs[i], n);
        std::copy(result.temperature.begin(), result.temperature.end(), out.begin() + i * stride);
    }
    return out;
}

static std::vector<double> run_parallel(const std::vector<ThermalConfig>& cs, int n, int workers) {
    const std::size_t stride = static_cast<std::size_t>(n) + 1;
    std::vector<double> out(cs.size() * stride);
    std::atomic<std::size_t> next{0};
    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (int w = 0; w < workers; ++w)
        pool.emplace_back([&] {
            for (;;) {
                auto i = next.fetch_add(1);
                if (i >= cs.size()) break;
                const auto result = solve_thermal(cs[i], n);
                std::copy(result.temperature.begin(), result.temperature.end(), out.begin() + i * stride);
            }
        });
    for (auto& t : pool) t.join();
    return out;
}

// Returns median of 5 repetitions of a timed workload
static double measure_time(int workers, const std::vector<ThermalConfig>& cs, int n) {
    std::vector<double> ms;
    ms.reserve(5);
    for (int rep = 0; rep < 5; ++rep) {
        auto st = std::chrono::steady_clock::now();
        if (workers == 1)
            (void)run_serial(cs, n);
        else
            (void)run_parallel(cs, n, workers);
        auto en = std::chrono::steady_clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(en - st).count());
    }
    std::sort(ms.begin(), ms.end());
    return ms[2]; // median of 5
}

std::vector<BenchmarkRow> run_benchmark(const std::string& path, int cases, int elements, unsigned seed) {
    if (cases < 1 || elements < 2)
        throw std::invalid_argument("cases must be >=1 and elements >=2");

    auto cs = make_cases(cases, seed);

    // Warm-up: one full serial pass (excluded from timing)
    (void)run_serial(cs, elements);

    // Hardware concurrency info
    unsigned hc = std::thread::hardware_concurrency();
    unsigned logical_cpus = hc ? hc : 1;
    int maxw = std::max(1, std::min(4, (int)logical_cpus));

    // Reference serial result (used for numerical agreement)
    auto ref = run_serial(cs, elements);

    // Measure serial baseline ONCE — this is the denominator for speedup
    double t_serial = measure_time(1, cs, elements);

    std::vector<BenchmarkRow> rows;

    // Serial row
    {
        BenchmarkRow r;
        r.workers = 1;
        r.median_ms = t_serial;
        r.speedup = 1.0;
        r.efficiency = 1.0;
        r.max_abs_difference_C = 0.0;
        r.cases = cases;
        r.elements = elements;
        r.repetitions = 5;
        r.seed = seed;
        r.logical_cpus = logical_cpus;
        rows.push_back(r);
    }

    // Measure every available worker count from 2 through the four-worker cap.
    for (int p = 2; p <= maxw; ++p) {
        (void)run_parallel(cs, elements, p); // per-worker-count warm-up, excluded from timing
        double t = measure_time(p, cs, elements);
        auto got = run_parallel(cs, elements, p);
        double diff = 0.0;
        for (std::size_t i = 0; i < ref.size(); ++i)
            diff = std::max(diff, std::abs(ref[i] - got[i]));

        BenchmarkRow r;
        r.workers = p;
        r.median_ms = t;
        r.speedup = t_serial / t;
        r.efficiency = r.speedup / p;
        r.max_abs_difference_C = diff;
        r.cases = cases;
        r.elements = elements;
        r.repetitions = 5;
        r.seed = seed;
        r.logical_cpus = logical_cpus;
        rows.push_back(r);
    }

    // Write CSV
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write benchmark CSV: " + path);
    f << "workers,median_ms,speedup,efficiency,max_abs_difference_C,"
      << "cases,elements,repetitions,seed,logical_cpus\n"
      << std::setprecision(12);
    for (const auto& r : rows) {
        f << r.workers << "," << r.median_ms << "," << r.speedup << ","
          << r.efficiency << "," << r.max_abs_difference_C << ","
          << r.cases << "," << r.elements << "," << r.repetitions << ","
          << r.seed << "," << r.logical_cpus << "\n";
    }

    std::cout << "Benchmark: logical_cpus=" << logical_cpus
              << ", max_workers=" << maxw
              << ", serial_median_ms=" << t_serial << "\n";

    return rows;
}
