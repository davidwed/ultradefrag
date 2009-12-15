/*
 *  ULTRADEFRAG - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by D. Arkhangelski (dmitriar@gmail.com).
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
* Installer source.
*/

/*
*  NOTE: The following symbols should be defined
*        through makensis command line:
*  ULTRADFGVER=<version number in form x.y.z>
*  ULTRADFGARCH=<i386 | amd64 | ia64>
*/

!ifndef ULTRADFGVER | ULTRADFGARCH
!error "One of the predefined symbols missing!"
!endif

!define MODERN_UI

!include "WinVer.nsh"
!include "x64.nsh"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

!include ".\UltraDefrag.nsh"

!ifndef MODERN_UI
!system 'move /Y lang-classical.ini lang.ini'
!endif

;-----------------------------------------

!ifdef MODERN_UI
  !include "MUI.nsh"
!endif

!macro LANG_PAGE
  Page custom LangShow LangLeave ""
!macroend

!macro DRIVER_PAGE
  Page custom DriverShow DriverLeave ""
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
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

XPStyle on
RequestExecutionLevel admin

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2009 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

ReserveFile "lang.ini"

ReserveFile "driver.ini"

!ifdef MODERN_UI
  !define MUI_COMPONENTSPAGE_SMALLDESC

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "${ROOTDIR}\src\LICENSE.TXT"
  !insertmacro MUI_PAGE_COMPONENTS
;  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro LANG_PAGE
  !insertmacro DRIVER_PAGE
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
  !insertmacro DRIVER_PAGE
  Page instfiles

  UninstPage uninstConfirm
  UninstPage instfiles
!endif

;-----------------------------------------

Var ShowBootsplash
Var LanguagePack
Var UserModeDriver

;-----------------------------------------

Function DriverShow

  push $R0

!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "Preferred Driver" \
      "Select which driver do you prefer."
!endif
  SetOutPath $PLUGINSDIR
  File "driver.ini"

  ClearErrors
  ReadRegStr $R0 HKLM "Software\UltraDefrag" "UserModeDriver"
  ${Unless} ${Errors}
    WriteINIStr "$PLUGINSDIR\driver.ini" "Field 1" "State" $R0
  ${EndUnless}

  InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\driver.ini"
  pop $R0
  InstallOptions::show
  pop $R0

  pop $R0
  Abort

FunctionEnd

;-----------------------------------------

Function DriverLeave

  push $R0

  ReadINIStr $R0 "$PLUGINSDIR\driver.ini" "Settings" "State"
  ${If} $R0 != "0"
    pop $R0
    Abort
  ${EndIf}

  ReadINIStr $UserModeDriver "$PLUGINSDIR\driver.ini" "Field 1" "State"
  WriteRegStr HKLM "Software\UltraDefrag" "UserModeDriver" $UserModeDriver
  pop $R0

FunctionEnd

;-----------------------------------------

Function LangShow

  push $R0
  
!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "Language Packs" \
      "Choose which language pack you want to install."
!endif
  SetOutPath $PLUGINSDIR
  File "lang.ini"
!ifdef MODERN_UI
  File "LanguageSelectorSmall.bmp"
!endif

  ClearErrors
  ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
  ${Unless} ${Errors}
    WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $R0
  ${EndUnless}

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
  ${If} $R0 != "0"
    pop $R0
    Abort
  ${EndIf}

  ReadINIStr $LanguagePack "$PLUGINSDIR\lang.ini" "Field 2" "State"
  WriteRegStr HKLM "Software\UltraDefrag" "Language" $LanguagePack
  pop $R0

FunctionEnd

;-----------------------------------------

Function ShowBootSplash

  ${Unless} ${Silent}
  ${AndUnless} $ShowBootsplash == "0"
    push $R0
    ${EnableX64FSRedirection}
    SetOutPath $PLUGINSDIR
    File "${ROOTDIR}\src\installer\UltraDefrag.bmp"
    advsplash::show 2000 400 0 -1 "$PLUGINSDIR\UltraDefrag.bmp"
    pop $R0
    Delete "$PLUGINSDIR\*.bmp"
    ${DisableX64FSRedirection}
    pop $R0
  ${EndUnless}

FunctionEnd

;-----------------------------------------

Function install_langpack

  push $R0
  push $R1

  SetOutPath $INSTDIR
  Delete "$INSTDIR\ud_i18n.lng"
  Delete "$INSTDIR\ud_config_i18n.lng"
  Delete "$INSTDIR\ud_scheduler_i18n.lng"

  ${If} $LanguagePack != "English (US)"
    StrCpy $R1 $LanguagePack
    StrCpy $R0 "$R1.lng"
    ${Select} $LanguagePack
      ${Case} "Chinese (Simplified)"
        StrCpy $R0 "Chinese(Simp).lng"
      ${Case} "Chinese (Traditional)"
        StrCpy $R0 "Chinese(Trad).lng"
      ${Case} "French (FR)"
        StrCpy $R0 "French(FR).lng"
      ${Case} "Portuguese (BR)"
        StrCpy $R0 "Portuguese(BR).lng"
      ${Case} "Spanish (AR)"
        StrCpy $R0 "Spanish(AR).lng"
    ${EndSelect}

    File "${ROOTDIR}\src\gui\i18n\*.lng"
    Rename "$R0" "ud_i18n.bk"
    Delete "$INSTDIR\*.lng"
    ;;;Rename "ud_i18n.bk" "ud_i18n.lng"

    File "${ROOTDIR}\src\udefrag-gui-config\i18n\*.lng"
    Rename "$R0" "ud_config_i18n.bk"
    Delete "$INSTDIR\*.lng"

    File "${ROOTDIR}\src\scheduler\C\i18n\*.lng"
    Rename "$R0" "ud_scheduler_i18n.bk"
    Delete "$INSTDIR\*.lng"

    Rename "ud_scheduler_i18n.bk" "ud_scheduler_i18n.lng"
    Rename "ud_config_i18n.bk" "ud_config_i18n.lng"
    Rename "ud_i18n.bk" "ud_i18n.lng"
  ${EndIf}

  pop $R1
  pop $R0
  
FunctionEnd

;------------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0

  SectionIn RO
  AddSize 24 /* for the components installed in system directories (driver) */

  DetailPrint "Install core files..."
  ${DisableX64FSRedirection}
  SetOutPath $INSTDIR
  File "${ROOTDIR}\src\LICENSE.TXT"
  File "${ROOTDIR}\src\CREDITS.TXT"
  File "${ROOTDIR}\src\HISTORY.TXT"
  File "${ROOTDIR}\src\README.TXT"
  SetOutPath "$INSTDIR\scripts"
  File "${ROOTDIR}\src\scripts\udreportcnv.lua"
  File "${ROOTDIR}\src\scripts\udsorting.js"
  SetOutPath "$INSTDIR\options"
  ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua"
    File "${ROOTDIR}\src\scripts\udreportopts.lua"
  ${EndUnless}

  ; install LanguagePack
  call install_langpack

  ${If} $UserModeDriver == '1'
    Delete "$SYSDIR\Drivers\ultradfg.sys"
  ${Else}
    SetOutPath "$SYSDIR\Drivers"
    File /nonfatal "ultradfg.sys"
  ${EndIf}

  ; install GUI apps to program's directory
  SetOutPath "$INSTDIR"
  File "dfrg.exe"
  Delete "$INSTDIR\ultradefrag.exe"
  Rename "$INSTDIR\dfrg.exe" "$INSTDIR\ultradefrag.exe"
  File "udefrag-gui-config.exe"

  SetOutPath "$SYSDIR"
  File "${ROOTDIR}\src\installer\boot-config.cmd"
  File "${ROOTDIR}\src\installer\boot-off.cmd"
  File "${ROOTDIR}\src\installer\boot-on.cmd"
  File "bootexctrl.exe"
  File "defrag_native.exe"
  File "lua5.1a.dll"
  File "lua5.1a.exe"
  File "lua5.1a_gui.exe"
  File "${ROOTDIR}\src\installer\ud-help.cmd"
  File "${ROOTDIR}\src\installer\udctxhandler.cmd"
  File "udefrag-kernel.dll"
  File "udefrag.dll"
  File "udefrag.exe"
  File "zenwinx.dll"
  File /oname=hibernate4win.exe "hibernate.exe"
  File "wgx.dll"

  DetailPrint "Register file extensions..."
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are using $SYSDIR Windows always converts them to C:\WINDOWS\SysWow64.

  WriteRegStr HKCR ".luar" "" "LuaReport"
  WriteRegStr HKCR "LuaReport" "" "Lua Report"
  WriteRegStr HKCR "LuaReport\DefaultIcon" "" "lua5.1a_gui.exe,1"
  WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
  WriteRegStr HKCR "LuaReport\shell\view\command" "" "lua5.1a_gui.exe $INSTDIR\scripts\udreportcnv.lua %1 $WINDIR -v"

  ClearErrors
  ReadRegStr $R0 HKCR ".lua" ""
  ${If} ${Errors}
    WriteRegStr HKCR ".lua" "" "Lua"
    WriteRegStr HKCR "Lua" "" "Lua Program"
    WriteRegStr HKCR "Lua\shell\open" "" "Open"
    WriteRegStr HKCR "Lua\shell\open\command" "" "notepad.exe %1"
  ${EndIf}

  ClearErrors
  ReadRegStr $R0 HKCR ".lng" ""
  ${If} ${Errors}
    WriteRegStr HKCR ".lng" "" "LanguagePack"
    WriteRegStr HKCR "LanguagePack" "" "Language Pack"
    WriteRegStr HKCR "LanguagePack\shell\open" "" "Open"
    WriteRegStr HKCR "LanguagePack\DefaultIcon" "" "shell32.dll,0"
    WriteRegStr HKCR "LanguagePack\shell\open\command" "" "notepad.exe %1"
  ${EndIf}

  DetailPrint "Write driver settings..."
  ${WriteDriverAndDbgSettings}

  DetailPrint "Write the uninstall keys..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegStr HKLM  $R0 "DisplayName" "Ultra Defragmenter"
  WriteRegStr HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  ; remove files of previous installations
  RMDir /r "$SYSDIR\UltraDefrag"
  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\presets"
  Delete "$INSTDIR\dfrg.exe"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\FAQ.TXT"
  Delete "$INSTDIR\scripts\udctxhandler.lua"
  Delete "$SYSDIR\udefrag-gui-dbg.cmd"
  DeleteRegKey HKLM "SYSTEM\UltraDefrag"
  Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
  Delete "$SYSDIR\udefrag-gui.exe"
  Delete "$SYSDIR\udefrag-gui.cmd"
  Delete "$SYSDIR\ultradefrag.exe"
  Delete "$SYSDIR\udefrag-gui-config.exe"
  Delete "$SYSDIR\udefrag-scheduler.exe"
  RMDir /r "$INSTDIR\logs"
  RMDir /r "$INSTDIR\portable_${ULTRADFGARCH}_package"

  ; create boot time script if it doesn't exists
  SetOutPath "$SYSDIR"
  ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
    File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
  ${EndUnless}
  
  ; write default GUI settings to guiopts.lua file
  SetOutPath "$INSTDIR"
  ExecWait '"$INSTDIR\ultradefrag.exe" --setup'

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

Section "Documentation" SecDocs

  DetailPrint "Install documentation..."
  ${DisableX64FSRedirection}
  SetOutPath "$INSTDIR\handbook"
  File "${ROOTDIR}\doc\html\handbook\*.*"
  ${EnableX64FSRedirection}

SectionEnd

/*Section /o "Scheduler.NET" SecSchedNET

  DetailPrint "Install Scheduler.NET..."
  ${DisableX64FSRedirection}
  SetOutPath $INSTDIR
  File "UltraDefragScheduler.NET.exe"
  ${EnableX64FSRedirection}

SectionEnd
*/
Section "Scheduler" SecScheduler

  DetailPrint "Install Scheduler..."
  ${DisableX64FSRedirection}
  SetOutPath "$INSTDIR"
  File "udefrag-scheduler.exe"
  ${EnableX64FSRedirection}

SectionEnd

Section "Context menu handler" SecContextMenuHandler

  ${SetContextMenuHandler}
  
SectionEnd

Section "Shortcuts" SecShortcuts

  push $R0
  AddSize 5

  DetailPrint "Install shortcuts..."
  ${DisableX64FSRedirection}
  SetShellVarContext all
  SetOutPath $INSTDIR

  ; remove shortcuts of any previous version of the program
  RMDir /r "$SMPROGRAMS\DASoft"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\FAQ.lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\User manual.url"
  Delete "$SMPROGRAMS\UltraDefrag\UltraDefrag (Debug mode).lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Portable package.lnk"

  RMDir /r "$SMPROGRAMS\UltraDefrag\Boot time options"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Preferences"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Debugging information"

  StrCpy $R0 "$SMPROGRAMS\UltraDefrag"
  CreateDirectory $R0
  CreateDirectory "$R0\Documentation"

  CreateShortCut "$R0\UltraDefrag.lnk" \
   "$INSTDIR\ultradefrag.exe"

  CreateShortCut "$R0\Preferences.lnk" \
   "$INSTDIR\udefrag-gui-config.exe"

  CreateShortCut "$R0\Documentation\LICENSE.lnk" \
   "$INSTDIR\LICENSE.TXT"
  CreateShortCut "$R0\Documentation\README.lnk" \
   "$INSTDIR\README.TXT"

/*  ${If} ${FileExists} "$INSTDIR\UltraDefragScheduler.NET.exe"
    CreateShortCut "$R0\Scheduler.NET.lnk" \
     "$INSTDIR\UltraDefragScheduler.NET.exe"
  ${EndIf}
*/
  Delete "$R0\Scheduler.NET.lnk"
  ${If} ${FileExists} "$INSTDIR\udefrag-scheduler.exe"
    CreateShortCut "$R0\Scheduler.lnk" \
     "$INSTDIR\udefrag-scheduler.exe"
  ${EndIf}

  ${If} ${FileExists} "$INSTDIR\handbook\index.html"
    WriteINIStr "$R0\Documentation\Handbook.url" "InternetShortcut" "URL" "file://$INSTDIR\handbook\index.html"
    WriteINIStr "$R0\Documentation\FAQ.url" "InternetShortcut" "URL" "file://$INSTDIR\handbook\faq.html"
  ${Else}
    WriteINIStr "$R0\Documentation\Handbook.url" "InternetShortcut" "URL" "http://ultradefrag.sourceforge.net/handbook/"
    WriteINIStr "$R0\Documentation\FAQ.url" "InternetShortcut" "URL" "http://ultradefrag.sourceforge.net/handbook/faq.html"
  ${EndIf}
  WriteINIStr "$R0\Documentation\Homepage.url" "InternetShortcut" "URL" "http://ultradefrag.sourceforge.net/"

  CreateShortCut "$R0\Uninstall UltraDefrag.lnk" \
   "$INSTDIR\uninstall.exe"

  CreateShortCut "$DESKTOP\UltraDefrag.lnk" \
   "$INSTDIR\ultradefrag.exe"
  CreateShortcut "$QUICKLAUNCH\UltraDefrag.lnk" \
   "$INSTDIR\ultradefrag.exe"

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

;----------------------------------------------

Function .onInit

  ${CheckWinVersion}

  ${EnableX64FSRedirection}
  InitPluginsDir

  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"
  push $R1

  /* variables initialization */
  StrCpy $ShowBootsplash 1

  StrCpy $UserModeDriver 1
  ClearErrors
  ReadRegStr $R1 HKLM "Software\UltraDefrag" "UserModeDriver"
  ${Unless} ${Errors}
    StrCpy $UserModeDriver $R1
  ${EndUnless}

  StrCpy $LanguagePack "English (US)"
  ClearErrors
  ReadRegStr $R1 HKLM "Software\UltraDefrag" "Language"
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R1
  ${EndUnless}

  call ShowBootSplash

!ifdef MODERN_UI
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "driver.ini"
!endif
  pop $R1
  ${EnableX64FSRedirection}

FunctionEnd

;----------------------------------------------

Section "Uninstall"

  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"

  DetailPrint "Remove shortcuts..."
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\UltraDefrag"
  ; remove shortcuts of any previous version of the program
  RMDir /r "$SMPROGRAMS\DASoft"
  Delete "$DESKTOP\UltraDefrag.lnk"
  Delete "$QUICKLAUNCH\UltraDefrag.lnk"

  /* remove useless registry settings */
  ${Unless} ${Silent}
  ExecWait '"$SYSDIR\bootexctrl.exe" /u defrag_native'
  ${Else}
  ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'
  ${EndUnless}

  DetailPrint "Remove program files..."
  Delete "$INSTDIR\LICENSE.TXT"
  Delete "$INSTDIR\CREDITS.TXT"
  Delete "$INSTDIR\HISTORY.TXT"
  Delete "$INSTDIR\README.TXT"
  ;Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
  Delete "$INSTDIR\ultradefrag.exe"
  Delete "$INSTDIR\udefrag-gui-config.exe"
  Delete "$INSTDIR\udefrag-scheduler.exe"
  Delete "$INSTDIR\uninstall.exe"

  ; delete files from previous installations
  Delete "$INSTDIR\boot_on.cmd"
  Delete "$INSTDIR\boot_off.cmd"
  Delete "$INSTDIR\ud_i18n.lng"
  Delete "$INSTDIR\ud_config_i18n.lng"
  Delete "$INSTDIR\ud_scheduler_i18n.lng"
  Delete "$INSTDIR\ud_i18n.dll"
  RMDir /r "$INSTDIR\presets"
  Delete "$SYSDIR\ud-config.cmd"

  RMDir /r "$INSTDIR\scripts"
  RMDir /r "$INSTDIR\handbook"
  RMDir /r "$INSTDIR\portable_${ULTRADFGARCH}_package"
  RMDir "$INSTDIR\options"
  RMDir $INSTDIR

  Delete "$SYSDIR\Drivers\ultradfg.sys"
  Delete "$SYSDIR\boot-config.cmd"
  Delete "$SYSDIR\boot-off.cmd"
  Delete "$SYSDIR\boot-on.cmd"
  Delete "$SYSDIR\bootexctrl.exe"
  Delete "$SYSDIR\defrag_native.exe"
  Delete "$SYSDIR\lua5.1a.dll"
  Delete "$SYSDIR\lua5.1a.exe"
  Delete "$SYSDIR\lua5.1a_gui.exe"
  Delete "$SYSDIR\ud-help.cmd"
  Delete "$SYSDIR\udctxhandler.cmd"
  Delete "$SYSDIR\udefrag-kernel.dll"
  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\udefrag.exe"
  Delete "$SYSDIR\wgx.dll"
  Delete "$SYSDIR\zenwinx.dll"
  Delete "$SYSDIR\hibernate4win.exe"

  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"

  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  ; this is important for portable application
  ; and from "anything related to the program should be cleaned up" point of view
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Control\UltraDefrag"

  DetailPrint "Uninstall the context menu handler..."
  DeleteRegKey HKCR "Drive\shell\udefrag"
  DeleteRegKey HKCR "Folder\shell\udefrag"
  DeleteRegKey HKCR "*\shell\udefrag"

  DeleteRegKey HKCR "LuaReport"
  DeleteRegKey HKCR ".luar"
  ${EnableX64FSRedirection}

SectionEnd

;----------------------------------------------

!ifdef MODERN_UI
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The core files required to use UltraDefrag.$\nIncluding console interface."
  ;!insertmacro MUI_DESCRIPTION_TEXT ${SecSchedNET} "Small and useful scheduler.$\nNET Framework 2.0 required."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecScheduler} "Small handy scheduler."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} "Handbook."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContextMenuHandler} "Defragment your volumes from their context menu."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "Adds icons to your start menu and your desktop for easy access."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif

;---------------------------------------------

/* EOF */
