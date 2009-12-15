@echo off

rem This script was made for myself (Dmitri Arkhangelski)
rem to simplify binary packages uploading.

mkdir archive

call build.cmd --use-winddk
copy .\bin\ultradefrag-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-%ULTRADFGVER%.bin.ia64.exe .\archive\
copy .\src_package\ultradefrag-%ULTRADFGVER%.src.7z .\archive\

call build-micro.cmd --use-winddk
copy .\bin\ultradefrag-micro-edition-%ULTRADFGVER%.bin.i386.exe .\archive\
copy .\bin\amd64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.amd64.exe .\archive\
copy .\bin\ia64\ultradefrag-micro-edition-%ULTRADFGVER%.bin.ia64.exe .\archive\

call build-portable.cmd --use-winddk
copy .\bin\ultradefrag-portable-%ULTRADFGVER%.bin.i386.zip .\archive\
copy .\bin\amd64\ultradefrag-portable-%ULTRADFGVER%.bin.amd64.zip .\archive\
copy .\bin\ia64\ultradefrag-portable-%ULTRADFGVER%.bin.ia64.zip .\archive\

call build-micro-portable.cmd --use-winddk
copy .\bin\ultradefrag-micro-portable-%ULTRADFGVER%.bin.i386.zip .\archive\
copy .\bin\amd64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.amd64.zip .\archive\
copy .\bin\ia64\ultradefrag-micro-portable-%ULTRADFGVER%.bin.ia64.zip .\archive\

cd archive

md5sum ultradefrag-%ULTRADFGVER%.bin.* > ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-micro-edition-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-portable-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-micro-portable-%ULTRADFGVER%.bin.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS
md5sum ultradefrag-%ULTRADFGVER%.src.* >> ultradefrag-%ULTRADFGVER%.MD5SUMS

cd ..
exit /B 0

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

mkdir HibernateForWindows-1.0.0.bin
mkdir HibernateForWindows-1.0.0.bin\amd64
mkdir HibernateForWindows-1.0.0.bin\ia64
copy ..\bin\hibernate.exe .\HibernateForWindows-1.0.0.bin\
copy ..\bin\amd64\hibernate.exe .\HibernateForWindows-1.0.0.bin\amd64\
copy ..\bin\ia64\hibernate.exe .\HibernateForWindows-1.0.0.bin\ia64\
copy ..\hibernate\readme.txt .\HibernateForWindows-1.0.0.bin\
zip -r -m -9 -X HibernateForWindows-1.0.0.bin.zip HibernateForWindows-1.0.0.bin

rem this method is not working now
rem psftp -b update -bc -2 frs.sourceforge.net

cd ..
