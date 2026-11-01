# Unified Engineering Workbench

C++17 engineering assessment prototype for PASS Science Works R&D Internship 2026–2027. The existing Antigravity-authored project has been retained and incrementally repaired. All application backend logic, numerical computation, concurrency and file validation are implemented in C++17. The browser is a presentation layer.

## Current verified status

- C++17 core, CLI and local HTTP server build with CMake on the audit Linux environment.
- Automated CTest suite passes, including positive and negative file-parser tests, thermal convergence, finite-difference gradients, training-loss reduction, embedded mock transitions and serial/parallel result agreement.
- HTTP endpoints were exercised end-to-end for health, thermal FEA, backpropagation, benchmark, STL upload and nodal CSV upload.
- Windows `Launch.exe` source and a Windows x64 packaging script are included. A Windows executable and clean Windows 11 launch have **not** been built or verified in this Linux environment; this remains a submission gate.

## One-click Windows run

1. Extract the packaged `CandidateID_Workbench` folder.
2. Double-click `Launch.exe`.
3. The launcher starts `uew_server.exe` from the same folder and opens `http://127.0.0.1:8080/` in the default browser.
4. The app uses loopback only and does not require internet access, an editor, administrator rights or a first-run build. Port 8080 must be available.

To create the Windows package on Windows 11 x64, open PowerShell in the source project and run:

```powershell
.\scripts\package_windows.ps1
```

This requires CMake, a supported x64 C++ toolchain (Visual Studio 2022 Build Tools or equivalent), and Windows SDK. The script builds, tests and stages `dist\CandidateID_Workbench`. Verify the extracted staged package on a clean Windows 11 x64 machine before submission. The `.vscode` launch configuration requires the Microsoft C/C++ extension and starts the packaged `Launch.exe`; it does not rebuild the application.

## Source build (CMake)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Windows with Visual Studio 2022, use `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`, then `cmake --build build --config Release` and `ctest --test-dir build -C Release --output-on-failure`. With the MSYS2 UCRT64 MinGW toolchain, use `cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release` (Ninja required), then `cmake --build build --parallel` and `ctest --test-dir build --output-on-failure`. The Windows packaging script detects MinGW/Ninja or falls back to Visual Studio 2022.

Run the CLI demonstrations from the project root:

```bash
./build/workbench demo
./build/workbench check-stl data/sample_cube_ascii.stl
./build/workbench check-csv data/sample_temperature.csv
```

Start the HTTP application from the project root with `./build/uew_server`. It serves the frontend at `http://127.0.0.1:8080/`. For test isolation, set `UEW_PORT` to an unused port. The frontend uses local JavaScript, CSS and Canvas APIs; no CDN, remote chart package or external font is required.

## Architecture

- `include/`, `src/core/`: public C++ APIs, 1D thermal finite elements, ASCII STL and nodal CSV validation, manual backpropagation.
- `src/modules/`: fixed-seed CPU benchmark and stateful MCU/FPGA/JTAG mock adapters.
- `src/app/`: CLI, loopback HTTP server and Windows launcher.
- `frontend/`: responsive single-page UI, interactive 3D projection/point-cloud inspection, result plots and export controls.
- `tests/`: deterministic regression and negative tests.
- `data/`: self-created ASCII STL fixture and explicitly synthetic nodal temperature CSV; see `data/PROVENANCE.md`.
- `results/`: raw result CSVs and plots. Benchmark values are machine-specific; regenerate on the target PC.
- `docs/`: assignment traceability, engineering method, report, validation and AI-use disclosure.

## Supported inputs and limits

- ASCII STL only, up to 20 MiB. Binary STL, native CAD solids and native ANSYS project/result formats are out of scope.
- CSV header must be exactly `node_id,x_m,y_m,z_m,temperature_C` (UTF-8/ASCII text; CRLF accepted). Node IDs must be positive unique integers. Coordinates and temperature must be finite. Maximum file size is 20 MiB.
- The nodal CSV viewer is a 3D coloured point cloud with temperature legend and click-to-inspect. STL viewer supports drag rotation, shift-drag pan, wheel zoom and double-click fit. Mesh-to-result registration is not implemented.
- Thermal solver: steady-state 1D rod, uniform two-node linear elements, constant properties, uniform generation, insulated lateral boundary and prescribed end temperatures.
- Hardware is explicitly simulated. No physical MCU, FPGA or JTAG adapter is accessed.

## Numerical methods and benchmark

Thermal element stiffness is `(kA/h)[[1,-1],[-1,1]]`; consistent element source load is `(qAh/2)[1,1]^T`. Prescribed temperatures are eliminated from the interior system. The exact solution is compared with the linearly interpolated FE temperature at element midpoints. Required mesh sizes are N=10,20,40,80.

The benchmark uses independently generated positive conductivity/source cases from a fixed seed, identical serial and parallel solvers, a bounded 1–4 worker pool, a warm-up per worker configuration and five measured repetitions. It reports median compute time, speedup, efficiency and the maximum absolute difference across **all nodes of all cases**. The UI exposes case count, mesh size and seed. Timing excludes file I/O and rendering. Results depend on CPU, compiler, build flags and host load.

Backpropagation implements the specified single-sample tanh regression by hand, checks each of the four initial gradients using central finite differences (`epsilon=1e-6`), and exports loss over 100 updates. The candidate must still produce and submit their own handwritten derivation appendix.

## Validation and remaining limitations

See `docs/requirements_audit.md` and `docs/validation_log.md`. A timestamped demonstration recording plan is provided in `docs/demo_walkthrough_template.md`; it is a template, not captured evidence. The Linux build/test/API checks do not prove Windows packaging. The candidate must record target-machine CPU model, physical/logical core counts, RAM, compiler and optimization flags, and measure launch-to-ready time separately. A screenshot walkthrough or 3–5 minute demo video is still required. A handwritten two-page backprop appendix is candidate-authored evidence and is not generated by this project.

## Dependencies and licensing

C++17 standard library, CMake, and the vendored `cpp-httplib` single-header HTTP library (`third_party/httplib.h`; license notice in `third_party/httplib.txt`). No third-party runtime frontend dependency. Build-time plot generation uses Python 3, Matplotlib and ReportLab; these are not runtime requirements. See the dependency/version record in `docs/dependencies.md`.

## Run on Windows (localhost)

Open this project folder in VS Code, open a terminal at the folder containing `CMakeLists.txt`, and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Run_Workbench.ps1
```

The script selects Visual Studio 2022 x64 or MSYS2 UCRT64 (not legacy MinGW GCC 6.3), creates a separate `build-windows` directory, builds and tests the backend, then starts the HTTP server from the project root. When it prints the server URL, open **http://127.0.0.1:8080/** in Chrome. Keep the terminal open; press `Ctrl+C` there to stop the server. See `docs/WINDOWS_LOCALHOST.md` for prerequisites and troubleshooting.



Windows runner note: `scripts/Run_Workbench.ps1` builds `uew_server`, `workbench`, and `workbench_tests` before running CTest. This ensures the test executable exists in a fresh Windows build directory.
