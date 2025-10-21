#pragma once

#include <windows.h>
#include <winsvc.h>
#include <string>
#include <memory>

class HttpServer;

class WindowsService
{
public:
    WindowsService(const std::wstring& serviceName, const std::wstring& displayName);
    ~WindowsService();

    // Service control functions
    static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD ctrl);
    
    // Service lifecycle
    bool Initialize();
    void Run();
    void Stop();
    void Pause();
    void Continue();

    // Installation/removal
    bool InstallService();
    bool UninstallService();

    // Static instance access
    static WindowsService* GetInstance() { return s_instance; }

private:
    void ReportServiceStatus(DWORD currentState, DWORD exitCode = NO_ERROR, DWORD waitHint = 0);
    void LogEvent(const std::wstring& message, WORD type = EVENTLOG_INFORMATION_TYPE);

    std::wstring m_serviceName;
    std::wstring m_displayName;
    SERVICE_STATUS_HANDLE m_statusHandle;
    SERVICE_STATUS m_status;
    bool m_running;
    
    std::unique_ptr<HttpServer> m_httpServer;
    
    static WindowsService* s_instance;
};