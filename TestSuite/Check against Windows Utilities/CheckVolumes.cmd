@rem 
@echo off

for /F "tokens=2 delims=[]" %%V in ('ver') do for /F "tokens=2" %%R in ( 'echo %%V' ) do set win_ver=%%R

echo.
for /F "tokens=1 skip=6" %%D in ( 'udefrag -l' ) do call :process %%D
echo.
goto :EOF

:process
    set drive_tmp=%1
    set drive=%drive_tmp:~0,1%
    
	echo Processing %1 ...
	echo      Running CHKDSK ....
	call :RunUtility chkdsk %1 >"%COMPUTERNAME%_%drive%_chkdsk.log" 2>nul
	
	if %win_ver% GEQ 5.1 (
		echo      Running DEFRAG ....
		call :RunUtility defrag %1 >"%COMPUTERNAME%_%drive%_defrag.log" 2>nul
	)
	
	echo      Running UDEFRAG ...
	call :RunUtility udefrag %1 >"%COMPUTERNAME%_%drive%_udefrag.log" 2>nul
	echo ------------------------
goto :EOF

:RunUtility
    echo.
    echo Started "%*" at %DATE% - %TIME%
    echo -------------------------------
    echo.
    if /i "%1" == "chkdsk"  chkdsk %2 | findstr /v /i "Prozent percent"
    if /i "%1" == "defrag"  defrag -a -v %2
    if /i "%1" == "udefrag" udefrag -a -v %2
    echo.
    echo -------------------------------
    echo Finished "%*" at %DATE% - %TIME%
goto :EOF
