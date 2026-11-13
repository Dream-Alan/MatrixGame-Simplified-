@echo off
rem ===========================================================================
rem  Simplified linear algebra game - build script (ASCII only, CRLF endings)
rem    build.bat        build LinearAlgebraLite.exe
rem    build.bat test   build and run the core logic tests
rem  Needs only a C++17 MinGW g++ (no third-party libraries).
rem ===========================================================================

setlocal
if "%MINGW_BIN%"=="" set MINGW_BIN=C:\Program Files\CodeBlocks\MinGW\bin
set PATH=%MINGW_BIN%;%PATH%

set CXX=g++.exe
set CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -D_WIN32_WINNT=0x0601 -finput-charset=UTF-8 -fexec-charset=UTF-8 -fwide-exec-charset=UTF-16LE
set SRC=fraction.cpp matrix.cpp game.cpp ui.cpp main.cpp

if /i "%~1"=="test" goto test

echo Building LinearAlgebraLite.exe ...
%CXX% %CXXFLAGS% -municode -mwindows -o LinearAlgebraLite.exe %SRC% -static -lgdi32 -luser32
if errorlevel 1 goto fail
echo Build OK: LinearAlgebraLite.exe
exit /b 0

:test
echo Building test_core.exe ...
%CXX% %CXXFLAGS% -o test_core.exe tests\test_core.cpp fraction.cpp matrix.cpp game.cpp -static
if errorlevel 1 goto fail
test_core.exe
if errorlevel 1 goto fail
del test_core.exe
exit /b 0

:fail
echo Build FAILED
exit /b 1
