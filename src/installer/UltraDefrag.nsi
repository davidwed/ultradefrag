/*
 *  ULTRADEFRAG - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by D. Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *  Installer + portable launcher source.
 */

/*
 *  NOTE: The following symbols should be defined
 *        through makensis command line:
 *  ULTRADFGVER=<version number in form x.y.z>
 *  ULTRADFGARCH=<i386 | amd64 | ia64>
 */

!ifndef ULTRADFGVER
!define ULTRADFGVER 1.10.0
!endif

!ifndef ULTRADFGARCH
!define ULTRADFGARCH i386
!endif

;!include "ultradefrag_globals.nsh"
!include "x64.nsh"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

;-----------------------------------------

!define MODERN_UI

!ifdef MODERN_UI
  !include "MUI.nsh"
!endif

!macro LANG_PAGE
  Page custom LangShow LangLeave ""
!macroend

;-----------------------------------------
!if ${ULTRADFGARCH} == 'amd64'
Name "Ultra Defragmenter v${ULTRADFGVER} (AMD64)"
!else if ${ULTRADFGARCH} == 'ia64'
Name "Ultra Defragmenter v${ULTRADFGVER} (IA64)"
!else
Name "Ultra Defragmenter v${ULTRADFGVER} (i386)"
!endif
OutFile "ultradefrag-${ULTRADFGVER}.bin.${ULTRADFGARCH}.exe"

LicenseData "${ROOTDIR}\src\LICENSE.TXT"
;InstallDir "$PROGRAMFILES64\UltraDefrag"
;InstallDirRegKey HKLM "Software\DASoft\NTDefrag" "Install_Dir"
ShowInstDetails show
ShowUninstDetails show

SetCompressor lzma

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007,2008 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

ReserveFile "lang.ini"

!ifdef MODERN_UI
  !define MUI_COMPONENTSPAGE_SMALLDESC

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "${ROOTDIR}\src\LICENSE.TXT"
  !insertmacro MUI_PAGE_COMPONENTS
;  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro LANG_PAGE
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
!else
  Page license
  Page components
;  Page directory
  !insertmacro LANG_PAGE
  Page instfiles

  UninstPage uninstConfirm
  UninstPage instfiles
!endif

;-----------------------------------------

Var NT4_TARGET
Var SchedulerNETinstalled
Var DocsInstalled
Var PortableInstalled
Var RunPortable
Var ShowBootsplash
Var IsInstalled
Var LanguagePack

Function .onInit

  push $R0
  push $R1

!insertmacro DisableX64FSRedirection

  ; ultradefrag 1.3.1+ will be installed in %sysdir%\UltraDefrag
  ; and ultradefrag 1.3.4+ - in %windir%\UltraDefrag
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"


  /* is already installed 1.3.0 or earlier version? */
  RMDir /r "$SYSDIR\UltraDefrag"
  ; don't work on some systems
  ;;IfFileExists "$SYSDIR\defrag_native.exe" 0 version_checked
;  ClearErrors
;  FileOpen $R0 "$SYSDIR\defrag_native.exe" "r"
;  FileClose $R0
;  IfErrors version_checked 0
;  IfFileExists "$INSTDIR\dfrg.exe" version_checked 0
;  MessageBox MB_OK|MB_ICONEXCLAMATION \
;   "$SYSDIR\defrag_native.exe found!$\n$\nUninstall any previous version of Ultra Defragmenter$\nbefore this installation!" \
;   /SD IDOK
;   goto abort_inst
;version_checked:

  /* variables initialization */
  StrCpy $NT4_TARGET 0
  StrCpy $SchedulerNETinstalled 0
  StrCpy $DocsInstalled 0
  StrCpy $PortableInstalled 0
  StrCpy $RunPortable 0
  StrCpy $ShowBootsplash 1
  StrCpy $IsInstalled 0
  StrCpy $LanguagePack "English"

  ClearErrors
  ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
  IfErrors 0 winnt
  MessageBox MB_OK|MB_ICONEXCLAMATION \
   "On Windows 9.x this program is absolutely unuseful!" \
   /SD IDOK
abort_inst:
!insertmacro EnableX64FSRedirection
  pop $R1
  pop $R0
  Abort
winnt:
  StrCpy $R1 $R0 1
  StrCmp $R1 '3' 0 winnt_456
  MessageBox MB_OK|MB_ICONEXCLAMATION \
   "On Windows NT 3.x this program is absolutely unuseful!" \
   /SD IDOK
  goto abort_inst
winnt_456:
  StrCmp $R1 '4' 0 winnt_56
  StrCpy $NT4_TARGET 1
winnt_56:
  
  /* is already installed? */
  IfFileExists "$SYSDIR\defrag_native.exe" 0 not_installed
  StrCpy $IsInstalled 1
not_installed:
  /* portable package? */
  ; if silent key specified than continue installation
  IfSilent init_ok
  IfFileExists "$EXEDIR\PORTABLE.X" 0 init_ok
  StrCpy $RunPortable 1
  ReadINIStr $ShowBootsplash "$EXEDIR\PORTABLE.X" "Bootsplash" "Show"
init_ok:
  call ShowBootSplash
  StrCmp $RunPortable '1' 0 init_done
  call PortableRun
  goto abort_inst
init_done:
!insertmacro EnableX64FSRedirection
!ifdef MODERN_UI
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"
!endif
  pop $R1
  pop $R0

FunctionEnd

;-----------------------------------------

Function LangShow

  push $R0
  
!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "LanguagePack Packs" \
      "Choose which LanguagePack pack you want to install."
!endif
  SetOutPath $PLUGINSDIR
  File "lang.ini"
  InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\lang.ini"
  pop $R0
  InstallOptions::show
  pop $R0

  pop $R0
  Abort

FunctionEnd

;-----------------------------------------

Function LangLeave

  push $R0

  ReadINIStr $R0 "$PLUGINSDIR\lang.ini" "Settings" "State"
  StrCmp $R0 0 exit_notify_handler
  pop $R0
  Abort

exit_notify_handler:
  ReadINIStr $LanguagePack "$PLUGINSDIR\lang.ini" "Field 2" "State"
  pop $R0

FunctionEnd

;-----------------------------------------

Function install_driver

  StrCpy $R0 "$SYSDIR\Drivers"
  SetOutPath $R0
  StrCmp $NT4_TARGET '1' 0 modern_win
  DetailPrint "NT 4.0 version"
!if ${ULTRADFGARCH} == 'i386'
  File "ultradfg_nt4.sys"
!else
  File /nonfatal "ultradfg_nt4.sys"
!endif
  Delete "$R0\ultradfg.sys"
  Rename "ultradfg_nt4.sys" "ultradfg.sys"
  goto driver_installed
modern_win:
  File "ultradfg.sys"
driver_installed:

FunctionEnd

;-----------------------------------------

Function ShowBootSplash

  push $R0
  push $R1

  IfSilent splash_done
!insertmacro EnableX64FSRedirection
  /* show bootsplash */
  InitPluginsDir
  SetOutPath $PLUGINSDIR
  StrCmp $RunPortable '1' 0 show_general_splash
  StrCmp $ShowBootsplash '1' 0 splash_done
  File "${ROOTDIR}\src\installer\PortableUltraDefrag.bmp"
  advsplash::show 2000 400 0 -1 "$PLUGINSDIR\PortableUltraDefrag"
  pop $R0
  Delete "$PLUGINSDIR\PortableUltraDefrag.bmp"
  goto splash_done
show_general_splash:
  File "${ROOTDIR}\src\installer\UltraDefrag.bmp"
  advsplash::show 2000 400 0 -1 "$PLUGINSDIR\UltraDefrag"
  pop $R0
  Delete "$PLUGINSDIR\UltraDefrag.bmp"
splash_done:
!insertmacro DisableX64FSRedirection
  pop $R1
  pop $R0

FunctionEnd

;-----------------------------------------

Function PortableRun

;;!insertmacro DisableX64FSRedirection
  ; perform silent installation
  ExecWait '"$EXEPATH" /S'
  ; start ultradefrag gui
  ExecWait "$INSTDIR\dfrg.exe"
  ; uninstall if necessary
  StrCmp $IsInstalled '1' portable_done 0
  ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"
portable_done:
;;!insertmacro EnableX64FSRedirection

FunctionEnd

;-----------------------------------------

Function WriteDriverSettings

  push $R2
  push $R3
  ; write settings only if control set exists
  StrCmp $R0 "SYSTEM\CurrentControlSet" write_settings 0
  StrCpy $R3 0
  EnumRegKey $R2 HKLM $R0 $R3
  StrCmp $R2 "" L2 0
write_settings:
  WriteRegStr HKLM "$R0\Services\ultradfg" "DisplayName" "ultradfg"
  WriteRegDWORD HKLM "$R0\Services\ultradfg" "ErrorControl" 0x0
  WriteRegExpandStr HKLM "$R0\Services\ultradfg" "ImagePath" "System32\DRIVERS\ultradfg.sys"
  WriteRegDWORD HKLM "$R0\Services\ultradfg" "Start" 0x3
  WriteRegDWORD HKLM "$R0\Services\ultradfg" "Type" 0x1
L2:
  pop $R3
  pop $R2

FunctionEnd

;-----------------------------------------

Function WriteDbgSettings

  push $R2
  ; write only if the key exists
  ClearErrors
  ReadRegDWORD $R2 HKLM $R0 "AutoReboot"
  IfErrors L1 0
  WriteRegDWORD HKLM $R0 "AutoReboot" 0x0
L1:
  pop $R2

FunctionEnd

;-----------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0
  push $R1

  SectionIn RO
  AddSize 44 /* for the components installed in system directories */
!insertmacro DisableX64FSRedirection
  DetailPrint "Install core files..."
  SetOutPath $INSTDIR
  File "Dfrg.exe"
  File "${ROOTDIR}\src\LICENSE.TXT"
  File "${ROOTDIR}\src\CREDITS.TXT"
  File "${ROOTDIR}\src\HISTORY.TXT"
  File "${ROOTDIR}\src\INSTALL.TXT"
  File "${ROOTDIR}\src\README.TXT"
  File "${ROOTDIR}\src\FAQ.TXT"
  SetOutPath "$INSTDIR\scripts"
  File "${ROOTDIR}\src\scripts\udctxhandler.lua"
  File "${ROOTDIR}\src\scripts\udreportcnv.lua"
  File "${ROOTDIR}\src\scripts\udsorting.js"
  SetOutPath "$INSTDIR\options"
  IfFileExists "$INSTDIR\options\udreportopts.lua" skip_opts 0
  File "${ROOTDIR}\src\scripts\udreportopts.lua"
skip_opts:
  ;;File "${ROOTDIR}\doc\html\images\powered_by_lua.png"
  SetOutPath "$INSTDIR\doc"
  ;File "${ROOTDIR}\doc\html\about.html"
  ;File "${ROOTDIR}\doc\html\about_simple.html"

  ; save %windir% path
  ClearErrors
  WriteRegStr HKLM "SYSTEM\UltraDefrag" "WindowsDirectory" $WINDIR
  IfErrors 0 L10
  MessageBox MB_OK|MB_ICONEXCLAMATION \
   "Can't save %windir% in registry!" \
   /SD IDOK
L10:

  ; install LanguagePack pack
  SetOutPath $INSTDIR
  Delete "$INSTDIR\ud_i18n.lng"
  StrCmp $LanguagePack "English" langpack_installed 0
  StrCmp $LanguagePack "Hungarian" hu_pack 0
  StrCmp $LanguagePack "Russian" ru_pack 0
  GoTo langpack_installed
hu_pack:
  File "${ROOTDIR}\src\gui\i18n\Hungarian.lng"
  Rename "Hungarian.lng" "ud_i18n.lng"
  GoTo langpack_installed
ru_pack:
  File "${ROOTDIR}\src\gui\i18n\Russian.lng"
  Rename "Russian.lng" "ud_i18n.lng"
  GoTo langpack_installed
langpack_installed:

  ;;Delete "$INSTDIR\defrag.exe"
  ;;Delete "$INSTDIR\defrag_native.exe" /* from previous 1.0.x installation */

  SetOutPath "$INSTDIR\presets"
  File "${ROOTDIR}\src\presets\standard"
  File "${ROOTDIR}\src\presets\system"

  DetailPrint "Install driver..."
  call install_driver

  DetailPrint "Install boot time defragger..."
  SetOutPath "$SYSDIR"
  File "udefrag.dll"
  File "zenwinx.dll"
  File "defrag_native.exe"
  DetailPrint "Install console interface..."
  File "udefrag.exe"

!if ${ULTRADFGARCH} == 'i386'
  WriteRegStr HKCR ".luar" "" "LuaReport"
  WriteRegStr HKCR "LuaReport" "" "Lua Report"
  WriteRegStr HKCR "LuaReport\DefaultIcon" "" "$SYSDIR\lua5.1a_gui.exe,1"
  WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
  WriteRegStr HKCR "LuaReport\shell\view\command" "" "$SYSDIR\lua5.1a_gui.exe $INSTDIR\scripts\udreportcnv.lua %1 $WINDIR -v"
!else
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are used $SYSDIR it was converted into C:\WINDOWS\SysWow64 by system.
  WriteRegStr HKCR ".luar" "" "LuaReport"
  WriteRegStr HKCR "LuaReport" "" "Lua Report"
  WriteRegStr HKCR "LuaReport\DefaultIcon" "" "lua5.1a_gui.exe,1"
  WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
  WriteRegStr HKCR "LuaReport\shell\view\command" "" "lua5.1a_gui.exe $INSTDIR\scripts\udreportcnv.lua %1 $WINDIR -v"
!endif

  DetailPrint "Install Lua 5.1 ..."
  File "lua5.1a.dll"
  File "lua5.1a.exe"
  File "lua5.1a_gui.exe"

  ClearErrors
  ReadRegStr $R0 HKCR ".lua" ""
  IfErrors register_lua 0

  ; fixes 1.3.1 & 1.3.2 x64 bug
  ReadRegStr $R0 HKCR "Lua\shell\open\command" ""
  ;If $R0 contains "SysWow64" 0 lua_registered
  push $R0
  push "SysWow64"
  call StrStr
  pop $R0
  StrCpy $R1 $R0 8
  StrCmp $R1 "SysWow64" 0 lua_registered

register_lua:
  WriteRegStr HKCR ".lua" "" "Lua"
  WriteRegStr HKCR "Lua" "" "Lua Program"
  WriteRegStr HKCR "Lua\shell\open" "" "Open"
  ;DeleteRegKey HKCR "Lua\shell\open\command"
!if ${ULTRADFGARCH} == 'i386'
  WriteRegStr HKCR "Lua\shell\open\command" "" "$SYSDIR\notepad.exe %1"
!else
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are used $SYSDIR it was converted into C:\WINDOWS\SysWow64 by system.
  WriteRegStr HKCR "Lua\shell\open\command" "" "notepad.exe %1"
!endif
lua_registered:

  ; register lng file extension
  ClearErrors
  ReadRegStr $R0 HKCR ".lng" ""
  IfErrors 0 lng_registered
  WriteRegStr HKCR ".lng" "" "LanguagePack"
  WriteRegStr HKCR "LanguagePack" "" "Language Pack"
  WriteRegStr HKCR "LanguagePack\shell\open" "" "Open"
  WriteRegStr HKCR "LanguagePack\DefaultIcon" "" "shell32.dll,0"
!if ${ULTRADFGARCH} == 'i386'
  WriteRegStr HKCR "LanguagePack\shell\open\command" "" "$SYSDIR\notepad.exe %1"
!else
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are used $SYSDIR it was converted into C:\WINDOWS\SysWow64 by system.
  WriteRegStr HKCR "LanguagePack\shell\open\command" "" "notepad.exe %1"
!endif
lng_registered:

  ; register cfg file extension
  ClearErrors
  ReadRegStr $R0 HKCR ".cfg" ""
  IfErrors 0 cfg_registered
  WriteRegStr HKCR ".cfg" "" "ConfigFile"
  WriteRegStr HKCR "ConfigFile" "" "Configuration File"
  WriteRegStr HKCR "ConfigFile\shell\open" "" "Open"
  WriteRegStr HKCR "ConfigFile\DefaultIcon" "" "shell32.dll,0"
!if ${ULTRADFGARCH} == 'i386'
  WriteRegStr HKCR "ConfigFile\shell\open\command" "" "$SYSDIR\notepad.exe %1"
!else
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are used $SYSDIR it was converted into C:\WINDOWS\SysWow64 by system.
  WriteRegStr HKCR "ConfigFile\shell\open\command" "" "notepad.exe %1"
!endif
cfg_registered:

  DetailPrint "Write driver settings..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "SYSTEM\CurrentControlSet"
  call WriteDriverSettings
  StrCpy $R0 "SYSTEM\ControlSet001"
  call WriteDriverSettings
  StrCpy $R0 "SYSTEM\ControlSet002"
  call WriteDriverSettings
  StrCpy $R0 "SYSTEM\ControlSet003"
  call WriteDriverSettings

  DetailPrint "Write debugging settings..."
  StrCpy $R0 "SYSTEM\CurrentControlSet\Control\CrashControl"
  call WriteDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet001\Control\CrashControl"
  call WriteDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet002\Control\CrashControl"
  call WriteDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet003\Control\CrashControl"
  call WriteDbgSettings

  DetailPrint "Write the uninstall keys..."
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegStr HKLM  $R0 "DisplayName" "Ultra Defragmenter"
  WriteRegStr HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  ; remove files of previous 1.3.1-1.3.3 installation
  RMDir /r "$SYSDIR\UltraDefrag"
  Delete "$INSTDIR\doc\about.html"
  Delete "$INSTDIR\doc\about_simple.html"
  Delete "$INSTDIR\doc\images\about.png"
  Delete "$INSTDIR\doc\images\console.png"
  Delete "$INSTDIR\doc\images\main_screen110.png"

  ; create configuration file if it doesn't exist
  ExecWait '"$INSTDIR\Dfrg.exe" /I'

!insertmacro EnableX64FSRedirection

  pop $R1
  pop $R0

SectionEnd

Section "Scheduler.NET" SecSchedNET

!insertmacro DisableX64FSRedirection
  DetailPrint "Install Scheduler.NET..."
  SetOutPath $INSTDIR
  File "UltraDefragScheduler.NET.exe"
  StrCpy $SchedulerNETinstalled 1
!insertmacro EnableX64FSRedirection

SectionEnd

Section "Documentation" SecDocs

!insertmacro DisableX64FSRedirection
  DetailPrint "Install documentation..."
  SetOutPath "$INSTDIR\doc"
  File "${ROOTDIR}\doc\html\manual.html"
  File "${ROOTDIR}\doc\html\udefrag.css"
  SetOutPath "$INSTDIR\doc\images"
  ;File "${ROOTDIR}\doc\html\images\console.png"
  File "${ROOTDIR}\doc\html\images\main_screen140.png"
  ;File "${ROOTDIR}\doc\html\images\about.png"
  File "${ROOTDIR}\doc\html\images\sched_net_vista.png"
  File "${ROOTDIR}\doc\html\images\valid-html401.png"
  File "${ROOTDIR}\doc\html\images\powered_by_lua.png"
  StrCpy $DocsInstalled 1
!insertmacro EnableX64FSRedirection

SectionEnd

Section "Portable UltraDefrag package" SecPortable

  push $R0
  
!insertmacro DisableX64FSRedirection
  DetailPrint "Build portable package..."
  StrCpy $R0 "$INSTDIR\portable_${ULTRADFGARCH}_package"
  CreateDirectory $R0
  CopyFiles /SILENT $EXEPATH $R0 200
  WriteINIStr "$R0\PORTABLE.X" "Bootsplash" "Show" "1"
  WriteINIStr "$R0\NOTES.TXT" "General" "Usage" \
    "Put this directory contents to your USB drive and enjoy!"
  StrCpy $PortableInstalled 1
!insertmacro EnableX64FSRedirection

  pop $R0
  
SectionEnd

Section "Context menu handler" SecContextMenuHandler

  WriteRegStr HKCR "Drive\shell\udefrag" "" "[--- &Ultra Defragmenter ---]"
!if ${ULTRADFGARCH} == 'i386'
  WriteRegStr HKCR "Drive\shell\udefrag\command" "" "$SYSDIR\lua5.1a.exe $INSTDIR\scripts\udctxhandler.lua %1"
!else
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are used $SYSDIR it was converted into C:\WINDOWS\SysWow64 by system.
  WriteRegStr HKCR "Drive\shell\udefrag\command" "" "lua5.1a.exe $INSTDIR\scripts\udctxhandler.lua %1"
!endif

SectionEnd

Section "Shortcuts" SecShortcuts

  push $R0
  AddSize 5

!insertmacro DisableX64FSRedirection
  DetailPrint "Install shortcuts..."
  SetShellVarContext all
  SetOutPath $INSTDIR
  StrCpy $R0 "$SMPROGRAMS\DASoft\UltraDefrag"
  CreateDirectory $R0
  CreateShortCut "$R0\UltraDefrag.lnk" \
   "$INSTDIR\Dfrg.exe"
  CreateShortCut "$R0\LICENSE.lnk" \
   "$INSTDIR\LICENSE.TXT"
  CreateShortCut "$R0\README.lnk" \
   "$INSTDIR\README.TXT"
  CreateShortCut "$R0\FAQ.lnk" \
   "$INSTDIR\FAQ.TXT"
  StrCmp $SchedulerNETinstalled '1' 0 no_sched_net
  CreateShortCut "$R0\Scheduler.NET.lnk" \
   "$INSTDIR\UltraDefragScheduler.NET.exe"
no_sched_net:
  StrCmp $DocsInstalled '1' 0 no_docs
  WriteINIStr "$R0\User manual.url" "InternetShortcut" "URL" "file://$INSTDIR\doc\manual.html"
  goto doc_url_ok
no_docs:
  WriteINIStr "$R0\User manual.url" "InternetShortcut" "URL" "http://ultradefrag.sourceforge.net/manual.html"
doc_url_ok:
  CreateShortCut "$R0\Uninstall UltraDefrag.lnk" \
   "$INSTDIR\uninstall.exe"
  CreateShortCut "$DESKTOP\UltraDefrag.lnk" \
   "$INSTDIR\Dfrg.exe"
  WriteINIStr "$R0\Homepage.url" "InternetShortcut" "URL" "http://ultradefrag.sourceforge.net/"
  StrCmp $PortableInstalled '1' 0 no_portable
  CreateShortCut "$R0\Portable package.lnk" \
   "$INSTDIR\portable_${ULTRADFGARCH}_package"
no_portable:
  CreateShortcut "$QUICKLAUNCH\UltraDefrag.lnk" \
   "$INSTDIR\Dfrg.exe"
!insertmacro EnableX64FSRedirection

  pop $R0
  
SectionEnd

Section "Uninstall"

!insertmacro DisableX64FSRedirection
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"

  DetailPrint "Remove shortcuts..."
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\DASoft\UltraDefrag"
  RMDir "$SMPROGRAMS\DASoft"
  Delete "$DESKTOP\UltraDefrag.lnk"
  Delete "$QUICKLAUNCH\UltraDefrag.lnk"

  DetailPrint "Remove program files..."
  /* remove unuseful registry settings */
  ExecWait '"$INSTDIR\Dfrg.exe" /U'
  Delete "$INSTDIR\Dfrg.exe"
  Delete "$INSTDIR\LICENSE.TXT"
  Delete "$INSTDIR\CREDITS.TXT"
  Delete "$INSTDIR\HISTORY.TXT"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\README.TXT"
  Delete "$INSTDIR\FAQ.TXT"
  Delete "$INSTDIR\scripts\udctxhandler.lua"
  Delete "$INSTDIR\scripts\udreportcnv.lua"
  Delete "$INSTDIR\scripts\udsorting.js"
  RMDir "$INSTDIR\scripts"
  ;;;Delete "$INSTDIR\options\udreportopts.lua"
  RMDir "$INSTDIR\options"
  Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
  Delete "$INSTDIR\uninstall.exe"
  Delete "$INSTDIR\presets\standard"
  Delete "$INSTDIR\presets\system"
  RMDir "$INSTDIR\presets"
  Delete "$INSTDIR\ud_i18n.lng"
  Delete "$INSTDIR\ud_i18n.dll"
  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\portable_${ULTRADFGARCH}_package"
  RMDir $INSTDIR

  DetailPrint "Uninstall driver and boot time defragger..."
  Delete "$SYSDIR\Drivers\ultradfg.sys"
  Delete "$SYSDIR\defrag_native.exe"
  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\zenwinx.dll"
  Delete "$SYSDIR\udefrag.exe"
  Delete "$SYSDIR\lua5.1a.dll"
  Delete "$SYSDIR\lua5.1a.exe"
  Delete "$SYSDIR\lua5.1a_gui.exe"

  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"

  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  ; remove settings of previous versions
  ;DeleteRegKey HKCU "SOFTWARE\DASoft\NTDefrag"
  ;DeleteRegKey /ifempty HKCU "Software\DASoft"
  ;DeleteRegKey HKLM "Software\DASoft\NTDefrag"
  ;DeleteRegKey /ifempty HKLM "Software\DASoft"
  
  DetailPrint "Uninstall the context menu handler..."
  DeleteRegKey HKCR "Drive\shell\udefrag"
  DeleteRegKey HKCR "LuaReport"
  DeleteRegKey HKCR ".luar"
!insertmacro EnableX64FSRedirection

SectionEnd

;----------------------------------------------

!ifdef MODERN_UI
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The core files required to use UltraDefrag.$\nIncluding console interface."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSchedNET} "Small and useful scheduler.$\nNET Framework 2.0 required."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} "User manual."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPortable} "Build portable package to place them on USB drive."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContextMenuHandler} "Defragment your volumes from their context menu."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "Adds icons to your start menu and your desktop for easy access."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif

;---------------------------------------------

; Additional functions from NSIS User Manual

; StrStr
 ; input, top of stack = string to search for
 ;        top of stack-1 = string to search in
 ; output, top of stack (replaces with the portion of the string remaining)
 ; modifies no other variables.
 ;
 ; Usage:
 ;   Push "this is a long ass string"
 ;   Push "ass"
 ;   Call StrStr
 ;   Pop $R0
 ;  ($R0 at this point is "ass string")

 Function StrStr
   Exch $R1 ; st=haystack,old$R1, $R1=needle
   Exch    ; st=old$R1,haystack
   Exch $R2 ; st=old$R1,old$R2, $R2=haystack
   Push $R3
   Push $R4
   Push $R5
   StrLen $R3 $R1
   StrCpy $R4 0
   ; $R1=needle
   ; $R2=haystack
   ; $R3=len(needle)
   ; $R4=cnt
   ; $R5=tmp
   loop:
     StrCpy $R5 $R2 $R3 $R4
     StrCmp $R5 $R1 done
     StrCmp $R5 "" done
     IntOp $R4 $R4 + 1
     Goto loop
 done:
   StrCpy $R1 $R2 "" $R4
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Exch $R1
 FunctionEnd

;---------------------------------------------

/* EOF */
