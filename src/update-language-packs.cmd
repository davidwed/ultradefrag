@echo off

echo This script will automatically update all UltraDefrag Language Packs.
echo.
echo But currently we have no idea how it will do the update.
echo Currently we are doing it manually in the following steps:
echo.
echo 1. make an export of all files on ultradefrag.wikispaces.com
echo 2. download an archive and unpack it somewhere
echo 3. move all translations to appropriate places in the source code directory
echo 4. run this script
echo.

cd bin

utf8-16 ..\gui\i18n\*.GUI
if %errorlevel% neq 0 cd .. && exit /B 1

utf8-16 ..\udefrag-gui-config\i18n\*.Config
if %errorlevel% neq 0 cd .. && exit /B 1

cd ..
exit /B 0
