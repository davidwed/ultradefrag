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
rem echo #define NGVERSIONINTITLE "UltraDefrag Next Generation v%ULTRADFGVER%" >> .\include\ultradfgver.h

rem force zenwinx version to be the same as ultradefrag version
rem echo #define ZENWINX_VERSION %ZENWINX_VERSION% > .\dll\zenwinx\zenwinxver.h
rem echo #define ZENWINX_VERSION2 %ZENWINX_VERSION2% >> .\dll\zenwinx\zenwinxver.h
echo #define ZENWINX_VERSION %VERSION% > .\dll\zenwinx\zenwinxver.h
echo #define ZENWINX_VERSION2 %VERSION2% >> .\dll\zenwinx\zenwinxver.h

mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

xcopy /I /Y /Q    .\bootexctrl .\obj\bootexctrl
xcopy /I /Y /Q    .\hibernate .\obj\hibernate
xcopy /I /Y /Q    .\console .\obj\console
xcopy /I /Y /Q    .\utf8-16 .\obj\utf8-16
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\share .\obj\share
xcopy /I /Y /Q    .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx .\obj\zenwinx

copy /Y .\obj\share\*.c .\obj\console\

rem we cannot link directly to wgx.dll, because it depends on Lua missing in Micro Edition
copy /Y .\dll\wgx\web-analytics.c .\obj\console\
copy /Y .\dll\wgx\wgx.h .\obj\console\
copy /Y .\dll\wgx\dbg.c .\obj\console\

rem xcopy /I /Y /Q /S source destination

mkdir obj\dll
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\

call build-targets.cmd %*
if %errorlevel% neq 0 goto fail

if %UD_BLD_FLG_IS_PORTABLE% equ 1 goto end

echo Build installer...
cd .\bin

:build_x86_installer
if %UD_BLD_FLG_BUILD_X86% EQU 0 goto build_amd64_installer

copy /Y ..\installer\MicroEdition.nsi .\
copy /Y ..\installer\UltraDefrag.nsh .\
copy /Y ..\installer\*.ico .\

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

:build_amd64_installer
if %UD_BLD_FLG_BUILD_AMD64% equ 0 goto build_ia64_installer

copy /Y ..\installer\MicroEdition.nsi .\amd64\
copy /Y ..\installer\UltraDefrag.nsh .\amd64\
copy /Y ..\installer\*.ico .\amd64\
cd amd64
"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail
cd..

:build_ia64_installer
if %UD_BLD_FLG_BUILD_IA64% equ 0 goto build_source_package

copy /Y ..\installer\MicroEdition.nsi .\ia64\
copy /Y ..\installer\UltraDefrag.nsh .\ia64\
copy /Y ..\installer\*.ico .\ia64\
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
