@echo off

echo Build script for the ZenWINX project.
echo Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

if "%1" equ "--help" goto usage

echo Set environment variables...
set OLD_PATH=%path%
call SETVARS_ZENWINX.CMD
if "%1" equ "--clean" goto clean

echo #define ZENWINX_VERSION %ZENWINX_VERSION% > .\zenwinxver.h
echo #define ZENWINX_VERSION2 %ZENWINX_VERSION2% >> .\zenwinxver.h
mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

mkdir obj
mkdir obj\_

copy /Y .\*.*  .\obj\_\

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
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is x64 ---------
set IA64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre AMD64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is ia64 ---------
set AMD64=
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre 64 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_source_package

:msvc_build

call "%MSVSBIN%\vcvars32.bat"

set BUILD_ENV=msvc
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto build_source_package

:mingw_build

set path=%MINGWBASE%\bin;%path%

set BUILD_ENV=mingw
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

:build_source_package

echo Build source code package...
rmdir /s /q .\src_package
mkdir .\src_package
set SRC_PKG_PATH=.\src_package\zenwinx-%ZENWINXVER%
mkdir %SRC_PKG_PATH%\src
xcopy /I /Y /Q /S .\bin %SRC_PKG_PATH%\bin
xcopy /I /Y /Q /S .\lib %SRC_PKG_PATH%\bin
copy .\*.* %SRC_PKG_PATH%\src\
cd .\src_package
del /s /q *.ncb,*.opt,*.plg,*.aps,*.7z
REM zip -r -m -9 -X zenwinx-%ZENWINXVER%.src.zip .
"%SEVENZIP_PATH%\7z.exe" a zenwinx-%ZENWINXVER%.src.7z *
if %errorlevel% neq 0 goto fail
rd /s /q zenwinx-%ZENWINXVER%
cd ..
echo Build success!

if "%1" equ "--install" goto install
if "%2" neq "--install" goto end
:install
echo copy zenwinx.dll to system directory
copy /Y .\bin\zenwinx.dll %windir%\system32\
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
echo Delete all intermediate files...
rd /s /q bin
rd /s /q obj
rd /s /q lib
echo Done.
exit /B

:usage
echo.
echo Synopsis:
echo     build_zenwinx              - perform the build using default options
echo     build_zenwinx --install    - run installer silently after the build
echo     build_zenwinx [--use-winddk or --use-mingw or --use-msvc] [--install]
echo                                - specify your favorite compiler
echo     build_zenwinx --clean      - perform full cleanup instead of the build
echo     build_zenwinx --help       - show this help message
