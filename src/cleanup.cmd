@echo off

rem This script performs UltraDefrag source directory cleanup
rem by removing all intermediate files.

call ParseCommandLine.cmd %*

echo Delete all intermediate files...
rd /s /q bin
rd /s /q lib
rd /s /q obj
rd /s /q doxy-doc
rd /s /q dll\wgx\doxy-doc
rd /s /q dll\udefrag\doxy-doc
rd /s /q dll\zenwinx\doxy-doc
rd /s /q ..\doc\html\handbook\doxy-doc
rd /s /q src_package
rd /s /q ..\src_package
rd /s /q pre-release
rd /s /q release

echo Done.
