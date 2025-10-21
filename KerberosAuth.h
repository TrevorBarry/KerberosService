#pragma once

#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>
#include <security.h>
#include <string>
#include <vector>

class KerberosAuth
{
public:
    KerberosAuth();
    ~KerberosAuth();

    bool Initialize();
    bool AuthenticateToken(const std::string& base64Token);
    void Cleanup();

private:
    bool InitializeSecurityContext();
    std::vector<BYTE> Base64Decode(const std::string& encoded);
    std::string Base64Encode(const std::vector<BYTE>& data);

    CredHandle m_hCreds;
    CtxtHandle m_hContext;
    bool m_bCredsInitialized;
    bool m_bContextInitialized;
    PSecurityFunctionTable m_pSSPI;
};