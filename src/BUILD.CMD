@echo off

echo Build script for the UltraDefrag project.
echo Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS.CMD

rem DELETE ALL PREVIOUSLY COMPILED FILES
call cleanup.cmd
if "%1" equ "--clean" goto end

set UDEFRAG_PORTABLE=
if "%2" neq "--portable" goto not_portable
set UDEFRAG_PORTABLE=1
:not_portable 

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
xcopy /I /Y /Q    .\gui     .\obj\gui
xcopy /I /Y /Q    .\gui\res .\obj\gui\res
xcopy /I /Y /Q    .\gui-launcher .\obj\gui-launcher
xcopy /I /Y /Q    .\udefrag-gui-config .\obj\udefrag-gui-config
xcopy /I /Y /Q    .\udefrag-gui-config\res .\obj\udefrag-gui-config\res
xcopy /I /Y /Q    .\scheduler\C .\obj\udefrag-scheduler
xcopy /I /Y /Q    .\scheduler\C\res .\obj\udefrag-scheduler\res
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\share .\obj\share
xcopy /I /Y /Q    .\dll\udefrag-kernel .\obj\udefrag-kernel
xcopy /I /Y /Q    .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx .\obj\zenwinx
xcopy /I /Y /Q    .\lua5.1  .\obj\lua5.1
xcopy /I /Y /Q    .\dll\wgx .\obj\wgx

copy /Y .\obj\share\*.c .\obj\console\

rem xcopy /I /Y /Q /S source destination

mkdir obj\dll
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\
mkdir obj\dll\wgx
copy /Y obj\wgx\wgx.h obj\dll\wgx

call build-targets.cmd %1
if %errorlevel% neq 0 goto fail

:build_scheduler
rem Since 3.2.0 version of the program we are using native Scheduler application
rem call BLDSCHED.CMD


:build_installer

if "%2" equ "--portable" goto end

echo Build installer...
cd .\bin
copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\UltraDefrag.nsh .\
copy /Y ..\installer\lang.ini .\
copy /Y ..\installer\lang-classical.ini .\
copy /Y ..\installer\driver.ini .\
copy /Y ..\installer\LanguageSelectorSmall.bmp .\

if "%1" equ "--use-mingw-x64" goto build_x64_installer

rem Executables are too small to use upx.
rem upx udefrag.exe
rem if %errorlevel% neq 0 goto fail

%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
if "%1" equ "--use-mingw" goto build_source_package

:build_x64_installer
rem copy /Y .\UltraDefragScheduler.NET.exe .\amd64\
copy /Y ..\installer\UltraDefrag.nsi .\amd64\
copy /Y ..\installer\UltraDefrag.nsh .\amd64\
copy /Y ..\installer\lang.ini .\amd64\
copy /Y ..\installer\lang-classical.ini .\amd64\
copy /Y ..\installer\driver.ini .\amd64\
copy /Y ..\installer\LanguageSelectorSmall.bmp .\amd64\

cd amd64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
if "%1" equ "--use-pellesc" goto build_source_package
if "%1" equ "--use-mingw-x64" goto build_source_package
rem copy /Y .\UltraDefragScheduler.NET.exe .\ia64\
copy /Y ..\installer\UltraDefrag.nsi .\ia64\
copy /Y ..\installer\UltraDefrag.nsh .\ia64\
copy /Y ..\installer\lang.ini .\ia64\
copy /Y ..\installer\lang-classical.ini .\ia64\
copy /Y ..\installer\driver.ini .\ia64\
copy /Y ..\installer\LanguageSelectorSmall.bmp .\ia64\
cd ia64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..

:build_source_package

echo Build source code package...
cd ..

rem Recreate src_package directory
rmdir /s /q .\src_package
mkdir .\src_package

rem Set SRC_PKG_PATH pointing to directory one level above the current directory
set SRC_PKG_PATH=..\src_package\ultradefrag-%ULTRADFGVER%
rmdir /s /q ..\src_package
mkdir ..\src_package

xcopy /I /Y /Q /S /EXCLUDE:exclude-from-sources.lst . %SRC_PKG_PATH%\src

mkdir %SRC_PKG_PATH%\doc
mkdir %SRC_PKG_PATH%\doc\html
mkdir %SRC_PKG_PATH%\doc\html\handbook
copy /Y ..\doc\html\handbook\*.* %SRC_PKG_PATH%\doc\html\handbook\

cd ..\src_package
REM zip -r -m -9 -X ultradefrag-%ULTRADFGVER%.src.zip .
"%SEVENZIP_PATH%\7z.exe" a ultradefrag-%ULTRADFGVER%.src.7z *
if %errorlevel% neq 0 goto fail
rd /s /q ultradefrag-%ULTRADFGVER%
cd ..\src
move /Y ..\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\src_package\
echo Build success!

if "%1" equ "--install" goto install
if "%2" neq "--install" goto end
:install
echo Start installer...
.\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe /S
if %errorlevel% neq 0 goto fail_inst
echo Install success!
goto end
:fail_inst
echo Install error!
goto end_1

:fail
echo Build error (code %ERRORLEVEL%)!

:end_1
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 1

:end
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 0

:usage
call build-targets.cmd --help build
