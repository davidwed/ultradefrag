@echo off

echo Creating default doxygen HTML header, footer and CSS script...
echo.

call :make_default .                    || goto fail
call :make_default .\gui                || goto fail
call :make_default .\dll\udefrag        || goto fail
call :make_default .\dll\wgx            || goto fail
call :make_default .\dll\zenwinx        || goto fail
call :make_default ..\doc\html\handbook || goto fail

echo.
echo Created default files successfully!
goto :END

:fail
echo.
echo Creating default files failed!

:END
echo.
pause
goto :EOF

rem Synopsis: call :make_default {path}
rem Example:  call :make_default .\dll\zenwinx
:make_default
	pushd %1
    echo ====
    echo %CD%
    echo.
	rd /s /q doxy-defaults
	doxygen -w html default_header.html default_footer.html default_styles.css DoxyFile || goto compilation_failed
    md doxy-defaults
	move /Y default_*.* doxy-defaults
	
	:compilation_succeeded
	popd
	exit /B 0
	:compilation_failed
	popd
	exit /B 1
goto :EOF
