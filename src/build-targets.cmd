@echo off

rem This script builds all binaries for UltraDefrag project
rem for all of the supported targets.
rem Usage:
rem     build-targets [<compiler>]
rem Available <compiler> values:
rem     --use-winddk (default)
rem     --use-winsdk
rem     --use-mingw
rem     --use-msvc
rem     --use-pellesc (experimental)
rem     --use-mingw-x64 (experimental)

if "%1" neq "--help" goto build
if "%2" equ "build" goto display_build_options
if "%2" equ "build-micro" goto display_build_micro_options

:display_build_options
echo.
echo Synopsis:
echo     build              - perform the build using default options
echo     build --install    - run installer silently after the build
echo     build [compiler] [--install] - specify your favorite compiler:
echo                        --use-winddk    (default)
echo                        --use-winsdk
echo                        --use-mingw
echo                        --use-msvc
echo                        --use-pellesc   (experimental)
echo                        --use-mingw-x64 (experimental)
echo     build --clean      - perform full cleanup instead of the build
echo     build --help       - show this help message
exit /B 0

:display_build_micro_options
echo.
echo Synopsis:
echo     build-micro              - perform the build using default options
echo     build-micro --install    - run installer silently after the build
echo     build-micro [compiler] [--install] - specify your favorite compiler:
echo                              --use-winddk    (default)
echo                              --use-winsdk
echo                              --use-mingw
echo                              --use-msvc
echo                              --use-pellesc   (experimental)
echo                              --use-mingw-x64 (experimental)
echo     build-micro --clean      - perform full cleanup instead of the build
echo     build-micro --help       - show this help message
exit /B 0

:build
if "%1" neq "" goto parse_first_parameter
echo No parameters specified, use defaults.
goto ddk_build

:parse_first_parameter
if "%1" equ "--use-msvc" goto msvc_build
if "%1" equ "--use-mingw" goto mingw_build
if "%1" equ "--use-pellesc" goto pellesc_build
if "%1" equ "--use-mingw-x64" goto mingw_x64_build
if "%1" equ "--use-winsdk" goto winsdk_build

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

goto success

:winsdk_build

set BUILD_ENV=winsdk

echo --------- Target is x86 ---------
set AMD64=
set IA64=
pushd ..
call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x86 /xp
popd
set UDEFRAG_LIB_PATH=..\..\lib
rem update manifests...
call make-manifests.cmd X86
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is x64 ---------
set IA64=
set AMD64=1
pushd ..
call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
popd
set UDEFRAG_LIB_PATH=..\..\lib\amd64
rem update manifests...
call make-manifests.cmd amd64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

echo --------- Target is ia64 ---------
set AMD64=
set IA64=1
pushd ..
call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /ia64 /xp
popd
set BUILD_DEFAULT=-nmake -i -g -P
set UDEFRAG_LIB_PATH=..\..\lib\ia64
rem update manifests...
call make-manifests.cmd ia64
call blditems.cmd
if %errorlevel% neq 0 goto fail
set Path=%OLD_PATH%

goto success

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

goto success

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

goto success

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

goto success

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

goto success


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

:success
exit /B 0

:fail
exit /B 1
