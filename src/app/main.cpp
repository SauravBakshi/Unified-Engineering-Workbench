#include "thermal.hpp"
#include "importers.hpp"
#include "backprop.hpp"
#include "benchmark.hpp"
#include "embedded.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <algorithm>
#include <iomanip>

int main(int argc, char** argv) {
    try {
        std::string cmd = argc > 1 ? argv[1] : "demo";
        std::filesystem::create_directories("results");

        if (cmd == "fea" || cmd == "demo") {
            ThermalConfig c;
            // Write convergence CSV
            {
                std::ofstream conv("results/thermal_convergence.csv");
                conv << "elements,max_midpoint_error_C\n" << std::setprecision(12);
                for (int n : {10, 20, 40, 80}) {
                    auto r = solve_thermal(c, n);
                    write_thermal_csv(r, "results/thermal_N" + std::to_string(n) + ".csv");
                    conv << n << "," << r.max_midpoint_error << "\n";
                    std::cout << "FEA N=" << n << " max midpoint error="
                              << std::setprecision(10) << r.max_midpoint_error << " C\n";
                }
            }
        }
        if (cmd == "benchmark" || cmd == "demo") {
            auto rows = run_benchmark("results/benchmark.csv", 2000, 200, 2026);
            std::cout << "Benchmark written: results/benchmark.csv ("
                      << rows.size() << " rows)\n";
        }
        if (cmd == "backprop" || cmd == "demo") {
            auto r = run_backprop();
            write_loss_csv("results/backprop_loss.csv");
            std::cout << "Backprop h=" << r.h
                      << " prediction=" << r.prediction
                      << " loss=" << r.loss << "\n";
            std::cout << "Gradients w,b,v,c: ";
            for (double g : r.gradient) std::cout << g << " ";
            std::cout << "\nFinite-difference max error="
                      << r.max_finite_difference_error << "\n";
        }
        if (cmd == "devices" || cmd == "demo") {
            for (const auto& d : demo_embedded_devices())
                std::cout << d.type << " | " << d.name
                          << " | " << d.state
                          << " | " << d.message << "\n";
        }
        if (cmd == "check-stl" && argc > 2) {
            auto s = validate_ascii_stl(argv[2]);
            std::cout << "Triangles=" << s.triangles
                      << " bounds=[" << s.min[0] << "," << s.max[0] << "] x ["
                      << s.min[1] << "," << s.max[1] << "] x ["
                      << s.min[2] << "," << s.max[2] << "]\n";
        } else if (cmd == "check-csv" && argc > 2) {
            auto s = validate_temperature_csv(argv[2]);
            std::cout << "Nodes=" << s.nodes
                      << " temperature range=" << s.min_temp
                      << ".." << s.max_temp << " C\n";
        } else if (cmd != "demo" && cmd != "fea" && cmd != "benchmark"
                   && cmd != "backprop" && cmd != "devices"
                   && cmd != "check-stl" && cmd != "check-csv") {
            std::cerr << "Usage: workbench [demo|fea|benchmark|backprop|devices|"
                         "check-stl FILE|check-csv FILE]\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
