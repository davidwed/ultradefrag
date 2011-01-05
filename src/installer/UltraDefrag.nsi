/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
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
* UltraDefrag regular installer.
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

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

!ifdef MODERN_UI
  !include "MUI.nsh"
  !define MUI_ICON "${ROOTDIR}\src\installer\udefrag-install.ico"
  !define MUI_UNICON "${ROOTDIR}\src\installer\udefrag-uninstall.ico"
!endif

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

Icon "${ROOTDIR}\src\installer\udefrag-install.ico"
UninstallIcon "${ROOTDIR}\src\installer\udefrag-uninstall.ico"

XPStyle on
RequestExecutionLevel admin

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2011 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

!include "${ROOTDIR}\src\installer\UltraDefrag.nsh"
!include "${ROOTDIR}\src\installer\LanguageSelector.nsh"

;-----------------------------------------

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

  ; install manual pages for the boot time interface
  SetOutPath "$INSTDIR\man"
  File "${ROOTDIR}\src\man\*.*"

  ; install all language packs
  SetOutPath "$INSTDIR\i18n"
  File /nonfatal "${ROOTDIR}\src\gui\i18n\*.lng"
  File /nonfatal "${ROOTDIR}\src\gui\i18n\*.template"

  ; install GUI apps to program's directory
  SetOutPath "$INSTDIR"
  Delete "$INSTDIR\ultradefrag.exe"
  File "ultradefrag.exe"
  File "wgx.dll"
  File "lua5.1a.dll"
  File "lua5.1a.exe"
  File "lua5.1a_gui.exe"
  
  ; install shell extension icon
  File "${ROOTDIR}\src\installer\shellex.ico"

  SetOutPath "$SYSDIR"
  File "${ROOTDIR}\src\installer\boot-config.cmd"
  File "${ROOTDIR}\src\installer\boot-off.cmd"
  File "${ROOTDIR}\src\installer\boot-on.cmd"
  File "bootexctrl.exe"

  ${InstallNativeDefragmenter}

  SetOutPath "$SYSDIR"
  File "${ROOTDIR}\src\installer\ud-help.cmd"
  File "udefrag.dll"
  File "udefrag.exe"
  File "zenwinx.dll"
  File /oname=hibernate4win.exe "hibernate.exe"

  ${RegisterFileExtensions}
  ${WriteTheUninstaller}
  ${RemoveObsoleteFiles}
  ${InstallConfigFiles}

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

;----------------------------------------------

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

;----------------------------------------------

Section "Context menu handler" SecContextMenuHandler

  ${DisableX64FSRedirection}
  ${SetContextMenuHandler}
  ${EnableX64FSRedirection}
  
SectionEnd

;----------------------------------------------

Section "Shortcuts" SecShortcuts

  push $R0
  AddSize 5

  DetailPrint "Install shortcuts..."
  ${DisableX64FSRedirection}
  SetShellVarContext all
  SetOutPath $INSTDIR
  
  ; install a single shortcut to the Start menu,
  ; because all important information can be easily
  ; accessed from the GUI
  CreateShortCut "$SMPROGRAMS\UltraDefrag.lnk" \
   "$INSTDIR\ultradefrag.exe"
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

  ${CollectOldLang}
  ${InitLanguageSelector}

!ifdef SHOW_BOOTSPLASH
  ${ShowBootSplash}
!endif

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
