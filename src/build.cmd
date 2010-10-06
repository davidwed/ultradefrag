@echo off

echo Build script for the UltraDefrag project.
echo Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).

call ParseCommandLine.cmd %*

if %UD_BLD_FLG_DIPLAY_HELP% equ 1 goto usage

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
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\share .\obj\share
xcopy /I /Y /Q    .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx .\obj\zenwinx
xcopy /I /Y /Q    .\lua5.1  .\obj\lua5.1
xcopy /I /Y /Q    .\dll\wgx .\obj\wgx

copy /Y .\obj\share\*.c .\obj\console\

rem we cannot link directly to wgx.dll, because it depends on Lua missing in Micro Edition
copy /Y .\obj\wgx\web-analytics.c .\obj\console\
copy /Y .\obj\wgx\wgx.h .\obj\console\
copy /Y .\obj\wgx\dbg.c .\obj\console\

rem xcopy /I /Y /Q /S source destination

mkdir obj\dll
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\
mkdir obj\dll\wgx
copy /Y obj\wgx\wgx.h obj\dll\wgx

call build-targets.cmd %*
if %errorlevel% neq 0 goto fail

call build-docs.cmd
if %errorlevel% neq 0 goto fail

if %UD_BLD_FLG_IS_PORTABLE% equ 1 goto end

echo Build installer...
cd .\bin

:build_x86_installer
if %UD_BLD_FLG_BUILD_X86% EQU 0 goto build_amd64_installer

copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\*.nsh .\
copy /Y ..\installer\lang.ini .\
copy /Y ..\installer\lang-classical.ini .\

rem Executables are too small to use upx.
rem upx udefrag.exe
rem if %errorlevel% neq 0 goto fail

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

:build_amd64_installer
if %UD_BLD_FLG_BUILD_AMD64% equ 0 goto build_ia64_installer

copy /Y ..\installer\UltraDefrag.nsi .\amd64\
copy /Y ..\installer\*.nsh .\amd64\
copy /Y ..\installer\lang.ini .\amd64\
copy /Y ..\installer\lang-classical.ini .\amd64\

cd amd64

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail
cd..

:build_ia64_installer
if %UD_BLD_FLG_BUILD_IA64% equ 0 goto build_source_package

copy /Y ..\installer\UltraDefrag.nsi .\ia64\
copy /Y ..\installer\*.nsh .\ia64\
copy /Y ..\installer\lang.ini .\ia64\
copy /Y ..\installer\lang-classical.ini .\ia64\

cd ia64

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail
cd..

:build_source_package
cd ..

if %UD_BLD_FLG_IS_PRE_RELEASE% equ 1 goto install

echo Build source code package...

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
"%SEVENZIP_PATH%\7z.exe" a -r -mx9 ultradefrag-%ULTRADFGVER%.src.7z *
if %errorlevel% neq 0 goto fail
rd /s /q ultradefrag-%ULTRADFGVER%
cd ..\src
move /Y ..\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\src_package\
echo Build success!

:install
if %UD_BLD_FLG_DO_INSTALL% equ 0 goto end

echo Start installer...
if not %PROCESSOR_ARCHITECTURE% == x86 goto install_amd64
.\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_amd64
if not %PROCESSOR_ARCHITECTURE% == AMD64 goto install_ia64
.\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_ia64
if not %PROCESSOR_ARCHITECTURE% == IA64 goto install_finished
.\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe /S
if %errorlevel% neq 0 goto fail_inst

:install_finished
echo Install success!
goto end

:fail_inst
echo Install error!
goto end_fail

:fail
echo Build error (code %ERRORLEVEL%)!

:end_fail
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 1

:end
set Path=%OLD_PATH%
set OLD_PATH=
exit /B 0

:usage
call build-help.cmd "%~n0"
