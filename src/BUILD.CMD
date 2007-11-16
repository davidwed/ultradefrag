@echo off

rem Build script for the UltraDefrag project.
rem Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).

echo Set environment variables...
call SETVARS.CMD
echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
REM echo #define VERSIONINTITLE_U L"UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "About UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
mkdir lib
mkdir lib\amd64
mkdir lib\ia64

if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--clean" goto clean
if "%1" equ "--use-msvc" goto msvc_build
:ddk_build
call DDK_BUILD.CMD
if %errorlevel% neq 0 goto fail
goto build_scheduler
:msvc_build
call MSVC_BUILD.CMD
if %errorlevel% neq 0 goto fail

:build_scheduler
cd ..
call BLDSCHED.CMD

:build_installer
echo Build installer...
cd .\bin
copy /Y ..\*.TXT .\
copy /Y ..\installer\UltraDefrag.nsi .\
copy /Y ..\installer\*.bmp .\

copy /Y ..\..\doc\html\manual.html .\
copy /Y ..\..\doc\html\images\console.png .\
copy /Y ..\..\doc\html\images\main_screen110.png .\
copy /Y ..\..\doc\html\images\about.png .\
copy /Y ..\..\doc\html\images\valid-html401.png .\

xcopy /I /Y ..\presets .\presets
rem upx udefrag.exe
rem if %errorlevel% neq 0 goto fail
upx dfrg.exe
if %errorlevel% neq 0 goto fail
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH i386 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

if "%1" equ "--use-msvc" goto build_source_package
copy /Y .\UltraDefrag.nsi .\amd64\
copy /Y .\UltraDefragScheduler.NET.exe .\amd64\
copy /Y .\*.TXT .\amd64\
copy /Y .\*.bmp .\amd64\

copy /Y ..\..\doc\html\manual.html .\amd64\
copy /Y ..\..\doc\html\images\console.png .\amd64\
copy /Y ..\..\doc\html\images\main_screen110.png .\amd64\
copy /Y ..\..\doc\html\images\about.png .\amd64\
copy /Y ..\..\doc\html\images\valid-html401.png .\amd64\

xcopy /I /Y .\presets .\amd64\presets
cd amd64
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH amd64 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
copy /Y .\UltraDefrag.nsi .\ia64\
copy /Y .\UltraDefragScheduler.NET.exe .\ia64\
copy /Y .\*.TXT .\ia64\
copy /Y .\*.bmp .\ia64\

copy /Y ..\..\doc\html\manual.html .\ia64\
copy /Y ..\..\doc\html\images\console.png .\ia64\
copy /Y ..\..\doc\html\images\main_screen110.png .\ia64\
copy /Y ..\..\doc\html\images\about.png .\ia64\
copy /Y ..\..\doc\html\images\valid-html401.png .\ia64\

xcopy /I /Y .\presets .\ia64\presets
cd ia64
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH ia64 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
:build_source_package
echo Build source code package...
cd ..
rmdir /s /q .\src_package
mkdir .\src_package
set SRC_PKG_PATH=.\src_package\ultradefrag-%ULTRADFGVER%
mkdir %SRC_PKG_PATH%\src
xcopy /I /Y .\driver %SRC_PKG_PATH%\src\driver
xcopy /I /Y .\console %SRC_PKG_PATH%\src\console
xcopy /I /Y .\gui %SRC_PKG_PATH%\src\gui
xcopy /I /Y .\gui\res %SRC_PKG_PATH%\src\gui\res
xcopy /I /Y .\native %SRC_PKG_PATH%\src\native
xcopy /I /Y .\installer %SRC_PKG_PATH%\src\installer
xcopy /I /Y .\include %SRC_PKG_PATH%\src\include
xcopy /I /Y .\presets %SRC_PKG_PATH%\src\presets
mkdir %SRC_PKG_PATH%\src\scheduler
xcopy /I /Y .\scheduler\DotNET %SRC_PKG_PATH%\src\scheduler\DotNET
xcopy /I /Y .\scheduler\DotNET\Properties %SRC_PKG_PATH%\src\scheduler\DotNET\Properties
mkdir %SRC_PKG_PATH%\src\dll
xcopy /I /Y .\dll\udefrag %SRC_PKG_PATH%\src\dll\udefrag

mkdir %SRC_PKG_PATH%\doc
mkdir %SRC_PKG_PATH%\doc\html
mkdir %SRC_PKG_PATH%\doc\html\images
copy /Y ..\doc\html\manual.html %SRC_PKG_PATH%\doc\html\
copy /Y ..\doc\html\images\console.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\main_screen110.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\about.png %SRC_PKG_PATH%\doc\html\images\
copy /Y ..\doc\html\images\valid-html401.png %SRC_PKG_PATH%\doc\html\images\

copy .\*.* %SRC_PKG_PATH%\src\
cd .\src_package
del /s /q *.ncb,*.opt,*.plg,*.aps,*.7z
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
exit /B

:fail
echo Build error (code %ERRORLEVEL%)!
rem pause
exit /B

:clean
call SETVARS.CMD
echo Delete all intermediate files...
cd bin
rd /s /q amd64
rd /s /q ia64
rd /s /q presets
del /s /q *.*
cd ..\obj
rd /s /q console
rd /s /q driver
rd /s /q gui
rd /s /q include
rd /s /q native
rd /s /q dll\udefrag.dll
rd /s /q dll
del /s /q *.*
cd ..
call BLDSCHED.CMD --clean
cd lib
del /s /q *.*
rd /s /q amd64
rd /s /q ia64
cd..
echo Done.
