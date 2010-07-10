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

!ifndef ULTRADFGVER
!error "ULTRADFGVER parameter must be specified on the command line!"
!endif

!ifndef ULTRADFGARCH
!error "ULTRADFGARCH parameter must be specified on the command line!"
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
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2010 UltraDefrag Development Team"
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

    StrCpy $R0 $LanguagePack
    ${Select} $LanguagePack
      ${Case} "English (US)"
        StrCpy $R0 "English(US)"
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

    RMDir /r "$INSTDIR\i18n\gui"
    SetOutPath "$INSTDIR\i18n\gui"
    File "${ROOTDIR}\src\gui\i18n\*.GUI"
    CopyFiles /SILENT "$INSTDIR\i18n\gui\$R0.GUI" "$INSTDIR\ud_i18n.lng"

    RMDir /r "$INSTDIR\i18n\gui-config"
    SetOutPath "$INSTDIR\i18n\gui-config"
    File "${ROOTDIR}\src\udefrag-gui-config\i18n\*.Config"
    CopyFiles /SILENT "$INSTDIR\i18n\gui-config\$R0.Config" "$INSTDIR\ud_config_i18n.lng"

    WriteRegStr HKLM "Software\UltraDefrag" "Language" $LanguagePack
    
  pop $R0
  
FunctionEnd

;------------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0
  ${DisableX64FSRedirection}

  SectionIn RO
  ;AddSize 24 /* for the components installed in system directories (driver) */

  DetailPrint "Install core files..."
  SetOutPath $INSTDIR
  File "${ROOTDIR}\src\LICENSE.TXT"
  File "${ROOTDIR}\src\CREDITS.TXT"
  File "${ROOTDIR}\src\HISTORY.TXT"
  File "${ROOTDIR}\src\README.TXT"
  SetOutPath "$INSTDIR\scripts"
  File "${ROOTDIR}\src\scripts\udreportcnv.lua"
  File "${ROOTDIR}\src\scripts\udsorting.js"

  ; install LanguagePack
  call install_langpack

  ; install GUI apps to program's directory
  SetOutPath "$INSTDIR"
  Delete "$INSTDIR\ultradefrag.exe"
  File /oname=ultradefrag.exe "dfrg.exe"
  File "udefrag-gui-config.exe"
  File "LanguageSelector.exe"

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

  ${RegisterFileExtensions}
  ${WriteTheUninstaller}
  ${RemoveObsoleteFiles}
  ${InstallConfigFiles}

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

Section "Documentation" SecDocs

  push $R0
  ${DisableX64FSRedirection}

  DetailPrint "Install documentation..."
  ; update the handbook
  RMDir /r "$INSTDIR\handbook"
  SetOutPath "$INSTDIR\handbook"
  File "${ROOTDIR}\doc\html\handbook\doxy-doc\html\*.*"

  ; update the uninstall size value
  ${UpdateUninstallSizeValue}

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

  CreateShortCut "$R0\Select Language.lnk" \
   "$INSTDIR\LanguageSelector.exe"

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

  ${UninstallTheProgram}

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
