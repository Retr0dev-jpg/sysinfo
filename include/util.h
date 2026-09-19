#pragma once

#include <windows.h>
#include <string>

#pragma comment(lib, "advapi32.lib")

std::string formatBytes(unsigned long long bytes);
std::string toLowerStr(const std::string& s);
std::string cleanVendorName(const std::string& provider);

std::string regGetString(HKEY root, const wchar_t* subkey, const wchar_t* value);
DWORD regGetDword(HKEY root, const wchar_t* subkey, const wchar_t* value);
std::string wcharToUtf8(const wchar_t* wstr);
std::string writeTextToDesktop(const std::string& filename, const std::string& utf8Text);
