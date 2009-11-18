@echo off

echo Build script for the UltraDefrag Micro Edition project.
echo Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set UD_MICRO_EDITION=1
set OLD_PATH=%path%
call SETVARS.CMD

rem DELETE ALL PREVIOUSLY COMPILED FILES
call cleanup.cmd
if "%1" equ "--clean" goto end

echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.h

echo #define ZENWINX_VERSION %ZENWINX_VERSION% > .\dll\zenwinx\zenwinxver.h
echo #define ZENWINX_VERSION2 %ZENWINX_VERSION2% >> .\dll\zenwinx\zenwinxver.h

mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

xcopy /I /Y /Q    .\driver  .\obj\driver
xcopy /I /Y /Q    .\bootexctrl .\obj\bootexctrl
xcopy /I /Y /Q    .\hibernate .\obj\hibernate
xcopy /I /Y /Q    .\console .\obj\console
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\share .\obj\share
xcopy /I /Y /Q    .\dll\udefrag-kernel .\obj\udefrag-kernel
xcopy /I /Y /Q    .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx .\obj\zenwinx

copy /Y .\obj\share\*.c .\obj\console\

rem xcopy /I /Y /Q /S source destination

mkdir obj\dll
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\

call build-targets.cmd %1
if %errorlevel% neq 0 goto fail


:build_scheduler
rem The scheduler is not included in Micro Edition.


:build_installer

echo Build installer...
cd .\bin
copy /Y ..\installer\MicroEdition.nsi .\
copy /Y ..\installer\UltraDefrag.nsh .\
copy /Y ..\installer\driver.ini .\

if "%1" equ "--use-mingw-x64" goto build_x64_installer

%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
if "%1" equ "--use-mingw" goto build_source_package

:build_x64_installer
copy /Y ..\installer\MicroEdition.nsi .\amd64\
copy /Y ..\installer\UltraDefrag.nsh .\amd64\
copy /Y ..\installer\driver.ini .\amd64\
cd amd64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

cd..
if "%1" equ "--use-pellesc" goto build_source_package
if "%1" equ "--use-mingw-x64" goto build_source_package
copy /Y ..\installer\MicroEdition.nsi .\ia64\
copy /Y ..\installer\UltraDefrag.nsh .\ia64\
copy /Y ..\installer\driver.ini .\ia64\
cd ia64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

cd ..

:build_source_package
rem This section was removed to increase compilation speed.
rem Run build.cmd to make an archive of sources.
cd ..

echo.
echo Build success!

if "%1" equ "--install" goto install
if "%2" neq "--install" goto end
:install
echo Start installer...
.\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe /S
if %errorlevel% neq 0 goto fail_inst
echo Install success!
goto end
:fail_inst
echo Install error!
goto end

:fail
echo.
echo Build error (code %ERRORLEVEL%)!

:end
set UD_MICRO_EDITION=
set Path=%OLD_PATH%
set OLD_PATH=
exit /B

:usage
call build-modules.cmd --help build-micro
