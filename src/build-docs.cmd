@echo off

echo Build UltraDefrag development docs...
echo.

:: set environment variables if they aren't already set
if "%ULTRADFGVER%" equ "" (
	call setvars.cmd
	if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
	if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

call :compile_docs .                                 || goto fail
call :compile_docs .\dll\udefrag        udefrag.dll  || goto fail
call :compile_docs .\dll\wgx            wgx          || goto fail
call :compile_docs .\dll\zenwinx                     || goto fail
call :compile_docs ..\doc\html\handbook              || goto fail

:: clean up the handbook
pushd ..\doc\html\handbook\doxy-doc\html
del /Q  header.html, footer.html
del /Q  pages.html
del /Q  doxygen.png
del /Q  tabs.css
del /Q  tab_*.png
del /Q  bc_*.png
del /Q  nav_*.png
del /Q  open.png
del /Q  closed.png
popd

echo.
echo Docs compiled successfully!
exit /B 0

:fail
echo.
echo Docs compilation failed!
exit /B 1

rem Synopsis: call :compile_docs {path} {name}
rem Note:     omit the second parameter to prevent copying docs to /src/doxy-doc directory
rem Example:  call :compile_docs .\dll\zenwinx zenwinx
:compile_docs
	pushd %1
	rd /s /q doxy-doc
	lua "%~dp0\tools\set-doxyfile-project-number.lua" Doxyfile %ULTRADFGVER% || goto compilation_failed
	doxygen || goto compilation_failed
	copy /Y .\rsc\*.* .\doxy-doc\html\
	if "%2" neq "" (
		xcopy /I /Y /Q /S .\doxy-doc\html "%~dp0\doxy-doc\%2\html" || goto compilation_failed
	)
	:compilation_succeeded
	popd
	exit /B 0
	:compilation_failed
	popd
	exit /B 1
goto :EOF
