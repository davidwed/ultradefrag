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
 *  UltraDefrag Micro Edition installer.
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

!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

!define MICRO_EDITION
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

Icon "udefrag-install.ico"
UninstallIcon "udefrag-uninstall.ico"

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
  ${DisableX64FSRedirection}

  SectionIn RO
  ;AddSize 44 /* for the components installed in system directories */
  
  ;;;DetailPrint "Uninstall the previous version..."
  /* waiting fails here => upgrade without deinstallation is preferred */
  ;;; ExecWait '"$INSTDIR\uninstall.exe" /S'

  DetailPrint "Install core files..."
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

  ${InstallNativeDefragmenter}

  File "${ROOTDIR}\src\installer\ud-help.cmd"
  File "${ROOTDIR}\src\installer\udctxhandler.cmd"
  File "udefrag-kernel.dll"
  File "udefrag.dll"
  File "udefrag.exe"
  File "zenwinx.dll"
  File /oname=hibernate4win.exe "hibernate.exe"

  ${WriteTheUninstaller}
  ${RemoveObsoleteFiles}
  ${InstallConfigFiles}
  ${SetContextMenuHandler}

  ${EnableX64FSRedirection}
  pop $R0

SectionEnd

Section "Uninstall"

  ${UninstallTheProgram}

SectionEnd

/* EOF */
