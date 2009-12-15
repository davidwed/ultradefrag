@echo off

echo Build script producing the UltraDefrag Portable - Micro Edition package.
echo Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

call build-micro.cmd %1 --portable

mkdir bin\ultradefrag-micro-portable-%ULTRADFGVER%.i386
set PORTABLE_DIR=bin\ultradefrag-micro-portable-%ULTRADFGVER%.i386
copy /Y CREDITS.TXT %PORTABLE_DIR%\
copy /Y HISTORY.TXT %PORTABLE_DIR%\
copy /Y LICENSE.TXT %PORTABLE_DIR%\
copy /Y README.TXT %PORTABLE_DIR%\
copy /Y bin\hibernate.exe %PORTABLE_DIR%\
copy /Y bin\udefrag.dll %PORTABLE_DIR%\
copy /Y bin\udefrag.exe %PORTABLE_DIR%\
copy /Y bin\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y bin\zenwinx.dll %PORTABLE_DIR%\

cd bin
zip -r -m -9 -X ultradefrag-micro-portable-%ULTRADFGVER%.bin.i386.zip ultradefrag-micro-portable-%ULTRADFGVER%.i386
if %errorlevel% neq 0 goto fail

:X64
if not exist amd64\udefrag.exe goto success
mkdir amd64\ultradefrag-micro-portable-%ULTRADFGVER%.amd64
set PORTABLE_DIR=amd64\ultradefrag-micro-portable-%ULTRADFGVER%.amd64
copy /Y ..\CREDITS.TXT %PORTABLE_DIR%\
copy /Y ..\HISTORY.TXT %PORTABLE_DIR%\
copy /Y ..\LICENSE.TXT %PORTABLE_DIR%\
copy /Y ..\README.TXT %PORTABLE_DIR%\
copy /Y amd64\hibernate.exe %PORTABLE_DIR%\
copy /Y amd64\udefrag.dll %PORTABLE_DIR%\
copy /Y amd64\udefrag.exe %PORTABLE_DIR%\
copy /Y amd64\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y amd64\zenwinx.dll %PORTABLE_DIR%\

cd amd64
zip -r -m -9 -X ultradefrag-micro-portable-%ULTRADFGVER%.bin.amd64.zip ultradefrag-micro-portable-%ULTRADFGVER%.amd64
if %errorlevel% neq 0 goto Lf
cd ..
goto IA64

:Lf
cd ..
goto fail

:IA64
if not exist ia64\udefrag.exe goto success
mkdir ia64\ultradefrag-micro-portable-%ULTRADFGVER%.ia64
set PORTABLE_DIR=ia64\ultradefrag-micro-portable-%ULTRADFGVER%.ia64
copy /Y ..\CREDITS.TXT %PORTABLE_DIR%\
copy /Y ..\HISTORY.TXT %PORTABLE_DIR%\
copy /Y ..\LICENSE.TXT %PORTABLE_DIR%\
copy /Y ..\README.TXT %PORTABLE_DIR%\
copy /Y ia64\hibernate.exe %PORTABLE_DIR%\
copy /Y ia64\udefrag.dll %PORTABLE_DIR%\
copy /Y ia64\udefrag.exe %PORTABLE_DIR%\
copy /Y ia64\udefrag-kernel.dll %PORTABLE_DIR%\
copy /Y ia64\zenwinx.dll %PORTABLE_DIR%\

cd ia64
zip -r -m -9 -X ultradefrag-micro-portable-%ULTRADFGVER%.bin.ia64.zip ultradefrag-micro-portable-%ULTRADFGVER%.ia64
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
