@echo off

cd dll\zenwinx
if %errorlevel% neq 0 (
	exit /B 1
)

call mingw_patch.cmd %1
if %errorlevel% neq 0 (
	cd ..\..
	exit /B 1
)
cd ..\..
exit /B 0
