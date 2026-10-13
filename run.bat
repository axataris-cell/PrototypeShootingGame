
@echo off
cd /d "%~dp0"

cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 pause

cmake --build build
if errorlevel 1 pause

cd build
Prototype.exe

pause