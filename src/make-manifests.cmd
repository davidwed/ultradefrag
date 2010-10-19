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
	type manifest.part1 > %1
	echo version="%3" name="%4" processorArchitecture="%2" >> %1
	type manifest.part2 >> %1
	echo %~5 >> %1
	type manifest.part3 >> %1
	echo processorArchitecture="%2" >> %1
	type manifest.part4 >> %1
goto :EOF
