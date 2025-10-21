#include "WindowsService.h"
#include "HttpServer.h"
#include <iostream>

WindowsService* WindowsService::s_instance = nullptr;

WindowsService::WindowsService(const std::wstring& serviceName, const std::wstring& displayName)
    : m_serviceName(serviceName)
    , m_displayName(displayName)
    , m_statusHandle(nullptr)
    , m_running(false)
{
    s_instance = this;
    
    ZeroMemory(&m_status, sizeof(m_status));
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
}

WindowsService::~WindowsService()
{
    s_instance = nullptr;
}

void WINAPI WindowsService::ServiceMain(DWORD argc, LPWSTR* argv)
{
    if (s_instance)
    {
        s_instance->m_statusHandle = RegisterServiceCtrlHandler(
            s_instance->m_serviceName.c_str(), 
            ServiceCtrlHandler
        );

        if (s_instance->m_statusHandle)
        {
            s_instance->ReportServiceStatus(SERVICE_START_PENDING);
            
            if (s_instance->Initialize())
            {
                s_instance->ReportServiceStatus(SERVICE_RUNNING);
                s_instance->Run();
            }
            else
            {
                s_instance->ReportServiceStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
            }
        }
    }
}

void WINAPI WindowsService::ServiceCtrlHandler(DWORD ctrl)
{
    if (s_instance)
    {
        switch (ctrl)
        {
        case SERVICE_CONTROL_STOP:
            s_instance->ReportServiceStatus(SERVICE_STOP_PENDING);
            s_instance->Stop();
            break;
        case SERVICE_CONTROL_PAUSE:
            s_instance->ReportServiceStatus(SERVICE_PAUSE_PENDING);
            s_instance->Pause();
            break;
        case SERVICE_CONTROL_CONTINUE:
            s_instance->ReportServiceStatus(SERVICE_CONTINUE_PENDING);
            s_instance->Continue();
            break;
        default:
            break;
        }
    }
}

bool WindowsService::Initialize()
{
    try
    {
        m_httpServer = std::make_unique<HttpServer>(8080);
        return m_httpServer->Initialize();
    }
    catch (const std::exception& e)
    {
        LogEvent(L"Failed to initialize HTTP server", EVENTLOG_ERROR_TYPE);
        return false;
    }
}

void WindowsService::Run()
{
    m_running = true;
    
    if (m_httpServer)
    {
        m_httpServer->Start();
        
        // Keep the service running
        while (m_running)
        {
            Sleep(1000);
        }
        
        m_httpServer->Stop();
    }
    
    ReportServiceStatus(SERVICE_STOPPED);
}

void WindowsService::Stop()
{
    m_running = false;
}

void WindowsService::Pause()
{
    if (m_httpServer)
    {
        m_httpServer->Pause();
    }
    ReportServiceStatus(SERVICE_PAUSED);
}

void WindowsService::Continue()
{
    if (m_httpServer)
    {
        m_httpServer->Resume();
    }
    ReportServiceStatus(SERVICE_RUNNING);
}

void WindowsService::ReportServiceStatus(DWORD currentState, DWORD exitCode, DWORD waitHint)
{
    static DWORD checkPoint = 1;

    m_status.dwCurrentState = currentState;
    m_status.dwWin32ExitCode = exitCode;
    m_status.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING)
        m_status.dwControlsAccepted = 0;
    else
        m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;

    if ((currentState == SERVICE_RUNNING) || (currentState == SERVICE_STOPPED))
        m_status.dwCheckPoint = 0;
    else
        m_status.dwCheckPoint = checkPoint++;

    SetServiceStatus(m_statusHandle, &m_status);
}

void WindowsService::LogEvent(const std::wstring& message, WORD type)
{
    HANDLE hEventSource = RegisterEventSource(nullptr, m_serviceName.c_str());
    if (hEventSource)
    {
        const wchar_t* strings[2];
        strings[0] = m_serviceName.c_str();
        strings[1] = message.c_str();

        ReportEvent(hEventSource, type, 0, 0, nullptr, 2, 0, strings, nullptr);
        DeregisterEventSource(hEventSource);
    }
}

bool WindowsService::InstallService()
{
    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager)
    {
        std::wcout << L"Failed to open Service Control Manager" << std::endl;
        return false;
    }

    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(nullptr, path, MAX_PATH))
    {
        CloseServiceHandle(hSCManager);
        return false;
    }

    SC_HANDLE hService = CreateService(
        hSCManager,
        m_serviceName.c_str(),
        m_displayName.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        path,
        nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (hService)
    {
        std::wcout << L"Service installed successfully" << std::endl;
        CloseServiceHandle(hService);
    }
    else
    {
        std::wcout << L"Failed to install service. Error: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(hSCManager);
    return hService != nullptr;
}

bool WindowsService::UninstallService()
{
    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        std::wcout << L"Failed to open Service Control Manager" << std::endl;
        return false;
    }

    SC_HANDLE hService = OpenService(hSCManager, m_serviceName.c_str(), DELETE);
    if (!hService)
    {
        std::wcout << L"Failed to open service" << std::endl;
        CloseServiceHandle(hSCManager);
        return false;
    }

    bool result = DeleteService(hService);
    if (result)
    {
        std::wcout << L"Service uninstalled successfully" << std::endl;
    }
    else
    {
        std::wcout << L"Failed to uninstall service. Error: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}