@echo off

echo Upgrading doxygen configuration files...
echo.

call :upgrade_config .                    || goto fail
call :upgrade_config .\gui                || goto fail
call :upgrade_config .\dll\udefrag        || goto fail
call :upgrade_config .\dll\wgx            || goto fail
call :upgrade_config .\dll\zenwinx        || goto fail
call :upgrade_config ..\doc\html\handbook || goto fail

echo.
echo Upgrade succeeded!
goto :END

:fail
echo.
echo Upgrade failed!

:END
echo.
pause
goto :EOF

rem Synopsis: call :upgrade_config {path}
rem Example:  call :upgrade_config .\dll\zenwinx
:upgrade_config
	pushd %1
    echo ====
    echo %CD%
	doxygen -u DoxyFile || goto compilation_failed
	
	:compilation_succeeded
    del /f /q DoxyFile.bak
	popd
	exit /B 0
	:compilation_failed
	popd
	exit /B 1
goto :EOF
