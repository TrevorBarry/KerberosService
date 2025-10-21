#include "WindowsService.h"
#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[])
{
    const std::wstring SERVICE_NAME = L"KerberosEchoService";
    const std::wstring DISPLAY_NAME = L"Kerberos Echo HTTP Service";

    WindowsService service(SERVICE_NAME, DISPLAY_NAME);

    if (argc > 1)
    {
        std::wstring arg = argv[1];
        
        if (arg == L"install" || arg == L"/install" || arg == L"-install")
        {
            if (service.InstallService())
            {
                std::wcout << L"Service installed successfully." << std::endl;
                std::wcout << L"Start the service with: sc start " << SERVICE_NAME << std::endl;
            }
            return 0;
        }
        else if (arg == L"uninstall" || arg == L"/uninstall" || arg == L"-uninstall")
        {
            if (service.UninstallService())
            {
                std::wcout << L"Service uninstalled successfully." << std::endl;
            }
            return 0;
        }
        else if (arg == L"console" || arg == L"/console" || arg == L"-console")
        {
            std::wcout << L"Running in console mode..." << std::endl;
            std::wcout << L"Press Ctrl+C to stop." << std::endl;
            
            if (service.Initialize())
            {
                service.Run();
            }
            return 0;
        }
        else if (arg == L"help" || arg == L"/help" || arg == L"-help" || arg == L"/?")
        {
            std::wcout << L"Usage: " << argv[0] << L" [option]" << std::endl;
            std::wcout << L"Options:" << std::endl;
            std::wcout << L"  install   - Install the service" << std::endl;
            std::wcout << L"  uninstall - Uninstall the service" << std::endl;
            std::wcout << L"  console   - Run in console mode for testing" << std::endl;
            std::wcout << L"  help      - Show this help" << std::endl;
            std::wcout << L"" << std::endl;
            std::wcout << L"When run without arguments, starts as a Windows service." << std::endl;
            std::wcout << L"" << std::endl;
            std::wcout << L"Prerequisites:" << std::endl;
            std::wcout << L"1. Run as Administrator to install/uninstall" << std::endl;
            std::wcout << L"2. Reserve URL with: netsh http add urlacl url=http://+:8080/ user=Everyone" << std::endl;
            std::wcout << L"3. Ensure machine is joined to domain for Kerberos authentication" << std::endl;
            return 0;
        }
    }

    // Start as service (default behavior when no arguments)
    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        { const_cast<LPWSTR>(SERVICE_NAME.c_str()), WindowsService::ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(serviceTable))
    {
        DWORD error = GetLastError();
        if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
        {
            std::wcout << L"Error: Cannot start as service. Run with 'console' argument for testing." << std::endl;
            std::wcout << L"Use '" << argv[0] << L" help' for more options." << std::endl;
        }
        else
        {
            std::wcout << L"StartServiceCtrlDispatcher failed with error: " << error << std::endl;
        }
        return 1;
    }

    return 0;
}