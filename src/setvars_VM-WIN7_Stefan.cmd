@echo off
echo Set Stefans' environment variables...

:: save username, since build process overwrites it
if not defined ORIG_USERNAME set ORIG_USERNAME=%USERNAME%

set WINDDKBASE=C:\WinDDK\6001.18002
set WINSDKBASE=
set MINGWBASE=
set MINGWx64BASE=
set NETRUNTIMEPATH=C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727
set NSISDIR=C:\Program Files\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip
set MSVSBIN=
set DDKINCDIR=
set PELLESC_BASE=

rem comment out next line to enable warnings to find unreachable code
goto :EOF

set CL=/Wall /wd4619 /wd4711 /wd4706
rem set LINK=/WX
