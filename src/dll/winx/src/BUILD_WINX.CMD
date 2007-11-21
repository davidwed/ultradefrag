@echo off

rem Build script for the WINX project.
rem Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).

echo Set environment variables...
call SETVARS_WINX.CMD
echo #define WINX_VERSION %WINX_VERSION% > .\winxver.h
echo #define WINX_VERSION2 %WINX_VERSION2% >> .\winxver.h

if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--clean" goto clean
if "%1" equ "--use-msvc" goto msvc_build
:ddk_build
call DDK_BUILD_WINX.CMD
if %errorlevel% neq 0 goto fail
goto build_source_package
:msvc_build
call MSVC_BUILD_WINX.CMD
if %errorlevel% neq 0 goto fail

:build_source_package
echo Build source code package...
rmdir /s /q .\src_package
mkdir .\src_package
set SRC_PKG_PATH=.\src_package\winx-%WINXVER%
mkdir %SRC_PKG_PATH%\src
mkdir %SRC_PKG_PATH%\bin
mkdir %SRC_PKG_PATH%\bin\amd64
mkdir %SRC_PKG_PATH%\bin\ia64
if "%1" equ "--use-msvc" goto msvc_bin
:winddk_bin
copy /Y .\objfre_wnet_x86\i386\winx.dll %SRC_PKG_PATH%\bin\
copy /Y .\objfre_wnet_x86\i386\winx.lib %SRC_PKG_PATH%\bin\
copy /Y .\objfre_wnet_amd64\amd64\winx.dll %SRC_PKG_PATH%\bin\amd64\
copy /Y .\objfre_wnet_amd64\amd64\winx.lib %SRC_PKG_PATH%\bin\amd64\
copy /Y .\objfre_wnet_ia64\ia64\winx.dll %SRC_PKG_PATH%\bin\ia64\
copy /Y .\objfre_wnet_ia64\ia64\winx.lib %SRC_PKG_PATH%\bin\ia64\
goto xxx
:msvs_bin
copy /Y .\objfre_wnet_x86\i386\winx.dll %SRC_PKG_PATH%\bin\
copy /Y .\objfre_wnet_x86\i386\winx.lib %SRC_PKG_PATH%\bin\
:xxx
copy .\*.* %SRC_PKG_PATH%\src\
cd .\src_package
del /s /q *.ncb,*.opt,*.plg,*.aps,*.7z
REM zip -r -m -9 -X winx-%WINXVER%.src.zip .
"%SEVENZIP_PATH%\7z.exe" a winx-%WINXVER%.src.7z *
if %errorlevel% neq 0 goto fail
rd /s /q winx-%WINXVER%
cd ..
echo Build success!

if "%1" equ "--install" goto install
if "%2" neq "--install" goto end
:install
echo copy winx.dll to system directory
if "%1" equ "--use-msvc" goto msvc_install
:winddk_install
copy /Y .\objfre_wnet_x86\i386\winx.dll %windir%\system32\
if %errorlevel% neq 0 goto fail_inst
echo Install success!
goto end
:msvc_install
copy /Y .\bin\winx.dll %windir%\system32\
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
