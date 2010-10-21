@echo off
::
:: Build script for the UltraDefrag project.
::

call ParseCommandLine.cmd %*

if %UD_BLD_FLG_DIPLAY_HELP% equ 1 (
	call build-help.cmd "%~n0"
	exit /B 0
)

:: set environment variables
set OLD_PATH=%path%
call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

:: save current directory
:: it will be restored on end/fail labels
pushd .

:: delete all previously compiled files
call cleanup.cmd %*
if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 goto end

if %UD_BLD_FLG_IS_PORTABLE% equ 1 (
	set UDEFRAG_PORTABLE=1
) else (
	set UDEFRAG_PORTABLE=
)

echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.h

echo %ULTRADFGVER% > ..\doc\html\version.ini

:: force zenwinx version to be the same as ultradefrag version
echo #define ZENWINX_VERSION %VERSION% > .\dll\zenwinx\zenwinxver.h
echo #define ZENWINX_VERSION2 %VERSION2% >> .\dll\zenwinx\zenwinxver.h

:: build all binaries
call build-targets.cmd %* || goto fail

:: build documentation
call build-docs.cmd || goto fail

:: in case of portable package there is nothing more to build
if %UD_BLD_FLG_IS_PORTABLE% equ 1 goto end

:: build installers
echo Build installers...
if %UD_BLD_FLG_BUILD_X86% neq 0 (
	cd .\bin
	copy /Y ..\installer\UltraDefrag.nsi .\
	copy /Y ..\installer\lang.ini .\
	copy /Y ..\installer\lang-classical.ini .\
	"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=i386 UltraDefrag.nsi || goto fail
	cd ..
)
if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
	cd .\bin\amd64
	copy /Y ..\..\installer\UltraDefrag.nsi .\
	copy /Y ..\..\installer\lang.ini .\
	copy /Y ..\..\installer\lang-classical.ini .\
	"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=amd64 UltraDefrag.nsi || goto fail
	cd ..\..
)
if %UD_BLD_FLG_BUILD_IA64% neq 0 (
	cd .\bin\ia64
	copy /Y ..\..\installer\UltraDefrag.nsi .\
	copy /Y ..\..\installer\lang.ini .\
	copy /Y ..\..\installer\lang-classical.ini .\
	"%NSISDIR%\makensis.exe" /DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=ia64 UltraDefrag.nsi || goto fail
	cd ..\..
)
echo.
echo Build success!

:: install the program if requested
if %UD_BLD_FLG_DO_INSTALL% equ 0 goto end

echo Start installer...
if %PROCESSOR_ARCHITECTURE% == x86 (
	.\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe /S || goto install_fail
	goto install_success
)
if %PROCESSOR_ARCHITECTURE% == AMD64 (
	.\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe /S || goto install_fail
	goto install_success
)
if %PROCESSOR_ARCHITECTURE% == IA64 (
	.\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe /S || goto install_fail
	goto install_success
)

:install_success
echo Install success!
goto end

:install_fail
echo Install error!
goto end_fail

:fail
echo Build error (code %ERRORLEVEL%)!

:end_fail
set Path=%OLD_PATH%
set OLD_PATH=
popd
exit /B 1

:end
set Path=%OLD_PATH%
set OLD_PATH=
popd
exit /B 0
