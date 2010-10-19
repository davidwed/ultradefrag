@echo off

echo This script will replace some libraries in your MinGW installation
echo with more adequate versions.
echo Usage: mingw_patch.cmd {path to mingw installation}
echo.

if "%1" equ "" (
	exit /B 0
)

echo Preparing to patch applying...
set path=%1\bin;%path%

:: save original versions
if not exist %1\lib\libntdll.a.orig (
	move %1\lib\libntdll.a %1\lib\libntdll.a.orig
)
if not exist %1\lib\libntoskrnl.a.orig (
	move %1\lib\libntoskrnl.a %1\lib\libntoskrnl.a.orig
)

:: generate more adequate versions
dlltool -k --output-lib libntdll.a --def ntdll.def
if %errorlevel% neq 0 goto fail
dlltool -k --output-lib libntoskrnl.a --def ntoskrnl.def
if %errorlevel% neq 0 goto fail

:: replace files
move /Y libntdll.a %1\lib\libntdll.a
move /Y libntoskrnl.a %1\lib\libntoskrnl.a

echo MinGW patched successfully!
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
exit /B 1
