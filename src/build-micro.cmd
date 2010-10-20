@echo off

echo Build script for the UltraDefrag Micro Edition project.
echo Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).

call ParseCommandLine.cmd %*

if %UD_BLD_FLG_DIPLAY_HELP% equ 1 goto usage

set UD_MICRO_EDITION=1

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS.CMD
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

rem DELETE ALL PREVIOUSLY COMPILED FILES
call cleanup.cmd %*
if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 goto end

if %UD_BLD_FLG_IS_PORTABLE% equ 1 (
	set UDEFRAG_PORTABLE=1
) else (
	set UDEFRAG_PORTABLE=
)

echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.h

:: force zenwinx version to be the same as ultradefrag version
echo #define ZENWINX_VERSION %VERSION% > .\dll\zenwinx\zenwinxver.h
echo #define ZENWINX_VERSION2 %VERSION2% >> .\dll\zenwinx\zenwinxver.h

:: build all binaries
call build-targets.cmd %*
if %errorlevel% neq 0 goto fail

if %UD_BLD_FLG_IS_PORTABLE% equ 1 goto end

echo Build installer...
cd .\bin

:build_x86_installer
if %UD_BLD_FLG_BUILD_X86% EQU 0 goto build_amd64_installer

copy /Y ..\installer\MicroEdition.nsi .\

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

:build_amd64_installer
if %UD_BLD_FLG_BUILD_AMD64% equ 0 goto build_ia64_installer

copy /Y ..\installer\MicroEdition.nsi .\amd64\
cd amd64
"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail
cd..

:build_ia64_installer
if %UD_BLD_FLG_BUILD_IA64% equ 0 goto build_source_package

copy /Y ..\installer\MicroEdition.nsi .\ia64\
cd ia64
"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail
cd ..

:build_source_package
rem This section was removed to increase compilation speed.
rem Run build.cmd to make an archive of sources.
cd ..

echo.
echo Build success!

if %UD_BLD_FLG_DO_INSTALL% equ 0 goto end

:install
echo Start installer...
if not %PROCESSOR_ARCHITECTURE% == x86 goto install_amd64
.\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_amd64
if not %PROCESSOR_ARCHITECTURE% == AMD64 goto install_ia64
.\bin\amd64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_ia64
if not %PROCESSOR_ARCHITECTURE% == IA64 goto install_finished
.\bin\ia64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_finished
echo Install success!
goto end

:fail_inst
echo Install error!
goto end_fail

:fail
echo.
echo Build error (code %ERRORLEVEL%)!

:end_fail
set UD_MICRO_EDITION=
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 1

:end
set UD_MICRO_EDITION=
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 0

:usage
call build-help.cmd "%~n0"
