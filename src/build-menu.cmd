@echo off

::
:: This script is a simple front-end for the build commands;
:: it includes the most common build situations.
:: Copyright (c) 2010-2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

set UD_BLD_MENU_DIR=%~dp0

:DisplayMenu
title UltraDefrag Project - Build Menu
cls
echo.
echo UltraDefrag Project - Build Menu
echo ================================
echo.
echo   Brought to You by the UltraDefrag Development Team
echo.
echo      1 ... Clean Project Folder
echo      2 ... Build with Defaults
echo      3 ... Build with Defaults and Install
echo.
echo      4 ... Build Release
echo.
echo      5 ... Build .................. using WinDDK, no IA64
echo      6 ... Build Portable ......... using WinDDK, no IA64
echo      7 ... Build Docs
echo.
echo      8 ... Build .................. with Custom Switches
echo      9 ... Build Portable ......... with Custom Switches
echo.
echo     10 ... Build Test Release for Stefan
echo     11 ... Build Test Installation for Stefan
echo     12 ... Build Test AMD64 for Stefan
echo     13 ... Build Test x86 for Stefan
echo.
echo      0 ... EXIT

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=13

:AskSelection
echo.
set /p UD_BLD_MENU_SELECT="Please select an Option: "
echo.

if %UD_BLD_MENU_SELECT% LEQ %UD_BLD_MENU_MAX_ENTRIES% goto %UD_BLD_MENU_SELECT%

color 8C
echo Wrong Selection !!!
color
goto AskSelection

:0
goto :EOF

:1
title Clean Project Folder
call build.cmd --clean
goto finished

:2
title Build with Defaults
call build.cmd
goto finished

:3
title Build with Defaults and Install
call build.cmd --install
goto finished

:4
title Build Release
call build-release.cmd
goto finished

:5
title Build .................. using WinDDK, no IA64
call build.cmd --use-winddk --no-ia64
goto finished

:6
title Build Portable ......... using WinDDK, no IA64
call build.cmd --portable --use-winddk --no-ia64
goto finished

:7
title Build Docs
call build-docs.cmd
goto finished

:8
title Build .................. with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build .................. %UD_BLD_MENU_SWITCH%
echo.
call build.cmd %UD_BLD_MENU_SWITCH%
goto finished

:9
title Build Portable ......... with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build Portable ......... %UD_BLD_MENU_SWITCH%
echo.
call build.cmd --portable %UD_BLD_MENU_SWITCH%
goto finished

:10
title Build Test Release for Stefan
echo.
call build.cmd --use-winddk --no-ia64
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*"        "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ia64\ultradefrag-*.bin.ia64.*"   "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\amd64\ultradefrag-*.bin.amd64.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip10
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip10
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:11
title Build Test Installation for Stefan
echo.
if %PROCESSOR_ARCHITECTURE% == AMD64 call build.cmd --use-winddk --no-ia64 --no-x86 --install
if %PROCESSOR_ARCHITECTURE% == x86 call build.cmd --use-winddk --no-ia64 --no-amd64 --install
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:12
title Build Test AMD64 for Stefan
echo.
call build.cmd --use-winddk --no-ia64 --no-x86
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*"        "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ia64\ultradefrag-*.bin.ia64.*"   "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\amd64\ultradefrag-*.bin.amd64.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip12
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip12
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:13
title Build Test x86 for Stefan
echo.
call build.cmd --use-winddk --no-ia64 --no-amd64
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*"        "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ia64\ultradefrag-*.bin.ia64.*"   "%USERPROFILE%\Downloads\UltraDefrag"
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\amd64\ultradefrag-*.bin.amd64.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\bin\ultradefrag-*.bin.i386.*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip13
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip13
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:finished
echo.
pause
