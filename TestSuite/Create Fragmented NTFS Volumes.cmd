@echo off

::
:: this script is used to create fragmented test volumes
::
:: it uses the fragmentation utility included in MyDefrag
:: available at http://www.mydefrag.com/
::
:: the percentage of free space is based on volumes of 1GB in size
::

:: specify volumes that should be used as test volumes
:: any file located in the root folder will be deleted
set ProcessVolumes=Q: R: S:

:: specify volume that should include compressed and regular files
set MixedVolume=S:

echo.
set /p answer="Enable DryRun (Y/[N])? "

if /i "%answer%" == "Y" (
    set DryRun=1
) else (
    set DryRun=0
)

for /d %%D in ( "%ProgramFiles%\MyDefrag*" ) do set MyDefragDir=%%~D

if %DryRun% == 0 if "%MyDefragDir%" == "" goto :EOF

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
	echo.
	title Deleting Test Files on "%~1" ...
	if %DryRun% == 0 del /f /q "%~1\*.*"
	
	call :delay
	
	echo.
	title Checking Drive "%~1" ...
	if %DryRun% == 0 chkdsk %~1 /r /v /f /x
	
	call :delay
    
    set size=%InitialSize%
    set fragments=0
    set count=0
    set total=0
	
	echo.
	title Creating Fragmented Files on Drive "%~1" ...
	echo Creating Fragmented Files on Drive "%~1" ...
	echo.
	for /L %%C in (0,1,100) do (
        call :increment
        call :doit "%~1" %%C
    )
	
	if ERRORLEVEL 1 (
		echo.
		echo Operation failed ...
		echo.
	) else (
		echo Operation succeeded ...
		echo.
	)
	
	call :delay
	
	title Checking Drive "%~1" ...
	if %DryRun% == 0 chkdsk %~1 /r /v /f /x
	echo.
	echo --------------------------------------------
	
	call :delay
goto :EOF

:doit
    set /a total+=size
    
    if %DryRun% == 0 (
        "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%~1\file_%~2.bin" >NUL
        if /i "%~1" == "%MixedVolume%" if %rest3% EQU 0 compact /c "%~1\file_%~2.bin" >NUL
    )
    if %DryRun% == 1 (
        echo "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%~1\file_%~2.bin" ... Total Usage: %total% kB
        if /i "%~1" == "%MixedVolume%" if %rest3% EQU 0 echo compact /c "%~1\file_%~2.bin"
    )
goto :EOF

:delay
	for /L %%N in (0,1,100000) do set delay=%%N
goto :EOF

:increment
    set /a rest1="count %% 9"
    set /a rest2="count %% 8"
    set /a rest3="count %% 5"
    
    if %rest1% EQU 0 set /a size="size * 2"
    if %rest2% EQU 0 set /a fragments+=2
    
    set /a count+=1
goto :EOF
