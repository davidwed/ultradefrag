@echo off
::
:: Build script for the UltraDefrag project.
::

call ParseCommandLine.cmd %*

:: handle help request
if %UD_BLD_FLG_DIPLAY_HELP% equ 1 (
	call :usage
	exit /B 0
)

:: delete all previously compiled files
call :cleanup
if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 exit /B 0

:: set environment
call :set_build_environment
echo %ULTRADFGVER% > ..\doc\html\version.ini

:: build all binaries
call build-targets.cmd %* || goto fail

:: build documentation
if "%UD_MICRO_EDITION%" equ "" (
	call build-docs.cmd || goto fail
)

:: build installers and portable packages
echo Build installers and/or portable packages...
if %UD_BLD_FLG_BUILD_X86% neq 0 (
	call :build_installer              .\bin i386 || goto fail
	call :build_micro_installer        .\bin i386 || goto fail
	call :build_portable_package       .\bin i386 || goto fail
	call :build_micro_portable_package .\bin i386 || goto fail
)
if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
	call :build_installer              .\bin\amd64 amd64 || goto fail
	call :build_micro_installer        .\bin\amd64 amd64 || goto fail
	call :build_portable_package       .\bin\amd64 amd64 || goto fail
	call :build_micro_portable_package .\bin\amd64 amd64 || goto fail
)
if %UD_BLD_FLG_BUILD_IA64% neq 0 (
	call :build_installer              .\bin\ia64 ia64 || goto fail
	call :build_micro_installer        .\bin\ia64 ia64 || goto fail
	call :build_portable_package       .\bin\ia64 ia64 || goto fail
	call :build_micro_portable_package .\bin\ia64 ia64 || goto fail
)
echo.
echo Build success!

:: install the program if requested
if %UD_BLD_FLG_DO_INSTALL% neq 0 (
	call :install || exit /B 1
)

:: all operations successfully completed!
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
exit /B 1

:: --------------------------------------------------------
::                      subroutines
:: --------------------------------------------------------

rem Sets environment for the build process.
:set_build_environment
	call setvars.cmd
	if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
	if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

	if %UD_BLD_FLG_IS_PORTABLE% equ 1 (
		set UDEFRAG_PORTABLE=1
	) else (
		set UDEFRAG_PORTABLE=
	)

	if %UD_BLD_FLG_IS_MICRO% equ 1 (
		set UD_MICRO_EDITION=1
	) else (
		set UD_MICRO_EDITION=
	)
	
	if %UD_BLD_FLG_BUILD_ALL% equ 1 (
		set UDEFRAG_PORTABLE=
		set UD_MICRO_EDITION=
	)

	echo #define VERSION %VERSION% > .\include\ultradfgver.h
	echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
	echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%" >> .\include\ultradfgver.h
	echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable" >> .\include\ultradfgver.h
	echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.h

	rem force zenwinx version to be the same as ultradefrag version
	echo #define ZENWINX_VERSION %VERSION% > .\dll\zenwinx\zenwinxver.h
	echo #define ZENWINX_VERSION2 %VERSION2% >> .\dll\zenwinx\zenwinxver.h
goto :EOF

rem Synopsis: call :build_installer {path to binaries} {arch}
rem Example:  call :build_installer .\bin\ia64 ia64
:build_installer
	if %UD_BLD_FLG_BUILD_ALL% neq 1 (
		if "%UDEFRAG_PORTABLE%" neq "" exit /B 0
		if "%UD_MICRO_EDITION%" neq "" exit /B 0
	)

	pushd %1
	copy /Y %~dp0\installer\UltraDefrag.nsi .\
	copy /Y %~dp0\installer\lang.ini .\
	copy /Y %~dp0\installer\lang-classical.ini .\
	"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=%2 UltraDefrag.nsi
	if %errorlevel% neq 0 (
		popd
		exit /B 1
	)
	popd
exit /B 0

rem Synopsis: call :build_micro_installer {path to binaries} {arch}
rem Example:  call :build_micro_installer .\bin\ia64 ia64
:build_micro_installer
	if %UD_BLD_FLG_BUILD_ALL% neq 1 (
		if "%UDEFRAG_PORTABLE%" neq "" exit /B 0
		if "%UD_MICRO_EDITION%" equ "" exit /B 0
	)

	pushd %1
	copy /Y %~dp0\installer\MicroEdition.nsi .\
	"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=%2 MicroEdition.nsi
	if %errorlevel% neq 0 (
		popd
		exit /B 1
	)
	popd
exit /B 0

rem Synopsis: call :build_portable_package {path to binaries} {arch}
rem Example:  call :build_portable_package .\bin\ia64 ia64
:build_portable_package
	if %UD_BLD_FLG_BUILD_ALL% neq 1 (
		if "%UDEFRAG_PORTABLE%" equ "" exit /B 0
		if "%UD_MICRO_EDITION%" neq "" exit /B 0
	)
	
	pushd %1
	set PORTABLE_DIR=ultradefrag-portable-%ULTRADFGVER%.%2
	mkdir %PORTABLE_DIR%
	copy /Y %~dp0\CREDITS.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\HISTORY.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\LICENSE.TXT %PORTABLE_DIR%\
	copy /Y %~dp0\README.TXT  %PORTABLE_DIR%\
	copy /Y hibernate.exe     %PORTABLE_DIR%\hibernate4win.exe
	copy /Y udefrag.dll       %PORTABLE_DIR%\
	copy /Y udefrag.exe       %PORTABLE_DIR%\
	copy /Y zenwinx.dll       %PORTABLE_DIR%\
	copy /Y ultradefrag.exe   %PORTABLE_DIR%
	copy /Y lua5.1a.dll       %PORTABLE_DIR%\
	copy /Y lua5.1a.exe       %PORTABLE_DIR%\
	copy /Y lua5.1a_gui.exe   %PORTABLE_DIR%\
	copy /Y wgx.dll           %PORTABLE_DIR%\
	mkdir %PORTABLE_DIR%\handbook
	copy /Y %~dp0\..\doc\html\handbook\doxy-doc\html\*.* %PORTABLE_DIR%\handbook\
	mkdir %PORTABLE_DIR%\scripts
	copy /Y %~dp0\scripts\udreportcnv.lua  %PORTABLE_DIR%\scripts\
	copy /Y %~dp0\scripts\udsorting.js     %PORTABLE_DIR%\scripts\
	copy /Y %~dp0\scripts\udreport.css     %PORTABLE_DIR%\scripts\
	mkdir %PORTABLE_DIR%\options
	copy /Y %~dp0\scripts\udreportopts.lua %PORTABLE_DIR%\options\
	mkdir %PORTABLE_DIR%\i18n
	copy /Y %~dp0\gui\i18n\*.lng           %PORTABLE_DIR%\i18n\
	copy /Y %~dp0\gui\i18n\*.template      %PORTABLE_DIR%\i18n\
	rem zip -r -m -9 -X ultradefrag-portable-%ULTRADFGVER%.bin.%2.zip %PORTABLE_DIR%
	"%SEVENZIP_PATH%\7z.exe" a -r -mx9 -tzip ultradefrag-portable-%ULTRADFGVER%.bin.%2.zip %PORTABLE_DIR%
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

rem Synopsis: call :build_micro_portable_package {path to binaries} {arch}
rem Example:  call :build_micro_portable_package .\bin\ia64 ia64
:build_micro_portable_package
	if %UD_BLD_FLG_BUILD_ALL% neq 1 (
		if "%UDEFRAG_PORTABLE%" equ "" exit /B 0
		if "%UD_MICRO_EDITION%" equ "" exit /B 0
	)
	
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

rem Installs the program.
:install
	if "%UDEFRAG_PORTABLE%" neq "" exit /B 0

	setlocal
	if /i %PROCESSOR_ARCHITECTURE% == x86 (
		set INSTALLER_PATH=.\bin
		set INSTALLER_ARCH=i386
	)
	if /i %PROCESSOR_ARCHITECTURE% == AMD64 (
		set INSTALLER_PATH=.\bin\amd64
		set INSTALLER_ARCH=amd64
	)
	if /i %PROCESSOR_ARCHITECTURE% == IA64 (
		set INSTALLER_PATH=.\bin\ia64
		set INSTALLER_ARCH=ia64
	)
	if "%UD_MICRO_EDITION%" equ "" (
		set INSTALLER_NAME=ultradefrag
	) else (
		set INSTALLER_NAME=ultradefrag-micro-edition
	)

	echo Start installer...
	%INSTALLER_PATH%\%INSTALLER_NAME%-%ULTRADFGVER%.bin.%INSTALLER_ARCH%.exe /S
	if %errorlevel% neq 0 (
		echo Install error!
		endlocal
		exit /B 1
	)
	echo Install success!
	endlocal
exit /B 0

rem Cleans up sources directory
rem by removing all intermediate files.
:cleanup
	echo Delete all intermediate files...
	rd /s /q bin
	rd /s /q lib
	rd /s /q obj
	rd /s /q doxy-doc
	rd /s /q gui\doxy-doc
	rd /s /q dll\wgx\doxy-doc
	rd /s /q dll\udefrag\doxy-doc
	rd /s /q dll\zenwinx\doxy-doc
	rd /s /q ..\doc\html\handbook\doxy-doc
	rd /s /q src_package
	rd /s /q ..\src_package
    if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 (
        rd /s /q pre-release
        rd /s /q release
    )
	echo Done.
goto :EOF

rem Displays usage information.
:usage
	echo Usage: build [options]
	echo.
	echo Common options:
	echo --micro         build the micro edition
	echo --portable      build portable packages instead of installers
	echo --all           build all packages: regular, micro and portable
	echo --install       perform silent installation after the build
	echo --clean         perform full cleanup instead of the build
	echo.
	echo Compiler:
	echo --use-mingw     (default)
	echo --use-winddk    (we use it for official releases)
	echo --use-winsdk
	echo --use-msvc      (the fastest compilation)
	echo --use-pellesc   (experimental, produces wrong code)
	echo --use-mingw-x64 (experimental, produces wrong x64 code)
	echo.
	echo Target architecture:
	echo --no-x86        skip build of 32-bit binaries
	echo --no-amd64      skip build of x64 binaries
	echo --no-ia64       skip build of IA-64 binaries
	echo.
	echo Without parameters the build command uses MinGW to build
	echo a 32-bit regular installer.
	echo.
	echo * To use MinGW run mingw_patch.cmd before:
	echo dll\zenwinx\mingw_patch.cmd {path to mingw installation}
	echo.
	echo * To use MS Visual Studio 6.0 run msvc_patch.cmd before:
	echo dll\zenwinx\msvc_patch.cmd {path to msvs6 installation}
goto :EOF
