@rem 
@echo off

::
:: This script is used to start a virtual host with the test disks attached.
:: Copyright (c) 2012 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

:: VirtualBox installation folder
set VBOX_INSTALL_DIR=%ProgramFiles%\Oracle\VirtualBox

:: root folder of virtual hosts
::
:: folders indicate virtual hosts
:: files indicate test disks
set VM_ROOT_DIR=D:\VirtualBox

:SelectHost
:: collect virtual hosts
cls
echo.
echo Collecting available hosts ...
echo.
set SelectedHost=0
set MenuItem=0
set ItemList=

for /d %%D in ( "%VM_ROOT_DIR%\*" ) do call :AddToItemList "%%~nD" & call :DisplayMenuItem "%%~nD"

echo.
echo 0 ... EXIT
echo.
set /p SelectedHost="Select host to start: "

if "%SelectedHost%" == "" goto :quit
if %SelectedHost% EQU 0 goto :quit

if %SelectedHost% LEQ %MenuItem% goto :GetHost
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectHost

:GetHost
for /f "tokens=%SelectedHost% delims=;" %%V in ("%ItemList%") do set ProcessHost=%%~V

set StorageController=SCSI-Controller
if "%ProcessHost%" == "Windows XP x86 Englisch" set StorageController=SATA-Controller

set PATH=%PATH%;%VBOX_INSTALL_DIR%
set PortNum=-1
echo.

for %%F in ( "%VM_ROOT_DIR%\*.vdi" ) do call :MountDisk "%%~F"

echo.
VBoxManage startvm "%ProcessHost%"

echo.
echo After the virtual host is turned off,
pause

echo.
for /l %%P in (%PortNum%,-1,0) do call :UnMountDisk %%P

echo.
echo Clearing Snapshots ...
for %%F in ( "%VM_ROOT_DIR%\%ProcessHost%\Snapshots\*.vdi" ) do VBoxManage closemedium disk "%%~F" --delete >nul 2>&1

goto :SelectHost

:quit
echo.
pause

goto :EOF

:UnMountDisk
    VBoxManage storageattach "%ProcessHost%" --storagectl "%StorageController%" --port %1 --device 0 --medium none
    if %ERRORLEVEL% equ 0 (
        echo Unmounting port %1 succeeded ...
    ) else (
        echo Unmounting port %1 failed ...
    )
    ping -n 3 localhost >nul 2>&1
goto :EOF

:MountDisk
    set /a PortNum+=1
    VBoxManage storageattach "%ProcessHost%" --storagectl "%StorageController%" --port %PortNum% --device 0 --type hdd --medium "%~1"
    if %ERRORLEVEL% equ 0 (
        echo Mounting disk "%~n1" to port %PortNum% succeeded ...
    ) else (
        echo Mounting disk "%~n1" to port %PortNum% failed ...
    )
    ping -n 3 localhost >nul 2>&1
goto :EOF

:DisplayMenuItem
    set /a MenuItem+=1
    echo %MenuItem% ... %*
goto :EOF

:AddToItemList
    if "%ItemList%" == "" (
        set ItemList=%~1
    ) else (
        set ItemList=%ItemList%;%~1
    )
goto :EOF
