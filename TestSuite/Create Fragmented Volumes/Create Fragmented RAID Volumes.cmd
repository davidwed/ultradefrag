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
set YES=J

:: specify volumes that should be used as test volumes
:: any file located in the root folder will be deleted
set ProcessVolumes=R:

:: specify volume that should be compressed
set CompressedVolume=S:

:: specify volume that should include compressed and regular files
set MixedVolume=T:

echo.
set /p answer="Enable DryRun (Y/[N])? "

if /i "%answer%" == "Y" (
    set DryRun=1
) else (
    set DryRun=0
)

for /d %%D in ( "%ProgramFiles%\MyDefrag*" ) do set MyDefragDir=%%~D

if %DryRun% == 0 if "%MyDefragDir%" == "" (
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
echo 1 ...  4%% of free Space
echo 2 ...  8%% of free Space
echo 3 ... 13%% of free Space

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
set /a InitialSize="23 - answer"

rem NTFS volumes
for %%L in ( %ProcessVolumes% ) do call :FragmentDrive "%%~L"

title Operation Completed ...
echo.
pause

goto :EOF

:FragmentDrive
	call :delay 0
	
    set ex_type=
	set c_switch=
    if /i "%~1" == "%CompressedVolume%" set ex_type=compressed
	if /i "%~1" == "%CompressedVolume%" set c_switch=/C
    if /i "%~1" == "%MixedVolume%" set ex_type=mixed

	call :answers >"%TMP%\answers.txt"
	
    title Setting Volume Label of "%~1" ...
	echo Executing ... label %~1 TestRAID1%ex_type%
    echo.
    if %DryRun% == 0 label %~1 TestRAID1%ex_type%
	
	title Formatting Drive "%~1" ...
	echo Executing ... format %~1 /FS:NTFS /V:TestRAID1%ex_type% %c_switch% /X
	if %DryRun% == 0 echo. & format %~1 /FS:NTFS /V:TestRAID1%ex_type% %c_switch% /X <"%TMP%\answers.txt"
	
	call :delay 5
	
	title Checking Drive "%~1" ...
	echo Executing ... chkdsk %~1 /r /f /v
	if %DryRun% == 0 echo. & chkdsk %~1 /r /f /v
	
	call :delay 2
    
    set size=%InitialSize%
    set fragments=0
    set count=0
    set total=0
    set dest=%~1
	
	title Creating Fragmented Files on Drive "%~1" ...
	echo Creating Fragmented Files on Drive "%~1" ...
	echo.
    
	for /L %%C in (1,1,101) do (
        call :increment
        call :doit "%~1" %%C
    )
	
	if ERRORLEVEL 1 (
		echo.
		echo Operation failed ...
	) else (
		echo.
		echo Operation succeeded ...
	)
	
	call :delay 5
	
	title Checking Drive "%~1" ...
	echo Executing ... chkdsk %~1 /r /f /v
	if %DryRun% == 0 echo. & chkdsk %~1 /r /f /v
	
	call :delay 2
goto :EOF

:answers
	echo TestRAID1%ex_type%
	echo %YES%
goto :EOF

:doit
    set /a total+=size
    
    if %count% EQU 1 goto :skip
        if %rest4% EQU 0 set dest=%~1\folder_%size%
        if %rest4% EQU 0 echo mkdir "%dest%"
        if %DryRun% == 0 if %rest4% EQU 0 mkdir "%dest%"
    :skip
    
    if %DryRun% == 0 (
        "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%dest%\file_%~2.bin" >NUL
        if /i "%~1" == "%MixedVolume%" if %rest3% EQU 0 compact /c "%dest%\file_%~2.bin" >NUL
    )
    if %DryRun% == 1 (
        echo "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%dest%\file_%~2.bin" ... Total Usage: %total% kB
        if /i "%~1" == "%MixedVolume%" if %rest3% EQU 0 echo compact /c "%dest%\file_%~2.bin"
    )
goto :EOF

:delay
    set /a seconds="%1 + 1"
	echo.
	if %seconds% == 1 (
		echo ============================================
	) else (
		echo --------------------------------------------
		if %DryRun% == 0 ping -n %seconds% localhost >NUL
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
