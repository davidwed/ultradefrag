@echo off
::
:: Build script for the UltraDefrag project.
:: Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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

call ParseCommandLine.cmd %*

:: handle help request
if %UD_BLD_FLG_DIPLAY_HELP% equ 1 (
    call :usage
    exit /B 0
)

if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 (
    echo Delete all intermediate files...
    call :cleanup > nul 2>&1
    echo Cleanup success!
    exit /B 0
)

title Build started

:: set environment
call :set_build_environment

:: build all binaries
call build-targets.cmd %* || goto fail

:: build documentation
call build-docs.cmd %* || goto fail

:: update translations
if "%UD_BLD_FLG_UPDATE_TRANSLATIONS%" == "1" (
    call update-translations.cmd || goto fail
)

:: build installers and portable packages
echo Build installers and/or portable packages...
if %UD_BLD_FLG_BUILD_X86% neq 0 (
    call :build_installer              .\bin i386 || goto fail
    call :build_portable_package       .\bin i386 || goto fail
)
if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
    call :build_installer              .\bin\amd64 amd64 || goto fail
    call :build_portable_package       .\bin\amd64 amd64 || goto fail
)
if %UD_BLD_FLG_BUILD_IA64% neq 0 (
    call :build_installer              .\bin\ia64 ia64 || goto fail
    call :build_portable_package       .\bin\ia64 ia64 || goto fail
)
echo.
echo Build success!
title Build success!

:: install the program if requested
if %UD_BLD_FLG_DO_INSTALL% neq 0 (
    call :install || exit /B 1
)

:: all operations completed successfully!
exit /B 0

:fail
echo Build failed with code %ERRORLEVEL%!
title Build failed with code %ERRORLEVEL%!
exit /B 1

:: --------------------------------------------------------
::                      subroutines
:: --------------------------------------------------------

rem Synopsis: call :build_version_header
rem Example: call :build_version_header > version.h
:build_version_header
    echo #define VERSION %VERSION%
    echo #define VERSION2 %VERSION2%
    if "%RELEASE_STAGE%" neq "" (
        echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER% %RELEASE_STAGE%"
        echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% %RELEASE_STAGE% Portable"
        echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER% %RELEASE_STAGE%"
        echo #define wxUD_ABOUT_VERSION "%ULTRADFGVER% %RELEASE_STAGE%"
    ) else (
        echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%"
        echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable"
        echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%"
        echo #define wxUD_ABOUT_VERSION "%ULTRADFGVER%"
    )
    echo.

    echo #ifndef _WIN64
    echo   #define UDEFRAG_ARCH "x86"
    echo #else
    echo  #if defined(_IA64_)
    echo   #define UDEFRAG_ARCH "ia64"
    echo  #else
    echo   #define UDEFRAG_ARCH "x64"
    echo  #endif
    echo #endif
    echo.

    rem remove the preview menu for stable releases
    if /I "%RELEASE_STAGE:~0,2%" equ "RC" echo #define _UD_HIDE_PREVIEW_
    if "%RELEASE_STAGE%" equ "" echo #define _UD_HIDE_PREVIEW_
goto :EOF

rem Sets environment for the build process.
:set_build_environment
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
    echo.

    if %UD_BLD_FLG_IS_PORTABLE% equ 1 (
        set UDEFRAG_PORTABLE=1
    ) else (
        set UDEFRAG_PORTABLE=
    )

    if %UD_BLD_FLG_BUILD_ALL% equ 1 (
        set UDEFRAG_PORTABLE=
    )

    rem update version.h file only when something got
    rem changed to speed up incremental compilation
    call :build_version_header > .\include\version.new
    fc .\include\version.new .\include\version.h >nul
    if errorlevel 1 move /Y .\include\version.new .\include\version.h
    del /q .\include\version.new
goto :EOF

rem Synopsis: call :build_readme_file
rem Example: call :build_readme_file > README.TXT
:build_readme_file
    echo -------------------------------------------------------------------------------
    echo          UltraDefrag %ULTRADFGVER% - a powerful disk defragmenter for Windows
    echo -------------------------------------------------------------------------------
    echo.
    echo The complete information about the program can be found in UltraDefrag Handbook.
    echo You should have received it along with this program; if not, go to:
    echo.
    echo   https://ultradefrag.net/handbook/
    echo.
    echo -------------------------------------------------------------------------------
exit /B 0

rem Synopsis: call :build_installer {path to binaries} {arch}
rem Example:  call :build_installer .\bin\ia64 ia64
:build_installer
    if %UD_BLD_FLG_BUILD_ALL% neq 1 if "%UDEFRAG_PORTABLE%" neq "" exit /B 0

    pushd %1
    copy /Y "%~dp0\installer\UltraDefrag.nsi" .\

    call :build_readme_file > README.TXT

    set ULTRADFGARCH=%2
    "%NSISDIR%\makensis.exe" UltraDefrag.nsi || goto fail
    
    :success
    popd
    exit /B 0
    
    :fail
    popd
exit /B 1

rem Synopsis: call :build_portable_package {path to binaries} {arch}
rem Example:  call :build_portable_package .\bin\ia64 ia64
:build_portable_package
    if %UD_BLD_FLG_BUILD_ALL% neq 1 if "%UDEFRAG_PORTABLE%" equ "" exit /B 0
    
    pushd %1
    set PORTABLE_DIR=ultradefrag-portable-%UDVERSION_SUFFIX%.%2
    mkdir %PORTABLE_DIR%
    copy /Y "%~dp0\HISTORY.TXT" %PORTABLE_DIR%\
    copy /Y "%~dp0\LICENSE.TXT" %PORTABLE_DIR%\
    
    call :build_readme_file  >  %PORTABLE_DIR%\README.TXT
    
    copy /Y hibernate.exe       %PORTABLE_DIR%\hibernate4win.exe
    copy /Y udefrag.dll         %PORTABLE_DIR%\
    copy /Y udefrag.exe         %PORTABLE_DIR%\
    copy /Y zenwinx.dll         %PORTABLE_DIR%\
    copy /Y ultradefrag.exe     %PORTABLE_DIR%\
    copy /Y lua5.1a.dll         %PORTABLE_DIR%\
    copy /Y lua5.1a.exe         %PORTABLE_DIR%\
    copy /Y lua5.1a_gui.exe     %PORTABLE_DIR%\
    copy /Y udefrag-dbg.exe     %PORTABLE_DIR%\
    mkdir %PORTABLE_DIR%\handbook
    copy /Y "%~dp0\..\doc\handbook\doxy-doc\html\*.*" %PORTABLE_DIR%\handbook\
    mkdir %PORTABLE_DIR%\scripts
    copy /Y "%~dp0\scripts\udreportcnv.lua"      %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\udsorting.js"         %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\udreport.css"         %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\upgrade-options.lua"  %PORTABLE_DIR%\scripts\
    mkdir %PORTABLE_DIR%\conf
    lua "%~dp0\scripts\upgrade-options.lua"      %PORTABLE_DIR%
    xcopy "%~dp0\wxgui\locale" %PORTABLE_DIR%\locale /I /Y /Q /S
    del /Q %PORTABLE_DIR%\locale\*.header
    del /Q %PORTABLE_DIR%\locale\*.pot
    mkdir %PORTABLE_DIR%\po
    copy /Y "%~dp0\wxgui\locale\*.pot" %PORTABLE_DIR%\po
    copy /Y "%~dp0\tools\transifex\translations\ultradefrag.main\*.po" %PORTABLE_DIR%\po

    "%SEVENZIP_PATH%\7z.exe" a -r -mx9 -tzip ultradefrag-portable-%UDVERSION_SUFFIX%.bin.%2.zip %PORTABLE_DIR%
    if %errorlevel% neq 0 (
        rd /s /q %PORTABLE_DIR%
        set PORTABLE_DIR=
        popd
        exit /B 1
    )
    rd /s /q %PORTABLE_DIR%
    set PORTABLE_DIR=
    popd
exit /B 0

rem Installs the program.
:install
    if "%UDEFRAG_PORTABLE%" neq "" exit /B 0

    setlocal
    if /i %PROCESSOR_ARCHITECTURE% == x86 (
        set INSTALLER_PATH=.\bin
        set INSTALLER_ARCH=i386
    )
    if /i %PROCESSOR_ARCHITECTURE% == AMD64 (
        set INSTALLER_PATH=.\bin\amd64
        set INSTALLER_ARCH=amd64
    )
    if /i %PROCESSOR_ARCHITECTURE% == IA64 (
        set INSTALLER_PATH=.\bin\ia64
        set INSTALLER_ARCH=ia64
    )
    set INSTALLER_NAME=ultradefrag

    echo Start installer...
    %INSTALLER_PATH%\%INSTALLER_NAME%-%UDVERSION_SUFFIX%.bin.%INSTALLER_ARCH%.exe /S
    if %errorlevel% neq 0 (
        echo Installation failed!
        endlocal
        exit /B 1
    )
    echo Installation success!
    endlocal
exit /B 0

rem Cleans up sources directory by
rem removal of all intermediate files.
:cleanup
    rd /s /q bin
    rd /s /q lib
    rd /s /q obj
    del /s /q Makefile.*
    del /s /q *.gch
    del /s /q *.pch
    rd /s /q dll\udefrag\doxy-doc
    rd /s /q dll\zenwinx\doxy-doc
    rd /s /q ..\doc\handbook\doxy-doc
    rd /s /q doxy-defaults
    rd /s /q dll\udefrag\doxy-defaults
    rd /s /q dll\zenwinx\doxy-defaults
    rd /s /q ..\doc\handbook\doxy-defaults
    rd /s /q ..\doc\handbook\doxy-defaults_a4
    rd /s /q ..\doc\handbook\doxy-defaults_letter
    if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 rd /s /q release

    del /f /q ..\..\web\doc\lib\udefrag\*.*
    del /f /q ..\..\web\doc\lib\zenwinx\*.*

    for %%F in ( "..\..\web\handbook\*.*" ) do if not "%%~nxF" == ".htaccess" del /f /q "%%~F"
goto :EOF

rem Displays usage information.
:usage
    echo Usage: build [options]
    echo.
    echo Common options:
    echo --portable      build portable packages instead of installers
    echo --all           build all packages: regular and portable
    echo --install       perform silent installation after the build
    echo --clean         perform full cleanup instead of the build
    echo.
    echo Compiler:
    echo --use-mingw     (default)
    echo --use-winsdk    (we use it for official releases)
    echo --use-mingw-x64 (experimental, produces wrong x64 code)
    echo.
    echo Target architecture (must always be after compiler):
    echo --no-x86        don't build 32-bit binaries
    echo --no-amd64      don't build x64 binaries
    echo --no-ia64       don't build IA-64 binaries
    echo.
    echo Additional stuff:
    echo --pdf           build PDF documentation
    echo --dev           build developer documentation
    echo --trans         update translations
    echo.
    echo Without parameters the build command uses MinGW
    echo to build 32-bit installer.
    echo.
    echo * Run patch-tools.cmd once before compilation!
goto :EOF
