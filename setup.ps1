# Setup script for Kerberos Echo Service
# Must be run as Administrator

param(
    [switch]$Install,
    [switch]$Uninstall,
    [switch]$Start,
    [switch]$Stop,
    [switch]$Status,
    [switch]$ReserveUrl
)

$ServiceName = "KerberosEchoService"
$ServiceExe = ".\KerberosEchoService.exe"
$ServiceUrl = "http://+:8080/"

function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Show-Usage {
    Write-Host "Kerberos Echo Service Setup Script" -ForegroundColor Green
    Write-Host ""
    Write-Host "Usage: .\setup.ps1 [options]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -ReserveUrl    Reserve HTTP URL (requires Administrator)" -ForegroundColor White
    Write-Host "  -Install       Install the Windows service" -ForegroundColor White
    Write-Host "  -Uninstall     Uninstall the Windows service" -ForegroundColor White
    Write-Host "  -Start         Start the service" -ForegroundColor White
    Write-Host "  -Stop          Stop the service" -ForegroundColor White
    Write-Host "  -Status        Show service status" -ForegroundColor White
    Write-Host ""
    Write-Host "Prerequisites:" -ForegroundColor Yellow
    Write-Host "  1. Build the service first (run build.bat)" -ForegroundColor White
    Write-Host "  2. Run as Administrator for installation" -ForegroundColor White
    Write-Host "  3. Ensure machine is domain-joined" -ForegroundColor White
}

if (-not $Install -and -not $Uninstall -and -not $Start -and -not $Stop -and -not $Status -and -not $ReserveUrl) {
    Show-Usage
    return
}

# Check if service executable exists
if (-not (Test-Path $ServiceExe)) {
    Write-Host "Error: Service executable not found at $ServiceExe" -ForegroundColor Red
    Write-Host "Please build the service first using build.bat" -ForegroundColor Yellow
    return
}

if ($ReserveUrl) {
    if (-not (Test-Administrator)) {
        Write-Host "Error: URL reservation requires Administrator privileges" -ForegroundColor Red
        return
    }
    
    Write-Host "Reserving URL $ServiceUrl..." -ForegroundColor Yellow
    
    try {
        $result = netsh http add urlacl url=$ServiceUrl user=Everyone
        if ($LASTEXITCODE -eq 0) {
            Write-Host "URL reserved successfully" -ForegroundColor Green
        } else {
            Write-Host "Failed to reserve URL. It might already be reserved." -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host "Error reserving URL: $($_.Exception.Message)" -ForegroundColor Red
    }
}

if ($Install) {
    if (-not (Test-Administrator)) {
        Write-Host "Error: Service installation requires Administrator privileges" -ForegroundColor Red
        return
    }
    
    Write-Host "Installing service..." -ForegroundColor Yellow
    
    try {
        & $ServiceExe install
        Write-Host "Service installed successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "Error installing service: $($_.Exception.Message)" -ForegroundColor Red
    }
}

if ($Uninstall) {
    if (-not (Test-Administrator)) {
        Write-Host "Error: Service uninstallation requires Administrator privileges" -ForegroundColor Red
        return
    }
    
    # Stop service first if running
    try {
        $service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
        if ($service -and $service.Status -eq "Running") {
            Write-Host "Stopping service first..." -ForegroundColor Yellow
            Stop-Service -Name $ServiceName -Force
        }
    }
    catch {
        # Ignore errors when stopping
    }
    
    Write-Host "Uninstalling service..." -ForegroundColor Yellow
    
    try {
        & $ServiceExe uninstall
        Write-Host "Service uninstalled successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "Error uninstalling service: $($_.Exception.Message)" -ForegroundColor Red
    }
}

if ($Start) {
    Write-Host "Starting service..." -ForegroundColor Yellow
    
    try {
        Start-Service -Name $ServiceName
        Write-Host "Service started successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "Error starting service: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Check Event Viewer for detailed error information" -ForegroundColor Yellow
    }
}

if ($Stop) {
    Write-Host "Stopping service..." -ForegroundColor Yellow
    
    try {
        Stop-Service -Name $ServiceName -Force
        Write-Host "Service stopped successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "Error stopping service: $($_.Exception.Message)" -ForegroundColor Red
    }
}

if ($Status) {
    try {
        $service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
        if ($service) {
            Write-Host "Service Status: $($service.Status)" -ForegroundColor Cyan
            Write-Host "Service Name: $($service.Name)" -ForegroundColor Gray
            Write-Host "Display Name: $($service.DisplayName)" -ForegroundColor Gray
            Write-Host "Start Type: $($service.StartType)" -ForegroundColor Gray
        } else {
            Write-Host "Service is not installed" -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host "Error checking service status: $($_.Exception.Message)" -ForegroundColor Red
    }
}