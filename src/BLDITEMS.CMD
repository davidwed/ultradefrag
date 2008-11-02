@echo off

pushd obj\driver
lua ..\..\tools\mkmod.lua ultradfg.build
if %errorlevel% neq 0 goto end

cd ..\zenwinx
lua ..\..\tools\mkmod.lua zenwinx.build
if %errorlevel% neq 0 goto end

cd ..\udefrag
lua ..\..\tools\mkmod.lua udefrag.build
if %errorlevel% neq 0 goto end

rem skip lua when building the micro edition
if "%UD_MICRO_EDITION%" equ "1" goto skip_lua

cd ..\lua5.1
lua ..\..\tools\mkmod.lua lua5.1a_dll.build
if %errorlevel% neq 0 goto end

lua ..\..\tools\mkmod.lua lua5.1a_exe.build
if %errorlevel% neq 0 goto end

lua ..\..\tools\mkmod.lua lua5.1a_gui.build
if %errorlevel% neq 0 goto end

:skip_lua

cd ..\console
lua ..\..\tools\mkmod.lua defrag.build
if %errorlevel% neq 0 goto end

if "%UD_MICRO_EDITION%" equ "1" goto skip_gui

cd ..\gui
lua ..\..\tools\mkmod.lua dfrg.build
if %errorlevel% neq 0 goto end

cd ..\gui-launcher
lua ..\..\tools\mkmod.lua udefrag-gui.build
if %errorlevel% neq 0 goto end

:skip_gui

cd ..\native
lua ..\..\tools\mkmod.lua defrag_native.build
if %errorlevel% neq 0 goto end

cd ..\bootexctrl
lua ..\..\tools\mkmod.lua bootexctrl.build
if %errorlevel% neq 0 goto end

popd
exit /B 0

:end
popd
exit /B 1
