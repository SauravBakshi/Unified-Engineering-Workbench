# Dependency and build-version record

| Component | Version / source | Runtime requirement |
|---|---|---|
| C++ language | C++17 | Yes |
| CMake | Minimum 3.16 | Build only |
| cpp-httplib | 0.64.0, vendored `third_party/httplib.h` | Statically compiled into server |
| Threads | C++ standard threading library | Yes; OS runtime |
| Browser rendering | HTML Canvas 2D, CSS and JavaScript | Default browser |
| Charting | Project-owned Canvas plotting routines | No external package |
| 3D preview | Project-owned 3D-to-2D projection and pointer controls | No external package |
| Plot regeneration | Python 3 + Matplotlib | Build/evidence generation only |

The vendored cpp-httplib header declares the MIT License and copyright notice. No other third-party frontend runtime assets are loaded. The Windows packaging configuration uses the static MSVC runtime setting; MinGW builds request static libgcc/libstdc++. Confirm the exact compiler/toolchain used for the final Windows build in the report.
