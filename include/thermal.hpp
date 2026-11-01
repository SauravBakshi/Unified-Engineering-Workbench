#pragma once
#include <vector>
#include <string>
struct ThermalConfig {
    double length=1.0, area=1e-4, conductivity=10.0, generation=10000.0;
    double left_temperature=100.0, right_temperature=20.0;
};
struct ThermalResult {
    std::vector<double> x, temperature;
    double max_midpoint_error=0.0;
};
ThermalResult solve_thermal(const ThermalConfig& cfg, int elements);
double exact_temperature(const ThermalConfig& cfg, double x);
void write_thermal_csv(const ThermalResult& result, const std::string& path);
