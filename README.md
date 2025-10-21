# Kerberos Echo HTTP Service

A Windows service written in C++ that listens on HTTP port 8080 and echoes back requests. The service uses Kerberos SPNEGO authentication to secure access.

## Features

- Windows Service implementation
- HTTP server on port 8080
- Kerberos SPNEGO authentication
- Request echoing functionality
- Console mode for testing
- Easy install/uninstall

## Prerequisites

1. **Administrator Rights**: Required for service installation and URL reservation
2. **Domain Environment**: Machine must be joined to a domain for Kerberos authentication
3. **URL Reservation**: Reserve the HTTP URL before running:
   ```cmd
   netsh http add urlacl url=http://+:8080/ user=Everyone
   ```

## Building

Build using Visual Studio or the following command line (requires MSVC):

```cmd
cl /EHsc main.cpp WindowsService.cpp HttpServer.cpp KerberosAuth.cpp /Fe:KerberosEchoService.exe httpapi.lib secur32.lib
```

## Usage

### Install as Windows Service
```cmd
KerberosEchoService.exe install
```

### Start the Service
```cmd
sc start KerberosEchoService
```

### Stop the Service
```cmd
sc stop KerberosEchoService
```

### Uninstall the Service
```cmd
KerberosEchoService.exe uninstall
```

### Run in Console Mode (for testing)
```cmd
KerberosEchoService.exe console
```

### Show Help
```cmd
KerberosEchoService.exe help
```

## Testing

1. **Start in Console Mode**:
   ```cmd
   KerberosEchoService.exe console
   ```

2. **Test with curl** (from domain-joined machine):
   ```cmd
   curl -k --negotiate -u : http://localhost:8080/test
   ```

3. **Test with PowerShell**:
   ```powershell
   $response = Invoke-WebRequest -Uri "http://localhost:8080/test" -UseDefaultCredentials
   $response.Content
   ```

## Authentication

The service uses Kerberos SPNEGO (Negotiate) authentication:

- Clients must be on the same domain or trusted domain
- Authentication is handled automatically by modern browsers and tools when properly configured
- The service will return `401 Unauthorized` with `WWW-Authenticate: Negotiate` header for unauthenticated requests

## Architecture

### Components

1. **WindowsService**: Main service controller and lifecycle management
2. **HttpServer**: HTTP.SYS-based web server implementation
3. **KerberosAuth**: SSPI-based Kerberos authentication handler
4. **main**: Entry point with command-line argument handling

### Flow

1. Service starts and initializes HTTP server on port 8080
2. HTTP server listens for incoming requests
3. Each request is checked for Kerberos authentication
4. Authenticated requests are echoed back with request details
5. Unauthenticated requests receive 401 with authentication challenge

## Security Notes

- The service runs under the Local System account by default
- Kerberos authentication provides mutual authentication and encryption
- Only domain-authenticated users can access the service
- Consider running under a service account with minimal privileges in production

## Troubleshooting

### Common Issues

1. **HTTP.SYS URL Reservation Error**:
   - Solution: Run `netsh http add urlacl url=http://+:8080/ user=Everyone` as Administrator

2. **Kerberos Authentication Failing**:
   - Ensure machine is properly joined to domain
   - Check DNS resolution and time synchronization
   - Verify SPN registration if using service account

3. **Service Won't Start**:
   - Check Event Viewer for service-specific errors
   - Ensure all dependencies are available
   - Verify service account permissions

### Event Logging

The service logs events to the Windows Event Log under the service name. Check Event Viewer for detailed error information.

## Files

- `main.cpp` - Entry point and command-line handling
- `WindowsService.h/cpp` - Windows service implementation
- `HttpServer.h/cpp` - HTTP server using HTTP.SYS API
- `KerberosAuth.h/cpp` - Kerberos SPNEGO authentication
- `CMakeLists.txt` - CMake build configuration (optional)
- `README.md` - This documentation