@echo off
REM Chat Server Startup Script (WSL Version)
REM This script compiles and runs the chat server using WSL

echo === Chat Server Startup ===
echo.

echo Compiling server.c using WSL...
wsl gcc -o server server.c -Wall

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo.
    echo Starting chat server on port 3490...
    echo Waiting for client connections...
    echo Press Ctrl+C to stop the server
    echo.
    wsl ./server
) else (
    echo Compilation failed!
    pause
    exit /b 1
)
