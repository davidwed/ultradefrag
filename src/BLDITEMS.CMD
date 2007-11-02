@echo off
pushd ..
call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre %1 WNET
popd
set BUILD_DEFAULT=-nmake -i -g -P

echo Build the driver...
cd .\driver

echo NT 5.0+ version...
set NT4_TARGET=false
build.exe -c
if %errorlevel% neq 0 goto fail

if "%1" neq "" goto L1
rem save modern version
copy .\objfre_wnet_x86\i386\ultradfg.sys ..\..\bin\

echo NT 4.0 version...
set NT4_TARGET=true
rem -c option is neccessary to rebuild driver
build.exe -c
if %errorlevel% neq 0 goto fail
rem save nt4 version
copy .\objfre_wnet_x86\i386\ultradfg_nt4.sys ..\..\bin\
:L1

echo Build the udefrag.dll ...
cd ..\dll\udefrag.dll
build.exe
if %errorlevel% neq 0 goto fail
if "%1" equ "AMD64" goto amd_lib_copy
if "%1" equ "64" goto ia_lib_copy
copy .\objfre_wnet_x86\i386\udefrag.lib ..\..\..\lib\
goto build_console
:amd_lib_copy
copy .\objfre_wnet_amd64\amd64\udefrag.lib ..\..\..\lib\amd64\
goto build_console
:ia_lib_copy
copy .\objfre_wnet_ia64\ia64\udefrag.lib ..\..\..\lib\ia64\

:build_console
echo Build the console interface...
cd ..\..\console
build.exe
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
build.exe
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native

echo NT 5.0+ version...
set NT4_TARGET=false
build.exe -c
if %errorlevel% neq 0 goto fail

if "%1" neq "" goto L2
rem save modern version
copy .\objfre_wnet_x86\i386\defrag_native.exe ..\..\bin\

echo NT 4.0 version...
set NT4_TARGET=true
rem -c option is neccessary to rebuild program
build.exe -c
if %errorlevel% neq 0 goto fail
rem save nt4 version
copy .\objfre_wnet_x86\i386\defrag_native_nt4.exe ..\..\bin\
:L2

:fail
