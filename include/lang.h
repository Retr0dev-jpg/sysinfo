#pragma once

// ============================================================================
// lang.h  —  All user-visible strings for sysinfo
//
// To add a new language: copy this file, change the constants, and swap the
// include in any TU that includes lang.h.
// ============================================================================

namespace Lang {

// ── Titles ────────────────────────────────────────────────────────────────────

inline constexpr const char* TITLE_APP     = "sysinfo v2.6";
inline constexpr const char* TITLE_OS      = "Operating System";
inline constexpr const char* TITLE_CPU     = "CPU";
inline constexpr const char* TITLE_THERMAL = "Temperature";
inline constexpr const char* TITLE_RAM     = "RAM";
inline constexpr const char* TITLE_GPU     = "GPU & DirectX";
inline constexpr const char* TITLE_STORAGE = "Storage";
inline constexpr const char* TITLE_MB      = "Motherboard";
inline constexpr const char* TITLE_PSU     = "Power Supply";

// ── OS ────────────────────────────────────────────────────────────────────────

inline constexpr const char* OS_LBL_NAME        = "Name:";
inline constexpr const char* OS_LBL_VERSION     = "Version:";
inline constexpr const char* OS_LBL_EDITION     = "Edition:";
inline constexpr const char* OS_LBL_SECURE_BOOT = "Secure Boot:";
inline constexpr const char* OS_SB_LEGACY       = "Not detected (legacy BIOS)";
inline constexpr const char* OS_SB_ENABLED      = "ENABLED";
inline constexpr const char* OS_SB_DISABLED     = "DISABLED";

// ── CPU ───────────────────────────────────────────────────────────────────────

inline constexpr const char* CPU_LBL_MODEL   = "Model:";
inline constexpr const char* CPU_LBL_VENDOR  = "Vendor:";
inline constexpr const char* CPU_LBL_DETAILS = "Details:";
inline constexpr const char* CPU_LBL_NPU     = "NPU:";
inline constexpr const char* CPU_LBL_USAGE   = "Usage:";
inline constexpr const char* CPU_SAMPLE_NOTE = "sampled over 2 seconds";
inline constexpr const char* CPU_NPU_NONE    = "Not detected";
inline constexpr const char* CPU_CORES       = "C / ";
inline constexpr const char* CPU_THREADS     = "T   ";
inline constexpr const char* CPU_GHZ         = " GHz";

// ── Thermal ───────────────────────────────────────────────────────────────────

inline constexpr const char* THERMAL_NO_DATA = "No thermal data.";
inline constexpr const char* THERMAL_UNIT    = "\u00b0C";

// ── RAM ───────────────────────────────────────────────────────────────────────

inline constexpr const char* RAM_LBL_TOTAL  = "Total:";
inline constexpr const char* RAM_LBL_AVAIL  = "Available:";
inline constexpr const char* RAM_LBL_TYPE   = "Type:";
inline constexpr const char* RAM_LBL_SLOTS  = "Slots:";
inline constexpr const char* RAM_LBL_USED   = "Used:";
inline constexpr const char* RAM_SPEED_SEP  = " @ ";
inline constexpr const char* RAM_SPEED_UNIT = " MT/s";
inline constexpr const char* RAM_SLOTS_USED = " used";
inline constexpr const char* RAM_SLOTS_FREE = " available";

// ── GPU ───────────────────────────────────────────────────────────────────────

inline constexpr const char* GPU_NONE         = "No dedicated GPU detected";
inline constexpr const char* GPU_PREFIX       = "GPU ";
inline constexpr const char* GPU_LBL_MODEL    = "Model:";
inline constexpr const char* GPU_LBL_VRAM     = "VRAM:";
inline constexpr const char* GPU_LBL_DRIVER   = "Driver:";
inline constexpr const char* GPU_LBL_DIRECTX  = "DirectX:";
inline constexpr const char* GPU_VRAM_SHARED  = "Shared / N/A";
inline constexpr const char* GPU_VRAM_UNIT    = " dedicated";

// ── Storage ───────────────────────────────────────────────────────────────────

inline constexpr const char* STORAGE_NONE = "No fixed drives detected.";

// ── Motherboard ───────────────────────────────────────────────────────────────

inline constexpr const char* MB_LBL_MODEL  = "Model:";
inline constexpr const char* MB_LBL_VENDOR = "Vendor:";
inline constexpr const char* MB_LBL_BIOS   = "BIOS:";
inline constexpr const char* MB_REV_PREFIX = "  (rev ";
inline constexpr const char* MB_REV_SUFFIX = ")";

// ── PSU ───────────────────────────────────────────────────────────────────────

inline constexpr const char* PSU_LBL_MODEL       = "Model:";
inline constexpr const char* PSU_LBL_MAX_POWER   = "Max Power:";
inline constexpr const char* PSU_WATTS           = " W";
inline constexpr const char* PSU_DETECTED_NOINFO = "Detected (no model info)";
inline constexpr const char* PSU_NO_SMBIOS       = "Not reported by firmware (SMBIOS Type 39)";
inline constexpr const char* PSU_NOTE            = "Most consumer motherboards do not expose PSU data";

// ── Common ────────────────────────────────────────────────────────────────────

inline constexpr const char* NA                = "N/A";
inline constexpr const char* SEP_SLASH         = " / ";
inline constexpr const char* EXIT_PROMPT       = "Press Enter to exit...";
inline constexpr const char* REPORT_FILENAME   = "sysinfo.txt";
inline constexpr const char* REPORT_SAVED      = "Saved:";
inline constexpr const char* REPORT_SAVE_FAIL  = "Could not save report to Desktop.";

} // namespace Lang
