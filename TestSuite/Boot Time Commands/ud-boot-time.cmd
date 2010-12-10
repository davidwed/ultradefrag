@echo off
;--------------------------------------------------------------------
;                UltraDefrag Boot Time Shell Script
;--------------------------------------------------------------------
; !!! NOTE: THIS FILE MUST BE SAVED IN UNICODE (UTF-16) ENCODING !!!
;--------------------------------------------------------------------

set UD_IN_FILTER=windows;winnt;ntuser;pagefile;hiberfil
set UD_EX_FILTER=temp;recycle;system volume information

echo.
echo Enable Debugging ...
echo.
;set UD_DRY_RUN=1
set UD_DBGPRINT_LEVEL=DETAILED
set UD_LOG_FILE_PATH=C:\Windows\UltraDefrag\Logs\defrag_native.log

;udefrag --all-fixed -a
udefrag --all-fixed
;udefrag --all-fixed -o

set UD_LOG_FILE_PATH=A:\UltraDefrag_Logs\defrag_native.log

udefrag --all-fixed -a
;udefrag --all-fixed
;udefrag --all-fixed -o

; comment out next line to run test suite
exit

; Test Suite

; Allow the user to break out of script execution
echo.
echo Hit 'ESC' to enter interactive mode
echo Script execution will continue in 5 seconds
pause 5000

; Execute boot time command test suite script
; general commands test
call C:\ud-boot-time-test-suite.cmd
; udefrag test
call C:\ud-boot-time-test-udefrag.cmd
echo.
echo Test Suite finished ...
echo.
