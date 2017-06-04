@echo off

::
:: This script is a simple front-end for the build commands;
:: it includes the most common build situations.
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

set UD_BLD_MENU_DIR=%~dp0

:DisplayMenu
title UltraDefrag Project - Build Menu
cls
echo.
echo UltraDefrag Project - Build Menu
echo ================================
echo.
echo   Brought to you by UltraDefrag Development Team
echo.
echo      1 ... Clean Project Folder
echo      2 ... Build with Defaults
echo      3 ... Build with Defaults and Install
echo.
echo      4 ... Build Release
echo.
echo      5 ... Build .................. using WinSDK, no IA64
echo      6 ... Build Portable ......... using WinSDK, no IA64
echo.
echo      7 ... Build Docs
echo.
echo      8 ... Build .................. with Custom Switches
echo      9 ... Build Portable ......... with Custom Switches
echo.
echo     10 ... Clean all wxWidgets libraries
echo     11 ... Build wxWidgets ........ using MinGW
echo     12 ... Build wxWidgets ........ using WinSDK
echo.
echo     13 ... Update translations
echo     14 ... Upload translations
echo.
echo     15 ... Build Test Release for Stefan
echo     16 ... Build Test Installation for Stefan
echo     17 ... Build Test AMD64 for Stefan
echo     18 ... Build Test x86 for Stefan
echo.
echo      0 ... Exit

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=18

:AskSelection
echo.
set /p UD_BLD_MENU_SELECT="Please select an option: "
echo.

if %UD_BLD_MENU_SELECT% LEQ %UD_BLD_MENU_MAX_ENTRIES% goto %UD_BLD_MENU_SELECT%

color 8C
echo Wrong selection!
color
set UD_BLD_MENU_SELECT=
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
title Build .................. using WinSDK, no IA64
call build.cmd --use-winsdk --no-ia64
goto finished

:6
title Build Portable ......... using WinSDK, no IA64
call build.cmd --portable --use-winsdk --no-ia64
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
set /p UD_BLD_MENU_SWITCH="Please enter switches: "

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
set /p UD_BLD_MENU_SWITCH="Please enter switches: "

title Build Portable ......... %UD_BLD_MENU_SWITCH%
echo.
call build.cmd --portable %UD_BLD_MENU_SWITCH%
goto finished

:10
title Clean all wxWidgets libraries
call wxbuild.cmd --use-mingw --clean
call wxbuild.cmd --use-winsdk --clean
goto finished

:11
title Build wxWidgets ........ using MinGW
call wxbuild.cmd --use-mingw
goto finished

:12
title Build wxWidgets ........ using WinSDK
call wxbuild.cmd --use-winsdk
goto finished

:13
title Update translations
call update-translations.cmd
goto finished

:14
title Upload translations
call upload-translations.cmd
goto finished

:15
title Build Test Release for Stefan
echo.
call build.cmd --use-winsdk --no-ia64
echo.
call :CopyInstallers -zip
goto finished

:16
title Build Test Installation for Stefan
echo.
if %PROCESSOR_ARCHITECTURE% == AMD64 call build.cmd --use-winsdk --no-ia64 --no-x86 --install
if %PROCESSOR_ARCHITECTURE% == x86 call build.cmd --use-winsdk --no-ia64 --no-amd64 --install
echo.
goto finished

:17
title Build Test AMD64 for Stefan
echo.
call build.cmd --use-winsdk --no-ia64 --no-x86
echo.
call :CopyInstallers
goto finished

:18
title Build Test x86 for Stefan
echo.
call build.cmd --use-winsdk --no-ia64 --no-amd64
echo.
call :CopyInstallers
goto finished

:finished
echo.
pause

goto :EOF

:CopyInstallers
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
cd /d "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ultradefrag-%UDVERSION_SUFFIX%.bin.i386.exe"        .
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\amd64\ultradefrag-%UDVERSION_SUFFIX%.bin.amd64.exe" .
echo.
if "%~1" == "" goto :noZip
del /f /q "ultradefrag-%UDVERSION_SUFFIX%.7z"
"%SEVENZIP_PATH%\7z.exe" a -t7z -mx9 -pud -mhe=on "ultradefrag-%UDVERSION_SUFFIX%.7z" "ultradefrag-%UDVERSION_SUFFIX%.bin.*.exe"
:noZip
echo.
goto :EOF