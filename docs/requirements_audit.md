# Assignment requirement-by-requirement audit

Audit basis: PASS Science Works, *Unified Engineering Workbench — Common Internship Assignment, R&D Internship 2026–2027*, three pages. Status is based on source inspection plus the Linux build, CTest and HTTP checks recorded in `validation_log.md`.

| Assignment requirement | Status | Evidence / remaining work |
|---|---|---|
| C++17 backend for numerical computation, parsing and concurrency | Complete in source; Linux build verified | `src/core`, `src/modules`, CMake. Frontend is presentation only. |
| One coherent Geometry, Thermal, Performance and Hardware UI | Implemented; HTTP tested | `frontend/index.html`, `frontend/app.js`; UI browser visual inspection on Windows still pending. |
| File selection, validation, progress/status, actionable errors and export | Implemented; endpoint tested | Upload APIs, status areas, loading states and result CSV links. No user-cancellable compute endpoint; calculations finish into local buffers before response. |
| Responsive computation / safe completion | Partial | Browser requests are asynchronous and results are assembled before response. No explicit cancel token; atomic replacement of all result files is not yet implemented. |
| Separate core from presentation/transport | Complete | `uew_core` is independent of server and browser code. |
| Windows 11 x64 standalone `Launch.exe`, no first-run build/internet/admin/editor | Source and package script present; NOT VERIFIED | `src/app/launcher.cpp`, `scripts/package_windows.ps1`. Must build and test extracted package on clean Windows 11 x64. This is a hard shortlist gate. |
| VS Code/Antigravity one-action configuration | Implemented, unverified on Windows | `.vscode/launch.json` and `.vscode/tasks.json`; Microsoft C/C++ extension prerequisite. Starts packaged launcher. |
| CMake source-build instructions | Complete | README. Linux Release build verified. |
| ASCII STL import, triangles and interactive view | Implemented; C++ endpoint tested | 20 MiB cap; drag rotate, shift-drag pan, wheel zoom, double-click fit. Only ASCII STL is supported. |
| ANSYS-style nodal temperature CSV schema | Implemented; C++ endpoint tested | Exact header, CRLF, finite values, positive unique integer IDs; synthetic fixture explicitly labelled. |
| 3D temperature point cloud, legend, units and node inspection | Implemented in frontend source; endpoint tested | Canvas projection, temperature colour scale and click-to-inspect. Browser visual check still pending. |
| Malformed/non-finite/empty data handling and file limits | Implemented; negative parser tests pass | ASCII STL and nodal CSV validators; 20 MiB documented limit. |
| 1D thermal FEM formulation, assembly and Dirichlet conditions | Complete; numerical tests pass | Two-node linear elements; exact required constants; elimination of boundary DOFs. |
| N=10,20,40,80 profiles and midpoint error | Complete; exact values verified | Errors 1.25, 0.3125, 0.078125, 0.01953125 °C; raw CSV and plots. |
| Mesh convergence and analytical comparison | Complete | Error reduces by factor four on each mesh doubling; midpoint FE interpolation used. |
| Serial and CPU-parallel independent thermal batch | Implemented; CTest and API verified | Fixed-seed cases; workers 1–4 (capped at logical CPU count); same solver and deterministic case order. |
| Benchmark five repetitions, median, speedup, efficiency, agreement | Implemented; measured | Warm-up per worker count, five runs, median; maximum absolute difference over every nodal temperature in every case. |
| CPU model, physical/logical cores, RAM, compiler and flags | Audit environment recorded; candidate machine pending | Current container: AMD EPYC 9V74; 5 logical CPUs exposed, 5 cores/1 socket visible; ~5.8 GiB RAM visible; GCC 14.2.0, CMake Release `-O3 -DNDEBUG`. This is not the candidate's Windows PC. |
| Launch-to-ready time separately | Pending | Measure on target Windows PC; benchmark compute excludes file I/O/rendering/compilation. |
| MCU, FPGA and JTAG integration design | Implemented; mock state tests pass | Device-specific assumptions, power/ground/voltage/level-shifting and JTAG ownership table in `embedded_design.md`; block diagram in results. Generic board classes only. |
| Mock discovery/capabilities/connect/disconnect/telemetry and recoverable failure | Implemented; C++ transition tests pass | Stateful mock adapters; JTAG timeout is recoverable by retry. All simulated, no physical board. |
| Manual backprop chain rule and simultaneous update | C++ implementation complete; candidate handwritten appendix pending | `backprop.cpp`, `backprop_appendix.md`; handwritten derivation/photo must be created and understood by candidate. |
| Central finite-difference check epsilon 1e-6, tolerance 1e-5 | Complete; verified | Observed max absolute gradient difference about 1.12e-10. |
| Loss plot over 100 subsequent updates | Implemented; generated | CSV includes initial state and 100 update steps. |
| Required ZIP contents, report <=6 pages, appendix <=2 pages | Partial | Project contents/docs present. Candidate details, handwritten appendix, demo evidence and final Windows package pending. |
| 3–5 minute video or timestamped screenshot walkthrough with logs | Pending | Must be recorded by candidate on the final Windows package. |
| Git bundle/local repository and readable commit log | In progress | Source ZIP had no `.git`; repository is being initialized from supplied snapshot and changes will be committed with provenance clear. |
| AI-use log, critical judgement and ownership statement | Partial | AI log records known tools and validation. Candidate must verify ownership statement and fill actual effort; do not claim unknown prompts or fabricated interactions. |

## Genuine blockers before submission

1. Windows x64 compilation, packaging, runtime dependency audit and clean-machine `Launch.exe` test require a Windows toolchain/host. The current environment cannot certify that gate.
2. Candidate-specific machine benchmark metadata and launch-to-ready measurement must be captured on the target Windows PC.
3. Candidate must produce the handwritten two-page backprop appendix and a 3–5 minute demonstration or timestamped screenshot walkthrough (recording plan: `demo_walkthrough_template.md`).
4. Candidate name/ID, actual effort and final ownership statement must be completed by the candidate.
