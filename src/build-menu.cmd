@echo off

rem this script is a simple front-end for the build commands
rem it includes the most common build situations

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
echo      4 ... Build alpha Release
echo      5 ... Build beta Release
echo      6 ... Build beta Release Installer
echo      7 ... Build Release Candidate
echo      8 ... Build Release
echo.
echo      9 ... Build .................. using WinDDK, no IA64
echo     10 ... Build Portable ......... using WinDDK, no IA64
echo     11 ... Build Docs
echo.
echo     12 ... Build .................. with Custom Switches
echo     13 ... Build Portable ......... with Custom Switches
echo.
echo     14 ... Build Test Release for Stefan
echo     15 ... Build Test Installation for Stefan
echo     16 ... Build Test AMD64 for Stefan
echo     17 ... Build Test x86 for Stefan
echo.
echo      0 ... EXIT

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=17

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
title Build alpha Release
call build-pre-release.cmd --alpha
goto finished

:5
title Build beta Release
call build-pre-release.cmd --beta
goto finished

:6
title Build beta Release Installer
call build-pre-release.cmd --betai
goto finished

:7
title Build Release Candidate
call build-pre-release.cmd --rc
goto finished

:8
title Build Release
call build-release.cmd
goto finished

:9
title Build .................. using WinDDK, no IA64
call build.cmd --use-winddk --no-ia64
goto finished

:10
title Build Portable ......... using WinDDK, no IA64
call build.cmd --portable --use-winddk --no-ia64
goto finished

:11
title Build Docs
call build-docs.cmd
goto finished

:12
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

:13
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

:14
title Build Test Release for Stefan
echo.
set /P UD_BLD_PRE_RELEASE_TYPE="Enter Release Type (alpha,beta,[betai],rc): "
if "%UD_BLD_PRE_RELEASE_TYPE%" == "" set UD_BLD_PRE_RELEASE_TYPE=betai

call build-pre-release.cmd --no-ia64 --%UD_BLD_PRE_RELEASE_TYPE%
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\pre-release\*.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\pre-release\*i386*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip14
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip14
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:15
title Build Test Installation for Stefan
echo.
set /P UD_BLD_PRE_RELEASE_TYPE="Enter Release Type (alpha,beta,[betai],rc): "
if "%UD_BLD_PRE_RELEASE_TYPE%" == "" set UD_BLD_PRE_RELEASE_TYPE=betai

call build-pre-release.cmd --no-ia64 --no-x86 --install --%UD_BLD_PRE_RELEASE_TYPE%
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:16
title Build Test AMD64 for Stefan
echo.
set /P UD_BLD_PRE_RELEASE_TYPE="Enter Release Type (alpha,beta,[betai],rc): "
if "%UD_BLD_PRE_RELEASE_TYPE%" == "" set UD_BLD_PRE_RELEASE_TYPE=betai

call build-pre-release.cmd --no-ia64 --no-x86 --%UD_BLD_PRE_RELEASE_TYPE%
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\pre-release\*.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\pre-release\*i386*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip16
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip16
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:17
title Build Test x86 for Stefan
echo.
set /P UD_BLD_PRE_RELEASE_TYPE="Enter Release Type (alpha,beta,[betai],rc): "
if "%UD_BLD_PRE_RELEASE_TYPE%" == "" set UD_BLD_PRE_RELEASE_TYPE=betai

call build-pre-release.cmd --no-ia64 --no-amd64 --%UD_BLD_PRE_RELEASE_TYPE%
echo.
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\pre-release\*.*" "%USERPROFILE%\Downloads\UltraDefrag"
set old_file=X
for %%F in ( "%UD_BLD_MENU_DIR%\pre-release\*i386*" ) do set old_file=%%~nxF
if "%old_file%" == "X" goto :skip17
set new_file=%old_file:i386=x86%
move /y "%USERPROFILE%\Downloads\UltraDefrag\%old_file%" "%USERPROFILE%\Downloads\UltraDefrag\%new_file%"
:skip17
echo.
cd /d %UD_BLD_MENU_DIR%
call build.cmd --clean
goto finished

:finished
echo.
pause
