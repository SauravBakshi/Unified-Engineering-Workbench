#pragma once
#include <array>
#include <string>
struct BackpropResult {
    double h, prediction, loss;
    std::array<double,4> gradient; // w,b,v,c
    std::array<double,4> updated;
    double max_finite_difference_error;
};
BackpropResult run_backprop(double x=1, double target=2, double w=.5,
                            double b=0, double v=1, double c=0,
                            double lr=.1, double epsilon=1e-6);
void write_loss_csv(const std::string& path, int steps=100, double lr=.1);
