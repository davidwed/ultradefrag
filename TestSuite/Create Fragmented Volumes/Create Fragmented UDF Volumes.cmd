@echo off

::
:: this script is used to create fragmented test volumes
::
:: it uses the fragmentation utility included in MyDefrag
:: available at http://www.mydefrag.com/
::
:: get install language from registry
set YES=
set InstallLanguage=X
for /f "tokens=3" %%L in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Nls\Language" /v InstallLanguage') do set InstallLanguage=%%L

:: German installation
for %%L in (0407 0c07 1407 1007 0807) do if %InstallLanguage% == %%L set YES=J

:: English installation
for %%L in (0409 0809 0c09 2809 1009 2409 3c09 4009 3809 1809 2009 4409 1409 3409 4809 1c09 2c09 3009) do if %InstallLanguage% == %%L set YES=Y

if not "%YES%" == "" goto :skipYES
:: the following must be set to the character representing "YES"
:: this is needed to run the format utility without user interaction
echo.
set /p YES="Enter the letter that represents YES in your language [Y]: "
if "%YES%" == "" set YES=Y

:skipYES
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
echo Values 1 to 9 will be multiplied by 10
echo any other value will be used as is

echo.
echo 0 ... EXIT
echo.
set /p answer="Enter percentage of free Space: "

if "%answer%" == "" goto :EOF
if %answer% EQU 0 goto :EOF

set PercentageFree=0

if %answer% LEQ 100 set PercentageFree=%answer%
if %answer% LSS 10 set /a PercentageFree="answer * 10"

if %PercentageFree% GTR 0 goto :StartProcess

echo.
echo Selection must be between 0 and 100 !!!
echo.
pause
goto :DisplayMenu

:StartProcess
echo.
echo --------------------------------------
echo.
echo Enter the fragmentation rate,
echo 1 out of x files should be fragmented.
echo.
set /p FragmentationRate="Enter a value between 1 and 100 for x: "

if "%FragmentationRate%" == "" set FragmentationRate=1
if %FragmentationRate% EQU 0 set FragmentationRate=1
if %FragmentationRate% GTR 100 set FragmentationRate=100

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
    
    if not exist "%~1\" goto :EOF
    
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
    
    set /a size="24 + %RANDOM% / 3"
    set /a fragments="%RANDOM% / 1365"
    set count=0
    set NoCompr=0
    set dest=%~1
    
    title Creating Fragmented Files on Drive "%~1" until %PercentageFree%%% free space left ...
    echo Creating Fragmented Files on Drive "%~1" until %PercentageFree%%% free space left ...
    echo.

    :loop
        call :doit "%~1"
        call :increment
        set ExitCode=%ERRORLEVEL%
        ping -n 3 localhost >NUL
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
    echo Executing ... chkdsk %~1 /r /f
    echo. & echo n | chkdsk %~1 /r /f
    
    call :delay 2
goto :EOF

:answers
    echo Test%ex_type%v%fs_ver_txt%
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

    echo File %count_fmt:~-3% ... %size_fmt:~-6% kB ... %frag_fmt:~-3% Fragments

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
    set /a count+=1
    
    set /a NoFolder="count %% 10"
    set /a NoCompr="count %% 5"

    set /a size="24 + %RANDOM% / 3"
    set /a fragments="%RANDOM% / 1365"
    
    set /a quotient="count %% %FragmentationRate%"
    
    if %quotient% EQU 0 goto :EOF
    
    set fragments=1
goto :EOF
