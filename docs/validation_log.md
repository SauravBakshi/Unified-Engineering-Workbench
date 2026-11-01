# Validation log — takeover build

## Build environment

- Audit date: 2026-11-01.
- OS/container: Linux x86_64 (not the candidate's Windows 11 target).
- Compiler: GCC 14.2.0; C++17.
- CMake: 3.31.6; Ninja generator; Release configuration (`-O3 -DNDEBUG`).
- CPU reported by environment: AMD EPYC 9V74 80-Core Processor; container exposes 5 logical CPUs, 5 cores on one socket, one thread per visible core.
- Memory visible through `/proc/meminfo`: 6,090,668 KiB (about 5.81 GiB).
- This is the execution container's resource view, not a claim about the candidate's laptop/Windows PC.

## Clean source build and tests

Commands:

```bash
cmake -S . -B build_linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DUEW_BUILD_TESTS=ON
cmake --build build_linux -j2
ctest --test-dir build_linux --output-on-failure
node --check frontend/app.js
```

Outcome: build PASS; JavaScript syntax check PASS; CTest 1/1 PASS. The test executable checks:

- Exact Dirichlet end temperatures and analytical midpoint errors for N=10,20,40,80.
- Four-to-one midpoint-error reduction with each mesh doubling.
- Invalid element count and non-finite thermal parameters.
- Valid ASCII STL; malformed/non-finite STL normal; empty STL.
- Valid CRLF nodal CSV; non-finite data; duplicate node IDs.
- Manual gradients against central finite differences; invalid epsilon rejection.
- Fixed-seed serial/parallel agreement for all nodal values.
- MCU connect/telemetry/disconnect and recoverable JTAG timeout retry; telemetry-before-connect rejection.
- Decreasing loss over 100 gradient-descent updates.

## HTTP integration checks

The C++ server was started on loopback port 18080 using `UEW_PORT=18080` to avoid the environment's occupied default port. Verified HTTP 200 and valid JSON/content for:

- `/`, `/app.js`, `/api/health`.
- `/api/thermal`; it generated the four thermal profile CSVs and convergence CSV.
- `/api/backprop`; it generated 101 loss samples (initial state plus 100 updates).
- `/api/hardware`, JTAG retry, MCU connect and telemetry state transition.
- `POST /api/geometry/stl` with `data/sample_cube_ascii.stl`: 2 triangles, 6 vertices.
- `POST /api/geometry/csv` with `data/sample_temperature.csv`: 6 nodes, 20–100 °C.
- `/api/benchmark?cases=2000&elements=200&seed=2026`.
- Thermal, backpropagation and benchmark result CSV download routes.

## Thermal numerical results

| Elements | Maximum absolute midpoint error (°C) |
|---:|---:|
| 10 | 1.25000000 |
| 20 | 0.31250000 |
| 40 | 0.07812500 |
| 80 | 0.01953125 |

The error ratio is 4.0 for each mesh doubling, as expected for linear interpolation of the quadratic exact temperature field. The boundary temperatures are imposed exactly.

## Benchmark measurement from this audit container

Workload: 2,000 independent cases, 200 linear elements per case, seed 2026; one warm-up per worker count; five timed repetitions; median wall-clock compute time; identical cases and solver; comparison across all nodal temperatures. Container exposes five logical CPUs, so the benchmark ran 1–4 workers.

| Workers | Median (ms) | Speedup | Efficiency | Max absolute difference (°C) |
|---:|---:|---:|---:|---:|
| 1 | 8.492121 | 1.0000 | 100.00% | 0 |
| 2 | 4.203106 | 2.0204 | 101.02% | 0 |
| 3 | 3.553149 | 2.3900 | 79.67% | 0 |
| 4 | 2.305053 | 3.6841 | 92.10% | 0 |

These values are a single reproducible run in the audit container and are not transferable to another CPU or compiler. The two-worker efficiency slightly above 100% is within the effect of short-run timing/cache/scheduling variation; it is not evidence of superlinear scalability. Re-run the packaged application on the candidate's PC and report its metadata and raw CSV.

## Backpropagation

Initial state: `h=0.46211715726000974`, prediction `0.46211715726000974`, loss `1.1825418189970167`. Initial gradients in `[w,b,v,c]` order are approximately `[-1.2094644752400612, -1.2094644752400612, -0.7106820474859469, -1.5378828427399902]`. Maximum central finite-difference absolute error observed: `1.117548276e-10`, below the required `1e-5`. Loss after 100 updates is approximately `1.70262242866e-20`.

## Not yet verified / target-machine work

- Windows x64 `Launch.exe` build and package script execution.
- Clean Windows 11 x64 extraction test, no-internet launch, default-browser opening, no administrator rights, and redistributable runtime check.
- Actual Windows PC CPU/core/RAM/compiler/flags and launch-to-ready time.
- Visual browser interaction on the target machine (rotation, pan, zoom, fit, point inspection and chart layout).
- Candidate-authored handwritten two-page backprop appendix and 3–5 minute demo video or timestamped screenshot walkthrough.

## Windows localhost fix (2026-11-02)

Added `scripts/Run_Workbench.ps1` and `scripts/Start_Localhost.bat`. These build the server in an isolated build directory, prefer the supported Visual Studio 2022 x64 toolchain or MSYS2 UCRT64, reject legacy MinGW GCC 6.3, run CTest, and launch the server with the project root as its working directory. The Windows GUI launcher now checks for an early backend exit before opening the browser. Linux C++ build, unit tests, health endpoint, and frontend HTTP response were verified after these source changes. Windows executable launch remains unverified until run on Windows 11.
