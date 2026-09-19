#include "ram.h"

#include <windows.h>

#include <vector>

void gatherRAM(SysInfo& si) {
    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        si.ramTotal = ms.ullTotalPhys;
        si.ramAvail = ms.ullAvailPhys;
    }
}

void gatherRAMDetails(SysInfo& si) {
    DWORD totalSize = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
    if (totalSize <= 8) return;

    std::vector<BYTE> rawBuf(totalSize);
    if (GetSystemFirmwareTable('RSMB', 0, rawBuf.data(), totalSize) != totalSize) return;

    BYTE* data = rawBuf.data() + 8;
    DWORD dataLen = totalSize - 8;

    DWORD pos = 0;
    while (pos + 4 <= dataLen) {
        BYTE type   = data[pos];
        BYTE length = data[pos + 1];

        if (length < 4) break;
        if (pos + length > dataLen) break;

        if (type == 17 && length >= 0x17) {
            si.ramSlotsTotal++;

            WORD rawSize = *(WORD*)(data + pos + 0x0C);
            if (rawSize != 0 && rawSize != 0xFFFF) {
                si.ramSlotsUsed++;

                BYTE memType = data[pos + 0x12];
                WORD speed   = *(WORD*)(data + pos + 0x15);

                switch (memType) {
                    case 18: si.ramType = "DDR";  break;
                    case 19: si.ramType = "DDR2"; break;
                    case 24: si.ramType = "DDR3"; break;
                    case 26: si.ramType = "DDR4"; break;
                    case 34: si.ramType = "DDR5"; break;
                }

                if (speed > 0 && speed != 0xFFFF && (int)speed > si.ramSpeedMHz)
                    si.ramSpeedMHz = speed;

                if (length >= 0x22 && pos + 0x22 <= dataLen) {
                    WORD configSpeed = *(WORD*)(data + pos + 0x20);
                    if (configSpeed > 0 && configSpeed != 0xFFFF)
                        si.ramSpeedMHz = configSpeed;
                }
            }
        }

        DWORD next = pos + length;
        while (next + 1 < dataLen && !(data[next] == 0 && data[next + 1] == 0))
            next++;
        next += 2;

        if (next <= pos + length) break;
        pos = next;
    }
}
