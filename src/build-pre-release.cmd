@echo off

:: This script was made for myself (Stefan Pendl)
:: to simplify pre-release (alpha, beta and RC) building.
::
:: The following arguments are accepted:
::      --[alpha|beta|betai|rc]
::          alpha ... builds a portable package and adds alpha{version} to the package names
::          beta .... builds a portable package and adds beta{version} to the package names
::          betai ... builds an installer package and adds beta{version} to the package names
::          rc ...... builds an installer package and adds RC{version} to the package names

echo.
set /P UD_BLD_PRE_RELEASE_NUM="Enter Version number [1]: "
if "%UD_BLD_PRE_RELEASE_NUM%" == "" set UD_BLD_PRE_RELEASE_NUM=1

call ParseCommandLine.cmd %*

for /f "tokens=%UD_BLD_FLG_BUILD_STAGE%" %%T in ( "alpha beta beta RC" ) do set UD_BLD_STAGE_NAME=%%T

set UD_BLD_PRE_RELEASE_VER=%UD_BLD_STAGE_NAME%%UD_BLD_PRE_RELEASE_NUM%

set script_dir=%~dp0

echo.

set UD_BLD_STAGE_EXT=exe
set UD_BLD_STAGE_PKG=

if %UD_BLD_FLG_BUILD_STAGE% GTR 2 goto :BuildPre

set UD_BLD_STAGE_EXT=zip
set UD_BLD_STAGE_PKG=-portable

:BuildPre
call build.cmd -%UD_BLD_STAGE_PKG% --use-winddk --pre-release %*
if %errorlevel% neq 0 goto build_failed

:CollectFiles
cd /D %script_dir%

rd /s /q pre-release
mkdir pre-release
echo.
copy /b /y /v .\bin\ultradefrag-*.bin.i386.*        .\pre-release\ultradefrag%UD_BLD_STAGE_PKG%-%ULTRADFGVER%-%UD_BLD_PRE_RELEASE_VER%.bin.i386.%UD_BLD_STAGE_EXT%
copy /b /y /v .\bin\amd64\ultradefrag-*.bin.amd64.* .\pre-release\ultradefrag%UD_BLD_STAGE_PKG%-%ULTRADFGVER%-%UD_BLD_PRE_RELEASE_VER%.bin.amd64.%UD_BLD_STAGE_EXT%
copy /b /y /v .\bin\ia64\ultradefrag-*.bin.ia64.*   .\pre-release\ultradefrag%UD_BLD_STAGE_PKG%-%ULTRADFGVER%-%UD_BLD_PRE_RELEASE_VER%.bin.ia64.%UD_BLD_STAGE_EXT%

echo.
echo %UD_BLD_PRE_RELEASE_VER% Pre-Release created successfully!
exit /B 0

:build_failed
echo.
echo Pre-Release building error!
exit /B 1
