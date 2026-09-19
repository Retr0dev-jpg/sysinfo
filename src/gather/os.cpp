#include "os.h"

#include "util.h"

#include <windows.h>

#include <string>

void gatherOS(SysInfo& si) {
    si.osName = regGetString(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        L"ProductName"
    );
    si.osEdition = regGetString(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        L"EditionID"
    );
    si.osBuild = regGetString(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        L"CurrentBuildNumber"
    );
    DWORD ubr = regGetDword(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        L"UBR"
    );
    std::string displayVersion = regGetString(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        L"DisplayVersion"
    );
    si.osVersion = displayVersion + "  (Build " + si.osBuild;
    if (ubr != 0xFFFFFFFF) si.osVersion += "." + std::to_string(ubr);
    si.osVersion += ")";
}

void gatherSecureBoot(SysInfo& si) {
    DWORD val = regGetDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
        L"UEFISecureBootEnabled"
    );
    si.secureBootDetected = (val != 0xFFFFFFFF);
    si.secureBoot = (val == 1);
}
