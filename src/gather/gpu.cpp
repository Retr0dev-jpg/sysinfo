#include "gpu.h"

#include "util.h"

#include <windows.h>

#include <string>

void gatherGPU(SysInfo& si) {
    HKEY hkDisplay;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
        0, KEY_READ, &hkDisplay) != ERROR_SUCCESS) {
        return;
    }

    wchar_t subName[16];
    DWORD idx = 0;
    while (true) {
        DWORD nameLen = sizeof(subName) / sizeof(wchar_t);
        if (RegEnumKeyExW(hkDisplay, idx++, subName, &nameLen,
            nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        HKEY hkAdapter;
        std::wstring path = std::wstring(
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\")
            + subName;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hkAdapter) != ERROR_SUCCESS)
            continue;

        wchar_t desc[256] = {};
        DWORD sz = sizeof(desc);
        if (RegQueryValueExW(hkAdapter, L"DriverDesc", nullptr, nullptr, (LPBYTE)desc, &sz) == ERROR_SUCCESS) {
            std::wstring ws(desc);
            if (ws.find(L"Microsoft") == std::wstring::npos &&
                ws.find(L"Remote") == std::wstring::npos) {

                GPUInfo gpu;
                gpu.name = wcharToUtf8(desc);

                wchar_t provider[256] = {};
                DWORD psz = sizeof(provider);
                if (RegQueryValueExW(hkAdapter, L"ProviderName", nullptr, nullptr,
                    (LPBYTE)provider, &psz) == ERROR_SUCCESS) {
                    gpu.vendor = cleanVendorName(wcharToUtf8(provider));
                }

                ULONGLONG vram = 0;
                DWORD vsz = sizeof(vram);
                if (RegQueryValueExW(hkAdapter, L"HardwareInformation.qwMemorySize",
                    nullptr, nullptr, (LPBYTE)&vram, &vsz) == ERROR_SUCCESS && vram > 0) {
                    gpu.vramDedicated = vram;
                }
                else {
                    DWORD vram32 = 0;
                    DWORD vsz32 = sizeof(vram32);
                    if (RegQueryValueExW(hkAdapter, L"HardwareInformation.MemorySize",
                        nullptr, nullptr, (LPBYTE)&vram32, &vsz32) == ERROR_SUCCESS) {
                        gpu.vramDedicated = (unsigned long long)vram32;
                        if (gpu.vramDedicated < 1024 * 1024)
                            gpu.vramDedicated *= 1024ULL * 1024ULL;
                    }
                }

                wchar_t drvVer[256] = {};
                DWORD dvSz = sizeof(drvVer);
                if (RegQueryValueExW(hkAdapter, L"DriverVersion", nullptr, nullptr,
                    (LPBYTE)drvVer, &dvSz) == ERROR_SUCCESS) {
                    gpu.driverVersion = wcharToUtf8(drvVer);
                }

                si.gpus.push_back(gpu);
            }
        }
        RegCloseKey(hkAdapter);
    }
    RegCloseKey(hkDisplay);
}

void gatherDirectX(SysInfo& si) {
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    std::wstring sys = sysDir;

    if (GetFileAttributesW((sys + L"\\d3d12.dll").c_str()) != INVALID_FILE_ATTRIBUTES)
        si.directXVersion = "DirectX 12";
    else if (GetFileAttributesW((sys + L"\\d3d11.dll").c_str()) != INVALID_FILE_ATTRIBUTES)
        si.directXVersion = "DirectX 11";
    else if (GetFileAttributesW((sys + L"\\d3d10.dll").c_str()) != INVALID_FILE_ATTRIBUTES)
        si.directXVersion = "DirectX 10";
    else if (GetFileAttributesW((sys + L"\\d3d9.dll").c_str()) != INVALID_FILE_ATTRIBUTES)
        si.directXVersion = "DirectX 9";
    else
        si.directXVersion = "N/A";

    std::string regVer = regGetString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\DirectX", L"Version");
    if (regVer != "N/A" && !regVer.empty())
        si.directXVersion += "  (v" + regVer + ")";
}
