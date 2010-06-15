/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

;!define SHOW_BOOTSPLASH

!include "WinVer.nsh"
!include "x64.nsh"

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

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
  !define MUI_ICON "udefrag-install.ico"
  !define MUI_UNICON "udefrag-uninstall.ico"
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
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

Icon "udefrag-install.ico"
UninstallIcon "udefrag-uninstall.ico"

XPStyle on
RequestExecutionLevel admin

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright � 2007-2010 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

ReserveFile "lang.ini"

!ifdef MODERN_UI
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${ROOTDIR}\src\installer\WelcomePageBitmap.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${ROOTDIR}\src\installer\WelcomePageBitmap.bmp"
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

Var ShowBootsplash
Var LanguagePack

;-----------------------------------------

Function LangShow

  push $R0
  push $R1
  push $R2
  
!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "Language Packs" \
      "Choose which language pack you want to install."
!endif
  SetOutPath $PLUGINSDIR
  File "lang.ini"

  ; --- get language from registry
  ClearErrors
  ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
  ${Unless} ${Errors}
    WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $R0
  ${EndUnless}

  ; --- get language from command line
  ${GetParameters} $R1
  ClearErrors
  ${GetOptions} $R1 /LANG= $R2
  ${Unless} ${Errors}
    WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $R2
  ${EndUnless}

  InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\lang.ini"
  pop $R0
  InstallOptions::show
  pop $R0
  
  pop $R2
  pop $R1
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

!ifdef SHOW_BOOTSPLASH
  ${Unless} ${Silent}
  ${AndUnless} $ShowBootsplash == "0"
    push $R0
    ${EnableX64FSRedirection}
    SetOutPath $PLUGINSDIR
    File "${ROOTDIR}\src\installer\UltraDefrag.bmp"
    advsplash::show 2000 400 0 -1 "$PLUGINSDIR\UltraDefrag"
    pop $R0
    Delete "$PLUGINSDIR\UltraDefrag.bmp"
    ${DisableX64FSRedirection}
    pop $R0
  ${EndUnless}
!endif

FunctionEnd

;-----------------------------------------

Function install_langpack

  push $R0

  SetOutPath $INSTDIR
  Delete "$INSTDIR\ud_i18n.lng"
  Delete "$INSTDIR\ud_config_i18n.lng"

  ${If} $LanguagePack != "English (US)"
    StrCpy $R0 $LanguagePack
    ${Select} $LanguagePack
      ${Case} "Chinese (Simplified)"
        StrCpy $R0 "Chinese(Simplified)"
      ${Case} "Chinese (Traditional)"
        StrCpy $R0 "Chinese(Traditional)"
      ${Case} "Filipino (Tagalog)"
        StrCpy $R0 "Filipino(Tagalog)"
      ${Case} "French (FR)"
        StrCpy $R0 "French(FR)"
      ${Case} "Portuguese (BR)"
        StrCpy $R0 "Portuguese(BR)"
      ${Case} "Spanish (AR)"
        StrCpy $R0 "Spanish(AR)"
    ${EndSelect}

    File "${ROOTDIR}\src\gui\i18n\*.GUI"
    Rename "$R0.GUI" "ud_i18n.bk"
    Delete "$INSTDIR\*.GUI"
    ;;;Rename "ud_i18n.bk" "ud_i18n.lng"

    File "${ROOTDIR}\src\udefrag-gui-config\i18n\*.Config"
    Rename "$R0.Config" "ud_config_i18n.bk"
    Delete "$INSTDIR\*.Config"

    Rename "ud_config_i18n.bk" "ud_config_i18n.lng"
    Rename "ud_i18n.bk" "ud_i18n.lng"
  ${EndIf}

  pop $R0
  
FunctionEnd

;------------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0

  SectionIn RO
  ;AddSize 24 /* for the components installed in system directories (driver) */

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
  ${Unless} ${FileExists} "$INSTDIR\scripts\udreport.css"
    File "${ROOTDIR}\src\scripts\udreport.css"
  ${EndUnless}
  SetOutPath "$INSTDIR\options"
  ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua"
    File "${ROOTDIR}\src\scripts\udreportopts.lua"
  ${Else}
    ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua.old"
      Rename "$INSTDIR\options\udreportopts.lua" "$INSTDIR\options\udreportopts.lua.old"
      File "${ROOTDIR}\src\scripts\udreportopts.lua"
    ${EndUnless}
  ${EndUnless}

  ; install LanguagePack
  call install_langpack

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

  ${InstallNativeDefragmenter}

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
    WriteRegStr HKCR ".lua" "" "Lua.Script"
    WriteRegStr HKCR "Lua.Script" "" "Lua Script File"
    WriteRegStr HKCR "Lua.Script\shell\Edit" "" "Edit Script"
    WriteRegStr HKCR "Lua.Script\shell\Edit\command" "" "notepad.exe %1"
  ${Else}
    StrCpy $0 $R0
    ClearErrors
    ReadRegStr $R0 HKCR "$0\shell\Edit" ""
    ${If} ${Errors}
      WriteRegStr HKCR "$0\shell\Edit" "" "Edit Script"
      WriteRegStr HKCR "$0\shell\Edit\command" "" "notepad.exe %1"
    ${EndIf}
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

  DetailPrint "Write the uninstall keys..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegStr   HKLM $R0 "DisplayName"     "Ultra Defragmenter"
  WriteRegStr   HKLM $R0 "DisplayVersion"  "${ULTRADFGVER}"
  WriteRegStr   HKLM $R0 "Publisher"       "UltraDefrag Development Team"
  WriteRegStr   HKLM $R0 "URLInfoAbout"    "http://ultradefrag.sourceforge.net/"
  WriteRegStr   HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM $R0 "DisplayIcon"     '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM $R0 "InstallLocation" '"$INSTDIR"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  ${RemoveObsoleteFiles}

  ; create boot time script if it doesn't exists
  SetOutPath "$SYSDIR"
  ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
    File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
  ${EndUnless}
  
  ; write default GUI settings to guiopts.lua file
  SetOutPath "$INSTDIR"
  ExecWait '"$INSTDIR\ultradefrag.exe" --setup'

  ; set the uninstall size value
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM $R0 "EstimatedSize" "$0"

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

Section "Documentation" SecDocs

  push $R0

  DetailPrint "Install documentation..."
  ${DisableX64FSRedirection}
  ; remove the old handbook
  RMDir /r "$INSTDIR\handbook"
  SetOutPath "$INSTDIR\handbook"
  File "${ROOTDIR}\doc\html\handbook\doxy-doc\html\*.*"

  ; update the uninstall size value
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegDWORD HKLM $R0 "EstimatedSize" "$0"

  ${EnableX64FSRedirection}
  pop $R0

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
  CreateShortCut "$QUICKLAUNCH\UltraDefrag.lnk" \
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
  push $R2

  /* variables initialization */
  StrCpy $ShowBootsplash 1

  StrCpy $LanguagePack "English (US)"

  ; --- get language from registry
  ClearErrors
  ReadRegStr $R1 HKLM "Software\UltraDefrag" "Language"
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R1
  ${EndUnless}

  ; --- get language from command line
  ; --- allows silent installation with: installer.exe /S /LANG="German"
  ${GetParameters} $R1
  ClearErrors
  ${GetOptions} $R1 /LANG= $R2
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R2
  ${EndUnless}

  call ShowBootSplash

!ifdef MODERN_UI
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"
!endif
  pop $R2
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
  Delete "$INSTDIR\ultradefrag.exe"
  Delete "$INSTDIR\udefrag-gui-config.exe"
  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\ud_i18n.lng"
  Delete "$INSTDIR\ud_config_i18n.lng"

  RMDir /r "$INSTDIR\scripts"
  RMDir /r "$INSTDIR\handbook"
  RMDir "$INSTDIR\options"
  RMDir $INSTDIR

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

  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"

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
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} "Handbook."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContextMenuHandler} "Defragment your volumes from their context menu."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "Adds icons to your start menu and your desktop for easy access."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif

;---------------------------------------------

/* EOF */
