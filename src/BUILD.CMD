@echo off

echo Build script for the UltraDefrag project.
echo Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS.CMD
if "%1" equ "--clean" goto clean

echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
REM echo #define VERSIONINTITLE_U L"UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "About UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

xcopy /I /Y /Q    .\driver  .\obj\driver
xcopy /I /Y /Q    .\console .\obj\console
xcopy /I /Y /Q    .\gui     .\obj\gui
xcopy /I /Y /Q    .\gui\res .\obj\gui\res
xcopy /I /Y /Q    .\native  .\obj\native
xcopy /I /Y /Q    .\include .\obj\include
xcopy /I /Y /Q    .\dll\udefrag     .\obj\udefrag
xcopy /I /Y /Q    .\dll\zenwinx\src .\obj\zenwinx
xcopy /I /Y /Q    .\lua5.1  .\obj\lua5.1
xcopy /I /Y /Q /S .\gui\i18n\*.* .\obj

mkdir obj\dll
mkdir obj\dll\zenwinx
mkdir obj\dll\zenwinx\src
copy /Y obj\zenwinx\ntndk.h obj\dll\zenwinx\src\
copy /Y obj\zenwinx\zenwinx.h obj\dll\zenwinx\src\
mkdir obj\zenwinx\src
copy /Y obj\zenwinx\zenwinx.h obj\zenwinx\src\

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

call BLDSCHED.CMD


:build_installer

echo Build installer...
cd .\bin
copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\lang.ini .\

rem Executables are too small to use upx.
rem But upx is used to pack installer
rem (see INSTALL.TXT for more details).
rem upx udefrag.exe
rem if %errorlevel% neq 0 goto fail
rem upx dfrg.exe
rem if %errorlevel% neq 0 goto fail

%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
if "%1" equ "--use-mingw" goto build_source_package
copy /Y .\UltraDefragScheduler.NET.exe .\amd64\
copy /Y ..\installer\UltraDefrag.nsi .\amd64\
copy /Y ..\installer\lang.ini .\amd64\
cd amd64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
copy /Y .\UltraDefragScheduler.NET.exe .\ia64\
copy /Y ..\installer\UltraDefrag.nsi .\ia64\
copy /Y ..\installer\lang.ini .\ia64\
cd ia64
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..

:build_source_package

echo Build source code package...
cd ..
rmdir /s /q .\src_package
mkdir .\src_package
set SRC_PKG_PATH=.\src_package\ultradefrag-%ULTRADFGVER%
mkdir %SRC_PKG_PATH%\src
xcopy /I /Y /Q    .\driver %SRC_PKG_PATH%\src\driver
xcopy /I /Y /Q    .\console %SRC_PKG_PATH%\src\console
xcopy /I /Y /Q /S .\gui %SRC_PKG_PATH%\src\gui
xcopy /I /Y /Q    .\native %SRC_PKG_PATH%\src\native
xcopy /I /Y /Q    .\installer %SRC_PKG_PATH%\src\installer
xcopy /I /Y /Q    .\include %SRC_PKG_PATH%\src\include
xcopy /I /Y /Q    .\presets %SRC_PKG_PATH%\src\presets
mkdir %SRC_PKG_PATH%\src\scheduler
xcopy /I /Y /Q .\scheduler\DotNET %SRC_PKG_PATH%\src\scheduler\DotNET
xcopy /I /Y /Q .\scheduler\DotNET\Properties %SRC_PKG_PATH%\src\scheduler\DotNET\Properties
mkdir %SRC_PKG_PATH%\src\dll
xcopy /I /Y .\dll\udefrag %SRC_PKG_PATH%\src\dll\udefrag
mkdir %SRC_PKG_PATH%\src\dll\zenwinx
xcopy /I /Y .\dll\zenwinx\src %SRC_PKG_PATH%\src\dll\zenwinx\src

xcopy /I /Y /Q    .\lua5.1 %SRC_PKG_PATH%\src\lua5.1

mkdir %SRC_PKG_PATH%\src\tools
copy /Y .\tools\*.pl %SRC_PKG_PATH%\src\tools\
copy /Y .\tools\*.lua %SRC_PKG_PATH%\src\tools\
copy /Y .\tools\readme.txt %SRC_PKG_PATH%\src\tools\

mkdir %SRC_PKG_PATH%\src\scripts
copy /Y .\scripts\*.* %SRC_PKG_PATH%\src\scripts\

mkdir %SRC_PKG_PATH%\doc
mkdir %SRC_PKG_PATH%\doc\html
mkdir %SRC_PKG_PATH%\doc\html\images
copy /Y ..\doc\html\manual.html %SRC_PKG_PATH%\doc\html\
copy /Y ..\doc\html\about.html %SRC_PKG_PATH%\doc\html\
copy /Y ..\doc\html\about_simple.html %SRC_PKG_PATH%\doc\html\
copy /Y ..\doc\html\images\console.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\main_screen110.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\about.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\sched_net_vista.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\valid-html401.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\powered_by_lua.png %SRC_PKG_PATH%\doc\html\images\

copy .\*.* %SRC_PKG_PATH%\src\
cd .\src_package
del /s /q *.ncb,*.opt,*.plg,*.aps,*.7z,*.bk
REM zip -r -m -9 -X ultradefrag-%ULTRADFGVER%.src.zip .
"%SEVENZIP_PATH%\7z.exe" a ultradefrag-%ULTRADFGVER%.src.7z *
if %errorlevel% neq 0 goto fail
rd /s /q ultradefrag-%ULTRADFGVER%
cd ..
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

:end
set OLD_PATH=
exit /B

:fail
echo Build error (code %ERRORLEVEL%)!
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
cd ..\dll\zenwinx\src
call BUILD_ZENWINX.CMD --clean
cd ..\..\..
echo Done.
set OLD_PATH=
exit /B

:usage
echo.
echo Synopsis:
echo     build              - perform the build using default options
echo     build --install    - run installer silently after the build
echo     build [--use-winddk or --use-mingw or --use-msvc] [--install]
echo                        - specify your favorite compiler
echo     build --clean      - perform full cleanup instead of the build
echo     build --help       - show this help message
