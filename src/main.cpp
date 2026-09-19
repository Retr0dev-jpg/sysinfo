#include <windows.h>
#include <shellapi.h>

#include <iostream>

#include "cpu.h"
#include "lang.h"
#include "gpu.h"
#include "motherboard.h"
#include "os.h"
#include "ram.h"
#include "render.h"
#include "storage.h"
#include "thermal.h"
#include "types.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    BOOL isAdmin = FALSE;
    PSID adminSid = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminSid)) {
        CheckTokenMembership(nullptr, adminSid, &isAdmin);
        FreeSid(adminSid);
    }
    if (!isAdmin) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize  = sizeof(sei);
        sei.lpVerb  = L"runas";
        sei.lpFile  = path;
        sei.nShow   = SW_SHOWNORMAL;
        sei.fMask   = SEE_MASK_NOCLOSEPROCESS;
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        ShellExecuteExW(&sei);
        if (sei.hProcess && sei.hProcess != INVALID_HANDLE_VALUE)
            CloseHandle(sei.hProcess);
        return 0;
    }

    SetConsoleOutputCP(CP_UTF8);

    SysInfo si;
    gatherOS(si);
    gatherSecureBoot(si);
    gatherCPU(si);
    gatherRAM(si);
    gatherRAMDetails(si);
    gatherGPU(si);
    gatherNPU(si);
    gatherDirectX(si);
    gatherDisks(si);
    gatherThermal(si);
    gatherMotherboardAndPSU(si);
    gatherCPUUsage(si);

    printReport(si);

    DWORD pidBuf = 0;
    DWORD attached = GetConsoleProcessList(&pidBuf, 1);
    if (attached <= 1) {
        std::cout << Lang::EXIT_PROMPT << std::flush;
        std::cin.get();
    }
    return 0;
}
