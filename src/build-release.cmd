@echo off

rem This script was made for myself (Dmitri Arkhangelski)
rem to simplify binary packages uploading.

mkdir release

call build-src-package.cmd || goto build_failed
copy .\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\release\

call build.cmd --all --use-winddk || goto build_failed

:: copy all packages to the release directory
copy .\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe .\release\
copy .\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe .\release\
copy .\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe .\release\

copy .\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe .\release\
copy .\bin\amd64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe .\release\
copy .\bin\ia64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe .\release\

copy .\bin\ultradefrag-portable-%ULTRADFGVER%.bin.i386.zip .\release\
copy .\bin\amd64\ultradefrag-portable-%ULTRADFGVER%.bin.amd64.zip .\release\
copy .\bin\ia64\ultradefrag-portable-%ULTRADFGVER%.bin.ia64.zip .\release\

copy .\bin\ultradefrag-micro-portable-%ULTRADFGVER%.bin.i386.zip .\release\
copy .\bin\amd64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.amd64.zip .\release\
copy .\bin\ia64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.ia64.zip .\release\

cd release
md5sum ultradefrag-%ULTRADFGVER%.bin.* > ultradefrag-%ULTRADFGVER%.MD5SUMS
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
