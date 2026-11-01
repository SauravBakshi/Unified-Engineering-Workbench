# Unified Engineering Workbench — Technical Report

**Candidate:** [Candidate to complete]  
**Candidate ID:** [Candidate to complete]  
**Version:** 1.1 takeover prototype  
**Actual effort:** [Candidate to record honestly]

## 1. Design and architecture

The workbench is a local Windows-targeted engineering prototype. C++17 owns numerical computation, file validation, concurrency and embedded-device simulation. A loopback HTTP server provides transport; a local HTML/CSS/JavaScript dashboard provides presentation. The core library has no dependency on the browser or HTTP layer. The dashboard is offline-capable and uses project-owned Canvas drawing rather than remote CDN libraries.

## 2. Thermal FEA verification

The model is a steady-state, one-dimensional uniform rod with L=1 m, A=1e-4 m², k=10 W/(m·K), q=10,000 W/m³, T(0)=100 °C and T(L)=20 °C. Two-node linear elements assemble `Ke=(kA/h)[[1,-1],[-1,1]]` and `fe=(qAh/2)[1,1]^T`. Both prescribed temperatures are eliminated from the interior system. The analytical solution is `T(x)=100−80x+500x(1−x)` °C. Error is evaluated at element midpoints using FE linear interpolation.

| Elements | Maximum midpoint error (°C) |
|---:|---:|
| 10 | 1.25000000 |
| 20 | 0.31250000 |
| 40 | 0.07812500 |
| 80 | 0.01953125 |

Halving element size reduces the maximum midpoint error by a factor of four. The exact boundary temperatures are preserved. The model assumes constant properties, uniform generation, perfect continuity and insulated lateral surfaces.

## 3. Multicore performance

A fixed-seed generator creates independent cases with positive conductivity and heat generation. Serial and bounded worker-pool implementations solve the same cases and preserve input order. Warm-up is excluded; five compute-only repetitions are measured and median time is reported. The agreement check compares every nodal temperature across every case.

Audit-container measurement: 2,000 cases, 200 elements, seed 2026; 1/2/3/4 workers gave median times 8.492121/4.203106/3.553149/2.305053 ms, speedups 1.000/2.020/2.390/3.684, and maximum absolute serial/parallel difference 0 °C. These measurements are environment-specific. Container metadata: AMD EPYC 9V74 80-Core Processor; 5 logical CPUs and 5 visible cores; about 5.81 GiB memory visible; GCC 14.2.0; CMake Release with `-O3 -DNDEBUG`. Candidate must replace or supplement these with their Windows-PC measurements and measure launch-to-ready time separately.

## 4. Geometry and embedded integration

The C++ parser accepts ASCII STL and nodal CSV (`node_id,x_m,y_m,z_m,temperature_C`) up to 20 MiB. It rejects empty, malformed and non-finite input; CSV node IDs must be positive unique integers. The dashboard provides an interactive projected 3D triangle view and temperature-coloured point cloud with node inspection. Native CAD solids, binary STL, native ANSYS formats and mesh/result registration are not supported.

MCU, FPGA and JTAG adapters are stateful C++ mocks. The design separates USB CDC/UART telemetry, FPGA board-specific host communication and JTAG debug transport. A JTAG timeout is deliberately injected and recovered by retry. Device pinouts and I/O bank voltages are board-specific; the integration note explains grounding, reference voltage, level shifting and signal ownership. No physical hardware is connected.

## 5. Backpropagation

The model is `h=tanh(wx+b)`, `ŷ=vh+c`, `J=0.5(ŷ−y)^2`, with x=1, y=2, initial parameters `[0.5,0,1,0]` and learning rate 0.1. The C++ code computes all four chain-rule gradients without automatic differentiation. Central finite differences use epsilon 1e-6; the observed maximum absolute difference is 1.117548276e-10, below the 1e-5 tolerance. Loss falls from 1.182541818997 to about 1.70e-20 after 100 updates. The candidate must independently handwrite the derivation and one simultaneous update on a maximum two-page appendix.

## 6. Validation, limitations and delivery

The Linux Release build and CTest suite pass. HTTP checks cover all primary endpoints, both upload formats, device transitions, thermal result exports, benchmark exports and backpropagation. Full details are in `validation_log.md`; the requirement matrix is in `requirements_audit.md`.

The Windows launcher source and packaging script are included, but no Windows executable or clean-machine Windows 11 test can be certified from the Linux build environment. That remains a hard submission/shortlisting gate. Also pending: candidate PC benchmark and launch timing, visual check of the browser on Windows, handwritten appendix, demonstration evidence, candidate ID/name, actual effort and candidate verification of the ownership statement.
