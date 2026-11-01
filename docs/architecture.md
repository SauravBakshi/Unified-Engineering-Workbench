# Architecture
- `include/`: stable public interfaces for the core.
- `src/core/`: numerical FEA, format validation and manual backpropagation.
- `src/modules/`: bounded worker-pool benchmark and embedded mock adapters.
- `src/app/`: CLI transport/orchestration only.
- `frontend/`: browser presentation and lightweight preview.
- `tests/`: deterministic smoke/regression tests.
The core has no UI dependency. The frontend is intentionally decoupled; it displays exported results and validates a quick preview. The C++ parser remains authoritative.
