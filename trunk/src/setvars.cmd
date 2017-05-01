@echo off
echo Set common environment variables...

:: UltraDefrag version
set VERSION_MAJOR=7
set VERSION_MINOR=0
set VERSION_REVISION=2

:: alpha1, beta2, RC3, etc.
:: unset for the final releases
set RELEASE_STAGE=

:: debugging facilities
:: - set to 1 to attach UltraDefrag debugger which
::   will handle application crashes in a special way
set ATTACH_DEBUGGER=1
:: - set to 1 to send crash reports via Google Analytics service
set SEND_CRASH_REPORTS=1
:: - set to 1 to enable wxWidgets asserts raising dialog boxes
::   NOTE: don't forget to recompile wxWidgets after adjustment
set ENABLE_WX_ASSERTS=0

:: paths to development tools
set WINSDKBASE=C:\Program Files\Microsoft SDKs\Windows\v7.1
set MINGWBASE=D:\Software\MinGW32
set MINGWx64BASE=D:\Software\mingw64
set WXWIDGETSDIR=D:\Software\wxWidgets-3.1.0
set NSISDIR=D:\Software\Tools\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip
set GNUWIN32_DIR=D:\Software\GnuWin32\bin
set CODEBLOCKS_EXE=D:\Software\CodeBlocks\codeblocks.exe

:: auxiliary stuff
set VERSION=%VERSION_MAJOR%,%VERSION_MINOR%,%VERSION_REVISION%,0
set VERSION2="%VERSION_MAJOR%, %VERSION_MINOR%, %VERSION_REVISION%, 0\0"
set ULTRADFGVER=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_REVISION%
if "%RELEASE_STAGE%" neq "" (
    set UDVERSION_SUFFIX=%ULTRADFGVER%-%RELEASE_STAGE%
) else (
    set UDVERSION_SUFFIX=%ULTRADFGVER%
)
