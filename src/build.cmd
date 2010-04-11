@echo off

echo Build script for the UltraDefrag project.
echo Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS.CMD
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

rem DELETE ALL PREVIOUSLY COMPILED FILES
call cleanup.cmd %1
if "%1" equ "--clean" goto end

set UDEFRAG_PORTABLE=
if "%1" equ "--portable" goto portable
if "%2" neq "--portable" goto not_portable
:portable
set UDEFRAG_PORTABLE=1
:not_portable 

echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.h
rem echo #define NGVERSIONINTITLE "UltraDefrag Next Generation v%ULTRADFGVER%" >> .\include\ultradfgver.h

echo %ULTRADFGVER% > ..\doc\html\version.ini

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
xcopy /I /Y /Q    .\gui     .\obj\gui
xcopy /I /Y /Q    .\gui\res .\obj\gui\res
xcopy /I /Y /Q    .\udefrag-gui-config .\obj\udefrag-gui-config
xcopy /I /Y /Q    .\udefrag-gui-config\res .\obj\udefrag-gui-config\res
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
:build_installer

call build-docs.cmd
if %errorlevel% neq 0 goto fail

if "%1" equ "--portable" goto end
if "%2" equ "--portable" goto end

echo Build installer...
cd .\bin
copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\UltraDefrag.nsh .\
copy /Y ..\installer\UltraDefrag.ico .\
copy /Y ..\installer\lang.ini .\
copy /Y ..\installer\lang-classical.ini .\

if "%1" equ "--use-mingw-x64" goto build_x64_installer

rem Executables are too small to use upx.
rem upx udefrag.exe
rem if %errorlevel% neq 0 goto fail

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
if "%1" equ "--use-mingw" goto build_source_package
if "%1" equ "--install" goto build_source_package
if "%1" equ "" goto build_source_package

:build_x64_installer
copy /Y ..\installer\UltraDefrag.nsi .\amd64\
copy /Y ..\installer\UltraDefrag.nsh .\amd64\
copy /Y ..\installer\UltraDefrag.ico .\amd64\
copy /Y ..\installer\lang.ini .\amd64\
copy /Y ..\installer\lang-classical.ini .\amd64\

cd amd64
"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
if "%1" equ "--use-pellesc" goto build_source_package
if "%1" equ "--use-mingw-x64" goto build_source_package
copy /Y ..\installer\UltraDefrag.nsi .\ia64\
copy /Y ..\installer\UltraDefrag.nsh .\ia64\
copy /Y ..\installer\UltraDefrag.ico .\ia64\
copy /Y ..\installer\lang.ini .\ia64\
copy /Y ..\installer\lang-classical.ini .\ia64\
cd ia64
"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..

if "%2" equ "--pre-release" goto end

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
xcopy /I /Y /Q /S .\doxy-doc %SRC_PKG_PATH%\src\doxy-doc
xcopy /I /Y /Q /S .\dll\zenwinx\doxy-doc %SRC_PKG_PATH%\src\dll\zenwinx\doxy-doc

mkdir %SRC_PKG_PATH%\doc
mkdir %SRC_PKG_PATH%\doc\html
mkdir %SRC_PKG_PATH%\doc\html\handbook
mkdir %SRC_PKG_PATH%\doc\html\handbook\rsc
mkdir %SRC_PKG_PATH%\doc\html\handbook\doxy-doc
mkdir %SRC_PKG_PATH%\doc\html\handbook\doxy-doc\html
copy /Y ..\doc\html\handbook\doxy-doc\html\*.* %SRC_PKG_PATH%\doc\html\handbook\doxy-doc\html\
copy /Y ..\doc\html\handbook\rsc\*.* %SRC_PKG_PATH%\doc\html\handbook\rsc\
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
