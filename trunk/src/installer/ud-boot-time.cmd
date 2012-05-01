@echo off
;--------------------------------------------------------------------
;                UltraDefrag Boot Time Shell Script
;--------------------------------------------------------------------
; !!! NOTE: THIS FILE MUST BE SAVED IN UNICODE (UTF-16) ENCODING !!!
;--------------------------------------------------------------------

set UD_IN_FILTER=*windows*;*winnt*;*ntuser*;*pagefile.sys;*hiberfil.sys
set UD_EX_FILTER=*temp*;*tmp*;*.zip;*.7z;*.rar;*dllcache*;*ServicePackFiles*

; uncomment the following line to create debugging output
; set UD_DBGPRINT_LEVEL=DETAILED

; uncomment the following line to save debugging information to a log file
; set UD_LOG_FILE_PATH=%SystemRoot%\UltraDefrag\Logs\defrag_native.log

udefrag c:

exit
