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
* Language selection routines.
* Intended both for the regular installer
* and for the Language Selector.
*/

!ifndef _LANGUAGE_SELECTOR_NSH_
!define _LANGUAGE_SELECTOR_NSH_

/*
* How to use it:
* 1. Insert ${InitLanguageSelector}
*    to .onInit function.
* 2. Use ${InstallLanguagePack}
*    to install selected language pack.
* 3. Use LANG_PAGE macro to add the
*    language selection page to the
*    installer.
*/

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

; --- define which page layout to use - classical or modern
!ifndef MODERN_UI
!system 'move /Y lang-classical.ini lang.ini'
!endif

Var LanguagePack
ReserveFile "lang.ini"

;-----------------------------------------

/*
 * This corrects the loaction of the language registry setting
 */
!macro CorrectLangReg

    push $R0
    
    ClearErrors
    ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
    ${Unless} ${Errors}
        SetRegView 64
        WriteRegStr HKLM "Software\UltraDefrag" "Language" $R0
        SetRegView 32
        DeleteRegKey HKLM "Software\UltraDefrag"
    ${EndUnless}
    
    pop $R0

!macroend

;-----------------------------------------------------------
;         LANG_PAGE macro and support routines
;-----------------------------------------------------------

!macro LANG_PAGE
  Page custom LangShow LangLeave ""
!macroend

;-----------------------------------------------------------

Function LangShow

  push $R0
  push $R1
  push $R2

!ifdef MODERN_UI
  !insertmacro MUI_HEADER_TEXT "Language Selector" \
      "Choose which language you want to use with Ultra Defragmenter."
!endif
  SetOutPath $PLUGINSDIR
  File "lang.ini"

!ifdef LANGUAGE_SELECTOR
  WriteINIStr "$PLUGINSDIR\lang.ini" "Settings" "Title" "UltraDefrag Language Selector v${ULTRADFGVER}"
  WriteINIStr "$PLUGINSDIR\lang.ini" "Settings" "NextButtonText" "OK"
!endif

!ifdef ISPORTABLE
  WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $LanguagePack
!else
  ; --- get language from registry
  ClearErrors
  SetRegView 64
  ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
  SetRegView 32
  ${Unless} ${Errors}
    WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $R0
  ${EndUnless}
!endif

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

;-----------------------------------------------------------

/**
 * @note
 * - Requires $INSTDIR to be set.
 * - Requires ${DisableX64FSRedirection} before.
 */
!macro InstallLanguagePack

  push $R0

  StrCpy $R0 $LanguagePack

!ifndef LANGUAGE_SELECTOR
  ; regular installer copies all language packs to installation subdirectories
  ; then it copies the selected pack to the installation directory
  RMDir /r "$INSTDIR\i18n\gui"
  SetOutPath "$INSTDIR\i18n\gui"
  File "${ROOTDIR}\src\gui\i18n\*.GUI"
  Delete "$INSTDIR\ud_i18n.lng"
  CopyFiles /SILENT "$INSTDIR\i18n\gui\$R0.GUI" "$INSTDIR\ud_i18n.lng"

  RMDir /r "$INSTDIR\i18n\gui-config"
  SetOutPath "$INSTDIR\i18n\gui-config"
  File "${ROOTDIR}\src\udefrag-gui-config\i18n\*.Config"
  Delete "$INSTDIR\ud_config_i18n.lng"
  CopyFiles /SILENT "$INSTDIR\i18n\gui-config\$R0.Config" "$INSTDIR\ud_config_i18n.lng"
!else
  ; language selector uses preinstalled repository of language packs
  Delete "$EXEDIR\ud_i18n.lng"
  Delete "$EXEDIR\ud_config_i18n.lng"
  CopyFiles /SILENT "$EXEDIR\i18n\gui\$R0.GUI" "$EXEDIR\ud_i18n.lng"
  CopyFiles /SILENT "$EXEDIR\i18n\gui-config\$R0.Config" "$EXEDIR\ud_config_i18n.lng"
!endif

!ifdef ISPORTABLE
  WriteINIStr "$EXEDIR\PORTABLE.X" "i18n" "Language" $LanguagePack
!else
  SetRegView 64
  WriteRegStr HKLM "Software\UltraDefrag" "Language" $LanguagePack
  SetRegView 32
!endif

  pop $R0

!macroend

;-----------------------------------------------------------

/**
 * @note Disables the x64 file system redirection.
 */
!macro InitLanguageSelector

  push $R1
  push $R2

  ${EnableX64FSRedirection}
  InitPluginsDir

  StrCpy $LanguagePack "English (US)"

  ${DisableX64FSRedirection}
!ifdef ISPORTABLE
  ; --- get language from the PORTABLE.X file
  ${If} ${FileExists} "$EXEDIR\PORTABLE.X"
    ReadINIStr $LanguagePack "$EXEDIR\PORTABLE.X" "i18n" "Language"
  ${EndIf}
!else
  ; --- get language from registry
  ClearErrors
  SetRegView 64
  ReadRegStr $R1 HKLM "Software\UltraDefrag" "Language"
  SetRegView 32
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R1
  ${EndUnless}
!endif

  ; --- get language from command line
  ; --- allows silent installation with: installer.exe /S /LANG="German"
  ${GetParameters} $R1
  ClearErrors
  ${GetOptions} $R1 /LANG= $R2
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R2
  ${EndUnless}

!ifdef MODERN_UI
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"
!endif

  pop $R2
  pop $R1

!macroend

;-----------------------------------------------------------

!define InstallLanguagePack "!insertmacro InstallLanguagePack"
!define InitLanguageSelector "!insertmacro InitLanguageSelector"
!define CorrectLangReg "!insertmacro CorrectLangReg"

;-----------------------------------------------------------

Function LangLeave

  push $R0

  ReadINIStr $R0 "$PLUGINSDIR\lang.ini" "Settings" "State"
  ${If} $R0 != "0"
    pop $R0
    Abort
  ${EndIf}

  ReadINIStr $LanguagePack "$PLUGINSDIR\lang.ini" "Field 2" "State"

!ifdef LANGUAGE_SELECTOR
  ${InstallLanguagePack}
!endif

  pop $R0

FunctionEnd

;-----------------------------------------------------------

!endif /* _LANGUAGE_SELECTOR_NSH_ */
