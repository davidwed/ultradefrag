@echo off
::
:: This script generates manifests for UltraDefrag binaries.
:: Copyright (c) 2009-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
::
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program; if not, write to the Free Software
:: Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::

:: Suggested by Kerem Gumrukcu (http://entwicklung.junetz.de/).

if "%1" equ "" (
    echo Processor architecture must be specified!
    exit /B 1
)

call :make_manifest %1 %ULTRADFGVER%.0  bootexctrl     "BootExecute Control Program"   > bootexctrl\bootexctrl.manifest
call :make_manifest %1 1.0.2.0          hibernate      "Hibernate for Windows"         > hibernate\hibernate.manifest
call :make_manifest %1 5.1.2.0          Lua            "Lua Console"                   > lua\lua.manifest
call :make_manifest %1 5.1.2.0          Lua            "Lua GUI"                       > lua-gui\lua.manifest
call :make_manifest %1 %ULTRADFGVER%.0  udefrag        "UltraDefrag console interface" > console\console.manifest
call :make_manifest %1 %ULTRADFGVER%.0  udefrag-dbg    "UltraDefrag debugger"          > dbg\dbg.manifest
call :make_manifest %1 %ULTRADFGVER%.0  ultradefrag    "UltraDefrag GUI"               > wxgui\res\ultradefrag.manifest

exit /B 0

rem Synopsis: call :make_manifest {arch} {version} {app_name} {full_app_name}
rem Example:  call :make_manifest ia64 1.0.0.0 app "Application Name"
:make_manifest
    echo ^<?xml version="1.0" encoding="UTF-8" standalone="yes"?^>
    echo ^<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"^>
    echo  ^<assemblyIdentity version="%2" name="%3" processorArchitecture="%1" type="win32"/^>
    echo   ^<description^>%~4^</description^>
    echo   ^<dependency^>
    echo    ^<dependentAssembly^>
    echo     ^<assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls"
    echo       version="6.0.0.0" processorArchitecture="%1"
    echo       publicKeyToken="6595b64144ccf1df" language="*" /^>
    echo    ^</dependentAssembly^>
    echo   ^</dependency^>
    echo   ^<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2"^>
    echo    ^<security^>
    echo     ^<requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3"^>
    echo      ^<requestedExecutionLevel level="requireAdministrator" uiAccess="false" /^>
    echo     ^</requestedPrivileges^>
    echo    ^</security^>
    echo   ^</trustInfo^>
    echo   ^<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3"^>
    echo    ^<asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings"^>
    echo     ^<dpiAware^>true^</dpiAware^>
    echo    ^</asmv3:windowsSettings^>
    echo   ^</asmv3:application^>
    echo   ^<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1"^>
    echo    ^<application^>
    echo     ^<!-- Windows 10 --^>
    echo     ^<supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/^>
    echo     ^<!-- Windows 8.1 --^>
    echo     ^<supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/^>
    echo     ^<!-- Windows Vista --^>
    echo     ^<supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/^>
    echo     ^<!-- Windows 7 --^>
    echo     ^<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/^>
    echo     ^<!-- Windows 8 --^>
    echo     ^<supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/^>
    echo    ^</application^>
    echo   ^</compatibility^>
    echo ^</assembly^>
goto :EOF
