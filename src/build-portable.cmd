@echo off

echo Build script producing the UltraDefrag Portable package.
echo Copyright (c) 2009-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).

call build.cmd %1 --portable

mkdir bin\ultradefrag-portable-%ULTRADFGVER%.i386
set PORTABLE_DIR=bin\ultradefrag-portable-%ULTRADFGVER%.i386
copy /Y CREDITS.TXT %PORTABLE_DIR%\
copy /Y HISTORY.TXT %PORTABLE_DIR%\
copy /Y LICENSE.TXT %PORTABLE_DIR%\
copy /Y README.TXT %PORTABLE_DIR%\
copy /Y bin\hibernate.exe %PORTABLE_DIR%\hibernate4win.exe
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
copy /Y ..\doc\html\handbook\doxy-doc\html\*.* %PORTABLE_DIR%\handbook\

mkdir %PORTABLE_DIR%\scripts
copy /Y scripts\udreportcnv.lua %PORTABLE_DIR%\scripts\
copy /Y scripts\udsorting.js %PORTABLE_DIR%\scripts\
copy /Y scripts\udreport.css %PORTABLE_DIR%\scripts\

mkdir %PORTABLE_DIR%\options
copy /Y scripts\udreportopts.lua %PORTABLE_DIR%\options\

mkdir %PORTABLE_DIR%\i18n
mkdir %PORTABLE_DIR%\i18n\gui
mkdir %PORTABLE_DIR%\i18n\gui-config
copy /Y gui\i18n\*.GUI %PORTABLE_DIR%\i18n\gui\
copy /Y udefrag-gui-config\i18n\*.Config %PORTABLE_DIR%\i18n\gui-config\

copy /Y installer\LanguageSelector.nsi bin\
copy /Y installer\lang.ini bin\
copy /Y installer\LanguageSelector.ico bin\

cd bin

call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% LanguageSelector.nsi
if %errorlevel% neq 0 goto fail

copy /Y LanguageSelector.exe ultradefrag-portable-%ULTRADFGVER%.i386\

cd ultradefrag-portable-%ULTRADFGVER%.i386
.\ultradefrag.exe --setup
cd..

copy /Y ultradefrag-portable-%ULTRADFGVER%.i386\options\guiopts.lua .\
rem copy /Y ultradefrag-portable-%ULTRADFGVER%.i386\options\font.lua .\

zip -r -m -9 -X ultradefrag-portable-%ULTRADFGVER%.bin.i386.zip ultradefrag-portable-%ULTRADFGVER%.i386
if %errorlevel% neq 0 goto fail

:X64
if not exist amd64\udefrag.exe goto success
mkdir amd64\ultradefrag-portable-%ULTRADFGVER%.amd64
set PORTABLE_DIR=amd64\ultradefrag-portable-%ULTRADFGVER%.amd64
copy /Y ..\CREDITS.TXT %PORTABLE_DIR%\
copy /Y ..\HISTORY.TXT %PORTABLE_DIR%\
copy /Y ..\LICENSE.TXT %PORTABLE_DIR%\
copy /Y ..\README.TXT %PORTABLE_DIR%\
copy /Y amd64\hibernate.exe %PORTABLE_DIR%\hibernate4win.exe
copy /Y amd64\udefrag.dll %PORTABLE_DIR%\
copy /Y amd64\udefrag.exe %PORTABLE_DIR%\
copy /Y amd64\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y amd64\zenwinx.dll %PORTABLE_DIR%\

copy /Y amd64\dfrg.exe %PORTABLE_DIR%\ultradefrag.exe
copy /Y amd64\lua5.1a.dll %PORTABLE_DIR%\
copy /Y amd64\lua5.1a.exe %PORTABLE_DIR%\
copy /Y amd64\lua5.1a_gui.exe %PORTABLE_DIR%\
copy /Y amd64\udefrag-gui-config.exe %PORTABLE_DIR%\
copy /Y amd64\wgx.dll %PORTABLE_DIR%\

mkdir %PORTABLE_DIR%\handbook
copy /Y ..\..\doc\html\handbook\doxy-doc\html\*.* %PORTABLE_DIR%\handbook\

mkdir %PORTABLE_DIR%\scripts
copy /Y ..\scripts\udreportcnv.lua %PORTABLE_DIR%\scripts\
copy /Y ..\scripts\udsorting.js %PORTABLE_DIR%\scripts\
copy /Y ..\scripts\udreport.css %PORTABLE_DIR%\scripts\

mkdir %PORTABLE_DIR%\options
copy /Y ..\scripts\udreportopts.lua %PORTABLE_DIR%\options\

mkdir %PORTABLE_DIR%\i18n
mkdir %PORTABLE_DIR%\i18n\gui
mkdir %PORTABLE_DIR%\i18n\gui-config
copy /Y ..\gui\i18n\*.GUI %PORTABLE_DIR%\i18n\gui\
copy /Y ..\udefrag-gui-config\i18n\*.Config %PORTABLE_DIR%\i18n\gui-config\

copy /Y .\LanguageSelector.exe %PORTABLE_DIR%\

copy /Y .\guiopts.lua %PORTABLE_DIR%\options\
rem copy /Y .\font.lua %PORTABLE_DIR%\options\

cd amd64

zip -r -m -9 -X ultradefrag-portable-%ULTRADFGVER%.bin.amd64.zip ultradefrag-portable-%ULTRADFGVER%.amd64
if %errorlevel% neq 0 goto Lf
cd ..
goto IA64

:Lf
cd ..
goto fail

:IA64
if not exist ia64\udefrag.exe goto success
mkdir ia64\ultradefrag-portable-%ULTRADFGVER%.ia64
set PORTABLE_DIR=ia64\ultradefrag-portable-%ULTRADFGVER%.ia64
copy /Y ..\CREDITS.TXT %PORTABLE_DIR%\
copy /Y ..\HISTORY.TXT %PORTABLE_DIR%\
copy /Y ..\LICENSE.TXT %PORTABLE_DIR%\
copy /Y ..\README.TXT %PORTABLE_DIR%\
copy /Y ia64\hibernate.exe %PORTABLE_DIR%\hibernate4win.exe
copy /Y ia64\udefrag.dll %PORTABLE_DIR%\
copy /Y ia64\udefrag.exe %PORTABLE_DIR%\
copy /Y ia64\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y ia64\zenwinx.dll %PORTABLE_DIR%\

copy /Y ia64\dfrg.exe %PORTABLE_DIR%\ultradefrag.exe
copy /Y ia64\lua5.1a.dll %PORTABLE_DIR%\
copy /Y ia64\lua5.1a.exe %PORTABLE_DIR%\
copy /Y ia64\lua5.1a_gui.exe %PORTABLE_DIR%\
copy /Y ia64\udefrag-gui-config.exe %PORTABLE_DIR%\
copy /Y ia64\wgx.dll %PORTABLE_DIR%\

mkdir %PORTABLE_DIR%\handbook
copy /Y ..\..\doc\html\handbook\doxy-doc\html\*.* %PORTABLE_DIR%\handbook\

mkdir %PORTABLE_DIR%\scripts
copy /Y ..\scripts\udreportcnv.lua %PORTABLE_DIR%\scripts\
copy /Y ..\scripts\udsorting.js %PORTABLE_DIR%\scripts\
copy /Y ..\scripts\udreport.css %PORTABLE_DIR%\scripts\

mkdir %PORTABLE_DIR%\options
copy /Y ..\scripts\udreportopts.lua %PORTABLE_DIR%\options\

mkdir %PORTABLE_DIR%\i18n
mkdir %PORTABLE_DIR%\i18n\gui
mkdir %PORTABLE_DIR%\i18n\gui-config
copy /Y ..\gui\i18n\*.GUI %PORTABLE_DIR%\i18n\gui\
copy /Y ..\udefrag-gui-config\i18n\*.Config %PORTABLE_DIR%\i18n\gui-config\

copy /Y .\LanguageSelector.exe %PORTABLE_DIR%\

copy /Y .\guiopts.lua %PORTABLE_DIR%\options\
rem copy /Y .\font.lua %PORTABLE_DIR%\options\

cd ia64

zip -r -m -9 -X ultradefrag-portable-%ULTRADFGVER%.bin.ia64.zip ultradefrag-portable-%ULTRADFGVER%.ia64
if %errorlevel% neq 0 goto Lf2
cd ..
goto success

:Lf2
cd ..
goto fail


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
