@echo off

rem This batch file use Windows Server 2003 DDK.

rem set environment
if "%DDK_ENV%" neq "" goto start_build
rem ...
set DDK_ENV=ok

:start_build
echo --------- WINX x86 ---------
set AMD64=
set IA64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
build.exe
if %errorlevel% neq 0 goto fail
echo --------- WINX x64 ---------
set IA64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre AMD64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
build.exe
if %errorlevel% neq 0 goto fail
echo --------- WINX ia64 --------
set AMD64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre 64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
build.exe
if %errorlevel% neq 0 goto fail

exit /B 0

:fail
exit /B 1
