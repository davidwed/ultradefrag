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
* UltraDefrag Language Selector.
*/

/*
*  NOTE: The following symbol should be defined
*        through makensis command line:
*  ULTRADFGVER=<version number in form x.y.z>
*/

!ifndef ULTRADFGVER
!error "ULTRADFGVER parameter must be specified on the command line!"
!endif

; designed especially for modern interface
!define MODERN_UI

!ifdef MODERN_UI
  !include "MUI.nsh"
  !define MUI_ICON "LanguageSelector.ico"
!endif
!include "x64.nsh"

;-----------------------------------------
Name "UltraDefrag Language Selector v${ULTRADFGVER}"

OutFile "LanguageSelector.exe"
SetCompressor /SOLID lzma

XPStyle on
!ifndef ISPORTABLE
RequestExecutionLevel admin
!endif

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2010 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "UltraDefrag Language Selector"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

!define LANGUAGE_SELECTOR
!include ".\LanguageSelector.nsh"

;-----------------------------------------

; the Language Selector consists of a single page
!insertmacro LANG_PAGE

!ifdef MODERN_UI
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
!endif

;-----------------------------------------

Function .onInit

!ifndef ISPORTABLE
    ${CorrectLangReg}
!endif

  ${InitLanguageSelector}
  ${EnableX64FSRedirection}

FunctionEnd

;---------------------------------------------

Section "Empty"

SectionEnd

;---------------------------------------------

/* EOF */
