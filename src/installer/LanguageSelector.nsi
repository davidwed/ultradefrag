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
* Language Selector source.
*/

/*
*  NOTE: The following symbol should be defined
*        through makensis command line:
*  ULTRADFGVER=<version number in form x.y.z>
*/

!ifndef ULTRADFGVER
!error "One of the predefined symbols missing!"
!endif

; designed especially for modern interface
!define MODERN_UI

!ifdef MODERN_UI
  !include "MUI.nsh"
  !define MUI_ICON "LanguageSelector.ico"
  ;;!define MUI_BGCOLOR "000000"
!endif

!include "x64.nsh"

!macro LANG_PAGE
  Page custom LangShow LangLeave ""
!macroend

!ifndef MODERN_UI
!system 'move /Y lang-classical.ini lang.ini'
!endif

;-----------------------------------------
Name "UltraDefrag Language Selector v${ULTRADFGVER}"

OutFile "LanguageSelector.exe"
SetCompressor /SOLID lzma

XPStyle on
;RequestExecutionLevel admin

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey "ProductName" "Ultra Defragmenter"
VIAddVersionKey "CompanyName" "UltraDefrag Development Team"
VIAddVersionKey "LegalCopyright" "Copyright © 2007-2009 UltraDefrag Development Team"
VIAddVersionKey "FileDescription" "UltraDefrag Language Selector"
VIAddVersionKey "FileVersion" "${ULTRADFGVER}"
;-----------------------------------------

ReserveFile "lang.ini"

!ifdef MODERN_UI
  !insertmacro LANG_PAGE
  ;!insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
!else
  !insertmacro LANG_PAGE
!endif

;-----------------------------------------

Var LanguagePack

;-----------------------------------------

Function install_langpack

  push $R0
  push $R1

  Delete "$EXEDIR\ud_i18n.lng"
  Delete "$EXEDIR\ud_config_i18n.lng"
  Delete "$EXEDIR\ud_scheduler_i18n.lng"

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
    
    CopyFiles /SILENT "$EXEDIR\i18n\gui\$R0" "$EXEDIR\ud_i18n.lng"
    CopyFiles /SILENT "$EXEDIR\i18n\gui-config\$R0" "$EXEDIR\ud_config_i18n.lng"
  ${EndIf}

  pop $R1
  pop $R0

FunctionEnd

;-----------------------------------------

Function LangShow

  push $R0

!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "Language Selector" \
      "Choose which language you want to use with Ultra Defragmenter."
!endif
  SetOutPath $PLUGINSDIR
  File "lang.ini"
!ifdef MODERN_UI
  File "LanguageSelectorSmall.bmp"
!endif

  WriteINIStr "$PLUGINSDIR\lang.ini" "Settings" "Title" "UltraDefrag Language Selector v${ULTRADFGVER}"
  WriteINIStr "$PLUGINSDIR\lang.ini" "Settings" "NextButtonText" "OK"
  WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $LanguagePack

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
  WriteINIStr "$EXEDIR\PORTABLE.X" "i18n" "Language" $LanguagePack
  call install_langpack
  pop $R0

FunctionEnd

;----------------------------------------------

Function .onInit

  ${EnableX64FSRedirection}
  InitPluginsDir
  ${DisableX64FSRedirection}
  StrCpy $LanguagePack "English (US)"
  ${If} ${FileExists} "$EXEDIR\PORTABLE.X"
    ReadINIStr $LanguagePack "$EXEDIR\PORTABLE.X" "i18n" "Language"
  ${EndIf}
!ifdef MODERN_UI
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"
!endif
  ${EnableX64FSRedirection}

FunctionEnd

;---------------------------------------------

Section "Empty"

SectionEnd

;---------------------------------------------

/* EOF */
