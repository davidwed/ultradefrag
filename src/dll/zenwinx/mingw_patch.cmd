@echo off

echo This script will replace the ntdll.a library
echo in MinGW installation with a more adequate version.
echo Usage: mingw_patch.cmd {path to mingw installation}
echo.

if "%1" equ "" (
	exit /B 0
)

echo Preparing to patch applying...
pushd %~dp0

:: save original versions
if not exist %1\lib\libntdll.a.orig (
	move %1\lib\libntdll.a %1\lib\libntdll.a.orig
)
::if not exist %1\lib\libntoskrnl.a.orig (
::	move %1\lib\libntoskrnl.a %1\lib\libntoskrnl.a.orig
::)

:: generate more adequate versions
%1\bin\dlltool -k --output-lib libntdll.a --def ntdll.def
if %errorlevel% neq 0 goto fail
::%1\bin\dlltool -k --output-lib libntoskrnl.a --def ntoskrnl.def
::if %errorlevel% neq 0 goto fail

:: replace files
move /Y libntdll.a %1\lib\libntdll.a
::move /Y libntoskrnl.a %1\lib\libntoskrnl.a

echo MinGW patched successfully!
popd
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
popd
exit /B 1
