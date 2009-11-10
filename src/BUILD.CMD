@echo off

echo Build script for the UltraDefrag project.
echo Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS.CMD
if "%1" equ "--clean" goto clean

rem DELETE ALL PREVIOUSLY COMPILED FILES
call BUILD.CMD --clean
set OLD_PATH=%path%
call SETVARS.CMD

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


if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--use-msvc" goto msvc_build
if "%1" equ "--use-mingw" goto mingw_build
if "%1" equ "--use-pellesc" goto pellesc_build
if "%1" equ "--use-mingw-x64" goto mingw_x64_build

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
rem update manifests...
call make-manifests.cmd X86
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
rem update manifests...
call make-manifests.cmd amd64
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
rem update manifests...
call make-manifests.cmd ia64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_scheduler

:pellesc_build

set BUILD_ENV=pellesc

echo --------- Target is x86 ---------
set AMD64=
set IA64=
set ARM=
set Path=%path%;%PELLESC_BASE%\Bin
set UDEFRAG_LIB_PATH=..\..\lib
rem update manifests...
call make-manifests.cmd X86
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_scheduler

echo --------- Target is x64 ---------
set IA64=
set AMD64=1
set ARM=
set Path=%path%;%PELLESC_BASE%\Bin
set UDEFRAG_LIB_PATH=..\..\lib\amd64
rem update manifests...
call make-manifests.cmd amd64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is ARM ---------
set IA64=
set AMD64=
set ARM=1
set Path=%path%;%PELLESC_BASE%\Bin
set UDEFRAG_LIB_PATH=..\..\lib\amd64
rem update manifests...
call make-manifests.cmd amd64
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
rem update manifests...
call make-manifests.cmd X86
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_scheduler

:mingw_x64_build

echo --------- Target is x64 ---------
set AMD64=1

set path=%MINGWx64BASE%\bin;%path%

set BUILD_ENV=mingw_x64
set UDEFRAG_LIB_PATH=..\..\lib\amd64
rem update manifests...
call make-manifests.cmd amd64
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
rem update manifests...
call make-manifests.cmd X86
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%


:build_scheduler
rem Since 3.2.0 version of the program we are using native Scheduler application
rem call BLDSCHED.CMD


:build_installer

echo Build installer...
cd .\bin
copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\UltraDefrag.nsh .\
copy /Y ..\installer\lang.ini .\
copy /Y ..\installer\driver.ini .\

if "%1" equ "--use-mingw-x64" goto build_x64_installer

rem echo Build the gui launcher program
rem copy /Y ..\installer\udefrag-gui.nsi .\
rem %NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% udefrag-gui.nsi
rem if %errorlevel% neq 0 goto fail

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

:build_x64_installer
rem copy /Y .\UltraDefragScheduler.NET.exe .\amd64\
copy /Y ..\installer\UltraDefrag.nsi .\amd64\
copy /Y ..\installer\UltraDefrag.nsh .\amd64\
copy /Y ..\installer\lang.ini .\amd64\
copy /Y ..\installer\driver.ini .\amd64\
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
copy /Y ..\installer\driver.ini .\ia64\
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
xcopy /I /Y /Q    .\bootexctrl %SRC_PKG_PATH%\src\bootexctrl
xcopy /I /Y /Q    .\hibernate %SRC_PKG_PATH%\src\hibernate
xcopy /I /Y /Q    .\console %SRC_PKG_PATH%\src\console
xcopy /I /Y /Q /S .\gui %SRC_PKG_PATH%\src\gui
xcopy /I /Y /Q    .\gui-launcher %SRC_PKG_PATH%\src\gui-launcher
xcopy /I /Y /Q /S .\udefrag-gui-config %SRC_PKG_PATH%\src\udefrag-gui-config
xcopy /I /Y /Q    .\native %SRC_PKG_PATH%\src\native
xcopy /I /Y /Q    .\installer %SRC_PKG_PATH%\src\installer
xcopy /I /Y /Q    .\include %SRC_PKG_PATH%\src\include
xcopy /I /Y /Q    .\share %SRC_PKG_PATH%\src\share

rem mkdir %SRC_PKG_PATH%\src\scheduler
rem xcopy /I /Y /Q .\scheduler\DotNET %SRC_PKG_PATH%\src\scheduler\DotNET
rem xcopy /I /Y /Q .\scheduler\DotNET\Properties %SRC_PKG_PATH%\src\scheduler\DotNET\Properties

xcopy /I /Y /Q /S .\scheduler %SRC_PKG_PATH%\src\scheduler

rem mkdir %SRC_PKG_PATH%\src\dll
rem xcopy /I /Y /Q .\dll\udefrag %SRC_PKG_PATH%\src\dll\udefrag
rem xcopy /I /Y /Q .\dll\zenwinx %SRC_PKG_PATH%\src\dll\zenwinx
rem xcopy /I /Y /Q .\dll\wgx %SRC_PKG_PATH%\src\dll\wgx

xcopy /I /Y /Q /S .\dll %SRC_PKG_PATH%\src\dll

xcopy /I /Y /Q .\lua5.1 %SRC_PKG_PATH%\src\lua5.1

rem mkdir %SRC_PKG_PATH%\src\tools
rem copy /Y .\tools\*.pl %SRC_PKG_PATH%\src\tools\
rem copy /Y .\tools\*.lua %SRC_PKG_PATH%\src\tools\
rem copy /Y .\tools\readme.txt %SRC_PKG_PATH%\src\tools\

xcopy /I /Y /Q /S .\tools %SRC_PKG_PATH%\src\tools

rem mkdir %SRC_PKG_PATH%\src\scripts
rem copy /Y .\scripts\*.* %SRC_PKG_PATH%\src\scripts\

xcopy /I /Y /Q /S .\scripts %SRC_PKG_PATH%\src\scripts

rem mkdir %SRC_PKG_PATH%\src\icons-for-mdsp-files
rem copy /Y .\icons-for-mdsp-files\*.* %SRC_PKG_PATH%\src\icons-for-mdsp-files\

xcopy /I /Y /Q /S .\icons-for-mdsp-files %SRC_PKG_PATH%\src\icons-for-mdsp-files

mkdir %SRC_PKG_PATH%\doc
mkdir %SRC_PKG_PATH%\doc\html
mkdir %SRC_PKG_PATH%\doc\html\handbook
copy /Y ..\doc\html\handbook\*.* %SRC_PKG_PATH%\doc\html\handbook\

copy .\*.* %SRC_PKG_PATH%\src\

rd /s /q %SRC_PKG_PATH%\src\dll\udefrag-kernel\doxy-doc
rd /s /q %SRC_PKG_PATH%\src\dll\udefrag-kernel\robo-doc
rd /s /q %SRC_PKG_PATH%\src\dll\udefrag\doxy-doc
rd /s /q %SRC_PKG_PATH%\src\dll\udefrag\robo-doc
rd /s /q %SRC_PKG_PATH%\src\dll\zenwinx\robo-doc
rd /s /q %SRC_PKG_PATH%\src\scheduler\DotNET\bin
rd /s /q %SRC_PKG_PATH%\src\scheduler\DotNET\obj

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
rd /s /q bootexctrl
rd /s /q hibernate
rd /s /q console
rd /s /q driver
rd /s /q gui
rd /s /q gui-launcher
rd /s /q udefrag-gui-config
rd /s /q include
rd /s /q share
rd /s /q native
rd /s /q dll
rd /s /q zenwinx
rd /s /q udefrag-kernel
rd /s /q udefrag
rd /s /q lua5.1
rd /s /q wgx
rd /s /q udefrag-scheduler
del /s /q *.*
cd ..
rem call BLDSCHED.CMD --clean
cd lib
del /s /q *.*
rd /s /q amd64
rd /s /q ia64
cd ..
echo Done.
set OLD_PATH=
exit /B

:usage
echo.
echo Synopsis:
echo     build              - perform the build using default options
echo     build --install    - run installer silently after the build
echo     build [compiler] [--install] - specify your favorite compiler:
echo                        --use-winddk
echo                        --use-mingw
echo                        --use-msvc 
echo                        --use-pellesc
echo                        --use-mingw-x64
echo     build --clean      - perform full cleanup instead of the build
echo     build --help       - show this help message
