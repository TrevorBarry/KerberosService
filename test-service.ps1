# Test script for Kerberos Echo Service
# Run this from a domain-joined machine

param(
    [string]$Url = "http://localhost:8080",
    [string]$Method = "GET",
    [string]$Body = "",
    [switch]$ShowHeaders
)

Write-Host "Testing Kerberos Echo Service" -ForegroundColor Green
Write-Host "URL: $Url" -ForegroundColor Cyan
Write-Host "Method: $Method" -ForegroundColor Cyan

try {
    $headers = @{}
    if ($ShowHeaders) {
        $headers["User-Agent"] = "PowerShell-Test-Client/1.0"
        $headers["X-Test-Header"] = "Test-Value"
    }

    $params = @{
        Uri = $Url
        Method = $Method
        UseDefaultCredentials = $true
        Headers = $headers
    }

    if ($Body -and $Method -in @("POST", "PUT", "PATCH")) {
        $params.Body = $Body
        $params.ContentType = "text/plain"
    }

    Write-Host "`nSending request..." -ForegroundColor Yellow
    
    $response = Invoke-WebRequest @params
    
    Write-Host "`nResponse received!" -ForegroundColor Green
    Write-Host "Status: $($response.StatusCode) $($response.StatusDescription)" -ForegroundColor Cyan
    
    if ($response.Headers) {
        Write-Host "`nResponse Headers:" -ForegroundColor Yellow
        $response.Headers.GetEnumerator() | ForEach-Object {
            Write-Host "  $($_.Key): $($_.Value)" -ForegroundColor Gray
        }
    }
    
    Write-Host "`nResponse Body:" -ForegroundColor Yellow
    Write-Host $response.Content -ForegroundColor White
}
catch {
    Write-Host "`nError occurred:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    
    if ($_.Exception.Response) {
        Write-Host "Status: $($_.Exception.Response.StatusCode)" -ForegroundColor Red
        if ($_.Exception.Response.Headers) {
            Write-Host "WWW-Authenticate: $($_.Exception.Response.Headers['WWW-Authenticate'])" -ForegroundColor Red
        }
    }
}

Write-Host "`nTest completed." -ForegroundColor Green