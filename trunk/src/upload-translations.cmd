:: @echo off
::
:: Script to upload translations to the translation project at
:: https://www.transifex.com/projects/p/ultradefrag/
:: Copyright (c) 2013 Stefan Pendl (stefanpe@users.sourceforge.net).
::
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program; if not, write to the Free Software
:: Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::

title Upload started

:: set environment
call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
echo.

if not exist "%GNUWIN32_DIR%" goto gnuwin32_missing

:: configure PATH
set OLD_PATH=%PATH%
set PATH=%GNUWIN32_DIR%;%PATH%

:: extract translations
pushd "%~dp0\wxgui"
copy /v /y locale\UltraDefrag.header locale\UltraDefrag.pot
set PO_INFO=--copyright-holder="UltraDefrag Development Team" --msgid-bugs-address="http://sourceforge.net/p/ultradefrag/bugs/"
xgettext -C -j --add-comments=: %PO_INFO% -k_ -kwxPLURAL:1,2 -kUD_UpdateMenuItemLabel:2 -o locale\UltraDefrag.pot *.cpp || popd && goto fail
lua ..\tools\adjust-locale-header.lua locale\UltraDefrag.pot || popd && goto fail
copy /v /y "locale\UltraDefrag.pot" "%~dp0\tools\transifex\translations\ultradefrag.main\en_US.po"
popd
echo.

:: update source language
pushd "%~dp0\tools\transifex"
if exist tx.exe tx.exe push -s --skip || popd && goto fail
echo.

:: upload all translations to transifex
if exist tx.exe tx.exe push -t --skip || popd && goto fail
echo.

:: download all translations from transifex
set force=-f
if "%~1" neq "--lng2po" set force=
if exist tx.exe tx.exe pull -a -s %force% || popd && goto fail
echo.
popd

:success
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo.
echo Translations uploaded sucessfully!
title Translations uploaded sucessfully!
exit /B 0

:gnuwin32_missing
echo !!! GNUwin32 path not set correctly !!!
set ERRORLEVEL=99

:fail
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo Build error (code %ERRORLEVEL%)!
title Build error (code %ERRORLEVEL%)!
exit /B 1
