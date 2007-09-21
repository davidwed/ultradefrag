@echo off
set DDK_TYPE=Windows_Server2003_DDK
set COMPILER_TYPE=MSVC

if "%1" neq "" goto no_defaults
echo No parameters specified, use defaults.
goto first_step

:no_defaults
if "%1" equ "--clean" goto clean
if "%1" equ "--install" goto set_inst
if "%1" equ "--use-winddk" goto parse_second_parameter
if "%1" equ "--use-rosddk" goto rosddk
echo No --use-xxxddk parameter specified, use winddk.
goto parse_second_parameter
:rosddk
set DDK_TYPE=ReactOS_DDK

:parse_second_parameter
if "%2" equ "--use-msvc" goto parse_third_parameter
if "%2" equ "--use-gcc" goto gcc
echo Neither --use-msvc nor --use-gcc parameter specified, use msvc.
goto parse_third_parameter
:gcc
set COMPILER_TYPE=GCC

:parse_third_parameter
if "%3" neq "--install" goto first_step
:set_inst
set _INST_=1
:first_step
echo Set environment variables...
call SETVARS.CMD
echo #define VERSION %VERSION% > .\include\ultradfgver.h
echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.h
echo #define VERSIONINTITLE "UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define VERSIONINTITLE_U L"UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h
echo #define ABOUT_VERSION "About UltraDefrag v%ULTRADFGVER%" >> .\include\ultradfgver.h

if "%DDK_TYPE%" equ "Windows_Server2003_DDK" goto build_with_winddk
:build_with_rosddk
echo Build the driver...
cd .\driver
echo NT 5.0+ version...
set NT4_TARGET=false
nmake /NOLOGO /A /f ultradfg.mak
if %errorlevel% neq 0 goto fail
echo NT 4.0 version...
set NT4_TARGET=true
nmake /NOLOGO /A /f ultradfg.mak
if %errorlevel% neq 0 goto fail

echo Build the console interface...
cd ..\console
nmake /NOLOGO /A /f defrag.mak CFG="Defrag - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
nmake /NOLOGO /A /f dfrg.mak CFG="Dfrg - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native
echo NT 5.0+ version...
set NT4_TARGET=false
nmake /NOLOGO /A /f defrag_native.mak CFG="RosDDK"
if %errorlevel% neq 0 goto fail
echo NT 4.0 version...
set NT4_TARGET=true
nmake /NOLOGO /A /f defrag_native.mak CFG="RosDDK"
if %errorlevel% neq 0 goto fail

echo Build the installation helper...
cd ..\inst_helper
nmake /NOLOGO /A /f inst_helper.mak
if %errorlevel% neq 0 goto fail

goto build_installer

:build_with_winddk
echo Copy sources to obj directory...
mkdir .\obj
xcopy /I /Y .\driver .\obj\driver
xcopy /I /Y .\console .\obj\console
xcopy /I /Y .\gui .\obj\gui
xcopy /I /Y .\gui\res .\obj\gui\res
xcopy /I /Y .\native .\obj\native
xcopy /I /Y .\include .\obj\include
xcopy /I /Y .\inst_helper .\obj\inst_helper
xcopy /I /Y .\x64_inst .\obj\x64_inst
copy /Y .\shared\misc.c .\obj\console\
copy /Y .\shared\misc.c .\obj\gui\
copy /Y .\shared\misc.c .\obj\native\
copy /Y .\blditems.cmd .\obj\
cd .\obj
echo --------- Target is x86 ---------
echo Because GUI produced by WINDDK has runtime errors,
echo we must build user interfaces using MSVC.
call "%MSVSBIN%\vcvars32.bat"

echo Build the console interface...
cd ..\console
nmake /NOLOGO /A /f defrag.mak CFG="Defrag - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the gui interface...
cd ..\gui
nmake /NOLOGO /A /f dfrg.mak CFG="Dfrg - Win32 Release"
if %errorlevel% neq 0 goto fail

echo Build the native interface...
cd ..\native
set DDKINCDIR=%WINDDKBASE%\inc\ddk\wnet
echo NT 5.0+ version...
set NT4_TARGET=false
nmake /NOLOGO /A /f defrag_native.mak CFG="WinDDK"
if %errorlevel% neq 0 goto fail
echo NT 4.0 version...
set NT4_TARGET=true
nmake /NOLOGO /A /f defrag_native.mak CFG="WinDDK"
if %errorlevel% neq 0 goto fail

echo Build the installation helper...
cd ..\inst_helper
nmake /NOLOGO /A /f inst_helper.mak
if %errorlevel% neq 0 goto fail

cd ..\obj
set AMD64=
set IA64=
call blditems.cmd
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin
rem blditems save driver versions to bin directory
REM copy .\console\objfre_wnet_x86\i386\udefrag.exe ..\bin\
REM copy .\gui\objfre_wnet_x86\i386\dfrg.exe ..\bin\
REM copy .\native\objfre_wnet_x86\i386\defrag_native.exe ..\bin\
REM copy .\inst_helper\objfre_wnet_x86\i386\inst_helper.exe ..\bin\
echo --------- Target is x64 ---------
set IA64=
call blditems.cmd AMD64
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin\amd64
copy .\driver\objfre_wnet_amd64\amd64\ultradfg.sys ..\bin\amd64\
copy .\console\objfre_wnet_amd64\amd64\udefrag.exe ..\bin\amd64\
copy .\gui\objfre_wnet_amd64\amd64\dfrg.exe ..\bin\amd64\
copy .\native\objfre_wnet_amd64\amd64\defrag_native.exe ..\bin\amd64\
copy .\inst_helper\objfre_wnet_amd64\amd64\inst_helper.exe ..\bin\amd64\
copy .\x64_inst\objfre_wnet_amd64\amd64\x64_inst.exe ..\bin\amd64\
echo --------- Target is ia64 ---------
set AMD64=
call blditems.cmd 64
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin\ia64
copy .\driver\objfre_wnet_ia64\ia64\ultradfg.sys ..\bin\ia64\
copy .\console\objfre_wnet_ia64\ia64\udefrag.exe ..\bin\ia64\
copy .\gui\objfre_wnet_ia64\ia64\dfrg.exe ..\bin\ia64\
copy .\native\objfre_wnet_ia64\ia64\defrag_native.exe ..\bin\ia64\
copy .\inst_helper\objfre_wnet_ia64\ia64\inst_helper.exe ..\bin\ia64\
copy .\x64_inst\objfre_wnet_ia64\ia64\x64_inst.exe ..\bin\ia64\

:build_installer
echo Build installer...
cd ..\bin
copy /Y ..\UltraDefrag.nsi .\
copy /Y ..\*.TXT .\
xcopy /I /Y ..\presets .\presets
upx udefrag.exe
if %errorlevel% neq 0 goto fail
upx dfrg.exe
if %errorlevel% neq 0 goto fail
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH i386 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail
REM ren ultradefrag-1.0.0.bin.i386.exe ultradefrag-%ULTRADFGVER%.bin.i386.exe

if "%DDK_TYPE%" equ "ReactOS_DDK" goto build_source_package
copy /Y .\UltraDefrag.nsi .\amd64\
copy /Y .\*.TXT .\amd64\
xcopy /I /Y .\presets .\amd64\presets
cd amd64
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH amd64 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
copy /Y .\UltraDefrag.nsi .\ia64\
copy /Y .\*.TXT .\ia64\
xcopy /I /Y .\presets .\ia64\presets
cd ia64
echo !define ULTRADFGVER %ULTRADFGVER% > ultradefrag_globals.nsh
echo !define ULTRADFGARCH ia64 >> ultradefrag_globals.nsh
%NSISDIR%\makensis.exe UltraDefrag.nsi
if %errorlevel% neq 0 goto fail

cd..
:build_source_package
echo Build source code package...
cd ..
rmdir /s /q .\rel_src
mkdir .\rel_src
xcopy /I /Y .\driver .\rel_src\driver
xcopy /I /Y .\console .\rel_src\console
xcopy /I /Y .\gui .\rel_src\gui
xcopy /I /Y .\gui\res .\rel_src\gui\res
xcopy /I /Y .\native .\rel_src\native
xcopy /I /Y .\inst_helper .\rel_src\inst_helper
xcopy /I /Y .\x64_inst .\rel_src\x64_inst
xcopy /I /Y .\include .\rel_src\include
xcopy /I /Y .\shared .\rel_src\shared
xcopy /I /Y .\presets .\rel_src\presets

copy .\*.* .\rel_src\
cd .\rel_src
del /s /q *.ncb,*.opt,*.plg,*.aps
zip -r -m ultradefrag-%ULTRADFGVER%.src.zip .
cd ..
echo Build success!

if "%_INST_%" neq "1" goto end
echo Start installer...
.\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe /S
if %errorlevel% neq 0 goto fail_inst
echo Install success!
goto end
echo Install error!

:end
set _INST_=
exit /B

:fail
echo Build error (code %ERRORLEVEL%)!
set _INST_=
rem pause
exit /B

:clean
echo Delete all intermediate files...
cd bin
rd /s /q amd64
rd /s /q ia64
del /s /q *.*
cd ..\obj
rd /s /q console
rd /s /q driver
rd /s /q gui
rd /s /q include
rd /s /q native
rd /s /q x64_inst
del /s /q *.*
cd ..
echo Done.
