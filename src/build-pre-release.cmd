@echo off

rem This script was made for myself (Stefan Pendl)
rem to simplify pre-release (RC) building.

echo.
set /P ULTRADFGRC="Enter RC Version number [1]: "

if "%ULTRADFGRC%" == "" set ULTRADFGRC=1

set script_dir=%~dp0

echo.
call build.cmd --use-winddk
if %errorlevel% neq 0 goto build_failed

cd /D %script_dir%

mkdir pre-release
echo.
copy /b /y /v .\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe        .\pre-release\ultradefrag-%ULTRADFGVER%-RC%ULTRADFGRC%.bin.i386.exe
copy /b /y /v .\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe .\pre-release\ultradefrag-%ULTRADFGVER%-RC%ULTRADFGRC%.bin.amd64.exe
copy /b /y /v .\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe   .\pre-release\ultradefrag-%ULTRADFGVER%-RC%ULTRADFGRC%.bin.ia64.exe

cd ..
echo.
echo RC%ULTRADFGRC% Pre-Release created successfully!
exit /B 0

:build_failed
echo.
echo Pre-Release building error!
exit /B 1
