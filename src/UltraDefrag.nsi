/*
 *  ULTRADEFRAG - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by D. Arkhangelski (dmitriar@gmail.com).
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
 *  Installer source.
 */

!include "ultradefrag_globals.nsh"

;-----------------------------------------

!define MODERN_UI

!ifdef MODERN_UI
  !include "MUI.nsh"
!endif

;-----------------------------------------
Name "Ultra Defrag"
OutFile "ultradefrag-${ULTRADFGVER}.bin.${ULTRADFGARCH}.exe"

LicenseData "LICENSE.TXT"
InstallDir "$PROGRAMFILES64\UltraDefrag"
InstallDirRegKey HKLM "Software\DASoft\NTDefrag" "Install_Dir"
ShowInstDetails show
ShowUninstDetails show

SetCompressor lzma

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "DASoft"
VIAddVersionKey "LegalCopyright" "Copyright © 2007 by DASoft"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

!ifdef MODERN_UI
  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_SMALLDESC

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "LICENSE.TXT"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

  !insertmacro MUI_LANGUAGE "English"
!else
  Page license
  Page components
  Page directory
  Page instfiles

  UninstPage uninstConfirm
  UninstPage instfiles
!endif

;-----------------------------------------

Function .onInit

  push $R0
  push $R1

  ClearErrors
  ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
  IfErrors 0 winnt
  MessageBox MB_OK|MB_ICONEXCLAMATION \
   "On Windows 9.x this program is absolutely unuseful!" \
   /SD IDOK
abort_inst:
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
  MessageBox MB_OK|MB_ICONEXCLAMATION \
   "Windows NT 4.x is not supported yet!" \
   /SD IDOK
  goto abort_inst

winnt_56:
  pop $R1
  pop $R0

FunctionEnd

;-----------------------------------------

Function WriteDriverSettings

  WriteRegStr HKLM $R0 "DisplayName" "ultradfg"
  WriteRegDWORD HKLM $R0 "ErrorControl" 0x0
  WriteRegExpandStr HKLM $R0 "ImagePath" "System32\DRIVERS\ultradfg.sys"
  WriteRegDWORD HKLM $R0 "Start" 0x3
  WriteRegDWORD HKLM $R0 "Type" 0x1

FunctionEnd

;-----------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0

  SectionIn RO
  DetailPrint "Install core files..."
  SetOutPath $INSTDIR
  File "Dfrg.exe"
  File "LICENSE.TXT"
  File "CREDITS.TXT"
  File "HISTORY.TXT"
  File "INSTALL.TXT"
  File "README.TXT"
  File "FAQ.TXT"
  Delete "$INSTDIR\defrag_native.exe" /* from previous 1.0.x installation */
  DetailPrint "Install driver..."
  SetOutPath "$WINDIR\System32\Drivers"
  File "ultradfg.sys"
  DetailPrint "Install boot time defragger..."
  SetOutPath "$WINDIR\System32"
  File "defrag_native.exe"
!if ${ULTRADFGARCH} != "i386"
  SetOutPath "$TEMP"
  File "x64_inst.exe"
  ExecWait "$TEMP\x64_inst.exe"
  Delete "$TEMP\x64_inst.exe"
!endif
  StrCpy $R0 "SYSTEM\CurrentControlSet\Services\ultradfg"
  call WriteDriverSettings
  StrCpy $R0 "SYSTEM\ControlSet001\Services\ultradfg"
  call WriteDriverSettings
  StrCpy $R0 "SYSTEM\ControlSet002\Services\ultradfg"
  call WriteDriverSettings
  DetailPrint "Write the uninstall keys..."
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegStr HKLM  $R0 "DisplayName" "DASoft Ultra Defragmenter"
  WriteRegStr HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  pop $R0

SectionEnd

Section "Console interface" SecConsole

  DetailPrint "Install console interface..."
  SetOutPath $INSTDIR
  File "defrag.exe"

SectionEnd

Section "Shortcuts" SecShortcuts

  push $R0
  AddSize 3
  DetailPrint "Install shortcuts..."
  SetShellVarContext all
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
  CreateShortCut "$R0\Uninstall UltraDefrag.lnk" \
   "$INSTDIR\uninstall.exe"
  CreateShortCut "$DESKTOP\UltraDefrag.lnk" \
   "$INSTDIR\Dfrg.exe"
  pop $R0
  
SectionEnd

Section "Uninstall"

  DetailPrint "Remove shortcuts..."
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\DASoft\UltraDefrag"
  RMDir "$SMPROGRAMS\DASoft"
  Delete "$DESKTOP\UltraDefrag.lnk"
  DetailPrint "Remove program files..."
  Delete "$INSTDIR\Dfrg.exe"
  Delete "$INSTDIR\defrag.exe"
  Delete "$INSTDIR\LICENSE.TXT"
  Delete "$INSTDIR\CREDITS.TXT"
  Delete "$INSTDIR\HISTORY.TXT"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\README.TXT"
  Delete "$INSTDIR\FAQ.TXT"
  Delete "$INSTDIR\uninstall.exe"
  RMDir $INSTDIR
  DetailPrint "Uninstall driver and boot time defragger..."
!if ${ULTRADFGARCH} != "i386"
  SetOutPath "$TEMP"
  File "x64_inst.exe"
  ExecWait '"$TEMP\x64_inst.exe" -u'
  Delete "$TEMP\x64_inst.exe"
!else
  Delete "$SYSDIR\Drivers\ultradfg.sys"
  Delete "$SYSDIR\defrag_native.exe"
!endif
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  ;;;;DeleteRegKey HKCU "SOFTWARE\DASoft\NTDefrag"
  DeleteRegKey HKLM "Software\DASoft\NTDefrag"
  DeleteRegKey /ifempty HKLM "Software\DASoft"

SectionEnd

;----------------------------------------------

!ifdef MODERN_UI
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The core files required to use UltraDefrag."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecConsole} "Useful for scripts and scheduling."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "Adds icons to your start menu and your desktop for easy access."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
!endif

;---------------------------------------------

/* EOF */