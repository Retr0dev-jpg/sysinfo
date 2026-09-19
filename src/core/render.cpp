#include "lang.h"
#include "render.h"
#include "util.h"

#include <iostream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <cstring>

namespace {

void section(std::ostream& out, const char* title) {
    out << title << '\n';
    out << std::string(std::strlen(title), '-') << '\n';
}

void line(std::ostream& out, const char* label, const std::string& value) {
    out << label << ' ' << value << '\n';
}

std::string pctStr(double pct) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << pct * 100.0 << "%";
    return ss.str();
}

void printOs(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_OS);
    const char* sb = Lang::OS_SB_LEGACY;
    if (si.secureBootDetected)
        sb = si.secureBoot ? Lang::OS_SB_ENABLED : Lang::OS_SB_DISABLED;
    line(out, Lang::OS_LBL_NAME, si.osName);
    line(out, Lang::OS_LBL_VERSION, si.osVersion);
    line(out, Lang::OS_LBL_EDITION, si.osEdition);
    line(out, Lang::OS_LBL_SECURE_BOOT, sb);
    out << '\n';
}

void printCpu(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_CPU);
    std::ostringstream details;
    details << std::fixed << std::setprecision(2)
            << si.cpuCores << Lang::CPU_CORES << si.cpuThreads << Lang::CPU_THREADS
            << si.cpuSpeedGHz << Lang::CPU_GHZ;
    line(out, Lang::CPU_LBL_MODEL, si.cpuName);
    line(out, Lang::CPU_LBL_VENDOR, si.cpuVendor);
    line(out, Lang::CPU_LBL_DETAILS, details.str());
    line(out, Lang::CPU_LBL_NPU, si.npuDetected ? si.npuName : Lang::CPU_NPU_NONE);
    line(out, Lang::CPU_LBL_USAGE, pctStr(si.cpuUsagePct) + " (" + Lang::CPU_SAMPLE_NOTE + ")");
    out << '\n';
}

void printRam(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_RAM);
    unsigned long long ramUsed = si.ramTotal - si.ramAvail;
    double ramPct = si.ramTotal > 0 ? (double)ramUsed / si.ramTotal : 0.0;

    std::ostringstream typeStr;
    if (!si.ramType.empty()) {
        typeStr << si.ramType;
        if (si.ramSpeedMHz > 0) typeStr << Lang::RAM_SPEED_SEP << si.ramSpeedMHz << Lang::RAM_SPEED_UNIT;
    } else {
        typeStr << Lang::NA;
    }

    std::ostringstream slotStr;
    if (si.ramSlotsTotal > 0) {
        slotStr << si.ramSlotsUsed << Lang::SEP_SLASH << si.ramSlotsTotal << Lang::RAM_SLOTS_USED;
        int free_ = si.ramSlotsTotal - si.ramSlotsUsed;
        if (free_ > 0) slotStr << "  (" << free_ << Lang::RAM_SLOTS_FREE << ")";
    } else {
        slotStr << Lang::NA;
    }

    line(out, Lang::RAM_LBL_TOTAL, formatBytes(si.ramTotal));
    line(out, Lang::RAM_LBL_AVAIL, formatBytes(si.ramAvail));
    line(out, Lang::RAM_LBL_TYPE, typeStr.str());
    line(out, Lang::RAM_LBL_SLOTS, slotStr.str());
    line(out, Lang::RAM_LBL_USED, pctStr(ramPct) + " (" + formatBytes(ramUsed)
        + Lang::SEP_SLASH + formatBytes(si.ramTotal) + ")");
    out << '\n';
}

void printGpu(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_GPU);
    if (si.gpus.empty()) {
        out << Lang::GPU_NONE << '\n';
    } else {
        for (int i = 0; i < (int)si.gpus.size(); i++) {
            const auto& gpu = si.gpus[i];
            std::string prefix = (si.gpus.size() > 1)
                ? (std::string(Lang::GPU_PREFIX) + std::to_string(i + 1) + ":")
                : Lang::GPU_LBL_MODEL;
            std::string label = gpu.name;
            if (!gpu.vendor.empty()) label += "  [" + gpu.vendor + "]";
            line(out, prefix.c_str(), label);

            std::string vramStr = gpu.vramDedicated > 0
                ? formatBytes(gpu.vramDedicated) + Lang::GPU_VRAM_UNIT
                : Lang::GPU_VRAM_SHARED;
            line(out, Lang::GPU_LBL_VRAM, vramStr);
            if (!gpu.driverVersion.empty())
                line(out, Lang::GPU_LBL_DRIVER, gpu.driverVersion);
        }
    }
    line(out, Lang::GPU_LBL_DIRECTX, si.directXVersion);
    out << '\n';
}

void printStorage(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_STORAGE);
    if (si.disks.empty()) {
        out << Lang::STORAGE_NONE << '\n';
    } else {
        for (int i = 0; i < (int)si.disks.size(); i++) {
            const auto& d = si.disks[i];
            double usedPct = d.total > 0 ? (double)d.used / d.total : 0.0;
            std::string driveLabel = std::string(1, d.letter) + ":  " + d.label + "  [" + d.fsType + "]";
            out << driveLabel << '\n';
            line(out, Lang::RAM_LBL_USED, formatBytes(d.used) + Lang::SEP_SLASH + formatBytes(d.total)
                + " (" + pctStr(usedPct) + ")");
            if (i < (int)si.disks.size() - 1) out << '\n';
        }
    }
    out << '\n';
}

void printMb(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_MB);
    std::string mbLabel = si.mbManufacturer.empty() ? Lang::NA
        : si.mbManufacturer + "  " + si.mbProduct;
    if (!si.mbVersion.empty() && si.mbVersion != Lang::NA)
        mbLabel += std::string(Lang::MB_REV_PREFIX) + si.mbVersion + Lang::MB_REV_SUFFIX;

    std::string biosLabel;
    if (!si.biosVersion.empty()) {
        biosLabel = si.biosVersion;
        if (!si.biosDate.empty())   biosLabel += "  (" + si.biosDate + ")";
        if (!si.biosVendor.empty()) biosLabel += "  [" + si.biosVendor + "]";
    } else {
        biosLabel = Lang::NA;
    }

    line(out, Lang::MB_LBL_MODEL, mbLabel);
    line(out, Lang::MB_LBL_VENDOR, si.mbManufacturer.empty() ? Lang::NA : si.mbManufacturer);
    line(out, Lang::MB_LBL_BIOS, biosLabel);
    out << '\n';
}

void printPsu(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_PSU);
    if (si.psuDetected) {
        std::string psuLabel = si.psuManufacturer.empty() ? "" : si.psuManufacturer + "  ";
        psuLabel += si.psuModel.empty() ? "" : si.psuModel;
        if (psuLabel.empty()) psuLabel = Lang::PSU_DETECTED_NOINFO;
        line(out, Lang::PSU_LBL_MODEL, psuLabel);
        std::string wattsStr = si.psuMaxWatts > 0
            ? std::to_string(si.psuMaxWatts) + Lang::PSU_WATTS
            : Lang::NA;
        line(out, Lang::PSU_LBL_MAX_POWER, wattsStr);
    } else {
        line(out, Lang::PSU_LBL_MODEL, Lang::PSU_NO_SMBIOS);
        out << Lang::PSU_NOTE << '\n';
    }
    out << '\n';
}

void printThermal(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_THERMAL);
    if (si.tempSensors.empty()) {
        std::string msg = si.thermalWmiNote.empty() ? Lang::THERMAL_NO_DATA : si.thermalWmiNote;
        out << msg << '\n';
    } else {
        for (const auto& t : si.tempSensors) {
            std::ostringstream val;
            val << std::fixed << std::setprecision(1) << t.celsius << Lang::THERMAL_UNIT;
            std::string label = t.name + ":";
            line(out, label.c_str(), val.str());
        }
    }
    out << '\n';
}

void writeReport(std::ostream& out, const SysInfo& si) {
    section(out, Lang::TITLE_APP);
    out << '\n';
    printOs(out, si);
    printCpu(out, si);
    printRam(out, si);
    printGpu(out, si);
    printStorage(out, si);
    printMb(out, si);
    printPsu(out, si);
    printThermal(out, si);
}

} // namespace

std::string formatReport(const SysInfo& si) {
    std::ostringstream out;
    writeReport(out, si);
    return out.str();
}

void printReport(const SysInfo& si) {
    std::string text = formatReport(si);
    std::cout << text;
    std::string saved = writeTextToDesktop(Lang::REPORT_FILENAME, text);
    if (!saved.empty())
        std::cout << Lang::REPORT_SAVED << ' ' << saved << '\n';
    else
        std::cout << Lang::REPORT_SAVE_FAIL << '\n';
}
