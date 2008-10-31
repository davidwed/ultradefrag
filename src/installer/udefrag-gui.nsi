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
 *  UltraDefrag 2.0.0 GUI launcher program.
 */

/*
 *  NOTE: The following symbol should be defined
 *        through makensis command line:
 *  ULTRADFGVER=<version number in form x.y.z>
 */

!ifndef ULTRADFGVER
!define ULTRADFGVER 1.10.0
!endif

!include "x64.nsh"

;Name "Ultra Defragmenter GUI launcher"
OutFile "udefrag-gui.exe"

SetCompressor /SOLID lzma
Icon "..\gui\res\app.ico"

;;RequestExecutionLevel

;SilentInstall silent

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2008 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "UltraDefrag GUI Launcher"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

;Page instfiles

;-----------------------------------------

Function .onInit

!insertmacro DisableX64FSRedirection

ExecShell "" "$SYSDIR\udefrag-gui.cmd" "" SW_HIDE
Abort

!insertmacro EnableX64FSRedirection

FunctionEnd

Section "" Section
SectionEnd

/* EOF */
