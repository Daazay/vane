@echo off

pushd %~dp0\..\

set TARGET=
set ACTION=
set PREMAKE=third-party\premake\premake5.exe
set PREMAKE_SCRIPT=scripts\premake\premake5.lua

if "%~1"=="" (
    set TARGET=vs
)

if /i "%TARGET%"=="vs" (
    set ACTION=vs2022
) else if /i "%TARGET%"=="make" (
    set ACTION=gmake
) else (
    echo Invalid target: %TARGET%
    echo Available targets: vs, make
    echo Default target: vs
    exit /b 1
)

%PREMAKE% --file=%PREMAKE_SCRIPT% %ACTION%

popd

pause