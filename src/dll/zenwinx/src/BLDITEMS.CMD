@echo off
pushd obj\_
lua ..\..\tools\mkmod.lua zenwinx.build
if %errorlevel% neq 0 goto end
popd
exit /B 0

:end
popd
exit /B 1
