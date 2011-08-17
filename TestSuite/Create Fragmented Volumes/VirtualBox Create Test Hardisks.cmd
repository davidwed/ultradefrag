@rem 
@echo off
::
:: This script creates virtual harddisks
:: It is tested with VirtualBox v4.1
::
:: It can be used to create NewHardDisk{Index}.vdi files
:: in the folders containing virtual machines
::

setlocal enabledelayedexpansion

title %~n0

:: installation folder of VirtualBox
for /f "tokens=2*" %%I in ('reg query "HKLM\SOFTWARE\Oracle\VirtualBox" /v InstallDir') do set VBoxRoot=%%~J
if "%VBoxRoot%" == "" goto :noVBroot
if not exist "%VBoxRoot%" goto :noVBroot

:: Folder containing the virtual machines
if not exist "%USERPROFILE%\.VirtualBox\VirtualBox.xml" goto :noVMRoot

for /f "tokens=2" %%D in ('findstr "defaultMachineFolder" "%USERPROFILE%\.VirtualBox\VirtualBox.xml"') do set VMRootTMP1=%%~D
set VMRootTMP=%VMRootTMP1:"=%
if "%VMRootTMP%" == "" goto :noVMRoot

for /f "tokens=2 delims==" %%D in ('echo "%VMRootTMP%"') do set VMRootTMP1=%%~D
set VMRoot=%VMRootTMP1:"=%
if "%VMRoot%" == "" goto :noVMRoot
if not exist "%VMRoot%" goto :noVMRoot

cd /d %VMRoot%

:DisplayVMlist
cls
set MenuItem=0
set MenuSelectionsTMP=
echo.
for /d %%V in ( * ) do set /a MenuItem+=1 & echo !MenuItem! ... "%%~V" & set MenuSelectionsTMP=!MenuSelectionsTMP!:%%~V

set MenuSelections=%MenuSelectionsTMP:~1%
echo.
echo 0 ... EXIT
echo.
set /p SelectedItem="Select the VM to create disks for: "

if "%SelectedItem%" == "" goto :quit
if %SelectedItem% EQU 0 goto :quit

if %SelectedItem% LEQ %MenuItem% goto :CreateDisks

echo.
echo Please enter a number in the range of 0 to %MenuItem%.
echo.
pause
goto :DisplayVMlist

:CreateDisks
for /f "tokens=%SelectedItem% delims=:" %%S in ('echo %MenuSelections%') do set SelectedVM=%%~S

echo.
set /p MaxIndex="Enter number of disks to create (0 to exit, maximum 20): "
if "%MaxIndex%" == "" goto :quit
if %MaxIndex% EQU 0 goto :quit
if %MaxIndex% GTR 20 set MaxIndex=20

cd /d %VBoxRoot%

for /L %%F in (1,1,%MaxIndex%) do (
    set HardDiskName="%VMRoot%\%SelectedVM%\%SelectedVM: =_%_TestDisk_%%F.vdi"
    
    echo.
    echo ---------------------------------------
    echo.
    
    if not exist !HardDiskName! (
        echo Creating !HardDiskName! ...
        echo.
        VBoxManage createhd --filename !HardDiskName! --size 1024
    ) else (
        echo Modifying !HardDiskName! ...
        echo.
        VBoxManage modifyhd !HardDiskName! --resize 1024
    )
)

:quit
echo.
echo ---------------------------------------
echo.
pause

goto :EOF

:noVBroot
echo.
echo VirtualBox not installed, aborting ...
goto :quit

:noVMRoot
echo.
echo Harddisk Root Folder missing, aborting ...
goto :quit
