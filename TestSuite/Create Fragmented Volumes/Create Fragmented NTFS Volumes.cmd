@echo off

::
:: this script is used to create fragmented test volumes
::
:: it uses the fragmentation utility included in MyDefrag
:: available at http://www.mydefrag.com/
::

:: the following must be set to the character representing "YES"
:: this is needed to run the format utility without user interaction
echo.
set /p YES="Enter the letter that represents YES in your language [Y]: "
if "%YES%" == "" set YES=Y

:: specify volumes that should be used as test volumes
:: any file located in the root folder will be deleted
set ProcessVolumes=O: P: Q:

:: specify volume that should be compressed
set CompressedVolume=P:

:: specify volume that should include compressed and regular files
set MixedVolume=Q:

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
echo 1 ... 10%% of free Space
echo 2 ... 20%% of free Space
echo 3 ... 30%% of free Space
echo 4 ... 40%% of free Space
echo 5 ... 50%% of free Space

set maxAnswers=5

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

rem NTFS volumes
for %%L in ( %ProcessVolumes% ) do call :FragmentDrive "%%~L"

title Operation Completed ...
echo.
pause

goto :EOF

:FragmentDrive
    call :delay 0
    
    if not exist "%~1\" goto :EOF
    
    set ex_type=
    set c_switch=
    if /i "%~1" == "%CompressedVolume%" set ex_type=compressed
    if /i "%~1" == "%CompressedVolume%" set c_switch=/C
    if /i "%~1" == "%MixedVolume%"      set ex_type=mixed

    call :answers >"%TMP%\answers.txt"
    
    title Setting Volume Label of "%~1" ...
    echo Executing ... label %~1 TestNTFS%ex_type%
    echo.
    label %~1 TestNTFS%ex_type%
    
    title Formatting Drive "%~1" ...
    echo Executing ... format %~1 /FS:NTFS /V:TestNTFS%ex_type% %c_switch% /X
    echo. & format %~1 /FS:NTFS /V:TestNTFS%ex_type% %c_switch% /X <"%TMP%\answers.txt"
    
    call :delay 5
    
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f /v
    echo. & echo %YES% | chkdsk %~1 /r /f /v
    
    call :delay 2
    
    set /a size="24 + %RANDOM%"
    set /a fragments="%RANDOM% / 1365"
    set count=0
    set NoCompr=0
    set dest=%~1
    
    title Creating Fragmented Files on Drive "%~1" ...
    echo Creating Fragmented Files on Drive "%~1" ...
    echo.

    :loop
        call :doit "%~1"
        call :increment
        set ExitCode=%ERRORLEVEL%
        ping -n 2 localhost >NUL
    for /f "tokens=1,5" %%X in ( 'udefrag -l' ) do if "%%~X" == "%~1" if %PercentageFree% LEQ %%Y goto :loop

    if %ExitCode% GEQ 1 (
        echo.
        echo Operation failed ...
    ) else (
        echo.
        echo Operation succeeded ...
    )
    
    call :delay 5
    
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f /v
    echo. & echo %YES% | chkdsk %~1 /r /f /v
    
    call :delay 2
goto :EOF

:answers
    echo TestNTFS%ex_type%
    echo %YES%
goto :EOF

:doit
    if %count% EQU 0 goto :skip
        if %NoFolder% EQU 0 set dest=%~1\folder_%count%
        if %NoFolder% EQU 0 echo Folder                     %dest%
        if %NoFolder% EQU 0 mkdir "%dest%"
    :skip

    set count_fmt=   %count%
    set size_fmt=      %size%
    set frag_fmt=   %fragments%

    echo File %count_fmt:~-3% ... %size_fmt:~-6% ... %frag_fmt:~-3%

    "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%dest%\file_%count%.bin" >NUL
    if /i "%~1" == "%MixedVolume%" if %NoCompr% EQU 0 compact /c "%dest%\file_%count%.bin" >NUL
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
    set /a count+=1
    
    set /a NoFolder="count %% 10"
    set /a NoCompr="count %% 5"

    set /a size="24 + %RANDOM%"
    set /a fragments="%RANDOM% / 1365"
goto :EOF
