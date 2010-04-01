/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  UltraDefrag Micro Edition Installer.
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

!include "x64.nsh"
!include "WinVer.nsh"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

!include ".\UltraDefrag.nsh"

;-----------------------------------------
!if ${ULTRADFGARCH} == 'amd64'
Name "Ultra Defragmenter Micro Edition v${ULTRADFGVER} (AMD64)"
!else if ${ULTRADFGARCH} == 'ia64'
Name "Ultra Defragmenter Micro Edition v${ULTRADFGVER} (IA64)"
!else
Name "Ultra Defragmenter Micro Edition v${ULTRADFGVER} (i386)"
!endif

OutFile "ultradefrag-micro-edition-${ULTRADFGVER}.bin.${ULTRADFGARCH}.exe"
LicenseData "${ROOTDIR}\src\LICENSE.TXT"
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

XPStyle on
RequestExecutionLevel admin

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2010 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Micro Edition Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

Page license
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;-----------------------------------------

Function .onInit

  ${CheckWinVersion}

  ${EnableX64FSRedirection}
  InitPluginsDir
  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"
  ${EnableX64FSRedirection}

FunctionEnd

;-----------------------------------------

Section "Ultra Defrag core files (required)" SecCore

  push $R0

  SectionIn RO
  AddSize 44 /* for the components installed in system directories */
  
  ;;;DetailPrint "Uninstall the previous version..."
  /* waiting fails here => manual deinstallation preferred */
  ;;; ExecWait '"$INSTDIR\uninstall.exe" /S'

  DetailPrint "Install core files..."
  ${DisableX64FSRedirection}
  SetOutPath $INSTDIR
  File "${ROOTDIR}\src\LICENSE.TXT"
  File "${ROOTDIR}\src\CREDITS.TXT"
  File "${ROOTDIR}\src\HISTORY.TXT"
  File "${ROOTDIR}\src\README.TXT"

  SetOutPath "$SYSDIR"
  File "${ROOTDIR}\src\installer\boot-config.cmd"
  File "${ROOTDIR}\src\installer\boot-off.cmd"
  File "${ROOTDIR}\src\installer\boot-on.cmd"
  File "bootexctrl.exe"
  File "defrag_native.exe"
  File "${ROOTDIR}\src\installer\ud-help.cmd"
  File "${ROOTDIR}\src\installer\udctxhandler.cmd"
  File "udefrag-kernel.dll"
  File "udefrag.dll"
  File "udefrag.exe"
  File "zenwinx.dll"
  File /oname=hibernate4win.exe "hibernate.exe"

  DetailPrint "Write the uninstall keys..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegStr HKLM  $R0 "DisplayName" "Ultra Defragmenter Micro Edition"
  WriteRegStr HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  ${RemoveObsoleteFiles}
  
  ; create boot time script if it doesn't exist
  SetOutPath "$SYSDIR"
  ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
    File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
  ${EndUnless}

  ; register context menu handler
  ${SetContextMenuHandler}

  ${EnableX64FSRedirection}

  pop $R0

SectionEnd

Section "Uninstall"

  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"

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

  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR\options"
  RMDir $INSTDIR

  Delete "$SYSDIR\boot-config.cmd"
  Delete "$SYSDIR\boot-off.cmd"
  Delete "$SYSDIR\boot-on.cmd"
  Delete "$SYSDIR\bootexctrl.exe"
  Delete "$SYSDIR\defrag_native.exe"

  Delete "$SYSDIR\ud-help.cmd"
  Delete "$SYSDIR\udctxhandler.cmd"
  Delete "$SYSDIR\udefrag-kernel.dll"
  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\udefrag.exe"
  Delete "$SYSDIR\zenwinx.dll"
  Delete "$SYSDIR\hibernate4win.exe"

  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"

  DetailPrint "Uninstall the context menu handler..."
  DeleteRegKey HKCR "Drive\shell\udefrag"
  DeleteRegKey HKCR "Folder\shell\udefrag"
  DeleteRegKey HKCR "*\shell\udefrag"

  ${EnableX64FSRedirection}

SectionEnd

/* EOF */
