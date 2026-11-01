#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

static void show_error(const std::string& message) {
    MessageBoxA(nullptr, message.c_str(), "Unified Engineering Workbench", MB_OK | MB_ICONERROR);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    char module[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, module, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        show_error("Cannot determine the Launch.exe location.");
        return 1;
    }

    const std::filesystem::path root = std::filesystem::path(module).parent_path();
    const auto server = root / "uew_server.exe";
    const auto frontend = root / "frontend" / "index.html";
    if (!std::filesystem::exists(server) || !std::filesystem::exists(frontend)) {
        show_error("The application package is incomplete. Launch.exe must be beside uew_server.exe and the frontend folder.");
        return 2;
    }

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::string command = "\"" + server.string() + "\"";
    if (!CreateProcessA(server.string().c_str(), command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, root.string().c_str(), &startup, &process)) {
        show_error("Windows could not start the C++ backend. Check the bundled runtime DLLs and whether port 8080 is available.\n\nWindows error: " + std::to_string(GetLastError()));
        return 3;
    }
    CloseHandle(process.hThread);

    // Do not open the browser if the server exits immediately (e.g. missing DLL or occupied port).
    for (int attempt = 0; attempt < 30; ++attempt) {
        const DWORD state = WaitForSingleObject(process.hProcess, 200);
        if (state == WAIT_OBJECT_0) {
            DWORD exitCode = 1;
            GetExitCodeProcess(process.hProcess, &exitCode);
            CloseHandle(process.hProcess);
            show_error("The backend stopped before becoming available. Exit code: " + std::to_string(exitCode) +
                       ". Run uew_server.exe from a terminal to see the detailed error, or check port 8080.");
            return 4;
        }
        if (attempt >= 4) break;
    }
    CloseHandle(process.hProcess);
    ShellExecuteA(nullptr, "open", "http://127.0.0.1:8080/", nullptr, root.string().c_str(), SW_SHOWNORMAL);
    return 0;
}
#else
int main() { return 0; }
#endif
