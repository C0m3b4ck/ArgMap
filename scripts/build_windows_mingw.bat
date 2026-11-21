@echo off
setlocal EnableExtensions

REM Build ArgMap with MinGW-w64 using the vendored FLTK source tree.
REM Requires CMake and a MinGW-w64 toolchain (g++, mingw32-make) on PATH.

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%ROOT%\build-windows-mingw"

echo Configuring MinGW build...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --parallel
if errorlevel 1 exit /b 1

echo.
echo Build finished: "%BUILD_DIR%\argmap.exe"
endlocal
exit /b 0
