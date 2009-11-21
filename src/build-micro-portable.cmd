@echo off

echo Build script producing the UltraDefrag Portable - Micro Edition package.
echo Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

call build-micro.cmd %1 --portable

mkdir bin\ultradefrag-micro-portable-%ULTRADFGVER%
set PORTABLE_DIR=bin\ultradefrag-micro-portable-%ULTRADFGVER%
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
zip -r -m -9 -X ultradefrag-micro-portable-%ULTRADFGVER%.bin.zip ultradefrag-micro-portable-%ULTRADFGVER%
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
