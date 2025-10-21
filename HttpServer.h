#pragma once

#include <windows.h>
#include <http.h>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

class KerberosAuth;

class HttpServer
{
public:
    HttpServer(int port);
    ~HttpServer();

    bool Initialize();
    void Start();
    void Stop();
    void Pause();
    void Resume();

private:
    void WorkerThread();
    bool ProcessRequest(HTTP_REQUEST_ID requestId, PHTTP_REQUEST pRequest);
    bool SendResponse(HTTP_REQUEST_ID requestId, PHTTP_REQUEST pRequest, const std::string& responseBody);
    bool HandleAuthentication(PHTTP_REQUEST pRequest);

    int m_port;
    HANDLE m_hReqQueue;
    std::unique_ptr<KerberosAuth> m_kerberosAuth;
    std::thread m_workerThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_paused;
    
    static const std::string UNAUTHORIZED_RESPONSE;
    static const std::string SERVER_ERROR_RESPONSE;
};