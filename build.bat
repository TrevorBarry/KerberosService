@echo off
setlocal

echo Building Kerberos Echo Service...

REM Check if Visual Studio is available
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio C++ compiler (cl.exe) not found in PATH
    echo Please run this from a Visual Studio Developer Command Prompt
    echo or run vcvars64.bat first
    pause
    exit /b 1
)

REM Compile the service
echo Compiling...
cl /EHsc /std:c++17 ^
   /DWIN32_LEAN_AND_MEAN ^
   /DSECURITY_WIN32 ^
   /D_CRT_SECURE_NO_WARNINGS ^
   main.cpp ^
   WindowsService.cpp ^
   HttpServer.cpp ^
   KerberosAuth.cpp ^
   /Fe:KerberosEchoService.exe ^
   httpapi.lib ^
   secur32.lib

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful! Created KerberosEchoService.exe
echo.
echo Next steps:
echo 1. Run as Administrator and reserve URL: netsh http add urlacl url=http://+:8080/ user=Everyone
echo 2. Install service: KerberosEchoService.exe install
echo 3. Start service: sc start KerberosEchoService
echo.
echo Or test in console mode: KerberosEchoService.exe console
echo.

REM Clean up intermediate files
del *.obj 2>nul

pause