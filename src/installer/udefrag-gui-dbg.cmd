@echo off

echo.
echo You must have DbgView installed. Download it from
echo www.sysinternals.com and place executable into your
echo system32 directory as described in User Manual.
echo.

echo Start DbgView program before UltraDefrag GUI startup...
start /B dbgview /f

echo.
echo Start UltraDefrag GUI...
call udefrag-gui.cmd
exit /B
