@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

echo Delete all intermediate files...
rem call SETVARS.CMD
rem call BLDSCHED.CMD --clean
rem Recreate bin, obj, lib, doxy-doc directories
rd /s /q bin
rd /s /q lib
rd /s /q obj
rd /s /q doxy-doc

if "%1" equ "--clean" goto end
mkdir bin
mkdir lib
mkdir obj
mkdir doxy-doc

:end
echo Done.
