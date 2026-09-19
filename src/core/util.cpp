#include "util.h"

#include <shlobj.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

std::string formatBytes(unsigned long long bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && i < 4) { val /= 1024.0; i++; }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << val << " " << units[i];
    return ss.str();
}

std::string toLowerStr(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

std::string cleanVendorName(const std::string& provider) {
    std::string lower = toLowerStr(provider);
    if (lower.find("nvidia") != std::string::npos) return "NVIDIA";
    if (lower.find("amd") != std::string::npos || lower.find("advanced micro") != std::string::npos)
        return "AMD";
    if (lower.find("intel") != std::string::npos) return "Intel";
    if (lower.find("qualcomm") != std::string::npos) return "Qualcomm";
    return provider;
}

std::string regGetString(HKEY root, const wchar_t* subkey, const wchar_t* value) {
    HKEY hk;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return "N/A";
    wchar_t buf[512] = {};
    DWORD sz = sizeof(buf);
    DWORD type = 0;
    LONG res = RegQueryValueExW(hk, value, nullptr, &type, (LPBYTE)buf, &sz);
    RegCloseKey(hk);
    if (res != ERROR_SUCCESS) return "N/A";
    int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buf, -1, &out[0], len, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

DWORD regGetDword(HKEY root, const wchar_t* subkey, const wchar_t* value) {
    HKEY hk;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hk) != ERROR_SUCCESS) return 0xFFFFFFFF;
    DWORD val = 0, sz = sizeof(val);
    LONG res = RegQueryValueExW(hk, value, nullptr, nullptr, (LPBYTE)&val, &sz);
    RegCloseKey(hk);
    return (res == ERROR_SUCCESS) ? val : 0xFFFFFFFF;
}

std::string wcharToUtf8(const wchar_t* wstr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &out[0], len, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

std::string writeTextToDesktop(const std::string& filename, const std::string& utf8Text) {
    PWSTR desk = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desk)) || !desk)
        return {};
    std::wstring path(desk);
    CoTaskMemFree(desk);
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
        path += L'\\';

    int n = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0);
    if (n <= 0) return {};
    std::vector<wchar_t> wfn((size_t)n);
    MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfn.data(), n);
    path += wfn.data();

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return {};
    const char bom[] = { '\xEF', '\xBB', '\xBF' };
    out.write(bom, 3);
    out.write(utf8Text.data(), (std::streamsize)utf8Text.size());
    if (!out) return {};
    return wcharToUtf8(path.c_str());
}
