@echo off

rem This script was made for myself (Dmitri Arkhangelski)
rem to simplify binary packages uploading.

call build.cmd
call build-micro.cmd

mkdir archive
copy .\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe .\archive\

copy .\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe .\archive\

copy .\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\archive\

cd archive

md5sum ultradefrag-%ULTRADFGVER%.bin.* > ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-micro-edition-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-%ULTRADFGVER%.src.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS

echo cd /incoming/d/dm/dmitriar/uploads > update
echo put ultradefrag-%ULTRADFGVER%.bin.amd64.exe >> update
echo put ultradefrag-%ULTRADFGVER%.bin.i386.exe >> update
echo put ultradefrag-%ULTRADFGVER%.bin.ia64.exe >> update
echo put ultradefrag-%ULTRADFGVER%.src.7z >> update
echo put ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe >> update
echo put ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe >> update
echo put ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe >> update
echo put ultradefrag-%ULTRADFGVER%.MD5SUMS >> update
echo quit >> update

psftp -b update -bc -2 frs.sourceforge.net

cd ..
