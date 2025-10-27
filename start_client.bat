@echo off
REM Chat Client Startup Script (WSL Version)
REM This script compiles and runs the chat client using WSL
REM Usage: start_client.bat [hostname]
REM Example: start_client.bat localhost

set SERVER_HOST=%1
if "%SERVER_HOST%"=="" set SERVER_HOST=localhost

echo === Chat Client Startup ===
echo.

echo Compiling client.c using WSL...
wsl gcc -o client client.c -Wall

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo.
    echo Connecting to server at %SERVER_HOST% on port 3490...
    echo.
    wsl ./client %SERVER_HOST%
) else (
    echo Compilation failed!
    pause
    exit /b 1
)
