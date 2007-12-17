@echo off

echo This script will replace some libraries in your MinGW installation
echo with more adequate versions.
echo Usage: mingw_patch.cmd {path to mingw installation}
echo.

if "%1" equ "" goto the_end
echo Preparing to patch applying...
set path=%path%;%1\bin
if exist %1\lib\libntdll.a.orig goto ntdll_saved
move %1\lib\libntdll.a %1\lib\libntdll.a.orig
:ntdll_saved
if exist %1\lib\libntoskrnl.a.orig goto ntoskrnl_saved
move %1\lib\libntoskrnl.a %1\lib\libntoskrnl.a.orig
:ntoskrnl_saved
dlltool -k --output-lib libntdll.a --def ntdll.def
if %errorlevel% neq 0 goto fail
dlltool -k --output-lib libntoskrnl.a --def ntoskrnl.def
if %errorlevel% neq 0 goto fail
move /Y libntdll.a %1\lib\libntdll.a
move /Y libntoskrnl.a %1\lib\libntoskrnl.a

echo MinGW patched successfully!
:the_end
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
exit /B 1
