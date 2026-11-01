#include "embedded.hpp"
#include <mutex>
#include <stdexcept>

namespace {
std::vector<DeviceStatus> initial_devices() {
    return {
      {"Microcontroller", "Generic Cortex-M MCU", "Discovered (SIMULATED)",
       "USB CDC/UART mock adapter ready.", "GPIO, ADC, UART telemetry; USB CDC host link"},
      {"FPGA", "Generic Artix-class FPGA", "Discovered (SIMULATED)",
       "Board-specific USB-UART mock adapter ready.", "Programmable logic fabric; board-specific JTAG/UART bridge"},
      {"JTAG adapter", "Generic USB-JTAG probe", "Timeout (SIMULATED)",
       "Mock probe timed out after 1000 ms; retry is available.", "TCK, TMS, TDI, TDO, GND and target-reference sensing"}
    };
}
}
std::vector<DeviceStatus> demo_embedded_devices() { return initial_devices(); }
std::vector<DeviceStatus> update_embedded_device(const std::string& action, int id) {
    static std::mutex guard;
    static std::vector<DeviceStatus> devices = initial_devices();
    std::lock_guard<std::mutex> lock(guard);
    if (action.empty()) return devices;
    if (id < 0 || id >= static_cast<int>(devices.size()))
        throw std::invalid_argument("Device id must be 0, 1 or 2.");
    auto& d=devices[static_cast<std::size_t>(id)];
    if(action=="connect") {
        d.state="Connected (SIMULATED)";d.message="Mock host connection established.";
    } else if(action=="disconnect") {
        d.state="Disconnected (SIMULATED)";d.message="Mock connection closed cleanly.";
    } else if(action=="retry") {
        if(d.type!="JTAG adapter") throw std::invalid_argument("Retry recovery is demonstrated on the JTAG timeout mock.");
        d.state="Connected after retry (SIMULATED)";d.message="Retry recovered the injected 1000 ms timeout.";
    } else if(action=="telemetry") {
        if(d.state.find("Connected")==std::string::npos) throw std::runtime_error("Connect the simulated device before requesting telemetry.");
        d.message="Telemetry (SIMULATED): temperature=24.6 C; voltage=3.3 V; uptime=1042 s";
    } else throw std::invalid_argument("Supported actions: connect, disconnect, telemetry, retry.");
    return devices;
}
