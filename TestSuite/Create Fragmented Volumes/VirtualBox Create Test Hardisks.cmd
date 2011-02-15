@echo off
::
:: This script creates virtual harddisks
::
:: It can be used to create {Prefix}FAT.vdi, {Prefix}NTFS.vdi, {Prefix}UDF.vdi
::

:: installation folder of VirtualBox
set VBoxRoot=%ProgramFiles%\Oracle\VirtualBox

:: Folder containing the virtual harddisks
set HDRoot=E:\VirtualBox\HardDisks

if not exist "%VBoxRoot%" goto :noVBroot
if not exist "%HDRoot%" goto :noHDroot

echo.
set /p prefix="Enter Prefix (Example: XPTest): "
echo.
set /p createFAT="Create FAT Disks ([Y]/N)? "
echo.
set /p createNTFS="Create NTFS Disks ([Y]/N)? "
echo.
set /p createUDF="Create UDF Disks ([Y]/N)? "

if "%prefix%" == "" goto :quit

cd /d %VBoxRoot%

:FAT
if /i "%createFAT%" == "N" goto :NTFS

for %%F in ( FAT FAT32 exFAT) do (
    echo.
    echo ---------------------------------------
    echo.
    echo Creating "%HDRoot%\%prefix%%%F.vdi" ...
    echo.
    VBoxManage createhd --filename "%HDRoot%\%prefix%%%F.vdi" --size 1024
)

:NTFS
if /i "%createNTFS%" == "N" goto :UDF

for %%F in ( NTFS NTFScompressed NTFSmixed ) do (
    echo.
    echo ---------------------------------------
    echo.
    echo Creating "%HDRoot%\%prefix%%%F.vdi" ...
    echo.
    VBoxManage createhd --filename "%HDRoot%\%prefix%%%F.vdi" --size 1024
)

:UDF
if /i "%createUDF%" == "N" goto :quit

for %%F in ( UDF102 UDF150 UDF200 UDF201 UDF250 UDF250mirror ) do (
    echo.
    echo ---------------------------------------
    echo.
    echo Creating "%HDRoot%\%prefix%%%F.vdi" ...
    echo.
    VBoxManage createhd --filename "%HDRoot%\%prefix%%%F.vdi" --size 1024
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

:noHDroot
echo.
echo Harddisk Root Folder missing, aborting ...
goto :quit
