@echo off

rem This batch file use mingw.

rem set environment
if "%MINGW_ENV%" neq "" goto start_build
set path=%path%;%MINGWBASE%\bin
set MINGW_ENV=ok

:start_build
echo Build the driver...
cd .\driver
echo NT 5.0+ version...
set NT4_TARGET=false
mingw32-make --always-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail
echo NT 4.0 version...
set NT4_TARGET=true
mingw32-make --always-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail

rem Build the zenwinx.dll ...
cd ..\dll\zenwinx\src
call MINGW_BUILD_ZENWINX.CMD
if %errorlevel% neq 0 goto fail
copy /Y .\bin\libzenwinx.dll.a ..\..\..\lib\
copy /Y .\bin\zenwinx.dll ..\..\..\bin\

rem delete res files produced by ms compiler
del /S /Q ..\..\..\obj\*.res

echo Build the udefrag.dll ...
cd ..\..\udefrag
mingw32-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail
copy /Y ..\..\lib\udefrag.dll ..\..\bin\

echo Build the console interface...
cd ..\..\console
mingw32-make --always-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
mingw32-make --always-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native
mingw32-make -f Makefile.mingw CFG=Release
if %errorlevel% neq 0 goto fail

exit /B 0

:fail
exit /B 1
