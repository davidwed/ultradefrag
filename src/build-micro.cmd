@echo off

echo Build script for the UltraDefrag Micro Edition project.
echo Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set UD_MICRO_EDITION=1
set OLD_PATH=%path%
call SETVARS.CMD
if "%1" equ "--clean" goto clean

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
xcopy /I /Y /Q    .\console .\obj\console
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx .\obj\zenwinx

rem xcopy /I /Y /Q /S source destination

mkdir obj\dll
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\


if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--use-msvc" goto msvc_build
if "%1" equ "--use-mingw" goto mingw_build

:ddk_build

set BUILD_ENV=winddk

echo --------- Target is x86 ---------
set AMD64=
set IA64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
set UDEFRAG_LIB_PATH=..\..\lib
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is x64 ---------
set IA64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre AMD64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
set UDEFRAG_LIB_PATH=..\..\lib\amd64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is ia64 ---------
set AMD64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre 64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
set UDEFRAG_LIB_PATH=..\..\lib\ia64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_scheduler

:msvc_build

rem if "%MSVC_ENV%" neq "" goto start_msvc_build

call "%MSVSBIN%\vcvars32.bat"

rem set MSVC_ENV=ok
rem :start_msvc_build

set BUILD_ENV=msvc
set UDEFRAG_LIB_PATH=..\..\lib
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_scheduler

:mingw_build

rem if "%MINGW_ENV%" neq "" goto start_mingw_build

set path=%MINGWBASE%\bin;%path%

rem set MINGW_ENV=ok
rem :start_mingw_build

set BUILD_ENV=mingw
set UDEFRAG_LIB_PATH=..\..\lib
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%


:build_scheduler
rem The scheduler is not included in Micro Edition.


:build_installer

echo Build installer...
cd .\bin
copy /Y ..\installer\MicroEdition.nsi .\

%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
if "%1" equ "--use-mingw" goto build_source_package
copy /Y ..\installer\MicroEdition.nsi .\amd64\
cd amd64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

cd..
copy /Y ..\installer\MicroEdition.nsi .\ia64\
cd ia64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 MicroEdition.nsi
if %errorlevel% neq 0 goto fail

cd ..\..

:build_source_package
rem This section was removed to increase compilation speed.
rem Run build.cmd to make an archive of sources.

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

:end
set UD_MICRO_EDITION=
set OLD_PATH=
exit /B

:fail
echo.
echo Build error (code %ERRORLEVEL%)!
set UD_MICRO_EDITION=
set Path=%OLD_PATH%
set OLD_PATH=
exit /B

:clean
call SETVARS.CMD
echo Delete all intermediate files...
cd bin
rd /s /q amd64
rd /s /q ia64
del /s /q *.*
cd ..\obj
rd /s /q bootexctrl
rd /s /q console
rd /s /q driver
rd /s /q gui
rd /s /q include
rd /s /q native
rd /s /q dll
rd /s /q zenwinx
rd /s /q udefrag
rd /s /q lua5.1
del /s /q *.*
cd ..
call BLDSCHED.CMD --clean
cd lib
del /s /q *.*
rd /s /q amd64
rd /s /q ia64
cd ..
echo Done.
set UD_MICRO_EDITION=
set OLD_PATH=
exit /B

:usage
echo.
echo Synopsis:
echo     build-micro              - perform the build using default options
echo     build-micro --install    - run installer silently after the build
echo     build-micro [--use-winddk or --use-mingw or --use-msvc] [--install]
echo                              - specify your favorite compiler
echo     build-micro --clean      - perform full cleanup instead of the build
echo     build-micro --help       - show this help message
