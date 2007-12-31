@echo off

rem This batch file use ms c compiler and reactos ddk.

rem set environment
if "%MINGW_ENV%" neq "" goto start_build
set path=%MINGWBASE%\bin;%path%
set MINGW_ENV=ok

:start_build
echo Build the zenwinx.dll ...
del obj\*.res
mingw32-make -f Makefile.mingw CFG=Release
