#pragma once

#include <string>
#include <vector>

struct GPUInfo {
    std::string name;
    std::string vendor;
    unsigned long long vramDedicated = 0;
    std::string driverVersion;
};

struct SysInfo {
    // CPU
    std::string cpuName;
    std::string cpuVendor;
    int         cpuCores = 0;
    int         cpuThreads = 0;
    double      cpuSpeedGHz = 0.0;
    double      cpuUsagePct = 0.0;

    // NPU
    std::string npuName;
    bool        npuDetected = false;

    // RAM
    unsigned long long ramTotal = 0;
    unsigned long long ramAvail = 0;
    int         ramSpeedMHz = 0;
    std::string ramType;
    int         ramSlotsTotal = 0;
    int         ramSlotsUsed = 0;

    // OS
    std::string osName;
    std::string osBuild;
    std::string osVersion;
    std::string osEdition;

    // GPU(s)
    std::vector<GPUInfo> gpus;

    // DirectX
    std::string directXVersion;

    // Secure boot
    bool secureBoot = false;
    bool secureBootDetected = false;

    // Motherboard
    std::string mbManufacturer;
    std::string mbProduct;
    std::string mbVersion;

    // BIOS
    std::string biosVendor;
    std::string biosVersion;
    std::string biosDate;

    // PSU (SMBIOS Type 39)
    std::string psuManufacturer;
    std::string psuModel;
    int         psuMaxWatts = 0;
    bool        psuDetected = false;

    // Disks
    struct DiskInfo {
        char   letter = '\0';
        std::string label;
        unsigned long long total = 0;
        unsigned long long used = 0;
        unsigned long long free_ = 0;
        std::string fsType;
    };
    std::vector<DiskInfo> disks;

    // Temperature (WMI, one reading per zone after dedupe)
    struct TempSensor {
        std::string source;
        std::string name;
        std::string property;
        double      celsius = 0.0;
        bool        valid = false;
    };
    std::vector<TempSensor> tempSensors;
    std::string             thermalWmiNote;
};
