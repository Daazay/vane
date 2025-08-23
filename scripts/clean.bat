@echo off
setlocal enabledelayedexpansion

pushd %~dp0\..\

set PREMAKE=third-party\premake\premake5.exe
set PREMAKE_SCRIPT=scripts\premake\premake5.lua

%PREMAKE% --file=%PREMAKE_SCRIPT% clean

popd

pause