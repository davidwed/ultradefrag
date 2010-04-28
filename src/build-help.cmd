@echo on

rem script to display help about command line switches

if "%1" == "" "%~0" build

set UD_BLD_HELP_SUPR_INST=0
set UD_BLD_HELP_SPACE=0
set UD_BLD_HELP_SPACE_STR_1="                              "
set UD_BLD_HELP_INST_STR=[--install]

for /f "tokens=1,2,3 delims=-" %%X in ('echo %~1') do (
	set part1=%%X
	set part2=%%Y
	set part3=%%Z
)

if "%part3%" == "portable" set UD_BLD_HELP_SUPR_INST=1
if "%part2%" == "portable" set UD_BLD_HELP_SUPR_INST=1
if "%part2%" == "micro"    set UD_BLD_HELP_SPACE=1

if %UD_BLD_HELP_SUPR_INST% EQU 1 set /a UD_BLD_HELP_SPACE+=2
if %UD_BLD_HELP_SUPR_INST% EQU 1 set UD_BLD_HELP_INST_STR=%UD_BLD_HELP_SPACE_STR_1:~1,1%

if %UD_BLD_HELP_SPACE% EQU 0 set UD_BLD_HELP_SPACE_STR=
if %UD_BLD_HELP_SPACE% EQU 1 set UD_BLD_HELP_SPACE_STR=%UD_BLD_HELP_SPACE_STR_1:~1,6%
if %UD_BLD_HELP_SPACE% EQU 2 set UD_BLD_HELP_SPACE_STR=%UD_BLD_HELP_SPACE_STR_1:~1,9%
if %UD_BLD_HELP_SPACE% EQU 3 set UD_BLD_HELP_SPACE_STR=%UD_BLD_HELP_SPACE_STR_1:~1,15%

@echo off
echo.
echo Synopsis:
echo     %~1              - perform the build using default options
if %UD_BLD_HELP_SUPR_INST% EQU 0 echo     %~1 --install    - run installer silently after the build
echo.
echo     %~1 [compiler] %UD_BLD_HELP_INST_STR% - specify your favorite compiler:
echo.
echo     %UD_BLD_HELP_SPACE_STR%                   --use-mingw     (default)
echo.
echo     %UD_BLD_HELP_SPACE_STR%                   --use-winddk
echo     %UD_BLD_HELP_SPACE_STR%                   --use-winsdk
echo     %UD_BLD_HELP_SPACE_STR%                   --use-msvc      (obsolete)
echo     %UD_BLD_HELP_SPACE_STR%                   --use-pellesc   (experimental, produces wrong code)
echo     %UD_BLD_HELP_SPACE_STR%                   --use-mingw-x64 (experimental, produces wrong x64 code)
echo.
echo     %~1 --clean      - perform full cleanup instead of the build
echo     %~1 --help       - show this help message
echo.
echo     %~1 [compiler] [--no-{arch}] %UD_BLD_HELP_INST_STR% - skip any processor architecture:
echo.
echo     %UD_BLD_HELP_SPACE_STR%                   --no-x86, --no-amd64, --no-ia64
exit /B 0
