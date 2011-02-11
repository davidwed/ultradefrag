@echo off

::
:: this script is used to create fragmented test volumes
::
:: it uses the fragmentation utility included in MyDefrag
:: available at http://www.mydefrag.com/
::
:: the percentage of free space is based on volumes of 1GB in size
::

:: the following must be set to the character representing "YES"
:: this is needed to run the format utility without user interaction
echo.
set /p YES="Enter the letter that represents YES in your language [Y]: "
if "%YES%" == "" set YES=Y

:: specify volumes that should be used as test volumes
:: any file located in the root folder will be deleted
set ProcessVolumes=R: S: T: U: V: W:

for /d %%D in ( "%ProgramFiles%\MyDefrag*" ) do set MyDefragDir=%%~D

if "%MyDefragDir%" == "" (
    echo.
    echo MyDefrag not installed ... aborting!
    echo.
    pause
    goto :EOF
)

:DisplayMenu
title UltraDefrag - Test Volume Fragmentation Creator
cls
echo.
echo The values are based on volumes of 1GB in size
echo.
echo 1 ... 10%% of free Space
echo 2 ... 20%% of free Space
echo 3 ... 30%% of free Space

set maxAnswers=3

echo.
echo 0 ... EXIT
echo.
set /p answer="Select an Option: "

if "%answer%" == "" goto :EOF
if %answer% EQU 0 goto :EOF

if %answer% LEQ %maxAnswers% goto :StartProcess

echo.
echo Selection must be between 0 and %maxAnswers% !!!
echo.
pause
goto :DisplayMenu

:StartProcess
set /a PercentageFree="answer * 10"

set ex_type=UDF
set fs_ver=X

rem UDF volumes
for %%L in ( %ProcessVolumes% ) do call :FragmentDrive "%%~L"

title Operation Completed ...
echo.
pause

goto :EOF

:FragmentDrive
    call :delay 0
    
    set mirror=
    
    if %fs_ver% == 2.50 set fs_ver=2.50 & set fs_ver_txt=250 & set mirror=/D
    if %fs_ver% == 2.01 set fs_ver=2.50 & set fs_ver_txt=250
    if %fs_ver% == 2.00 set fs_ver=2.01 & set fs_ver_txt=201
    if %fs_ver% == 1.50 set fs_ver=2.00 & set fs_ver_txt=200
    if %fs_ver% == 1.02 set fs_ver=1.50 & set fs_ver_txt=150
    if %fs_ver% == X    set fs_ver=1.02 & set fs_ver_txt=102

    call :answers >"%TMP%\answers.txt"
    
    title Setting Volume Label of "%~1" ...
    echo Executing ... label %~1 Test%ex_type%v%fs_ver_txt%
    echo.
    label %~1 Test%ex_type%v%fs_ver_txt%
    
    title Formatting Drive "%~1" ...
    echo Executing ... format %~1 /FS:%ex_type% /V:Test%ex_type%v%fs_ver_txt% /X /R:%fs_ver% %mirror%
    echo. & format %~1 /FS:%ex_type% /V:Test%ex_type%v%fs_ver_txt% /X /R:%fs_ver% %mirror% <"%TMP%\answers.txt"
    
    call :delay 5
    
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f
    echo. & echo n | chkdsk %~1 /r /f
    
    call :delay 2

    set size=24
    set fragments=0
    set count=0
    set total=0
    set dest=%~1
    
    title Creating Fragmented Files on Drive "%~1" ...
    echo Creating Fragmented Files on Drive "%~1" ...
    echo.

:loop
    call :increment
    call :doit "%~1"
    set ExitCode=%ERRORLEVEL%
    for /f "tokens=1,5" %%X in ( 'udefrag -l' ) do if "%%~X" == "%~1" if %PercentageFree% LSS %%Y goto :loop

    if %ExitCode% GEQ 1 (
        echo.
        echo Operation failed ...
    ) else (
        echo.
        echo Operation succeeded ...
    )
    
    call :delay 5
    
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f
    echo. & echo n | chkdsk %~1 /r /f
    
    call :delay 2
goto :EOF

:answers
    echo Test%ex_type%v%fs_ver_txt%
    echo %YES%
goto :EOF

:doit
    set /a total+=size

    if %count% EQU 1 goto :skip
        if %rest4% EQU 0 set dest=%~1\folder_%size%
        if %rest4% EQU 0 echo mkdir "%dest%"
        if %rest4% EQU 0 mkdir "%dest%"
    :skip

    "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%dest%\file_%count%.bin" >NUL
goto :EOF

:delay
    set /a seconds="%1 + 1"
    echo.
    if %seconds% == 1 (
        echo ============================================
    ) else (
        echo --------------------------------------------
        ping -n %seconds% localhost >NUL
    )
    echo.
goto :EOF

:increment
    set /a rest1="count %% 9"
    set /a rest2="count %% 8"
    set /a rest3="count %% 5"
    set /a rest4="count %% 10"

    if %rest1% EQU 0 set /a size="size * 2"
    if %rest2% EQU 0 set /a fragments+=2

    set /a count+=1
goto :EOF
