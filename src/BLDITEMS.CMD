@echo off
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre %1 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P

echo Build the driver...
cd .\driver
build.exe
if %errorlevel% neq 0 goto fail

if "%1" equ "" goto x86

echo Build the console interface...
cd ..\console
build.exe
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
build.exe
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native
build.exe
if %errorlevel% neq 0 goto fail

if "%1" equ "" goto x86
cd ..\x64_inst
build.exe
if %errorlevel% neq 0 goto fail

:x86

:fail
