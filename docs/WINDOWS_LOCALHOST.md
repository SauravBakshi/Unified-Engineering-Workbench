# Run the Workbench on Windows (localhost)

## Easiest method

1. Open the `Unified_Engineering_Workbench` folder in VS Code.
2. Open **Terminal → New Terminal** and ensure the terminal is at the project root (the folder containing `CMakeLists.txt`).
3. Run:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Run_Workbench.ps1
   ```

4. Wait until the script prints `Starting C++ backend` and the server URL.
5. Open Chrome and visit **http://127.0.0.1:8080/**.
6. Keep the terminal open while using the app. Press **Ctrl+C** in that terminal to stop it.

You can alternatively double-click `scripts\Start_Localhost.bat`.

## Compiler selection

The script prefers Visual Studio 2022 with the **Desktop development with C++** workload. If that is unavailable, it uses MSYS2 UCRT64 GCC and Ninja from `C:\msys64`. It deliberately does not use the legacy `C:\MinGW\bin\g++.exe` GCC 6.3 toolchain. It builds in a new `build-windows` directory and does not overwrite the existing `build` directory or its working `workbench.exe`.

For MSYS2 UCRT64, install the compiler and Ninja from the UCRT64 shell:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

CMake, CTest, and the backend build must succeed before the server starts. If port 8080 is occupied, close the other process and rerun. The server uses project-relative `frontend/` and `results/` paths, so it must be launched with the project root as its working directory; the supplied script enforces this.
