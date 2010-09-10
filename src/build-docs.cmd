@echo off

echo Build UltraDefrag development docs...
echo.

rem Set environment variables if they aren't already set.
if "%ULTRADFGVER%" neq "" goto build_docs
call SETVARS.CMD
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

:build_docs
rd /s /q doxy-doc
mkdir doxy-doc

cd dll\udefrag
rd /s /q doxy-doc
lua ..\..\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 cd ..\.. && exit /B 1
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag.dll\html
cd ..\..

cd dll\wgx
rd /s /q doxy-doc
lua ..\..\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 cd ..\.. && exit /B 1
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\wgx\html
cd ..\..

cd dll\zenwinx
rd /s /q doxy-doc
lua ..\..\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 cd ..\.. && exit /B 1
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
rem xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\zenwinx\html
cd ..\..

cd dll\udefrag-kernel
rd /s /q doxy-doc
lua ..\..\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 cd ..\.. && exit /B 1
doxygen
if %errorlevel% neq 0 cd ..\.. && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
xcopy /I /Y /Q /S .\doxy-doc\html ..\..\doxy-doc\udefrag-kernel\html
cd ..\..

cd ..\doc\html\handbook
rd /s /q doxy-doc
lua ..\..\..\src\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 cd ..\..\..\src && exit /B 1
doxygen
if %errorlevel% neq 0 cd ..\..\..\src && exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
echo optimize the handbook...
del /Q .\doxy-doc\html\pages.html
del /Q .\doxy-doc\html\doxygen.png
del /Q .\doxy-doc\html\tabs.css
del /Q .\doxy-doc\html\tab_*.png
del /Q .\doxy-doc\html\bc_*.png
del /Q .\doxy-doc\html\nav_*.png
del /Q .\doxy-doc\html\open.png
del /Q .\doxy-doc\html\closed.png
cd ..\..\..\src

rem finally build the main docs
lua .\tools\set-doxyfile-project-number.lua Doxyfile %ULTRADFGVER%
if %errorlevel% neq 0 exit /B 1
doxygen
if %errorlevel% neq 0 exit /B 1
copy /Y .\rsc\*.* .\doxy-doc\html\
del /Q .\doxy-doc\html\header.html,.\doxy-doc\html\footer.html
del /Q .\doxy-doc\html\*.dox

exit /B 0
