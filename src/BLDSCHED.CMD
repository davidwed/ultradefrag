@echo off
echo Build the scheduler...

cd .\scheduler\DotNET
echo UltraDefragScheduler.NET ...

if "%1" equ "--clean" goto clean.NET

:build.NET
%NETRUNTIMEPATH%\MSBuild.exe /p:Configuration=Release
if %errorlevel% neq 0 goto exit.NET
goto success.NET

:clean.NET
%NETRUNTIMEPATH%\MSBuild.exe /t:Clean /p:Configuration=Release
%NETRUNTIMEPATH%\MSBuild.exe /t:Clean /p:Configuration=Debug
goto exit.NET

:success.NET
copy /Y .\bin\Release\UltraDefragScheduler.NET.exe ..\..\bin\

:exit.NET
cd ..\..
