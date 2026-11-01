#!/usr/bin/env python3
"""Regenerate labelled engineering plots from C++-generated result CSV files."""
from pathlib import Path
import csv
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"

def read_csv(name):
    with (RESULTS / name).open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))

# Thermal profiles: FE nodal values from each C++ solver run and analytical curve.
fig, ax = plt.subplots(figsize=(8.5, 5.2))
for n in (10, 20, 40, 80):
    rows = read_csv(f"thermal_N{n}.csv")
    ax.plot([float(r['x_m']) for r in rows], [float(r['temperature_C']) for r in rows], marker='.', linewidth=1.2, label=f"FE, N={n}")
x = [i / 500 for i in range(501)]
y = [100 - 80 * v + 500 * v * (1 - v) for v in x]
ax.plot(x, y, linestyle='--', linewidth=2, label='Analytical exact solution')
ax.set(title='1D Steady-State Thermal FEA — Rod Temperature', xlabel='Position x (m)', ylabel='Temperature (°C)')
ax.grid(True, alpha=.25); ax.legend(); fig.tight_layout(); fig.savefig(RESULTS / 'thermal_profiles.png', dpi=180); plt.close(fig)

# Midpoint convergence from the actual solver convergence CSV.
rows = read_csv('thermal_convergence.csv')
fig, ax = plt.subplots(figsize=(7.5, 4.8))
ax.loglog([int(r['elements']) for r in rows], [float(r['max_midpoint_error_C']) for r in rows], marker='o', linewidth=2)
ax.set(title='Mesh Convergence — Maximum Element-Midpoint Error', xlabel='Number of linear elements, N', ylabel='Maximum absolute error (°C)')
ax.grid(True, which='both', alpha=.3); fig.tight_layout(); fig.savefig(RESULTS / 'thermal_convergence.png', dpi=180); plt.close(fig)

# Actual serial/parallel measurements from the benchmark endpoint.
rows = read_csv('benchmark.csv')
workers = [int(r['workers']) for r in rows]
fig, ax = plt.subplots(figsize=(7.5, 4.8))
ax.bar([str(w) for w in workers], [float(r['median_ms']) for r in rows])
ax.set(title='Serial / Parallel Thermal Batch — Median Compute Time', xlabel='Worker threads', ylabel='Median wall-clock time (ms)')
ax.grid(True, axis='y', alpha=.25); fig.tight_layout(); fig.savefig(RESULTS / 'benchmark.png', dpi=180); fig.savefig(RESULTS / 'benchmark.svg'); plt.close(fig)

# Loss over the initial point and 100 subsequent gradient-descent updates.
rows = read_csv('backprop_loss.csv')
fig, ax = plt.subplots(figsize=(7.5, 4.8))
ax.plot([int(r['step']) for r in rows], [float(r['loss']) for r in rows], linewidth=2)
ax.set(title='Manual Backpropagation — Training Loss', xlabel='Gradient-descent update step', ylabel='Squared-error loss J')
ax.grid(True, alpha=.25); fig.tight_layout(); fig.savefig(RESULTS / 'backprop_loss.png', dpi=180); plt.close(fig)
print('Generated thermal_profiles.png, thermal_convergence.png, benchmark.png, backprop_loss.png from result CSVs.')
