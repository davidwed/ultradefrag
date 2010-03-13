@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

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
rd /s /q pre-release

if "%1" equ "--clean" goto end

rem Recreate bin, obj, lib, doxy-doc directories
mkdir bin
mkdir lib
mkdir obj
mkdir doxy-doc

:end
echo Done.
