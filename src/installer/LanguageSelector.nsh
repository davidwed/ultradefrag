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
* Language selection routines.
* Intended for the regular installer.
*/

!ifndef _LANGUAGE_SELECTOR_NSH_
!define _LANGUAGE_SELECTOR_NSH_

/*
* How to use it:
* 1. Insert ${InitLanguageSelector}
*    to .onInit function.
* 2. Use LANG_PAGE macro to add the
*    language selection page to the
*    installer.
*/

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

Var LanguagePack
ReserveFile "lang.ini"

/*
 * This collects the previous language setting
 */
!macro CollectOldLang

    push $R0
    
    ClearErrors
    ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
    ${Unless} ${Errors}
        WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $R0
    ${EndUnless}
    
    SetRegView 64
    ClearErrors
    ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
    ${Unless} ${Errors}
        WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $R0
    ${EndUnless}
    SetRegView 32
    
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

  StrCpy $LanguagePack "English (US)"

  ; --- get language from $INSTDIR\lang.ini file
  ${DisableX64FSRedirection}
  ${If} ${FileExists} "$INSTDIR\lang.ini"
    ReadINIStr $LanguagePack "$INSTDIR\lang.ini" "Language" "Selected"
  ${EndIf}
  ${EnableX64FSRedirection}

  ; --- get language from command line
  ; --- allows silent installation with: installer.exe /S /LANG="German"
  ${GetParameters} $R1
  ClearErrors
  ${GetOptions} $R1 /LANG= $R2
  ${Unless} ${Errors}
    StrCpy $LanguagePack $R2
  ${EndUnless}

  WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $LanguagePack

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
 * @note Disables the x64 file system redirection.
 */
!macro InitLanguageSelector

  ${EnableX64FSRedirection}
  InitPluginsDir
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"

!macroend

;-----------------------------------------------------------

!define InitLanguageSelector "!insertmacro InitLanguageSelector"
!define CollectOldLang "!insertmacro CollectOldLang"

;-----------------------------------------------------------

Function LangLeave

  push $R0

  ReadINIStr $R0 "$PLUGINSDIR\lang.ini" "Settings" "State"
  ${If} $R0 != "0"
    pop $R0
    Abort
  ${EndIf}

  ; save selected language to $INSTDIR\lang.ini file before exit
  ReadINIStr $LanguagePack "$PLUGINSDIR\lang.ini" "Field 2" "State"
  WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $LanguagePack
  pop $R0

FunctionEnd

;-----------------------------------------------------------

!endif /* _LANGUAGE_SELECTOR_NSH_ */
