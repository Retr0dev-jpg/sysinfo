#include "cpu.h"

#include "util.h"

#include <windows.h>
#include <intrin.h>

#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

void gatherCPU(SysInfo& si) {
    int cpuInfo[4] = {};
    __cpuid(cpuInfo, 0);
    char vendor[13] = {};
    *reinterpret_cast<int*>(vendor)     = cpuInfo[1];
    *reinterpret_cast<int*>(vendor + 4) = cpuInfo[3];
    *reinterpret_cast<int*>(vendor + 8) = cpuInfo[2];
    vendor[12] = '\0';

    if (strstr(vendor, "GenuineIntel"))  si.cpuVendor = "Intel";
    else if (strstr(vendor, "AuthenticAMD")) si.cpuVendor = "AMD";
    else si.cpuVendor = vendor;

    si.cpuName = regGetString(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"ProcessorNameString"
    );
    size_t s = si.cpuName.find_first_not_of(' ');
    size_t e = si.cpuName.find_last_not_of(' ');
    if (s != std::string::npos) si.cpuName = si.cpuName.substr(s, e - s + 1);

    DWORD mhz = regGetDword(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"~MHz"
    );
    si.cpuSpeedGHz = (mhz != 0xFFFFFFFF) ? mhz / 1000.0 : 0.0;

    SYSTEM_INFO sysInfo = {};
    GetSystemInfo(&sysInfo);
    si.cpuThreads = (int)sysInfo.dwNumberOfProcessors;

    DWORD bufLen = 0;
    GetLogicalProcessorInformation(nullptr, &bufLen);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(bufLen / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (GetLogicalProcessorInformation(buf.data(), &bufLen)) {
        int cores = 0;
        for (auto& info : buf)
            if (info.Relationship == RelationProcessorCore) cores++;
        si.cpuCores = cores;
    }
    else {
        si.cpuCores = si.cpuThreads;
    }
}

void gatherCPUUsage(SysInfo& si) {
    FILETIME idle1, kern1, usr1, idle2, kern2, usr2;
    GetSystemTimes(&idle1, &kern1, &usr1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    GetSystemTimes(&idle2, &kern2, &usr2);
    auto ft2ull = [](FILETIME ft) -> unsigned long long {
        return ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    };
    unsigned long long idle = ft2ull(idle2) - ft2ull(idle1);
    unsigned long long kern = ft2ull(kern2) - ft2ull(kern1);
    unsigned long long usr  = ft2ull(usr2)  - ft2ull(usr1);
    unsigned long long total = kern + usr;
    si.cpuUsagePct = (total > 0) ? (double)(total - idle) / total : 0.0;
}

void gatherNPU(SysInfo& si) {
    const wchar_t* enumRoots[] = {
        L"SYSTEM\\CurrentControlSet\\Enum\\PCI",
        L"SYSTEM\\CurrentControlSet\\Enum\\ACPI",
    };

    for (int r = 0; r < 2 && !si.npuDetected; r++) {
        HKEY hkRoot;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, enumRoots[r], 0, KEY_READ, &hkRoot) != ERROR_SUCCESS)
            continue;

        wchar_t devKey[512];
        DWORD dIdx = 0;
        while (!si.npuDetected) {
            DWORD dkLen = 512;
            if (RegEnumKeyExW(hkRoot, dIdx++, devKey, &dkLen,
                nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            HKEY hkDev;
            if (RegOpenKeyExW(hkRoot, devKey, 0, KEY_READ, &hkDev) != ERROR_SUCCESS)
                continue;

            wchar_t instKey[512];
            DWORD iIdx = 0;
            while (!si.npuDetected) {
                DWORD ikLen = 512;
                if (RegEnumKeyExW(hkDev, iIdx++, instKey, &ikLen,
                    nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                    break;

                std::wstring fullPath = std::wstring(enumRoots[r]) + L"\\" + devKey + L"\\" + instKey;

                std::string name = regGetString(HKEY_LOCAL_MACHINE, fullPath.c_str(), L"FriendlyName");
                if (name == "N/A" || name.empty()) {
                    name = regGetString(HKEY_LOCAL_MACHINE, fullPath.c_str(), L"DeviceDesc");
                    size_t semi = name.rfind(';');
                    if (semi != std::string::npos)
                        name = name.substr(semi + 1);
                }

                std::string lower = toLowerStr(name);
                if (lower.find("npu") != std::string::npos ||
                    lower.find("neural") != std::string::npos ||
                    lower.find("ai boost") != std::string::npos ||
                    lower.find("xdna") != std::string::npos ||
                    lower.find("ipu device") != std::string::npos ||
                    lower.find("myriad") != std::string::npos ||
                    lower.find("hexagon") != std::string::npos) {
                    si.npuName = name;
                    si.npuDetected = true;
                }
            }
            RegCloseKey(hkDev);
        }
        RegCloseKey(hkRoot);
    }
}
