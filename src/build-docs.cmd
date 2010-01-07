@echo off
echo Build UltraDefrag development docs...
echo.

mkdir .\doxy-doc
copy /Y .\rsc\*.* .\doxy-doc\html\
doxygen
if %errorlevel% neq 0 exit /B 1

cd driver
mkdir .\doxy-doc
copy /Y .\rsc\*.* .\doxy-doc\html\
doxygen
if %errorlevel% neq 0 cd .. && exit /B 1
xcopy /I /Y /Q /S .\doxy-doc\html ..\doxy-doc\driver\html
cd ..

cd dll\udefrag
mkdir .\doxy-doc
copy /Y .\rsc\*.* .\doxy-doc\html\
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag.dll\html
cd ..\..

cd dll\zenwinx
mkdir .\doxy-doc
copy /Y .\rsc\*.* .\doxy-doc\html\
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
rem xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\zenwinx\html
cd ..\..

exit /B 0
