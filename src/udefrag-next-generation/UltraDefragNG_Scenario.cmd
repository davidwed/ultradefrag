@echo off
cls
echo.
echo ##  ## #  ########     #   #####  ##### ##### ###     #  ###
echo  #   # #    #  #  #   ##    #   #  #  #  #  # #  #   ## #   
echo  #   # #    #  #  #  # #    #   #  #\    #    #  #  # # #   
echo  #   # #    #  ###  ####    #   #  #/    ##   ###  #### # ## 
echo  #   # #  # #  # #  #  #    #   #  #  #  #    # #  #  # #  #
echo   #### #### #  #  # #  #   #####  #####  #    #  # #  #  ###     
echo.
echo                               Let's defragment everything...
echo.
echo UltraDefrag Next Generation Scenario           Version 3.3.1
echo ============================================================
echo.
echo.

echo 1. Check UltraDefrag installation
udefrag --help > nul
if %errorlevel% neq 0 goto ultradefrag_missing

echo 2. Exclude all temporary stuff

set UD_EX_FILTER=system volume information;temp;recycler;.zip;.7z;.rar


echo.
echo 3. Defragment small files

set UD_SIZELIMIT=50Mb


udefrag c:


echo.
echo 4. Defragment large files which have at least 5 fragments

set UD_SIZELIMIT=
set UD_FRAGMENTS_THRESHOLD=5


udefrag c:


echo.
pause
exit /B 0

:ultradefrag_missing
echo.
echo UltraDefrag not found on your system. Download and install it:
echo http://sourceforge.net/projects/ultradefrag/files/
echo.
pause
exit /B 1
