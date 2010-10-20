@echo off

::
:: This script generates manifests for Vista UAC, 
:: suggested by Kerem Gumrukcu (http://entwicklung.junetz.de/).
::

if "%1" equ "" (
	echo Processor Architecture must be specified!
	exit /B 1
)

call :make_manifest .\obj\hibernate\hibernate.manifest    %1 1.0.0.0          hibernate    "Hibernate for Windows"
call :make_manifest .\obj\console\defrag.manifest         %1 %ULTRADFGVER%.0  udefrag      "UltraDefrag console interface"
call :make_manifest .\obj\gui\res\ultradefrag.manifest    %1 %ULTRADFGVER%.0  ultradefrag  "UltraDefrag GUI"
call :make_manifest .\obj\bootexctrl\bootexctrl.manifest  %1 %ULTRADFGVER%.0  bootexctrl   "BootExecute Control Program"
call :make_manifest .\obj\lua5.1\lua.manifest             %1 5.1.2.0          Lua          "Lua Console"

rem we have no need to compile it
rem call :make_manifest .\obj\utf8-16\utf8-16.manifest %1 0.0.0.1 utf8-16 "UTF-8 to UTF-16 converter"

rem update manifests in working copy of sources
if /i "%1" equ "X86" (
	copy /Y .\obj\hibernate\hibernate.manifest .\hibernate\hibernate.manifest
	copy /Y .\obj\console\defrag.manifest .\console\defrag.manifest
	copy /Y .\obj\gui\res\ultradefrag.manifest .\gui\res\ultradefrag.manifest
	copy /Y .\obj\bootexctrl\bootexctrl.manifest .\bootexctrl\bootexctrl.manifest
	copy /Y .\obj\lua5.1\lua.manifest .\lua5.1\lua.manifest
	rem copy /Y .\obj\utf8-16\utf8-16.manifest .\utf8-16\utf8-16.manifest
)
exit /B 0

rem Synopsis: call :make_manifest {path} {arch} {version} {app_name} {full_app_name}
rem Example:  call :make_manifest .\obj\app\app.manifest ia64 1.0.0.0 app "Application Name"
:make_manifest
	echo ^<?xml version="1.0" encoding="UTF-8" standalone="yes"?^>                                   > %1
	echo ^<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"^>                >> %1
	echo. ^<assemblyIdentity version="%3" name="%4" processorArchitecture="%2" type="win32"/^>      >> %1
	echo.  ^<description^>%~5^</description^>                                                       >> %1
	echo.  ^<dependency^>                                                                           >> %1
	echo.   ^<dependentAssembly^>                                                                   >> %1
	echo.    ^<assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls"               >> %1
	echo.      version="6.0.0.0" processorArchitecture="%2"                                         >> %1
	echo.      publicKeyToken="6595b64144ccf1df" language="*" /^>                                   >> %1
	echo.   ^</dependentAssembly^>                                                                  >> %1
	echo.  ^</dependency^>                                                                          >> %1
	echo.  ^<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2"^>                                   >> %1
	echo.   ^<security^>                                                                            >> %1
	echo.    ^<requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3"^>                       >> %1
	echo.     ^<requestedExecutionLevel level="requireAdministrator" uiAccess="false" /^>           >> %1
	echo.    ^</requestedPrivileges^>                                                               >> %1
	echo.   ^</security^>                                                                           >> %1
	echo.  ^</trustInfo^>                                                                           >> %1
	echo.  ^<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3"^>                     >> %1
	echo.   ^<asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings"^> >> %1
	echo.    ^<dpiAware^>true^</dpiAware^>                                                          >> %1
	echo.   ^</asmv3:windowsSettings^>                                                              >> %1
	echo.  ^</asmv3:application^>                                                                   >> %1
	echo ^</assembly^>                                                                              >> %1
	goto :EOF
	rem the following sequence is faster, but misty
	type manifest.part1 > %1
	echo version="%3" name="%4" processorArchitecture="%2" >> %1
	type manifest.part2 >> %1
	echo %~5 >> %1
	type manifest.part3 >> %1
	echo processorArchitecture="%2" >> %1
	type manifest.part4 >> %1
goto :EOF
