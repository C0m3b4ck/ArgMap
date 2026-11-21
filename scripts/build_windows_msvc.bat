@echo off
setlocal EnableExtensions

REM Build ArgMap with MSVC using the vendored FLTK source tree.
REM Requires CMake and the Visual Studio C++ desktop workload.
REM Pass a different generator as the first argument if needed, e.g.:
REM   build_windows_msvc.bat "Visual Studio 17 2022"

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%ROOT%\build-windows-msvc"
set "GENERATOR=Visual Studio 17 2022"

if not "%~1"=="" set "GENERATOR=%~1"

echo Configuring with generator: %GENERATOR%
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A x64
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 exit /b 1

echo.
echo Build finished: "%BUILD_DIR%\Release\argmap.exe"
endlocal
exit /b 0
