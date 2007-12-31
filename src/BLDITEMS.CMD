@echo off
pushd obj\driver
perl ..\..\tools\mkmod.pl ultradfg.build
if %errorlevel% neq 0 goto end
cd ..\zenwinx
perl ..\..\tools\mkmod.pl zenwinx.build
if %errorlevel% neq 0 goto end
cd ..\udefrag
perl ..\..\tools\mkmod.pl udefrag.build
if %errorlevel% neq 0 goto end
cd ..\console
perl ..\..\tools\mkmod.pl defrag.build
if %errorlevel% neq 0 goto end
cd ..\gui
perl ..\..\tools\mkmod.pl dfrg.build
if %errorlevel% neq 0 goto end
cd ..\native
perl ..\..\tools\mkmod.pl defrag_native.build
rem if %errorlevel% neq 0 goto end

:end
popd
