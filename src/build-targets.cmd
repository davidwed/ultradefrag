@echo off

rem This script builds all binaries for UltraDefrag project
rem for all of the supported targets.
rem Usage:
rem     build-targets [<compiler>]
rem Available <compiler> values:
rem     --use-mingw (default)
rem     --use-msvc
rem     --use-winddk
rem     --use-winsdk
rem     --use-pellesc (experimental)
rem     --use-mingw-x64 (experimental)

rem NOTE: IA-64 targeting binaries were never tested by the authors 
rem due to missing appropriate hardware and appropriate 64-bit version 
rem of Windows.

if "%1" neq "--help" goto build
if "%2" equ "build" goto display_build_options
if "%2" equ "build-micro" goto display_build_micro_options

:display_build_options
echo.
echo Synopsis:
echo     build              - perform the build using default options
echo     build --install    - run installer silently after the build
echo     build [compiler] [--install] - specify your favorite compiler:
echo.
echo                        --use-mingw     (default)
echo.
echo                        --use-msvc      (obsolete)
echo                        --use-winsdk    (obsolete, produces wrong x64 code)
echo                        --use-winddk    (obsolete, produces wrong x64 code)
echo                        --use-pellesc   (experimental, produces wrong code)
echo                        --use-mingw-x64 (experimental, produces wrong x64 code)
echo     build --clean      - perform full cleanup instead of the build
echo     build --help       - show this help message
exit /B 0

:display_build_micro_options
echo.
echo Synopsis:
echo     build-micro              - perform the build using default options
echo     build-micro --install    - run installer silently after the build
echo     build-micro [compiler] [--install] - specify your favorite compiler:
echo.
echo                              --use-mingw (default)
echo.
echo                              --use-msvc  (obsolete)
echo                              --use-winsdk
echo                                   (obsolete, produces wrong x64 code)
echo                              --use-winddk
echo                                   (obsolete, produces wrong x64 code)
echo                              --use-pellesc
echo                                   (experimental, produces wrong code)
echo                              --use-mingw-x64
echo                                   (experimental, produces wrong x64 code)
echo     build-micro --clean      - perform full cleanup instead of the build
echo     build-micro --help       - show this help message
exit /B 0

:build
if "%1" equ "--portable" goto default_build
if "%1" equ "--install" goto default_build
if "%1" neq "" goto parse_first_parameter

:default_build
echo No parameters specified, use defaults.
goto mingw_build

:parse_first_parameter
if "%1" equ "--use-winddk" goto ddk_build
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

call "%MSVSBIN%\vcvars32.bat"

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

set path=%MINGWBASE%\bin;%path%

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
