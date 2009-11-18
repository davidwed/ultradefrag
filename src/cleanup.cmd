@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

echo Delete all intermediate files...
rem call SETVARS.CMD
rem call BLDSCHED.CMD --clean
rem Recreate bin, obj, lib directories
rd /s /q bin
mkdir bin
rd /s /q lib
mkdir lib
rd /s /q obj
mkdir obj
echo Done.
