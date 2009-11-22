@echo off

echo Build script producing the UltraDefrag Portable package.
echo Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

call build.cmd %1 --portable

mkdir bin\ultradefrag-portable-%ULTRADFGVER%
set PORTABLE_DIR=bin\ultradefrag-portable-%ULTRADFGVER%
copy /Y CREDITS.TXT %PORTABLE_DIR%\
copy /Y HISTORY.TXT %PORTABLE_DIR%\
copy /Y LICENSE.TXT %PORTABLE_DIR%\
copy /Y README.TXT %PORTABLE_DIR%\
copy /Y bin\hibernate.exe %PORTABLE_DIR%\
copy /Y bin\udefrag.dll %PORTABLE_DIR%\
copy /Y bin\udefrag.exe %PORTABLE_DIR%\
copy /Y bin\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y bin\zenwinx.dll %PORTABLE_DIR%\

copy /Y bin\dfrg.exe %PORTABLE_DIR%\ultradefrag.exe
copy /Y bin\lua5.1a.dll %PORTABLE_DIR%\
copy /Y bin\lua5.1a.exe %PORTABLE_DIR%\
copy /Y bin\lua5.1a_gui.exe %PORTABLE_DIR%\
copy /Y bin\udefrag-gui-config.exe %PORTABLE_DIR%\
copy /Y bin\wgx.dll %PORTABLE_DIR%\

mkdir %PORTABLE_DIR%\handbook
copy /Y ..\doc\html\handbook\*.* %PORTABLE_DIR%\handbook\

mkdir %PORTABLE_DIR%\scripts
copy /Y scripts\udreportcnv.lua %PORTABLE_DIR%\scripts\
copy /Y scripts\udsorting.js %PORTABLE_DIR%\scripts\

mkdir %PORTABLE_DIR%\options
copy /Y scripts\udreportopts.lua %PORTABLE_DIR%\options\

mkdir %PORTABLE_DIR%\i18n
mkdir %PORTABLE_DIR%\i18n\gui
mkdir %PORTABLE_DIR%\i18n\gui-config
copy /Y gui\i18n\*.lng %PORTABLE_DIR%\i18n\gui\
copy /Y udefrag-gui-config\i18n\*.lng %PORTABLE_DIR%\i18n\gui-config\

copy /Y installer\LanguageSelector.nsi bin\
copy /Y installer\lang.ini bin\
copy /Y installer\LanguageSelectorSmall.bmp bin\
copy /Y installer\LanguageSelector.ico bin\

cd bin

call setvars.cmd
%NSISDIR%\makensis.exe /DULTRADFGVER=%ULTRADFGVER% LanguageSelector.nsi
if %errorlevel% neq 0 goto fail

copy /Y LanguageSelector.exe ultradefrag-portable-%ULTRADFGVER%\

cd ultradefrag-portable-%ULTRADFGVER%
.\ultradefrag.exe --setup
cd..

zip -r -m -9 -X ultradefrag-portable-%ULTRADFGVER%.bin.zip ultradefrag-portable-%ULTRADFGVER%
if %errorlevel% neq 0 goto fail

:success
echo.
echo Build success!

cd ..
set PORTABLE_DIR=
exit /B 0

:fail
echo.
echo Build error (code %ERRORLEVEL%)!

cd ..
set PORTABLE_DIR=
exit /B 1
