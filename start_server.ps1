# Chat Server Startup Script
# This script compiles and runs the chat server

Write-Host "=== Chat Server Startup ===" -ForegroundColor DarkGreen
Write-Host ""

# Check if we're on Windows with WSL or need to use a Linux environment
$useWSL = $false
if (Get-Command wsl -ErrorAction SilentlyContinue) {
    $useWSL = $true
    Write-Host "WSL detected - using Linux compilation" -ForegroundColor Green
} else {
    Write-Host "WARNING: This code requires a Unix-like environment (WSL, MinGW, Cygwin)" -ForegroundColor Yellow
    Write-Host "Please ensure you have WSL installed or use a Linux system" -ForegroundColor Yellow
    Write-Host ""
    $response = Read-Host "Do you have a C compiler available? (y/n)"
    if ($response -ne 'y') {
        Write-Host "Please install WSL or a Unix-like environment first." -ForegroundColor Red
        exit 1
    }
}

Write-Host "Compiling server.c..." -ForegroundColor Yellow

if ($useWSL) {
    # Convert Windows path to WSL path
    $wslPath = wsl wslpath -a "$PSScriptRoot\server.c" 2>$null
    
    if ([string]::IsNullOrEmpty($wslPath)) {
        # Fallback: use the script directory directly
        $wslPath = wsl wslpath "$PSScriptRoot" 2>$null
        if ([string]::IsNullOrEmpty($wslPath)) {
            Write-Host "Warning: Could not convert path, using current directory" -ForegroundColor Yellow
            $wslDir = "."
        } else {
            $wslDir = $wslPath
        }
    } else {
        $wslDir = Split-Path $wslPath
    }
    
    # Compile using WSL
    wsl bash -c "cd '$wslDir' && gcc -o server server.c -Wall" 2>&1 | Where-Object { $_ -notmatch "wslpath" }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Starting chat server on port 3490..." -ForegroundColor Cyan
        Write-Host "Waiting for client connections..." -ForegroundColor Cyan
        Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Yellow
        Write-Host ""
        
        # Run the server
        wsl bash -c "cd '$wslDir' && ./server"
    } else {
        Write-Host "Compilation failed!" -ForegroundColor Red
        exit 1
    }
} else {
    # Try direct compilation (MinGW/Cygwin)
    gcc -o "$PSScriptRoot\server.exe" "$PSScriptRoot\server.c" -lws2_32 -Wall 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Starting chat server on port 3490..." -ForegroundColor Cyan
        Write-Host "Waiting for client connections..." -ForegroundColor Cyan
        Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Yellow
        Write-Host ""
        
        & "$PSScriptRoot\server.exe"
    } else {
        Write-Host "Compilation failed!" -ForegroundColor Red
        Write-Host "This code requires Unix sockets. Please use WSL." -ForegroundColor Yellow
        exit 1
    }
}
