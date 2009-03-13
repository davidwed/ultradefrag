@echo off
rem UltraDefrag context menu handler

echo %1
echo.

set UD_IN_FILTER=%1

%windir%\system32\udefrag.exe %1
echo.

pause
