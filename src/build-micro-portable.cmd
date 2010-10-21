@echo off

::
:: This script produces the UltraDefrag Portable - Micro Edition package.
::

call ParseCommandLine.cmd %*

if %UD_BLD_FLG_DIPLAY_HELP% equ 1 (
	call build-help.cmd "%~n0"
	exit /B 0
)

call build-micro.cmd %* --portable || exit /B 1
if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 goto :EOF

if %UD_BLD_FLG_BUILD_X86% neq 0 (
	call :build_portable_package .\bin i386 || goto fail
)
if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
	call :build_portable_package .\bin\amd64 amd64 || goto fail
)
if %UD_BLD_FLG_BUILD_IA64% neq 0 (
	call :build_portable_package .\bin\ia64 ia64 || goto fail
)
echo.
echo Build success!
exit /B 0

:fail
echo.
echo Build error (code %ERRORLEVEL%)!
exit /B 1

rem Synopsis: call :build_portable_package {path to binaries} {arch}
rem Example:  call :build_portable_package .\bin\ia64 ia64
:build_portable_package
	pushd %1
	set PORTABLE_DIR=ultradefrag-micro-portable-%ULTRADFGVER%.%2
	mkdir %PORTABLE_DIR%
	copy /Y %~dp0\CREDITS.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\HISTORY.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\LICENSE.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\README.TXT  %PORTABLE_DIR%\
	copy /Y hibernate.exe     %PORTABLE_DIR%\hibernate4win.exe
	copy /Y udefrag.dll       %PORTABLE_DIR%\
	copy /Y udefrag.exe       %PORTABLE_DIR%\
	copy /Y zenwinx.dll       %PORTABLE_DIR%\
	rem zip -r -m -9 -X ultradefrag-micro-portable-%ULTRADFGVER%.bin.%2.zip %PORTABLE_DIR%
	"%SEVENZIP_PATH%\7z.exe" a -r -mx9 -tzip ultradefrag-micro-portable-%ULTRADFGVER%.bin.%2.zip %PORTABLE_DIR%
	if %errorlevel% neq 0 (
		rd /s /q %PORTABLE_DIR%
		set PORTABLE_DIR=
		popd
		exit /B 1
	)
	rd /s /q %PORTABLE_DIR%
	set PORTABLE_DIR=
	popd
exit /B 0
