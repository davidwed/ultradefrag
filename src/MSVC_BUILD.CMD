@echo off

rem This batch file use ms c compiler and reactos ddk.

rem set environment
if "%MSVC_ENV%" neq "" goto start_build
call "%MSVSBIN%\vcvars32.bat"
set MSVC_ENV=ok

:start_build
echo Build the driver...
cd .\driver
echo NT 5.0+ version...
set NT4_TARGET=false
nmake /NOLOGO /A /f ultradfg.mak
if %errorlevel% neq 0 goto fail
echo NT 4.0 version...
set NT4_TARGET=true
nmake /NOLOGO /A /f ultradfg.mak
if %errorlevel% neq 0 goto fail

rem Build the zenwinx.dll ...
cd ..\dll\zenwinx\src
call MSVC_BUILD_ZENWINX.CMD
if %errorlevel% neq 0 goto fail
copy /Y .\bin\zenwinx.lib ..\..\..\lib\
copy /Y .\bin\zenwinx.dll ..\..\..\bin\

echo Build the udefrag.dll ...
cd ..\..\udefrag
nmake /NOLOGO /A /f udefrag.mak CFG="udefrag - Win32 Release"
if %errorlevel% neq 0 goto fail
copy /Y ..\..\bin\udefrag.lib ..\..\lib\

echo Build the console interface...
cd ..\..\console
nmake /NOLOGO /A /f defrag.mak CFG="Defrag - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
nmake /NOLOGO /A /f dfrg.mak CFG="Dfrg - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native
nmake /NOLOGO /A /f defrag_native.mak
if %errorlevel% neq 0 goto fail

exit /B 0

:fail
exit /B 1
