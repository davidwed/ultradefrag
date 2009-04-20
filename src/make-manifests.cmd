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

type manifest.part1 > .\obj\udefrag-gui-config\res\config.manifest
echo version="%ULTRADFGVER%.0" name="udefrag-gui-config" processorArchitecture="%1" >> .\obj\udefrag-gui-config\res\config.manifest
type manifest.part2 >> .\obj\udefrag-gui-config\res\config.manifest
echo UltraDefrag GUI Configurator >> .\obj\udefrag-gui-config\res\config.manifest
type manifest.part3 >> .\obj\udefrag-gui-config\res\config.manifest
echo processorArchitecture="%1" >> .\obj\udefrag-gui-config\res\config.manifest
type manifest.part4 >> .\obj\udefrag-gui-config\res\config.manifest

type manifest.part1 > .\obj\bootexctrl\bootexctrl.manifest
echo version="%ULTRADFGVER%.0" name="bootexctrl" processorArchitecture="%1" >> .\obj\bootexctrl\bootexctrl.manifest
type manifest.part2 >> .\obj\bootexctrl\bootexctrl.manifest
echo BootExecute Control Program >> .\obj\bootexctrl\bootexctrl.manifest
type manifest.part3 >> .\obj\bootexctrl\bootexctrl.manifest
echo processorArchitecture="%1" >> .\obj\bootexctrl\bootexctrl.manifest
type manifest.part4 >> .\obj\bootexctrl\bootexctrl.manifest

type manifest.part1 > .\obj\lua5.1\lua.manifest
echo version="5.1.2.0" name="Lua" processorArchitecture="%1" >> .\obj\lua5.1\lua.manifest
type manifest.part2 >> .\obj\lua5.1\lua.manifest
echo Lua Console >> .\obj\lua5.1\lua.manifest
type manifest.part3 >> .\obj\lua5.1\lua.manifest
echo processorArchitecture="%1" >> .\obj\lua5.1\lua.manifest
type manifest.part4a >> .\obj\lua5.1\lua.manifest
echo processorArchitecture="%1" >> .\obj\lua5.1\lua.manifest
type manifest.part5 >> .\obj\lua5.1\lua.manifest

rem update manifests in working copy of sources
if "%1" neq "X86" goto L1
copy /Y .\obj\console\defrag.manifest .\console\defrag.manifest
copy /Y .\obj\gui\res\dfrg.manifest .\gui\res\dfrg.manifest
copy /Y .\obj\gui-launcher\udefrag-gui.manifest .\gui-launcher\udefrag-gui.manifest
copy /Y .\obj\udefrag-gui-config\res\config.manifest .\udefrag-gui-config\res\config.manifest
copy /Y .\obj\bootexctrl\bootexctrl.manifest .\bootexctrl\bootexctrl.manifest
copy /Y .\obj\lua5.1\lua.manifest .\lua5.1\lua.manifest

:L1
exit /B 0

:failure
echo Processor Architecture must be specified!
exit /B 1
