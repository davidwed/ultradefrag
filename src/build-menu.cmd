@echo off

rem this script is a simple front-end for the build commands
rem it includes the most common build situations

set UD_BLD_MENU_DIR=%~dp0

:DisplayMenu
cls
echo.
echo UltraDefrag Project - Build Menu
echo ================================
echo.
echo   Brought to You by the UltraDefrag Development Team
echo.
echo      1 ... Clean Project Folder
echo      2 ... Build Release Candidate
echo      3 ... Build Release
echo      4 ... Build with Defaults
echo      5 ... Build with Defaults and Install
echo.
echo      6 ... Build .................. using WinDDK, no IA64
echo      7 ... Build Portable ......... using WinDDK, no IA64
echo      8 ... Build Micro ............ using WinDDK, no IA64
echo      9 ... Build Micro Portable ... using WinDDK, no IA64
echo     10 ... Build Docs
echo.
echo     11 ... Build .................. with Custom Switches
echo     12 ... Build Portable ......... with Custom Switches
echo     13 ... Build Micro ............ with Custom Switches
echo     14 ... Build Micro Portable ... with Custom Switches
echo.
echo     15 ... Build Test Release for Stefan
echo.
echo      0 ... EXIT

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=15

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
call cleanup.cmd --clean
goto finished

:2
call build-pre-release.cmd
goto finished

:3
call build-release.cmd
goto finished

:4
call build.cmd
goto finished

:5
call build.cmd --install
goto finished

:6
call build.cmd --use-winddk --no-ia64
goto finished

:7
call build-portable.cmd --use-winddk --no-ia64
goto finished

:8
call build-micro.cmd --use-winddk --no-ia64
goto finished

:9
call build-micro-portable.cmd --use-winddk --no-ia64
goto finished

:10
call build-docs.cmd
goto finished

:11
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "
echo.
call build.cmd %UD_BLD_MENU_SWITCH%
goto finished

:12
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "
echo.
call build-portable.cmd %UD_BLD_MENU_SWITCH%
goto finished

:13
cls
echo.
call build-micro.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "
echo.
call build-micro.cmd %UD_BLD_MENU_SWITCH%
goto finished

:14
cls
echo.
call build-micro.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "
echo.
call build-micro-portable.cmd %UD_BLD_MENU_SWITCH%
goto finished

:15
call build-pre-release.cmd --no-ia64 --install

if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v %UD_BLD_MENU_DIR%\pre-release\*.* "%USERPROFILE%\Downloads\UltraDefrag"
goto finished

:finished
echo.
pause
