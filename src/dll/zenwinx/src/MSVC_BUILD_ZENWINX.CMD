@echo off

rem This batch file use ms c compiler and reactos ddk.

rem set environment
if "%MSVC_ENV%" neq "" goto start_build
call "%MSVSBIN%\vcvars32.bat"
set MSVC_ENV=ok

:start_build
echo Build the zenwinx.dll ...
nmake /NOLOGO /A /f zenwinx.mak
