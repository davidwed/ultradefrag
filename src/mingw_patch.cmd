@echo off

cd dll\zenwinx\src
if %errorlevel% neq 0 goto fail

call mingw_patch.cmd %1
if %errorlevel% equ 0 goto the_end
cd ..\..\..
goto fail

:the_end
cd ..\..\..
exit /B 0

:fail
exit /B 1
