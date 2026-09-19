# sysinfo

Windows console app (C++20) that prints a system report (OS, CPU, RAM, GPU, disks, motherboard, PSU, temperatures) and also saves it as `sysinfo.txt` on the Desktop.

Requires **Administrator** (UAC).

```powershell
& "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" sysinfo.vcxproj /p:Configuration=Release /p:Platform=x64
```

Binary: `x64\Release\sysinfo.exe`. A prebuilt exe is in the repo **Latest** release.
