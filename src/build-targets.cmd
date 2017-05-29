@echo off
::
:: This script builds UltraDefrag binaries for all the supported target platforms.
:: Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
::
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program; if not, write to the Free Software
:: Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::

rem Usage:
rem     build-targets [<compiler>] [options]
rem
rem Available <compiler> values:
rem     --use-mingw     (default)
rem     --use-winsdk    (we use it for official releases)
rem     --use-mingw-x64 (experimental, produces wrong x64 code)
rem
rem Options:
rem     --no-x86        don't build 32-bit binaries
rem     --no-amd64      don't build x64 binaries
rem     --no-ia64       don't build IA-64 binaries

rem NOTE: IA-64 binaries have never been tested by
rem the authors because of lack of Itanium hardware.

echo Build UltraDefrag binaries...
echo.

call ParseCommandLine.cmd %*

:: set environment variables
if "%ULTRADFGVER%" equ "" (
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

set UD_ROOT=%cd%

:: build list of headers (later on we'll 
:: use them as dependencies in makefiles)
if not exist obj mkdir obj
dir /S /B *.h > obj\headers || exit /B 1

:: build all modules by the selected compiler
if %UD_BLD_FLG_USE_COMPILER% equ 0 (
    echo No parameters specified, using defaults.
    goto mingw_build
)

if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW%   goto mingw_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW64% goto mingw_x64_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINSDK%  goto winsdk_build

:winsdk_build

    set BUILD_ENV=winsdk
    set OLD_PATH=%path%

    if %UD_BLD_FLG_BUILD_X86% neq 0 (
        echo --------- Target is x86 ---------
        set AMD64=
        set IA64=
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x86 /xp
        popd
        call :build_modules X86 || exit /B 1
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
        echo --------- Target is x64 ---------
        set AMD64=1
        set IA64=
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
        popd
        call :build_modules amd64 || exit /B 1
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_IA64% neq 0 (
        echo --------- Target is ia64 ---------
        set AMD64=
        set IA64=1
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /ia64 /xp
        popd
        set BUILD_DEFAULT=-nmake -i -g -P
        call :build_modules ia64 || exit /B 1
    )
    
    :: remove perplexing manifests
    del /S /Q .\bin\*.manifest
    
    :: get rid of annoying dark green color
    color
    
    set path=%OLD_PATH%
    set OLD_PATH=
    
exit /B 0


:mingw_x64_build

    set OLD_PATH=%path%

    echo --------- Target is x64 ---------
    set AMD64=1
    set IA64=
    set path=%MINGWx64BASE%\bin;%path%
    set BUILD_ENV=mingw_x64
    call :build_modules amd64 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:mingw_build

    set OLD_PATH=%path%

    echo --------- Target is x86 ---------
    set AMD64=
    set IA64=
    set path=%MINGWBASE%\bin;%path%
    set BUILD_ENV=mingw
    call :build_modules X86 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:: Builds all UltraDefrag modules
:: Example: call :build_modules X86
:build_modules
    rem update manifests
    call make-manifests.cmd %1 || exit /B 1

    rem set environment
    if %WindowsSDKVersionOverride%x neq v7.1x goto NoWin7SDK
    if x%CommandPromptType% neq xCross goto NoWin7SDK
    set path=%PATH%;%VS100COMNTOOLS%\..\..\VC\Bin

    :NoWin7SDK
    set WXWIDGETS_INC_PATH=%WXWIDGETSDIR%\include
    set WX_CONFIG=%BUILD_ENV%-%1
    if %BUILD_ENV% equ winsdk if %1 equ X86 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ winsdk if %1 equ amd64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_x64_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ winsdk if %1 equ ia64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_ia64_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ mingw (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ mingw_x64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%
    )
    set WXWIDGETS_INC2_PATH=%WXWIDGETS_LIB_PATH%\mswu
    
    rem build modules
    call :build_mod dll\zenwinx  zenwinx.build        || goto fail
    call :build_mod dll\udefrag  udefrag.build        || goto fail
    call :build_mod native       defrag_native.build  || goto fail
    call :build_mod lua5.1       lua.build            || goto fail
    call :build_mod lua          lua.build            || goto fail
    call :build_mod lua-gui      lua-gui.build        || goto fail
    call :build_mod bootexctrl   bootexctrl.build     || goto fail
    call :build_mod hibernate    hibernate.build      || goto fail
    call :build_mod console      console.build        || goto fail
    call :build_mod wxgui        wxgui.build          || goto fail
    call :build_mod dbg          dbg.build            || goto fail

    :: compress gui and command line tools for
    :: a bit faster startup on older machines
    if "%OFFICIAL_RELEASE%" equ "1" if %1 equ X86 (
        upx -q -9 .\bin\udefrag.exe
        upx -q -9 .\bin\ultradefrag.exe
    )
    
    :success
    :: revert manifests to their default state
    if %1 neq X86 call make-manifests.cmd X86

    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    exit /B 0

    :fail
    :: revert manifests to their default state
    if %1 neq X86 call make-manifests.cmd X86

    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    exit /B 1
    
exit /B 0

:: Builds a single module.
:: Synopsis: call :build_mod {path} {build_file}
:: Example:  call :build_mod dll\udefrag udefrag.build
:build_mod
    pushd %1
    for %%* in (.) do set UD_MOD_FOLDER_NAME=%%~nx*
    lua "%UD_ROOT%\tools\mkmod.lua" %2 || goto fail

    :success
    set UD_MOD_FOLDER_NAME=
    popd
    exit /B 0

    :fail
    set UD_MOD_FOLDER_NAME=
    popd
    exit /B 1

exit /B 0
