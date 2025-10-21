#include "HttpServer.h"
#include "KerberosAuth.h"
#include <iostream>
#include <sstream>

#pragma comment(lib, "httpapi.lib")

const std::string HttpServer::UNAUTHORIZED_RESPONSE = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Negotiate\r\nContent-Length: 12\r\n\r\nUnauthorized";
const std::string HttpServer::SERVER_ERROR_RESPONSE = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error";

HttpServer::HttpServer(int port)
    : m_port(port)
    , m_hReqQueue(nullptr)
    , m_running(false)
    , m_paused(false)
{
}

HttpServer::~HttpServer()
{
    Stop();
}

bool HttpServer::Initialize()
{
    // Initialize HTTP Server API
    ULONG result = HttpInitialize(HTTPAPI_VERSION_2, HTTP_INITIALIZE_SERVER, nullptr);
    if (result != NO_ERROR)
    {
        std::wcout << L"HttpInitialize failed with error: " << result << std::endl;
        return false;
    }

    // Create request queue
    result = HttpCreateHttpHandle(&m_hReqQueue, 0);
    if (result != NO_ERROR)
    {
        std::wcout << L"HttpCreateHttpHandle failed with error: " << result << std::endl;
        HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
        return false;
    }

    // Create URL
    std::wstring url = L"http://+:" + std::to_wstring(m_port) + L"/";
    
    // Add URL to queue
    result = HttpAddUrl(m_hReqQueue, url.c_str(), nullptr);
    if (result != NO_ERROR)
    {
        std::wcout << L"HttpAddUrl failed with error: " << result << std::endl;
        std::wcout << L"Make sure to run as Administrator or reserve the URL with: " << std::endl;
        std::wcout << L"netsh http add urlacl url=" << url << L" user=Everyone" << std::endl;
        CloseHandle(m_hReqQueue);
        HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
        return false;
    }

    // Initialize Kerberos authentication
    m_kerberosAuth = std::make_unique<KerberosAuth>();
    if (!m_kerberosAuth->Initialize())
    {
        std::wcout << L"Failed to initialize Kerberos authentication" << std::endl;
        CloseHandle(m_hReqQueue);
        HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
        return false;
    }

    return true;
}

void HttpServer::Start()
{
    m_running = true;
    m_paused = false;
    m_workerThread = std::thread(&HttpServer::WorkerThread, this);
    std::wcout << L"HTTP Server started on port " << m_port << std::endl;
}

void HttpServer::Stop()
{
    m_running = false;
    
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }

    if (m_hReqQueue)
    {
        CloseHandle(m_hReqQueue);
        m_hReqQueue = nullptr;
    }

    HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
    std::wcout << L"HTTP Server stopped" << std::endl;
}

void HttpServer::Pause()
{
    m_paused = true;
    std::wcout << L"HTTP Server paused" << std::endl;
}

void HttpServer::Resume()
{
    m_paused = false;
    std::wcout << L"HTTP Server resumed" << std::endl;
}

void HttpServer::WorkerThread()
{
    const DWORD requestBufferSize = sizeof(HTTP_REQUEST) + 2048;
    std::unique_ptr<BYTE[]> requestBuffer(new BYTE[requestBufferSize]);
    PHTTP_REQUEST pRequest = reinterpret_cast<PHTTP_REQUEST>(requestBuffer.get());

    while (m_running)
    {
        if (m_paused)
        {
            Sleep(100);
            continue;
        }

        HTTP_REQUEST_ID requestId = HTTP_NULL_ID;
        DWORD bytesReturned;

        ULONG result = HttpReceiveHttpRequest(
            m_hReqQueue,
            requestId,
            0,
            pRequest,
            requestBufferSize,
            &bytesReturned,
            nullptr
        );

        if (result == NO_ERROR)
        {
            ProcessRequest(pRequest->RequestId, pRequest);
        }
        else if (result == ERROR_MORE_DATA)
        {
            // Request is larger than our buffer, but for this echo server, we'll just handle what we can
            ProcessRequest(pRequest->RequestId, pRequest);
        }
        else if (result != ERROR_CONNECTION_INVALID)
        {
            std::wcout << L"HttpReceiveHttpRequest failed with error: " << result << std::endl;
        }
    }
}

bool HttpServer::ProcessRequest(HTTP_REQUEST_ID requestId, PHTTP_REQUEST pRequest)
{
    // Check authentication
    if (!HandleAuthentication(pRequest))
    {
        // Send 401 Unauthorized with WWW-Authenticate header
        HTTP_RESPONSE response;
        ZeroMemory(&response, sizeof(response));
        
        response.StatusCode = 401;
        response.pReason = "Unauthorized";
        response.ReasonLength = static_cast<USHORT>(strlen("Unauthorized"));

        // Add WWW-Authenticate header
        HTTP_UNKNOWN_HEADER authHeader;
        authHeader.pName = "WWW-Authenticate";
        authHeader.NameLength = static_cast<USHORT>(strlen("WWW-Authenticate"));
        authHeader.pRawValue = "Negotiate";
        authHeader.RawValueLength = static_cast<USHORT>(strlen("Negotiate"));

        response.Headers.pUnknownHeaders = &authHeader;
        response.Headers.UnknownHeaderCount = 1;

        std::string body = "Authentication required";
        HTTP_DATA_CHUNK dataChunk;
        dataChunk.DataChunkType = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer = const_cast<char*>(body.c_str());
        dataChunk.FromMemory.BufferLength = static_cast<ULONG>(body.length());

        response.EntityChunkCount = 1;
        response.pEntityChunks = &dataChunk;

        DWORD bytesSent;
        HttpSendHttpResponse(m_hReqQueue, requestId, 0, &response, nullptr, &bytesSent, nullptr, 0, nullptr, nullptr);
        return false;
    }

    // Read request body if present
    std::string requestBody;
    if (pRequest->BytesReceived < pRequest->EntityChunkCount && pRequest->EntityChunkCount > 0)
    {
        // For simplicity, we'll just echo back the URL and headers info
        // In a real implementation, you'd read the full request body
    }

    // Create echo response
    std::stringstream ss;
    ss << "Echo Response\n";
    ss << "=============\n";
    ss << "Method: ";
    
    switch (pRequest->Verb)
    {
    case HttpVerbGET: ss << "GET"; break;
    case HttpVerbPOST: ss << "POST"; break;
    case HttpVerbPUT: ss << "PUT"; break;
    case HttpVerbDELETE: ss << "DELETE"; break;
    default: ss << "OTHER"; break;
    }
    
    ss << "\n";
    ss << "URL: " << std::string(pRequest->CookedUrl.pAbsPath, pRequest->CookedUrl.AbsPathLength / sizeof(wchar_t)) << "\n";
    ss << "Headers:\n";
    
    // Echo known headers
    for (int i = 0; i < HttpHeaderRequestMaximum; i++)
    {
        if (pRequest->Headers.KnownHeaders[i].pRawValue)
        {
            ss << "  " << i << ": " << std::string(pRequest->Headers.KnownHeaders[i].pRawValue, 
                                                  pRequest->Headers.KnownHeaders[i].RawValueLength) << "\n";
        }
    }

    // Echo unknown headers
    for (USHORT i = 0; i < pRequest->Headers.UnknownHeaderCount; i++)
    {
        ss << "  " << std::string(pRequest->Headers.pUnknownHeaders[i].pName,
                                  pRequest->Headers.pUnknownHeaders[i].NameLength) << ": "
           << std::string(pRequest->Headers.pUnknownHeaders[i].pRawValue,
                          pRequest->Headers.pUnknownHeaders[i].RawValueLength) << "\n";
    }

    return SendResponse(requestId, pRequest, ss.str());
}

bool HttpServer::SendResponse(HTTP_REQUEST_ID requestId, PHTTP_REQUEST pRequest, const std::string& responseBody)
{
    HTTP_RESPONSE response;
    ZeroMemory(&response, sizeof(response));

    response.StatusCode = 200;
    response.pReason = "OK";
    response.ReasonLength = static_cast<USHORT>(strlen("OK"));

    // Set content type
    response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/plain";
    response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = static_cast<USHORT>(strlen("text/plain"));

    // Set content length
    std::string contentLength = std::to_string(responseBody.length());
    response.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue = contentLength.c_str();
    response.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength = static_cast<USHORT>(contentLength.length());

    // Set response body
    HTTP_DATA_CHUNK dataChunk;
    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = const_cast<char*>(responseBody.c_str());
    dataChunk.FromMemory.BufferLength = static_cast<ULONG>(responseBody.length());

    response.EntityChunkCount = 1;
    response.pEntityChunks = &dataChunk;

    DWORD bytesSent;
    ULONG result = HttpSendHttpResponse(m_hReqQueue, requestId, 0, &response, nullptr, &bytesSent, nullptr, 0, nullptr, nullptr);

    return result == NO_ERROR;
}

bool HttpServer::HandleAuthentication(PHTTP_REQUEST pRequest)
{
    // Look for Authorization header
    if (!pRequest->Headers.KnownHeaders[HttpHeaderAuthorization].pRawValue)
    {
        return false; // No authorization header
    }

    std::string authHeader(
        pRequest->Headers.KnownHeaders[HttpHeaderAuthorization].pRawValue,
        pRequest->Headers.KnownHeaders[HttpHeaderAuthorization].RawValueLength
    );

    // Check if it's Negotiate authentication
    if (authHeader.substr(0, 9) != "Negotiate")
    {
        return false;
    }

    // Extract the token
    std::string token;
    if (authHeader.length() > 10)
    {
        token = authHeader.substr(10); // Skip "Negotiate "
    }

    // Authenticate with Kerberos
    return m_kerberosAuth->AuthenticateToken(token);
}