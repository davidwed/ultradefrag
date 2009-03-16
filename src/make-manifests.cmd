REM This script generates manifests for Vista UAC, 
REM suggested by Kerem Gumrukcu (http://entwicklung.junetz.de/).

if "%1" equ "" goto failure

type manifest.part1 > .\obj\console\defrag.manifest
echo version="%ULTRADFGVER%.0" name="udefrag" processorArchitecture="%1" >> .\obj\console\defrag.manifest
type manifest.part2 >> .\obj\console\defrag.manifest
echo UltraDefrag console interface >> .\obj\console\defrag.manifest
type manifest.part3 >> .\obj\console\defrag.manifest
echo processorArchitecture="%1" >> .\obj\console\defrag.manifest
type manifest.part4 >> .\obj\console\defrag.manifest

type manifest.part1 > .\obj\gui\res\dfrg.manifest
echo version="%ULTRADFGVER%.0" name="ultradefrag" processorArchitecture="%1" >> .\obj\gui\res\dfrg.manifest
type manifest.part2 >> .\obj\gui\res\dfrg.manifest
echo UltraDefrag GUI >> .\obj\gui\res\dfrg.manifest
type manifest.part3 >> .\obj\gui\res\dfrg.manifest
echo processorArchitecture="%1" >> .\obj\gui\res\dfrg.manifest
type manifest.part4 >> .\obj\gui\res\dfrg.manifest

type manifest.part1 > .\obj\gui-launcher\udefrag-gui.manifest
echo version="%ULTRADFGVER%.0" name="udefrag-gui" processorArchitecture="%1" >> .\obj\gui-launcher\udefrag-gui.manifest
type manifest.part2 >> .\obj\gui-launcher\udefrag-gui.manifest
echo UltraDefrag GUI launcher >> .\obj\gui-launcher\udefrag-gui.manifest
type manifest.part3 >> .\obj\gui-launcher\udefrag-gui.manifest
echo processorArchitecture="%1" >> .\obj\gui-launcher\udefrag-gui.manifest
type manifest.part4 >> .\obj\gui-launcher\udefrag-gui.manifest

rem update manifests in working copy of sources
copy /Y .\obj\console\defrag.manifest .\console\defrag.manifest
copy /Y .\obj\gui\res\dfrg.manifest .\gui\res\dfrg.manifest
copy /Y .\obj\gui-launcher\udefrag-gui.manifest .\gui-launcher\udefrag-gui.manifest

exit /B 0

:failure
echo Processor Architecture must be specified!
exit /B 1
