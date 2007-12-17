@echo off

rem Build script for the ZenWINX project.
rem Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).

echo Set environment variables...
call SETVARS_ZENWINX.CMD
echo #define ZENWINX_VERSION %ZENWINX_VERSION% > .\zenwinxver.h
echo #define ZENWINX_VERSION2 %ZENWINX_VERSION2% >> .\zenwinxver.h

if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--clean" goto clean
if "%1" equ "--use-msvc" goto msvc_build
if "%1" equ "--use-mingw" goto mingw_build
:ddk_build
call DDK_BUILD_ZENWINX.CMD
if %errorlevel% neq 0 goto fail
goto build_source_package
:msvc_build
call MSVC_BUILD_ZENWINX.CMD
if %errorlevel% neq 0 goto fail
goto build_source_package
:mingw_build
call MINGW_BUILD_ZENWINX
if %errorlevel% neq 0 goto fail

:build_source_package
echo Build source code package...
rmdir /s /q .\src_package
mkdir .\src_package
set SRC_PKG_PATH=.\src_package\zenwinx-%ZENWINXVER%
mkdir %SRC_PKG_PATH%\src
mkdir %SRC_PKG_PATH%\bin
mkdir %SRC_PKG_PATH%\bin\amd64
mkdir %SRC_PKG_PATH%\bin\ia64
if "%1" equ "--use-msvc" goto msvc_bin
if "%1" equ "--use-mingw" goto mingw_bin
:winddk_bin
copy /Y .\objfre_wnet_x86\i386\zenwinx.dll %SRC_PKG_PATH%\bin\
copy /Y .\objfre_wnet_x86\i386\zenwinx.lib %SRC_PKG_PATH%\bin\
copy /Y .\objfre_wnet_amd64\amd64\zenwinx.dll %SRC_PKG_PATH%\bin\amd64\
copy /Y .\objfre_wnet_amd64\amd64\zenwinx.lib %SRC_PKG_PATH%\bin\amd64\
copy /Y .\objfre_wnet_ia64\ia64\zenwinx.dll %SRC_PKG_PATH%\bin\ia64\
copy /Y .\objfre_wnet_ia64\ia64\zenwinx.lib %SRC_PKG_PATH%\bin\ia64\
goto xxx
:msvs_bin
copy /Y .\bin\zenwinx.dll %SRC_PKG_PATH%\bin\
copy /Y .\bin\zenwinx.lib %SRC_PKG_PATH%\bin\
goto xxx
:mingw_bin
copy /Y .\bin\zenwinx.dll %SRC_PKG_PATH%\bin\
copy /Y .\bin\libzenwinx.dll.a %SRC_PKG_PATH%\bin\
:xxx
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
if "%1" equ "--use-msvc" goto msvc_install
:winddk_install
copy /Y .\objfre_wnet_x86\i386\zenwinx.dll %windir%\system32\
if %errorlevel% neq 0 goto fail_inst
echo Install success!
goto end
:msvc_install
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
rd /s /q objfre_wnet_x86
rd /s /q objfre_wnet_amd64
rd /s /q objfre_wnet_ia64
echo Done.
