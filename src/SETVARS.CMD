@echo off
echo Set common environment variables...
set ULTRADFGVER=1.2.1
set VERSION=1,2,1,0
set VERSION2="1, 2, 1, 0\0"
set NSISDIR=D:\Software\Tools\NSIS
set MSVSBIN=D:\Software\Microsoft Visual Studio\VC98\Bin
set WINDDKBASE=D:\WINDDK\3790~1.183
set NETRUNTIMEPATH=C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727

echo Set DDK and compiler specific environment...
goto __%DDK_TYPE%

:__ReactOS_DDK
set DDKINCDIR=D:\MyDocs\ReactOS030\include
goto __%COMPILER_TYPE%

:__MSVC
call "%MSVSBIN%\vcvars32.bat"
goto _exit

:__GCC
echo GCC is currently unsupported!
goto _exit

:__Windows_Server2003_DDK
goto _exit

:__

:_exit