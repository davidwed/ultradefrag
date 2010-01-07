@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

echo Delete all intermediate files...
rem call SETVARS.CMD
rem call BLDSCHED.CMD --clean
rem Recreate bin, obj, lib, doxy-doc directories
rd /s /q bin
mkdir bin
rd /s /q lib
mkdir lib
rd /s /q obj
mkdir obj
rd /s /q doxy-doc
mkdir doxy-doc
echo Done.
