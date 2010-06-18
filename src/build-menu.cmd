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
echo     16 ... Build Test Installation for Stefan
echo.
echo      0 ... EXIT

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=16

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
call cleanup.cmd --clean
goto finished

:2
title Build Release Candidate
call build-pre-release.cmd
goto finished

:3
title Build Release
call build-release.cmd
goto finished

:4
title Build with Defaults
call build.cmd
goto finished

:5
title Build with Defaults and Install
call build.cmd --install
goto finished

:6
title Build .................. using WinDDK, no IA64
call build.cmd --use-winddk --no-ia64
goto finished

:7
title Build Portable ......... using WinDDK, no IA64
call build-portable.cmd --use-winddk --no-ia64
goto finished

:8
title Build Micro ............ using WinDDK, no IA64
call build-micro.cmd --use-winddk --no-ia64
goto finished

:9
title Build Micro Portable ... using WinDDK, no IA64
call build-micro-portable.cmd --use-winddk --no-ia64
goto finished

:10
title Build Docs
call build-docs.cmd
goto finished

:11
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

:12
title Build Portable ......... with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build Portable ......... %UD_BLD_MENU_SWITCH%
echo.
call build-portable.cmd %UD_BLD_MENU_SWITCH%
goto finished

:13
title Build Micro ............ with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build Micro ............ %UD_BLD_MENU_SWITCH%
echo.
call build-micro.cmd %UD_BLD_MENU_SWITCH%
goto finished

:14
title Build Micro Portable ... with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build Micro Portable ... %UD_BLD_MENU_SWITCH%
echo.
call build-micro-portable.cmd %UD_BLD_MENU_SWITCH%
goto finished

:15
title Build Test Release for Stefan
call build-pre-release.cmd --no-ia64 --install

if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v %UD_BLD_MENU_DIR%\pre-release\*.* "%USERPROFILE%\Downloads\UltraDefrag"
goto finished

:16
title Build Test Installation for Stefan
call build-pre-release.cmd --no-ia64 --no-x86 --install

goto finished

:finished
echo.
pause
