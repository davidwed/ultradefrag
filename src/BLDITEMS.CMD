@echo off
pushd obj\driver
rem perl ..\..\tools\mkmod.pl ultradfg.build
lua ..\..\tools\mkmod.lua ultradfg.build
if %errorlevel% neq 0 goto end
cd ..\zenwinx
rem perl ..\..\tools\mkmod.pl zenwinx.build
lua ..\..\tools\mkmod.lua zenwinx.build
if %errorlevel% neq 0 goto end
cd ..\udefrag
rem perl ..\..\tools\mkmod.pl udefrag.build
lua ..\..\tools\mkmod.lua udefrag.build
if %errorlevel% neq 0 goto end
cd ..\console
rem perl ..\..\tools\mkmod.pl defrag.build
lua ..\..\tools\mkmod.lua defrag.build
if %errorlevel% neq 0 goto end
cd ..\gui
rem perl ..\..\tools\mkmod.pl dfrg.build
lua ..\..\tools\mkmod.lua dfrg.build
if %errorlevel% neq 0 goto end
cd ..\native
rem perl ..\..\tools\mkmod.pl defrag_native.build
lua ..\..\tools\mkmod.lua defrag_native.build
if %errorlevel% neq 0 goto end
cd ..\lua5.1
rem perl ..\..\tools\mkmod.pl lua5.1_dll.build
lua ..\..\tools\mkmod.lua lua5.1a_dll.build
if %errorlevel% neq 0 goto end
rem perl ..\..\tools\mkmod.pl lua5.1_exe.build
lua ..\..\tools\mkmod.lua lua5.1a_exe.build
if %errorlevel% neq 0 goto end
rem perl ..\..\tools\mkmod.pl lua5.1a_gui.build
lua ..\..\tools\mkmod.lua lua5.1a_gui.build
if %errorlevel% neq 0 goto end

popd
exit /B 0

:end
popd
exit /B 1
