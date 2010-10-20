@echo off

rem This script was made for myself (Dmitri Arkhangelski)
rem to simplify binary packages uploading.

mkdir archive

call build-src-package.cmd
if %errorlevel% neq 0 goto build_failed
copy .\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\archive\

call build.cmd --use-winddk
if %errorlevel% neq 0 goto build_failed
copy .\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe .\archive\

call build-micro.cmd --use-winddk
if %errorlevel% neq 0 goto build_failed
copy .\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe .\archive\

call build-portable.cmd --use-winddk
if %errorlevel% neq 0 goto build_failed
copy .\bin\ultradefrag-portable-%ULTRADFGVER%.bin.i386.zip .\archive\
copy .\bin\amd64\ultradefrag-portable-%ULTRADFGVER%.bin.amd64.zip .\archive\
copy .\bin\ia64\ultradefrag-portable-%ULTRADFGVER%.bin.ia64.zip .\archive\

call build-micro-portable.cmd --use-winddk
if %errorlevel% neq 0 goto build_failed
copy .\bin\ultradefrag-micro-portable-%ULTRADFGVER%.bin.i386.zip .\archive\
copy .\bin\amd64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.amd64.zip .\archive\
copy .\bin\ia64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.ia64.zip .\archive\

cd archive

md5sum ultradefrag-%ULTRADFGVER%.bin.* > ultradefrag-%ULTRADFGVER%.MD5SUMS
rem md5sum ultradefrag-next-generation-gui-%ULTRADFGVER%.bin.* > ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-micro-edition-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-portable-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-micro-portable-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-%ULTRADFGVER%.src.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS

cd ..
echo.
echo Release made successfully!
exit /B 0

:build_failed
echo.
echo Release building error!
exit /B 1
