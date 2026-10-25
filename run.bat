
@echo off
setlocal
cd /d "%~dp0"

echo.
echo [1/3] Configuring CMake...
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 goto error

echo.
echo [2/3] Building Prototype...
cmake --build build --parallel 8
if errorlevel 1 goto error

echo.
echo [3/3] Launching Prototype...
cd /d "%~dp0build"
Prototype.exe
if errorlevel 1 goto error

echo.
echo Game exited normally.
pause
exit /b 0

:error
echo.
echo ERROR: A command failed. Check the messages above.
pause
exit /b 1
