@echo off
cd obj\driver
perl ..\..\tools\mkmod.pl ultradfg.build
if %errorlevel% neq 0 goto fail
cd ..\zenwinx
perl ..\..\tools\mkmod.pl zenwinx.build
if %errorlevel% neq 0 goto fail
cd ..\udefrag
perl ..\..\tools\mkmod.pl udefrag.build
if %errorlevel% neq 0 goto fail
cd ..\console
perl ..\..\tools\mkmod.pl defrag.build
if %errorlevel% neq 0 goto fail
cd ..\gui
perl ..\..\tools\mkmod.pl dfrg.build
if %errorlevel% neq 0 goto fail
cd ..\native
perl ..\..\tools\mkmod.pl defrag_native.build
if %errorlevel% neq 0 goto fail
cd ..\..

exit /B




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
build.exe
if %errorlevel% neq 0 goto fail

:fail
