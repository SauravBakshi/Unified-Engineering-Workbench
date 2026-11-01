#pragma once
#include <string>
#include <vector>
struct DeviceStatus {
    std::string type, name, state, message, capabilities;
};
std::vector<DeviceStatus> demo_embedded_devices();
std::vector<DeviceStatus> update_embedded_device(const std::string& action, int id);
