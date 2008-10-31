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
 *  UltraDefrag Micro Edition Installer.
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

!include "x64.nsh"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

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

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007,2008 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "Ultra Defragmenter Micro Edition Setup"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

Page license
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;-----------------------------------------

Var NT4_TARGET

Function .onInit

  push $R0
  push $R1

!insertmacro DisableX64FSRedirection

  /* variables initialization */
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"
  StrCpy $NT4_TARGET 0

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

!insertmacro EnableX64FSRedirection
  pop $R1
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
  File "${ROOTDIR}\src\LICENSE.TXT"
  File "${ROOTDIR}\src\CREDITS.TXT"
  File "${ROOTDIR}\src\HISTORY.TXT"
  File "${ROOTDIR}\src\INSTALL.TXT"
  File "${ROOTDIR}\src\README.TXT"
  File "${ROOTDIR}\src\FAQ.TXT"

  DetailPrint "Install driver..."
  call install_driver

  SetOutPath "$SYSDIR"

  DetailPrint "Install DLL's..."
  File "udefrag.dll"
  File "zenwinx.dll"

  DetailPrint "Install boot time defragger..."
  File "defrag_native.exe"
  File "bootexctrl.exe"
  File "${ROOTDIR}\src\installer\boot-on.cmd"
  File "${ROOTDIR}\src\installer\boot-off.cmd"

  DetailPrint "Install scripts..."
  File "${ROOTDIR}\src\installer\ud-config.cmd"
  File "${ROOTDIR}\src\installer\boot-config.cmd"

  DetailPrint "Install console interface..."
  File "udefrag.exe"

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
  WriteRegStr HKLM  $R0 "DisplayName" "Ultra Defragmenter Micro Edition"
  WriteRegStr HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  ; remove files of previous 1.3.1-1.3.3 installation
  RMDir /r "$SYSDIR\UltraDefrag"
  RMDir /r "$INSTDIR\presets"
  DeleteRegKey HKLM "SYSTEM\UltraDefrag"

  ; create boot time script if it doesn't exist
  SetOutPath "$SYSDIR"
  IfFileExists "$SYSDIR\ud-boot-time.cmd" bt_ok 0
  File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
bt_ok:

!insertmacro EnableX64FSRedirection

  pop $R1
  pop $R0

SectionEnd

Section "Uninstall"

!insertmacro DisableX64FSRedirection
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"

  DetailPrint "Remove program files..."
  /* remove unuseful registry settings */
  ExecWait '"$SYSDIR\bootexctrl.exe" /u defrag_native'
  Delete "$INSTDIR\LICENSE.TXT"
  Delete "$INSTDIR\CREDITS.TXT"
  Delete "$INSTDIR\HISTORY.TXT"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\README.TXT"
  Delete "$INSTDIR\FAQ.TXT"

  ; delete two scripts from the 1.4.0 version
  Delete "$INSTDIR\boot_on.cmd"
  Delete "$INSTDIR\boot_off.cmd"
  
  RMDir "$INSTDIR\options"
  Delete "$INSTDIR\uninstall.exe"
  RMDir $INSTDIR

  DetailPrint "Uninstall driver and boot time defragger..."
  Delete "$SYSDIR\Drivers\ultradfg.sys"
  Delete "$SYSDIR\bootexctrl.exe"
  Delete "$SYSDIR\defrag_native.exe"
  Delete "$SYSDIR\boot-on.cmd"
  Delete "$SYSDIR\boot-off.cmd"
  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\zenwinx.dll"

  DetailPrint "Uninstall scripts and console interface..."
  Delete "$SYSDIR\ud-config.cmd"
  Delete "$SYSDIR\boot-config.cmd"
  Delete "$SYSDIR\udefrag.exe"

  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"

  DetailPrint "Clear registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"

!insertmacro EnableX64FSRedirection

SectionEnd

/* EOF */
