@echo off

::
:: This script builds all binaries for UltraDefrag project
:: for all of the supported target platforms.
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
rem     build-targets [<compiler>]
rem
rem Available <compiler> values:
rem     --use-mingw     (default)
rem     --use-winsdk    (we use it for official releases)
rem     --use-mingw-x64 (experimental, produces wrong x64 code)
rem
rem Skip any processor architectures to reduce compilation time
rem     --no-x86
rem     --no-amd64
rem     --no-ia64

rem NOTE: IA-64 binaries were never tested by the authors
rem because of lack of Itanium hardware.

call ParseCommandLine.cmd %*

:: create all directories required to store target binaries
mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

:: copy sources to obj subdirectory
(
    echo doxyfile
    echo .dox
    echo .html
    echo .mdsp
    echo .cbp
    echo .depend
    echo .layout
    echo .gch
    echo .pch
) >"%~n0_exclude.txt"

set XCOPY_OPTS=/I /Y /Q /EXCLUDE:%~n0_exclude.txt

xcopy .\bootexctrl  .\obj\bootexctrl  %XCOPY_OPTS%
xcopy .\console     .\obj\console     %XCOPY_OPTS% /S
xcopy .\dbg         .\obj\dbg         %XCOPY_OPTS%
xcopy .\dll\udefrag .\obj\udefrag     %XCOPY_OPTS%
xcopy .\dll\zenwinx .\obj\zenwinx     %XCOPY_OPTS%
xcopy .\hibernate   .\obj\hibernate   %XCOPY_OPTS%
xcopy .\include     .\obj\include     %XCOPY_OPTS%
xcopy .\lua5.1      .\obj\lua5.1      %XCOPY_OPTS%
xcopy .\lua         .\obj\lua         %XCOPY_OPTS%
xcopy .\lua-gui     .\obj\lua-gui     %XCOPY_OPTS%
xcopy .\native      .\obj\native      %XCOPY_OPTS%
xcopy .\wxgui       .\obj\wxgui       %XCOPY_OPTS%
xcopy .\wxgui\res   .\obj\wxgui\res   %XCOPY_OPTS% /S

set XCOPY_OPTS=

del /f /q "%~n0_exclude.txt"

:: copy header files to different locations
:: to make relative paths of them the same
:: as in /src directory
mkdir obj\dll
mkdir obj\dll\udefrag
mkdir obj\dll\zenwinx
copy /Y obj\udefrag\udefrag.h obj\dll\udefrag\
copy /Y obj\zenwinx\*.h obj\dll\zenwinx\

:: build list of headers to produce
:: dependencies for makefiles from
cd obj
dir /S /B *.h >headers || exit /B 1
copy /Y .\headers .\bootexctrl || exit /B 1
copy /Y .\headers .\console    || exit /B 1
copy /Y .\headers .\udefrag    || exit /B 1
copy /Y .\headers .\zenwinx    || exit /B 1
copy /Y .\headers .\wxgui      || exit /B 1
copy /Y .\headers .\hibernate  || exit /B 1
copy /Y .\headers .\lua5.1     || exit /B 1
copy /Y .\headers .\lua        || exit /B 1
copy /Y .\headers .\lua-gui    || exit /B 1
copy /Y .\headers .\native     || exit /B 1
copy /Y .\headers .\dbg        || exit /B 1
cd ..

:: let's build all modules by the selected compiler
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
        set UDEFRAG_LIB_PATH=..\..\lib
        call :build_modules X86 || exit /B 1
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
        echo --------- Target is x64 ---------
        set IA64=
        set AMD64=1
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
        popd
        set UDEFRAG_LIB_PATH=..\..\lib\amd64
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
        set UDEFRAG_LIB_PATH=..\..\lib\ia64
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
    set path=%MINGWx64BASE%\bin;%path%
    set BUILD_ENV=mingw_x64
    set UDEFRAG_LIB_PATH=..\..\lib\amd64
    call :build_modules amd64 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:mingw_build

    set OLD_PATH=%path%

    set path=%MINGWBASE%\bin;%path%
    set BUILD_ENV=mingw
    set UDEFRAG_LIB_PATH=..\..\lib
    call :build_modules X86 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:: Builds all UltraDefrag modules
:: Example: call :build_modules X86
:build_modules
    rem update manifests
    call make-manifests.cmd %1 || exit /B 1

    if %WindowsSDKVersionOverride%x neq v7.1x goto NoWin7SDK
    if x%CommandPromptType% neq xCross goto NoWin7SDK
    set path=%PATH%;%VS100COMNTOOLS%\..\..\VC\Bin

    :NoWin7SDK
    rem rebuild modules
    set UD_BUILD_TOOL=lua ..\..\tools\mkmod.lua
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
    
    pushd obj\zenwinx
    %UD_BUILD_TOOL% zenwinx.build || goto fail
    cd ..\udefrag
    %UD_BUILD_TOOL% udefrag.build || goto fail
    
    echo Compile monolithic native interface...
    cd ..\native
    :: copy external files on which monolithic native interface depends
    move /Y udefrag.c udefrag-native.c
    copy /Y ..\udefrag\*.* .
    move /Y volume.c udefrag-volume.c
    copy /Y ..\zenwinx\*.* .
    del /Q udefrag.rc zenwinx.rc
    %UD_BUILD_TOOL% defrag_native.build || goto fail
    
    cd ..\lua5.1
    %UD_BUILD_TOOL% lua.build || goto fail
    cd ..\lua
    %UD_BUILD_TOOL% lua.build || goto fail
    cd ..\lua-gui
    %UD_BUILD_TOOL% lua-gui.build || goto fail
    cd ..\bootexctrl
    %UD_BUILD_TOOL% bootexctrl.build || goto fail
    cd ..\hibernate
    %UD_BUILD_TOOL% hibernate.build || goto fail
    cd ..\console
    %UD_BUILD_TOOL% console.build || goto fail
    cd ..\wxgui
    %UD_BUILD_TOOL% wxgui.build || goto fail
    cd ..\dbg
    %UD_BUILD_TOOL% dbg.build || goto fail
    popd

    :: compress gui and command line tools for
    :: a bit faster startup on older machines
    if %1 equ X86 (
        upx -q -9 .\bin\udefrag.exe
        upx -q -9 .\bin\ultradefrag.exe
    )
    
    :success
    set UD_BUILD_TOOL=
    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    exit /B 0

    :fail
    set UD_BUILD_TOOL=
    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    popd
    exit /B 1
    
exit /B 0
