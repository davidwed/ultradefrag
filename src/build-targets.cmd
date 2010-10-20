@echo off

rem This script builds all binaries for UltraDefrag project
rem for all of the supported targets.
rem Usage:
rem     build-targets [<compiler>]
rem
rem Available <compiler> values:
rem     --use-mingw (default)
rem     --use-msvc
rem     --use-winddk
rem     --use-winsdk
rem     --use-pellesc (experimental)
rem     --use-mingw-x64 (experimental)
rem
rem Skip any processor architecture to reduce compile time
rem     --no-x86
rem     --no-amd64
rem     --no-ia64

rem NOTE: IA-64 targeting binaries were never tested by the authors 
rem due to missing appropriate hardware and appropriate 64-bit version 
rem of Windows.

call ParseCommandLine.cmd %*

if %UD_BLD_FLG_USE_COMPILER% equ 0 (
	echo No parameters specified, using defaults.
	goto mingw_build
)

if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINDDK%  goto ddk_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MSVC%    goto msvc_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW%   goto mingw_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_PELLESC% goto pellesc_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW64% goto mingw_x64_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINSDK%  goto winsdk_build


:ddk_build

	set BUILD_ENV=winddk

	if %UD_BLD_FLG_BUILD_X86% neq 0 (
		echo --------- Target is x86 ---------
		set AMD64=
		set IA64=
		pushd ..
		call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre WNET
		popd
		set BUILD_DEFAULT=-nmake -i -g -P
		set UDEFRAG_LIB_PATH=..\..\lib
		call :build_modules X86 || exit /B 1
		set Path=%OLD_PATH%
	)
	
	if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
		echo --------- Target is x64 ---------
		set IA64=
		pushd ..
		rem WDK 7 uses x64 instead of AMD64,
		rem so setenv.bat of WDK 7 must be changed to support AMD64 switch
		rem call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre x64 WNET
		call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre AMD64 WNET
		popd
		set BUILD_DEFAULT=-nmake -i -g -P
		set UDEFRAG_LIB_PATH=..\..\lib\amd64
		call :build_modules amd64 || exit /B 1
		set Path=%OLD_PATH%
	)
	
	if %UD_BLD_FLG_BUILD_IA64% neq 0 (
		echo --------- Target is ia64 ---------
		set AMD64=
		pushd ..
		call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre 64 WNET
		popd
		set BUILD_DEFAULT=-nmake -i -g -P
		set UDEFRAG_LIB_PATH=..\..\lib\ia64
		call :build_modules ia64 || exit /B 1
		set Path=%OLD_PATH%
	)
	
exit /B 0


:winsdk_build

	set BUILD_ENV=winsdk

	if %UD_BLD_FLG_BUILD_X86% neq 0 (
		echo --------- Target is x86 ---------
		set AMD64=
		set IA64=
		pushd ..
		call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x86 /xp
		popd
		set UDEFRAG_LIB_PATH=..\..\lib
		call :build_modules X86 || exit /B 1
		set Path=%OLD_PATH%
	)

	if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
		echo --------- Target is x64 ---------
		set IA64=
		set AMD64=1
		pushd ..
		call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
		popd
		set UDEFRAG_LIB_PATH=..\..\lib\amd64
		call :build_modules amd64 || exit /B 1
		set Path=%OLD_PATH%
	)

	if %UD_BLD_FLG_BUILD_IA64% neq 0 (
		echo --------- Target is ia64 ---------
		set AMD64=
		set IA64=1
		pushd ..
		call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /ia64 /xp
		popd
		set BUILD_DEFAULT=-nmake -i -g -P
		set UDEFRAG_LIB_PATH=..\..\lib\ia64
		call :build_modules ia64 || exit /B 1
		set Path=%OLD_PATH%
	)
	
exit /B 0


:pellesc_build

	set BUILD_ENV=pellesc

	rem x64 and arm targets aren't supported yet
	rem they require more investigation to be properly set

	echo --------- Target is x86 ---------
	set AMD64=
	set IA64=
	set ARM=
	set Path=%path%;%PELLESC_BASE%\Bin
	set UDEFRAG_LIB_PATH=..\..\lib
	call :build_modules X86 || exit /B 1
	set Path=%OLD_PATH%

exit /B 0


:msvc_build

	call "%MSVSBIN%\vcvars32.bat"
	set BUILD_ENV=msvc
	set UDEFRAG_LIB_PATH=..\..\lib
	call :build_modules X86 || exit /B 1
	set Path=%OLD_PATH%

exit /B 0


:mingw_x64_build

	echo --------- Target is x64 ---------
	set AMD64=1
	set path=%MINGWx64BASE%\bin;%path%
	set BUILD_ENV=mingw_x64
	set UDEFRAG_LIB_PATH=..\..\lib\amd64
	call :build_modules amd64 || exit /B 1
	set Path=%OLD_PATH%

exit /B 0


:mingw_build

	set path=%MINGWBASE%\bin;%path%
	set BUILD_ENV=mingw
	set UDEFRAG_LIB_PATH=..\..\lib
	call :build_modules X86 || exit /B 1
	set Path=%OLD_PATH%

exit /B 0


:: Builds all UltraDefrag modules
:: Example: call :build_modules X86
:build_modules

	rem update manifests
	call make-manifests.cmd %1 || exit /B 1

	rem rebuild modules
	echo Compile monolithic native interface...
	pushd obj\zenwinx
	lua ..\..\tools\mkmod.lua zenwinx.build static-lib || goto fail
	cd ..\udefrag
	lua ..\..\tools\mkmod.lua udefrag.build static-lib || goto fail
	cd ..\native
	lua ..\..\tools\mkmod.lua defrag_native.build || goto fail

	echo Compile native DLL's...
	cd ..\zenwinx
	lua ..\..\tools\mkmod.lua zenwinx.build || goto fail
	cd ..\udefrag
	lua ..\..\tools\mkmod.lua udefrag.build || goto fail

	echo Compile other modules...
	cd ..\bootexctrl
	lua ..\..\tools\mkmod.lua bootexctrl.build || goto fail
	cd ..\hibernate
	lua ..\..\tools\mkmod.lua hibernate.build || goto fail
	cd ..\console
	lua ..\..\tools\mkmod.lua defrag.build || goto fail

	rem Lua, WGX and GUI are missing in Micro Edition
	if "%UD_MICRO_EDITION%" equ "1" goto success

	rem workaround for WDK 7
	rem set IGNORE_LINKLIB_ABUSE=1
	cd ..\lua5.1
	lua ..\..\tools\mkmod.lua lua5.1a_dll.build || goto fail
	rem set IGNORE_LINKLIB_ABUSE=
	lua ..\..\tools\mkmod.lua lua5.1a_exe.build || goto fail
	lua ..\..\tools\mkmod.lua lua5.1a_gui.build || goto fail

	rem workaround for WDK 7
	rem set IGNORE_LINKLIB_ABUSE=1
	cd ..\wgx
	lua ..\..\tools\mkmod.lua wgx.build ||  goto fail
	rem set IGNORE_LINKLIB_ABUSE=
	cd ..\gui
	lua ..\..\tools\mkmod.lua ultradefrag.build && goto success

	:fail
	rem workaround for WDK 7
	rem set IGNORE_LINKLIB_ABUSE=
	popd
	exit /B 1
	
	:success
	popd

exit /B 0
