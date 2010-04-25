@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

call ParseCommandLine.cmd %*

echo Delete all intermediate files...
rem call SETVARS.CMD
rem call BLDSCHED.CMD --clean
rd /s /q bin
rd /s /q lib
rd /s /q obj
rd /s /q doxy-doc
rd /s /q driver\doxy-doc
rd /s /q dll\wgx\doxy-doc
rd /s /q dll\udefrag\doxy-doc
rd /s /q dll\zenwinx\doxy-doc
rd /s /q dll\udefrag-kernel\doxy-doc
rd /s /q ..\doc\html\handbook\doxy-doc
rd /s /q src_package
rd /s /q ..\src_package
rd /s /q pre-release

rem do we need to keep the archive?
rem rd /s /q archive

if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 goto end

rem Recreate bin, obj, lib, doxy-doc directories
rem
rem obj and lib are created by build and build-micro
rem so we could skip them here
rem
mkdir bin
mkdir lib
mkdir obj
mkdir doxy-doc

:end
echo Done.
