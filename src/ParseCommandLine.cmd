@echo off
:: This script is used to set the initial build defaults,
:: which are changed by the command line switches described
:: in "build-targets.cmd --help [build | build-micro]"

echo.
echo %~n0 ... %*
echo.

:: set to 1 to debug parameter checking
set UD_BLD_FLG_PARSER_DEBUG=0

:: set compiler constants
set UD_BLD_FLG_USE_MINGW=1
set UD_BLD_FLG_USE_MSVC=2
set UD_BLD_FLG_USE_MINGW64=3
set UD_BLD_FLG_USE_PELLESC=4
set UD_BLD_FLG_USE_WINDDK=5
set UD_BLD_FLG_USE_WINSDK=6

:: set defaults, if not already defined
if not defined UD_BLD_FLG_USE_COMPILER   set UD_BLD_FLG_USE_COMPILER=0
if not defined UD_BLD_FLG_DO_INSTALL     set UD_BLD_FLG_DO_INSTALL=0
if not defined UD_BLD_FLG_ONLY_CLEANUP   set UD_BLD_FLG_ONLY_CLEANUP=0
if not defined UD_BLD_FLG_DIPLAY_HELP    set UD_BLD_FLG_DIPLAY_HELP=0
if not defined UD_BLD_FLG_IS_MICRO       set UD_BLD_FLG_IS_MICRO=0
if not defined UD_BLD_FLG_IS_PORTABLE    set UD_BLD_FLG_IS_PORTABLE=0
if not defined UD_BLD_FLG_IS_PRE_RELEASE set UD_BLD_FLG_IS_PRE_RELEASE=0
if not defined UD_BLD_FLG_BUILD_X86      set UD_BLD_FLG_BUILD_X86=1
if not defined UD_BLD_FLG_BUILD_AMD64    set UD_BLD_FLG_BUILD_AMD64=0
if not defined UD_BLD_FLG_BUILD_IA64     set UD_BLD_FLG_BUILD_IA64=0

:ParseArgs
if "%1" == "--use-mingw" (
	set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_MINGW%
	set UD_BLD_FLG_BUILD_X86=1
	set UD_BLD_FLG_BUILD_AMD64=0
	set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--use-winddk" (
	set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_WINDDK%
	set UD_BLD_FLG_BUILD_X86=1
	set UD_BLD_FLG_BUILD_AMD64=1
	set UD_BLD_FLG_BUILD_IA64=1
)
if "%1" == "--use-winsdk" (
	set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_WINSDK%
	set UD_BLD_FLG_BUILD_X86=1
	set UD_BLD_FLG_BUILD_AMD64=1
	set UD_BLD_FLG_BUILD_IA64=1
)
if "%1" == "--use-msvc" (
    set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_MSVC%
	set UD_BLD_FLG_BUILD_X86=1
	set UD_BLD_FLG_BUILD_AMD64=0
	set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--use-pellesc" (
	set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_PELLESC%
	set UD_BLD_FLG_BUILD_X86=1
	set UD_BLD_FLG_BUILD_AMD64=0
	set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--use-mingw-x64" (
	set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_MINGW64%
	set UD_BLD_FLG_BUILD_X86=0
	set UD_BLD_FLG_BUILD_AMD64=1
	set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--install"       set UD_BLD_FLG_DO_INSTALL=1
if "%1" == "--clean"         set UD_BLD_FLG_ONLY_CLEANUP=1
if "%1" == "--help"          set UD_BLD_FLG_DIPLAY_HELP=1
if "%1" == "--micro"         set UD_BLD_FLG_IS_MICRO=1
if "%1" == "--portable"      set UD_BLD_FLG_IS_PORTABLE=1
if "%1" == "--pre-release"   set UD_BLD_FLG_IS_PRE_RELEASE=1
if "%1" == "--no-x86"        set UD_BLD_FLG_BUILD_X86=0
if "%1" == "--no-amd64"      set UD_BLD_FLG_BUILD_AMD64=0
if "%1" == "--no-ia64"       set UD_BLD_FLG_BUILD_IA64=0

if "%1" == "build"           set UD_BLD_FLG_IS_MICRO=0
if "%1" == "build-micro"     set UD_BLD_FLG_IS_MICRO=1
shift
if not "%1" == "" goto :ParseArgs

if %UD_BLD_FLG_PARSER_DEBUG% == 0 goto :EOF

:: display variables
set ud

:: clear variables
for %%V in ( UD_BLD_FLG_USE_COMPILER UD_BLD_FLG_DO_INSTALL UD_BLD_FLG_ONLY_CLEANUP UD_BLD_FLG_DIPLAY_HELP ) do set %%V=
for %%V in ( UD_BLD_FLG_IS_MICRO UD_BLD_FLG_IS_PORTABLE UD_BLD_FLG_IS_PRE_RELEASE ) do set %%V=
for %%V in ( UD_BLD_FLG_BUILD_X86 UD_BLD_FLG_BUILD_AMD64 UD_BLD_FLG_BUILD_IA64 ) do set %%V=
