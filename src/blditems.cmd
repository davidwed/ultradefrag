@echo off

echo Compile monolithic native interface...

pushd obj\zenwinx
lua ..\..\tools\mkmod.lua zenwinx.build static-lib
if %errorlevel% neq 0 goto end

cd ..\udefrag-kernel
lua ..\..\tools\mkmod.lua udefrag-kernel.build static-lib
if %errorlevel% neq 0 goto end

cd ..\udefrag
lua ..\..\tools\mkmod.lua udefrag.build static-lib
if %errorlevel% neq 0 goto end

cd ..\native
lua ..\..\tools\mkmod.lua defrag_native.build
if %errorlevel% neq 0 goto end

echo Compile native DLL's...

cd ..\zenwinx
lua ..\..\tools\mkmod.lua zenwinx.build
if %errorlevel% neq 0 goto end

cd ..\udefrag-kernel
lua ..\..\tools\mkmod.lua udefrag-kernel.build
if %errorlevel% neq 0 goto end

cd ..\udefrag
lua ..\..\tools\mkmod.lua udefrag.build
if %errorlevel% neq 0 goto end

echo Compile other modules...

cd ..\hibernate
lua ..\..\tools\mkmod.lua hibernate.build
if %errorlevel% neq 0 goto end

rem skip lua when building the micro edition
if "%UD_MICRO_EDITION%" equ "1" goto skip_lua

rem workaround for WDK 7
rem set IGNORE_LINKLIB_ABUSE=1

cd ..\lua5.1
lua ..\..\tools\mkmod.lua lua5.1a_dll.build
if %errorlevel% neq 0 goto end

rem workaround for WDK 7
rem set IGNORE_LINKLIB_ABUSE=

lua ..\..\tools\mkmod.lua lua5.1a_exe.build
if %errorlevel% neq 0 goto end

lua ..\..\tools\mkmod.lua lua5.1a_gui.build
if %errorlevel% neq 0 goto end

:skip_lua

cd ..\console
lua ..\..\tools\mkmod.lua defrag.build
if %errorlevel% neq 0 goto end

cd ..\utf8-16
lua ..\..\tools\mkmod.lua utf8-16.build
if %errorlevel% neq 0 goto end

rem skip GUI when building the micro edition
if "%UD_MICRO_EDITION%" equ "1" goto skip_gui

rem workaround for WDK 7
rem set IGNORE_LINKLIB_ABUSE=1

cd ..\wgx
lua ..\..\tools\mkmod.lua wgx.build
if %errorlevel% neq 0 goto end

rem workaround for WDK 7
rem set IGNORE_LINKLIB_ABUSE=

cd ..\gui
lua ..\..\tools\mkmod.lua dfrg.build
if %errorlevel% neq 0 goto end

cd ..\udefrag-gui-config
lua ..\..\tools\mkmod.lua udefrag-gui-config.build
if %errorlevel% neq 0 goto end

:skip_gui

cd ..\bootexctrl
lua ..\..\tools\mkmod.lua bootexctrl.build
if %errorlevel% neq 0 goto end

popd
exit /B 0

:end
rem workaround for WDK 7
rem set IGNORE_LINKLIB_ABUSE=

popd
exit /B 1
