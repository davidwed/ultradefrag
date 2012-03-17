@rem 
@echo off

::
:: This script is used to create a log file for a bug report.
:: Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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

:: check for administrative rights
for /f "tokens=2 delims=[" %%V in ('ver') do set test=%%V
for /f "tokens=2" %%V in ('echo %test%') do set test1=%%V
for /f "tokens=1,2 delims=." %%V in ('echo %test1%') do set OSversion=%%V.%%W

if %OSversion% LSS 6.0 goto :IsAdmin

if /i %USERNAME% == Administrator goto :IsAdmin
if not defined SESSIONNAME goto :IsAdmin

echo.
echo This script must be run by the Administrator !!!
goto :quit

:IsAdmin
title UltraDefrag - Create Bug Report Log File

:SelectVolume
:: collect volumes that can be processed
cls
echo.
echo Collecting available volumes ...
echo.
set MenuItem=0
set ItemList=

for /f "tokens=1,6* skip=8" %%D in ('udefrag -l') do call :AddToItemList %%~D & call :DisplayMenuItem %%~D - "%%~F"

echo.
echo 99 ... All except system drive
echo.
echo 0 ... EXIT
echo.
set /p SelectedVolume="Select volume to process: "

if "%SelectedVolume%" == "" goto :EOF
if %SelectedVolume% EQU 0 goto :EOF
if %SelectedVolume% EQU 99 goto :GetDriveLetter

if %SelectedVolume% LEQ %MenuItem% goto :GetDriveLetter
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectVolume

:GetDriveLetter
if %SelectedVolume% EQU 99 (
    set ProcessVolume=%ItemList%
) else (
    for /f "tokens=%SelectedVolume%" %%V in ("%ItemList%") do set ProcessVolume=%%~V
)

:SelectAction
:: select action to take
cls
echo.
set MenuItem=0
set ItemList=-a -d -q -o --optimize-mft

for %%D in ("Analyze" "Defrag" "Quick Optimize" "Full Optimize" "Optimize MFT") do call :DisplayMenuItem %%~D

echo.
echo 0 ... EXIT
echo.
set /p SelectedAction="Select action to take: "

if "%SelectedAction%" == "" goto :EOF
if %SelectedAction% EQU 0 goto :EOF

if %SelectedAction% LEQ %MenuItem% goto :GetAction
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectAction

:GetAction
for /f "tokens=%SelectedAction%" %%V in ("%ItemList%") do set ProcessAction=%%~V
if "%ProcessAction%" == "-d" set ProcessAction=

set ItemList=Analyze Defrag QuickOptimize FullOptimize OptimizeMFT
for /f "tokens=%SelectedAction%" %%V in ("%ItemList%") do set ActionName=%%~V

:AskRepeatAction
:: ask if we like to repeat the action
echo.
set /p RepeatAction="Repeat action for volume %ProcessVolume%? (Y/[N]) "
if /i not "%RepeatAction%" == "Y" (
    set RepeatAction=
) else (
    set RepeatAction=-r
)

:AskDryRun
:: ask if we like to use dryrun
echo.
set /p DryRun="Use dryrun? (Y/[N]) "
if /i "%DryRun%" == "Y" set UD_DRY_RUN=1

:SelectLogLevel
:: select log level to use
cls
echo.
set MenuItem=0
set ItemList=NORMAL DETAILED PARANOID

for %%D in ("Normal" "Detailed" "Paranoid") do call :DisplayMenuItem %%~D

echo.
echo 0 ... EXIT
echo.
set /p SelectedLogLevel="Select log level to use: "

if "%SelectedLogLevel%" == "" goto :EOF
if %SelectedLogLevel% EQU 0 goto :EOF

if %SelectedLogLevel% LEQ %MenuItem% goto :GetLogLevel
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectLogLevel

:GetLogLevel
for /f "tokens=%SelectedLogLevel%" %%V in ("%ItemList%") do set ProcessLogLevel=%%~V

cls
echo.
set UD_DBGPRINT_LEVEL=%ProcessLogLevel%
echo Debug level set to "%UD_DBGPRINT_LEVEL%"
if "%UD_DRY_RUN%" == "1" echo Dryrun enabled ...

if %SelectedVolume% EQU 99 (
    for %%V in ( %ProcessVolume% ) do if %%V neq %SystemDrive% call :ProcessVolumes %%V
) else (
    call :ProcessVolumes %ProcessVolume%
)

title Operation Completed ...

:quit
echo.
pause

goto :EOF

:ProcessVolumes
    set CurrentVolume=%~1
    if "%UD_DRY_RUN%" == "1" (
        set UD_LOG_FILE_PATH=%SystemRoot%\UltraDefrag\Logs\udefrag_%ActionName%_%CurrentVolume:~0,1%_dryrun.log
    ) else (
        set UD_LOG_FILE_PATH=%SystemRoot%\UltraDefrag\Logs\udefrag_%ActionName%_%CurrentVolume:~0,1%.log
    )
    echo.
    echo Using log file "%UD_LOG_FILE_PATH%"
    echo.
    echo.
    echo Executing "udefrag %RepeatAction% %ProcessAction% %CurrentVolume%"... 
    echo.
    echo.
    udefrag %RepeatAction% %ProcessAction% %CurrentVolume%
    echo.
    ping -n 11 localhost >nul 2>&1
    if "%UD_DRY_RUN%" == "1" (
        for %%F in ( "%CurrentVolume%\fraglist.*" ) do copy /v /y "%%~F" "%SystemRoot%\UltraDefrag\Logs\%%~nF_%CurrentVolume:~0,1%_dryrun%%~xF"
    ) else (
        for %%F in ( "%CurrentVolume%\fraglist.*" ) do copy /v /y "%%~F" "%SystemRoot%\UltraDefrag\Logs\%%~nF_%CurrentVolume:~0,1%%%~xF"
    )
goto :EOF

:DisplayMenuItem
    set /a MenuItem+=1
    echo %MenuItem% ... %*
goto :EOF

:AddToItemList
    if "%ItemList%" == "" (
        set ItemList=%~1
    ) else (
        set ItemList=%ItemList% %~1
    )
goto :EOF
