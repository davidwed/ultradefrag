@echo off

rem This batch file use Windows Server 2003 DDK.

rem set environment
if "%DDK_ENV%" neq "" goto start_build
rem ...
set DDK_ENV=ok

:start_build
echo Build the zenwinx.dll ...
cd .\dll\zenwinx\src
call DDK_BUILD_ZENWINX.CMD
if %errorlevel% neq 0 goto fail
rem copy files
mkdir ..\..\..\obj\dll\zenwinx
mkdir ..\..\..\obj\dll\zenwinx\src
copy /Y ntndk.h ..\..\..\obj\dll\zenwinx\src\
copy /Y zenwinx.h ..\..\..\obj\dll\zenwinx\src\
copy /Y .\objfre_wnet_x86\i386\zenwinx.lib ..\..\..\lib\
copy /Y .\objfre_wnet_amd64\amd64\zenwinx.lib ..\..\..\lib\amd64\
copy /Y .\objfre_wnet_ia64\ia64\zenwinx.lib ..\..\..\lib\ia64\
copy /Y .\objfre_wnet_x86\i386\zenwinx.dll ..\..\..\bin\
copy /Y .\objfre_wnet_amd64\amd64\zenwinx.dll ..\..\..\bin\amd64\
copy /Y .\objfre_wnet_ia64\ia64\zenwinx.dll ..\..\..\bin\ia64\
cd ..\..\..
echo Copy sources to obj directory...
mkdir .\obj
xcopy /I /Y .\driver .\obj\driver
xcopy /I /Y .\console .\obj\console
xcopy /I /Y .\gui .\obj\gui
xcopy /I /Y .\gui\res .\obj\gui\res
xcopy /I /Y .\native .\obj\native
xcopy /I /Y .\include .\obj\include
mkdir .\obj\dll
xcopy /I /Y .\dll\udefrag .\obj\dll\udefrag.dll
copy /Y .\blditems.cmd .\obj\
cd .\obj
echo --------- Target is x86 ---------
set AMD64=
set IA64=
set UDEFRAG_LIB_PATH=..\..\lib
call blditems.cmd
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin
rem blditems save driver versions to bin directory
copy .\console\objfre_wnet_x86\i386\udefrag.exe ..\bin\
copy .\gui\objfre_wnet_x86\i386\dfrg.exe ..\bin\
copy .\native\objfre_wnet_x86\i386\defrag_native.exe ..\bin\
copy .\dll\udefrag.dll\objfre_wnet_x86\i386\udefrag.dll ..\bin\
echo --------- Target is x64 ---------
set IA64=
set UDEFRAG_LIB_PATH=..\..\lib\amd64
call blditems.cmd AMD64
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin\amd64
copy .\driver\objfre_wnet_amd64\amd64\ultradfg.sys ..\bin\amd64\
copy .\console\objfre_wnet_amd64\amd64\udefrag.exe ..\bin\amd64\
copy .\gui\objfre_wnet_amd64\amd64\dfrg.exe ..\bin\amd64\
copy .\native\objfre_wnet_amd64\amd64\defrag_native.exe ..\bin\amd64\
copy .\dll\udefrag.dll\objfre_wnet_amd64\amd64\udefrag.dll ..\bin\amd64\
echo --------- Target is ia64 ---------
set AMD64=
set UDEFRAG_LIB_PATH=..\..\lib\ia64
call blditems.cmd 64
if %errorlevel% neq 0 goto fail
cd ..
mkdir ..\bin\ia64
copy .\driver\objfre_wnet_ia64\ia64\ultradfg.sys ..\bin\ia64\
copy .\console\objfre_wnet_ia64\ia64\udefrag.exe ..\bin\ia64\
copy .\gui\objfre_wnet_ia64\ia64\dfrg.exe ..\bin\ia64\
copy .\native\objfre_wnet_ia64\ia64\defrag_native.exe ..\bin\ia64\
copy .\dll\udefrag.dll\objfre_wnet_ia64\ia64\udefrag.dll ..\bin\ia64\

exit /B 0

:fail
exit /B 1
