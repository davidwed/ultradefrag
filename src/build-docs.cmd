@echo off
echo Build UltraDefrag development docs...
echo.

cd driver
doxygen
if %errorlevel% neq 0 cd .. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\doxy-doc\driver\html
cd ..

cd dll\udefrag
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag.dll\html
cd ..\..

cd dll\zenwinx
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
rem xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\zenwinx\html
cd ..\..

cd ..\doc\html\handbook
doxygen
if %errorlevel% neq 0 cd ..\..\..\src && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
cd ..\..\..\src

rem finally build the main docs
doxygen
if %errorlevel% neq 0 exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html

exit /B 0
