#include "thermal.h"

#include "util.h"

#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>

#include <cctype>
#include <cmath>
#include <string>
#include <vector>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace {

bool variantToDouble(const VARIANT& v, double* out) {
    switch (v.vt) {
        case VT_I4:  *out = (double)v.lVal; return true;
        case VT_UI4: *out = (double)v.ulVal; return true;
        case VT_I2:  *out = (double)v.iVal; return true;
        case VT_UI2: *out = (double)v.uiVal; return true;
        case VT_R8:  *out = v.dblVal; return true;
        case VT_R4:  *out = (double)v.fltVal; return true;
        case VT_BSTR:
        case VT_NULL:
        case VT_EMPTY:
        default: return false;
    }
}

std::wstring bstrToW(BSTR b) {
    if (!b) return L"";
    return std::wstring(b, SysStringLen(b));
}

std::string bstrToUtf(BSTR b) {
    std::wstring ws = bstrToW(b);
    int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string s((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), n, nullptr, nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

void addSensor(SysInfo& si, const std::string& source, const std::string& name,
    const std::string& prop, double celsius, bool valid) {
    SysInfo::TempSensor t;
    t.source = source;
    t.name = name;
    t.property = prop;
    t.celsius = celsius;
    t.valid = valid;
    si.tempSensors.push_back(std::move(t));
}

bool isCurrentTemperatureProp(const std::string& prop) {
    std::string l = toLowerStr(prop);
    if (l.find("temp") == std::string::npos) return false;
    if (l.find("percent") != std::string::npos) return false;
    if (l.find("throttle") != std::string::npos) return false;
    if (l.find("limit") != std::string::npos) return false;
    if (l.find("reason") != std::string::npos) return false;
    if (l.find("trip") != std::string::npos) return false;
    if (l.find("critical") != std::string::npos) return false;
    if (l.find("passive") != std::string::npos) return false;
    return true;
}

int temperatureQuality(const std::string& prop) {
    std::string l = toLowerStr(prop);
    if (l.find("highprecision") != std::string::npos) return 3;
    if (l.find("temp") != std::string::npos) return 2;
    return 1;
}

bool plausibleCelsius(double c) {
    return std::isfinite(c) && c >= -20.0 && c <= 150.0;
}

std::string lastPathSegment(const std::string& s) {
    size_t i = s.find_last_of("\\/");
    return (i == std::string::npos) ? s : s.substr(i + 1);
}

std::string shortZoneName(std::string n) {
    size_t br = n.find(" [");
    if (br != std::string::npos) n.resize(br);
    n = lastPathSegment(n);
    size_t dot = n.find_last_of('.');
    if (dot != std::string::npos) n = n.substr(dot + 1);
    size_t us = n.find_last_of('_');
    if (us != std::string::npos && us + 1 < n.size()) {
        bool allDigit = true;
        for (size_t i = us + 1; i < n.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(n[i]))) {
                allDigit = false;
                break;
            }
        }
        if (allDigit) n.resize(us);
    }
    return n.empty() ? "Thermal zone" : n;
}

void compactSensors(SysInfo& si) {
    std::vector<SysInfo::TempSensor> out;
    for (auto& t : si.tempSensors) {
        if (toLowerStr(t.source) == "monitor")
            t.name = "Monitor";
        else
            t.name = shortZoneName(t.name);

        bool merged = false;
        for (auto& o : out) {
            if (toLowerStr(o.name) != toLowerStr(t.name)) continue;
            if (temperatureQuality(t.property) > temperatureQuality(o.property))
                o = t;
            merged = true;
            break;
        }
        if (!merged) out.push_back(std::move(t));
    }
    si.tempSensors = std::move(out);
}

double guessCelsiusFromRaw(double raw) {
    if (!std::isfinite(raw)) return raw;
    if (raw >= 200.0 && raw <= 500.0)
        return raw - 273.15;
    if (raw > 500.0 && raw < 5000.0)
        return raw / 10.0 - 273.15;
    if (raw > 1000.0)
        return (raw / 10.0) - 273.15;
    return raw;
}

HRESULT setBlanket(IUnknown* p) {
    return CoSetProxyBlanket(p, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
}

IWbemServices* connectServer(const wchar_t* namespacePath) {
    IWbemLocator* pLoc = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<void**>(&pLoc));
    if (FAILED(hr) || !pLoc) return nullptr;

    IWbemServices* pSvc = nullptr;
    hr = pLoc->ConnectServer(_bstr_t(namespacePath), nullptr, nullptr, nullptr, 0,
        nullptr, nullptr, &pSvc);
    pLoc->Release();
    if (FAILED(hr) || !pSvc) return nullptr;

    if (FAILED(setBlanket(pSvc))) {
        pSvc->Release();
        return nullptr;
    }
    return pSvc;
}

void gatherMSAcpi(SysInfo& si, IWbemClassObject* pObj) {
    VARIANT vName = {}, vTemp = {};
    if (pObj->Get(L"InstanceName", 0, &vName, nullptr, nullptr) != S_OK) return;
    if (pObj->Get(L"CurrentTemperature", 0, &vTemp, nullptr, nullptr) != S_OK) {
        VariantClear(&vName);
        return;
    }
    std::string name = (vName.vt == VT_BSTR) ? bstrToUtf(vName.bstrVal) : "N/A";
    double raw = 0;
    if (!variantToDouble(vTemp, &raw)) {
        VariantClear(&vName);
        VariantClear(&vTemp);
        return;
    }
    VariantClear(&vName);
    VariantClear(&vTemp);
    double c = (raw / 10.0) - 273.15;
    if (!plausibleCelsius(c)) return;
    addSensor(si, "ACPI", name, "CurrentTemperature", c, true);
}

void gatherTemperatureProbes(SysInfo& si, IWbemClassObject* pObj) {
    VARIANT vName = {}, vDesc = {}, vRead = {};
    pObj->Get(L"Name", 0, &vName, nullptr, nullptr);
    pObj->Get(L"Description", 0, &vDesc, nullptr, nullptr);
    if (pObj->Get(L"CurrentReading", 0, &vRead, nullptr, nullptr) != S_OK) {
        VariantClear(&vName);
        VariantClear(&vDesc);
        return;
    }
    std::string name = (vName.vt == VT_BSTR && vName.bstrVal) ? bstrToUtf(vName.bstrVal) : "";
    std::string desc = (vDesc.vt == VT_BSTR && vDesc.bstrVal) ? bstrToUtf(vDesc.bstrVal) : "";
    if (name.empty()) name = desc.empty() ? "TemperatureProbe" : desc;
    double raw = 0;
    if (!variantToDouble(vRead, &raw)) {
        VariantClear(&vName);
        VariantClear(&vDesc);
        VariantClear(&vRead);
        return;
    }
    VariantClear(&vName);
    VariantClear(&vDesc);
    VariantClear(&vRead);
    double c = raw / 10.0;
    if (!plausibleCelsius(c)) return;
    addSensor(si, "Probe", name, "CurrentReading", c, true);
}

void gatherNumericSensors(SysInfo& si, IWbemClassObject* pObj) {
    VARIANT vName = {}, vRead = {};
    pObj->Get(L"Name", 0, &vName, nullptr, nullptr);
    if (pObj->Get(L"CurrentReading", 0, &vRead, nullptr, nullptr) != S_OK) {
        VariantClear(&vName);
        return;
    }
    std::string name = (vName.vt == VT_BSTR && vName.bstrVal) ? bstrToUtf(vName.bstrVal) : "NumericSensor";
    double raw = 0;
    if (!variantToDouble(vRead, &raw)) {
        VariantClear(&vName);
        VariantClear(&vRead);
        return;
    }
    VariantClear(&vName);
    VariantClear(&vRead);
    double c = raw / 10.0;
    if (!plausibleCelsius(c)) return;
    addSensor(si, "Sensor", name, "CurrentReading", c, true);
}

bool tryAddPerfTemp(SysInfo& si, IWbemClassObject* pObj, const std::string& instName,
    const wchar_t* propW, const char* prop) {
    if (!isCurrentTemperatureProp(prop)) return false;
    VARIANT v = {};
    if (pObj->Get(propW, 0, &v, nullptr, nullptr) != S_OK) return false;
    double raw = 0;
    bool ok = variantToDouble(v, &raw);
    VariantClear(&v);
    if (!ok) return false;
    double c = guessCelsiusFromRaw(raw);
    if (!plausibleCelsius(c)) return false;
    addSensor(si, "ACPI", instName, prop, c, true);
    return true;
}

void gatherPerfThermalZone(SysInfo& si, IWbemClassObject* pObj) {
    VARIANT vName = {};
    std::string instName;
    if (pObj->Get(L"Name", 0, &vName, nullptr, nullptr) == S_OK && vName.vt == VT_BSTR && vName.bstrVal)
        instName = bstrToUtf(vName.bstrVal);
    VariantClear(&vName);
    if (instName.empty()) instName = "Thermal zone";

    if (tryAddPerfTemp(si, pObj, instName, L"HighPrecisionTemperature", "HighPrecisionTemperature"))
        return;
    tryAddPerfTemp(si, pObj, instName, L"Temperature", "Temperature");
}

} // namespace

void gatherThermal(SysInfo& si) {
    si.thermalWmiNote.clear();
    si.tempSensors.clear();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        si.thermalWmiNote = "COM init failed (thermal)";
        return;
    }
    const bool didCoInit = (hr == S_OK);

    HRESULT sec = CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(sec) && sec != RPC_E_TOO_LATE) {
        si.thermalWmiNote = "CoInitializeSecurity failed";
        if (didCoInit) CoUninitialize();
        return;
    }

    IWbemServices* pWmi = connectServer(L"ROOT\\WMI");
    if (pWmi) {
        IEnumWbemClassObject* pEnum = nullptr;
        hr = pWmi->ExecQuery(bstr_t("WQL"),
            bstr_t(L"SELECT * FROM MSAcpi_ThermalZoneTemperature"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
        if (SUCCEEDED(hr) && pEnum) {
            for (;;) {
                IWbemClassObject* pObj = nullptr;
                ULONG ret = 0;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret);
                if (FAILED(hr) || ret == 0) break;
                gatherMSAcpi(si, pObj);
                pObj->Release();
            }
            pEnum->Release();
        }

        pEnum = nullptr;
        hr = pWmi->ExecQuery(bstr_t("WQL"),
            bstr_t(L"SELECT * FROM WmiMonitorTemperature"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
        if (SUCCEEDED(hr) && pEnum) {
            for (;;) {
                IWbemClassObject* pObj = nullptr;
                ULONG ret = 0;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret);
                if (FAILED(hr) || ret == 0) break;
                VARIANT vName = {}, vTemp = {};
                std::string name = "Monitor";
                if (pObj->Get(L"InstanceName", 0, &vName, nullptr, nullptr) == S_OK && vName.vt == VT_BSTR)
                    name = bstrToUtf(vName.bstrVal);
                VariantClear(&vName);
                if (pObj->Get(L"CurrentTemperature", 0, &vTemp, nullptr, nullptr) == S_OK) {
                    double raw = 0;
                    if (variantToDouble(vTemp, &raw)) {
                        double c = (raw / 10.0) - 273.15;
                        if (plausibleCelsius(c))
                            addSensor(si, "Monitor", name, "CurrentTemperature", c, true);
                    }
                }
                VariantClear(&vTemp);
                pObj->Release();
            }
            pEnum->Release();
        }

        pWmi->Release();
    }

    IWbemServices* pCim = connectServer(L"root\\cimv2");
    if (pCim) {
        IEnumWbemClassObject* pEnum = nullptr;
        hr = pCim->ExecQuery(bstr_t("WQL"),
            bstr_t(L"SELECT * FROM Win32_TemperatureProbe"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
        if (SUCCEEDED(hr) && pEnum) {
            for (;;) {
                IWbemClassObject* pObj = nullptr;
                ULONG ret = 0;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret);
                if (FAILED(hr) || ret == 0) break;
                gatherTemperatureProbes(si, pObj);
                pObj->Release();
            }
            pEnum->Release();
        }

        pEnum = nullptr;
        hr = pCim->ExecQuery(bstr_t("WQL"),
            bstr_t(L"SELECT * FROM Win32_NumericSensor WHERE SensorType = 4"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
        if (SUCCEEDED(hr) && pEnum) {
            for (;;) {
                IWbemClassObject* pObj = nullptr;
                ULONG ret = 0;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret);
                if (FAILED(hr) || ret == 0) break;
                gatherNumericSensors(si, pObj);
                pObj->Release();
            }
            pEnum->Release();
        }

        pEnum = nullptr;
        hr = pCim->ExecQuery(bstr_t("WQL"),
            bstr_t(L"SELECT * FROM Win32_PerfFormattedData_Counters_ThermalZoneInformation"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
        if (SUCCEEDED(hr) && pEnum) {
            for (;;) {
                IWbemClassObject* pObj = nullptr;
                ULONG ret = 0;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret);
                if (FAILED(hr) || ret == 0) break;
                gatherPerfThermalZone(si, pObj);
                pObj->Release();
            }
            pEnum->Release();
        }

        pCim->Release();
    }

    compactSensors(si);

    if (si.tempSensors.empty() && si.thermalWmiNote.empty())
        si.thermalWmiNote = "No thermal sensors exposed via WMI on this system.";

    if (didCoInit) CoUninitialize();
}
