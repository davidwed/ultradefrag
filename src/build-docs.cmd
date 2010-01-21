@echo off
echo Build UltraDefrag development docs...
echo.

rd /s /q doxy-doc
mkdir doxy-doc

cd driver
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd .. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\doxy-doc\driver\html
cd ..

cd dll\udefrag
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag.dll\html
cd ..\..

cd dll\wgx
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\wgx\html
cd ..\..

cd dll\zenwinx
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
rem xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\zenwinx\html
cd ..\..

cd dll\udefrag-kernel
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag-kernel\html
cd ..\..

cd ..\doc\html\handbook
rd /s /q doxy-doc
doxygen
if %errorlevel% neq 0 cd ..\..\..\src && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
echo optimize the handbook...
del /Q .\doxy-doc\html\doxygen.png
del /Q .\doxy-doc\html\tabs.css
del /Q .\doxy-doc\html\tab_b.gif
del /Q .\doxy-doc\html\tab_l.gif
del /Q .\doxy-doc\html\tab_r.gif
del /Q .\doxy-doc\html\pages.html
cd ..\..\..\src

rem finally build the main docs
doxygen
if %errorlevel% neq 0 exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
del /Q .\doxy-doc\html\*.dox

exit /B 0
