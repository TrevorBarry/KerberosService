#include "KerberosAuth.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "secur32.lib")

KerberosAuth::KerberosAuth()
    : m_bCredsInitialized(false)
    , m_bContextInitialized(false)
    , m_pSSPI(nullptr)
{
    ZeroMemory(&m_hCreds, sizeof(m_hCreds));
    ZeroMemory(&m_hContext, sizeof(m_hContext));
}

KerberosAuth::~KerberosAuth()
{
    Cleanup();
}

bool KerberosAuth::Initialize()
{
    // Initialize the security function table
    m_pSSPI = InitSecurityInterface();
    if (!m_pSSPI)
    {
        std::wcout << L"Failed to initialize security interface" << std::endl;
        return false;
    }

    // Acquire credentials handle for the server
    TimeStamp tsExpiry;
    SECURITY_STATUS ss = m_pSSPI->AcquireCredentialsHandle(
        nullptr,                    // Principal (use default)
        const_cast<SEC_WCHAR*>(NEGOSSP_NAME),  // Package name (Negotiate)
        SECPKG_CRED_INBOUND,        // Credentials use
        nullptr,                    // Logon ID
        nullptr,                    // Auth data
        nullptr,                    // Get key function
        nullptr,                    // Get key argument
        &m_hCreds,                  // Credentials handle
        &tsExpiry                   // Expiry time
    );

    if (ss != SEC_E_OK)
    {
        std::wcout << L"AcquireCredentialsHandle failed with error: 0x" << std::hex << ss << std::endl;
        return false;
    }

    m_bCredsInitialized = true;
    std::wcout << L"Kerberos authentication initialized successfully" << std::endl;
    return true;
}

bool KerberosAuth::AuthenticateToken(const std::string& base64Token)
{
    if (!m_bCredsInitialized)
    {
        std::wcout << L"Credentials not initialized" << std::endl;
        return false;
    }

    // Decode the base64 token
    std::vector<BYTE> tokenData = Base64Decode(base64Token);
    if (tokenData.empty())
    {
        std::wcout << L"Failed to decode authentication token" << std::endl;
        return false;
    }

    // Setup input buffer
    SecBuffer inSecBuffer;
    inSecBuffer.BufferType = SECBUFFER_TOKEN;
    inSecBuffer.cbBuffer = static_cast<ULONG>(tokenData.size());
    inSecBuffer.pvBuffer = tokenData.data();

    SecBufferDesc inSecBufferDesc;
    inSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    inSecBufferDesc.cBuffers = 1;
    inSecBufferDesc.pBuffers = &inSecBuffer;

    // Setup output buffer
    const DWORD dwMaxTokenSize = 12288; // Typical max token size
    std::vector<BYTE> outTokenBuffer(dwMaxTokenSize);
    
    SecBuffer outSecBuffer;
    outSecBuffer.BufferType = SECBUFFER_TOKEN;
    outSecBuffer.cbBuffer = dwMaxTokenSize;
    outSecBuffer.pvBuffer = outTokenBuffer.data();

    SecBufferDesc outSecBufferDesc;
    outSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    outSecBufferDesc.cBuffers = 1;
    outSecBufferDesc.pBuffers = &outSecBuffer;

    DWORD dwContextAttributes;
    TimeStamp tsExpiry;

    // Accept the security context
    SECURITY_STATUS ss = m_pSSPI->AcceptSecurityContext(
        &m_hCreds,                  // Credentials handle
        m_bContextInitialized ? &m_hContext : nullptr,  // Existing context
        &inSecBufferDesc,           // Input buffer
        ASC_REQ_CONNECTION,         // Context requirements
        SECURITY_NATIVE_DREP,       // Target data representation
        &m_hContext,                // New context handle
        &outSecBufferDesc,          // Output buffer
        &dwContextAttributes,       // Context attributes
        &tsExpiry                   // Context expiry
    );

    m_bContextInitialized = true;

    if (ss == SEC_E_OK)
    {
        std::wcout << L"Authentication successful" << std::endl;
        
        // Get the authenticated user name
        SecPkgContext_Names names;
        if (m_pSSPI->QueryContextAttributes(&m_hContext, SECPKG_ATTR_NAMES, &names) == SEC_E_OK)
        {
            std::wcout << L"Authenticated user: " << names.sUserName << std::endl;
            m_pSSPI->FreeContextBuffer(names.sUserName);
        }
        
        return true;
    }
    else if (ss == SEC_I_CONTINUE_NEEDED)
    {
        std::wcout << L"Authentication requires continuation (multi-step)" << std::endl;
        // In a real implementation, you would send the output token back to the client
        return false; // For simplicity, we'll treat this as failure
    }
    else
    {
        std::wcout << L"AcceptSecurityContext failed with error: 0x" << std::hex << ss << std::endl;
        return false;
    }
}

void KerberosAuth::Cleanup()
{
    if (m_bContextInitialized)
    {
        m_pSSPI->DeleteSecurityContext(&m_hContext);
        m_bContextInitialized = false;
    }

    if (m_bCredsInitialized)
    {
        m_pSSPI->FreeCredentialsHandle(&m_hCreds);
        m_bCredsInitialized = false;
    }
}

std::vector<BYTE> KerberosAuth::Base64Decode(const std::string& encoded)
{
    std::vector<BYTE> result;
    
    // Remove any whitespace
    std::string cleanEncoded;
    for (char c : encoded)
    {
        if (!isspace(c))
            cleanEncoded += c;
    }

    // Simple base64 decode (basic implementation)
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T[chars[i]] = i;

    int pad = cleanEncoded.size() % 4;
    if (pad)
        cleanEncoded.append(4 - pad, '=');

    for (size_t i = 0; i < cleanEncoded.size(); i += 4)
    {
        int n = T[cleanEncoded[i]] << 18 | T[cleanEncoded[i + 1]] << 12 | T[cleanEncoded[i + 2]] << 6 | T[cleanEncoded[i + 3]];
        result.push_back((n >> 16) & 0xFF);
        if (cleanEncoded[i + 2] != '=')
            result.push_back((n >> 8) & 0xFF);
        if (cleanEncoded[i + 3] != '=')
            result.push_back(n & 0xFF);
    }

    return result;
}

std::string KerberosAuth::Base64Encode(const std::vector<BYTE>& data)
{
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    int val = 0, valb = -6;
    for (BYTE c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6)
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        
    while (result.size() % 4)
        result.push_back('=');
        
    return result;
}