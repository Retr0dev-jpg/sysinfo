#include "motherboard.h"

#include <windows.h>

#include <string>
#include <vector>

void gatherMotherboardAndPSU(SysInfo& si) {
    DWORD totalSize = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
    if (totalSize <= 8) return;

    std::vector<BYTE> rawBuf(totalSize);
    if (GetSystemFirmwareTable('RSMB', 0, rawBuf.data(), totalSize) != totalSize) return;

    BYTE* data = rawBuf.data() + 8;
    DWORD dataLen = totalSize - 8;

    auto getSmStr = [](BYTE* base, BYTE length, DWORD maxLen, BYTE idx) -> std::string {
        if (idx == 0) return "";
        BYTE* strArea = base + length;
        BYTE curIdx = 1;
        while (curIdx < idx) {
            while (*strArea != 0 && (strArea - base) < (ptrdiff_t)maxLen) strArea++;
            strArea++;
            if (*strArea == 0) return "";
            curIdx++;
        }
        return std::string(reinterpret_cast<char*>(strArea));
    };

    DWORD pos = 0;
    bool biosDone = false, mbDone = false, psuDone = false;
    while (pos + 4 <= dataLen) {
        BYTE type   = data[pos];
        BYTE length = data[pos + 1];
        if (length < 4) break;
        if (pos + length > dataLen) break;

        if (type == 0 && !biosDone && length >= 9) {
            BYTE vendor  = data[pos + 4];
            BYTE version = data[pos + 5];
            BYTE date    = data[pos + 8];
            si.biosVendor  = getSmStr(data + pos, length, dataLen - pos, vendor);
            si.biosVersion = getSmStr(data + pos, length, dataLen - pos, version);
            si.biosDate    = getSmStr(data + pos, length, dataLen - pos, date);
            biosDone = true;
        }

        if (type == 2 && !mbDone && length >= 8) {
            BYTE mfr  = data[pos + 4];
            BYTE prod = data[pos + 5];
            BYTE ver  = data[pos + 6];
            si.mbManufacturer = getSmStr(data + pos, length, dataLen - pos, mfr);
            si.mbProduct      = getSmStr(data + pos, length, dataLen - pos, prod);
            si.mbVersion      = getSmStr(data + pos, length, dataLen - pos, ver);
            mbDone = true;
        }

        if (type == 39 && !psuDone && length >= 0x16) {
            BYTE mfrIdx   = data[pos + 7];
            BYTE modelIdx = data[pos + 9];
            WORD maxPwr   = *(WORD*)(data + pos + 0x12);

            si.psuManufacturer = getSmStr(data + pos, length, dataLen - pos, mfrIdx);
            si.psuModel        = getSmStr(data + pos, length, dataLen - pos, modelIdx);
            if (maxPwr != 0x8000 && maxPwr > 0)
                si.psuMaxWatts = maxPwr;
            si.psuDetected = true;
            psuDone = true;
        }

        DWORD next = pos + length;
        while (next + 1 < dataLen && !(data[next] == 0 && data[next + 1] == 0))
            next++;
        next += 2;
        if (next <= pos + length) break;
        pos = next;
    }
}
