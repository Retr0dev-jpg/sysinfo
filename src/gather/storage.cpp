#include "storage.h"

#include "util.h"

#include <windows.h>

#include <string>

void gatherDisks(SysInfo& si) {
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(drives & (1 << i))) continue;
        char letter = 'A' + i;
        std::string root = std::string(1, letter) + ":\\";
        std::wstring wroot(root.begin(), root.end());

        UINT type = GetDriveTypeW(wroot.c_str());
        if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) continue;

        ULARGE_INTEGER total = {}, free_ = {}, freeCaller = {};
        if (!GetDiskFreeSpaceExW(wroot.c_str(), &freeCaller, &total, &free_)) continue;

        wchar_t volLabel[MAX_PATH] = {};
        wchar_t fsType[32] = {};
        GetVolumeInformationW(wroot.c_str(), volLabel, MAX_PATH,
            nullptr, nullptr, nullptr, fsType, 32);

        SysInfo::DiskInfo d;
        d.letter = letter;
        d.label = wcharToUtf8(volLabel);
        if (d.label.empty()) d.label = "Local Disk";
        d.fsType = wcharToUtf8(fsType);
        d.total = total.QuadPart;
        d.free_ = free_.QuadPart;
        d.used = d.total - d.free_;
        si.disks.push_back(d);
    }
}
