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
* Routines to read the options from the command line,
* the previous selections from the registry, to apply these
* and to save them for the future.
*/

/*
* All macros must be added to .onInit except PreserveInRegistry
* which should be added to the end of a mandatory section.
*/

!ifndef _PRESET_SECTIONS_NSH_
!define _PRESET_SECTIONS_NSH_

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

; --- simple section handling
!include "Sections.nsh"
!define SelectSection   "!insertmacro SelectSection"
!define UnselectSection "!insertmacro UnselectSection"

Var InstallGUI
Var InstallHelp
Var InstallShellHandler
Var InstallStartMenuIcon
Var InstallDesktopIcon
Var InstallQuickLaunchIcon

/*
 * This presets the selections to the full installation
 */
!macro InitSelection
    StrCpy $InstallGUI             "1"
    StrCpy $InstallHelp            "1"
    StrCpy $InstallShellHandler    "1"
    StrCpy $InstallStartMenuIcon   "1"
    StrCpy $InstallDesktopIcon     "1"
    StrCpy $InstallQuickLaunchIcon "1"
!macroend

!define InitSelection "!insertmacro InitSelection"

;-----------------------------------------------------------

/*
 * This collects the previous selections from the registry
 */
!macro CollectFromRegistry

    push $R0
    push $R1
    
    DetailPrint "Collecting previous selections from registry ..."
    StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallGUI"
    ${Unless} ${Errors}
        StrCpy $InstallGUI $R1
    ${EndUnless}
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallHelp"
    ${Unless} ${Errors}
        StrCpy $InstallHelp $R1
    ${EndUnless}
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallShellHandler"
    ${Unless} ${Errors}
        StrCpy $InstallShellHandler $R1
    ${EndUnless}
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallStartMenuIcon"
    ${Unless} ${Errors}
        StrCpy $InstallStartMenuIcon $R1
    ${EndUnless}
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallDesktopIcon"
    ${Unless} ${Errors}
        StrCpy $InstallDesktopIcon $R1
    ${EndUnless}
    
    ClearErrors
    ReadRegStr $R1 HKLM $R0 "Var::InstallQuickLaunchIcon"
    ${Unless} ${Errors}
        StrCpy $InstallQuickLaunchIcon $R1
    ${EndUnless}
    
    pop $R1
    pop $R0

!macroend

;-----------------------------------------------------------

/*
 * This collects selections from the command line
 */
!macro ParseCommandLine

    push $R0
    push $R1
    
    DetailPrint "Collecting selections from the command line ..."
    ${GetParameters} $R0
    
    ClearErrors
    ${GetOptions} $R0 /FULL= $R1
    ${Unless} ${Errors}
        ${InitSelection}
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /MICRO= $R1
    ${Unless} ${Errors}
        StrCpy $InstallGUI             "0"
        StrCpy $InstallHelp            "0"
        StrCpy $InstallShellHandler    "1"
        StrCpy $InstallDesktopIcon     "0"
        StrCpy $InstallQuickLaunchIcon "0"
        StrCpy $InstallStartMenuIcon   "0"
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /ICONS= $R1
    ${Unless} ${Errors}
        StrCpy $InstallDesktopIcon     $R1
        StrCpy $InstallQuickLaunchIcon $R1
        StrCpy $InstallStartMenuIcon   $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /GUI= $R1
    ${Unless} ${Errors}
        StrCpy $InstallGUI $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /HELP= $R1
    ${Unless} ${Errors}
        StrCpy $InstallHelp $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /SHELLEXTENSION= $R1
    ${Unless} ${Errors}
        StrCpy $InstallShellHandler $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /STARTMENUICON= $R1
    ${Unless} ${Errors}
        StrCpy $InstallStartMenuIcon $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /DESKTOPICON= $R1
    ${Unless} ${Errors}
        StrCpy $InstallDesktopIcon $R1
    ${EndUnless}
    
    ClearErrors
    ${GetOptions} $R0 /QUICKLAUNCHICON= $R1
    ${Unless} ${Errors}
        StrCpy $InstallQuickLaunchIcon $R1
    ${EndUnless}
    
    pop $R1
    pop $R0

!macroend

;-----------------------------------------------------------

/*
 * This writes the selections to the registry for future reference
 */
!macro PreserveInRegistry

    push $R0
    push $0
    push $1
    
    ${DisableX64FSRedirection}
    SetShellVarContext all

    DetailPrint "Saving selections to registry ..."
    StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
    
    SectionGetFlags ${SecGUI} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallGUI" $1
    ${If} $1 == "0"
        RMDir /r "$INSTDIR\i18n"
        Delete "$INSTDIR\ultradefrag.exe"
        Delete "$INSTDIR\wgx.dll"
        Delete "$INSTDIR\lua5.1a.dll"
        Delete "$INSTDIR\lua5.1a.exe"
        Delete "$INSTDIR\lua5.1a_gui.exe"
        Delete "$INSTDIR\scripts\udreportcnv.lua"
        Delete "$INSTDIR\scripts\udsorting.js"
        Delete "$INSTDIR\scripts\udsorting.js"
        DeleteRegKey HKCR "LuaReport"
        DeleteRegKey HKCR ".luar"
    ${EndIf}

    SectionGetFlags ${SecHelp} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallHelp" $1
    ${If} $1 == "0"
        RMDir /r "$INSTDIR\handbook"
    ${EndIf}

    SectionGetFlags ${SecShellHandler} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallShellHandler" $1
    ${If} $1 == "0"
        DeleteRegKey HKCR "Drive\shell\udefrag"
        DeleteRegKey HKCR "Folder\shell\udefrag"
        DeleteRegKey HKCR "*\shell\udefrag"
        Delete "$INSTDIR\shellex.ico"
    ${EndIf}
    
    SectionGetFlags ${SecStartMenuIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallStartMenuIcon" $1
    ${If} $1 == "0"
        Delete "$SMPROGRAMS\UltraDefrag.lnk"
    ${EndIf}

    SectionGetFlags ${SecDesktopIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallDesktopIcon" $1
    ${If} $1 == "0"
        Delete "$DESKTOP\UltraDefrag.lnk"
    ${EndIf}

    SectionGetFlags ${SecQuickLaunchIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM $R0 "Var::InstallQuickLaunchIcon" $1
    ${If} $1 == "0"
        Delete "$QUICKLAUNCH\UltraDefrag.lnk"
    ${EndIf}

    ${EnableX64FSRedirection}
    pop $1
    pop $0
    pop $R0

!macroend

;-----------------------------------------------------------

/*
 * This sets the selections
 */
!macro PresetSections

    push $0
    push $1
    
    DetailPrint "Setting selections ..."
    
    ; handle GUI section
    ${If} $InstallGUI == "1"
        ${SelectSection} ${SecGUI}
    ${Else}
        ${UnselectSection} ${SecGUI}
    ${EndIf}

    ; handle help section
    ${If} $InstallHelp == "1"
        ${SelectSection} ${SecHelp}
    ${Else}
        ${UnselectSection} ${SecHelp}
    ${EndIf}

    ; handle shell extension section
    ${If} $InstallShellHandler == "1"
        ${SelectSection} ${SecShellHandler}
    ${Else}
        ${UnselectSection} ${SecShellHandler}
    ${EndIf}
    
    ; handle start menu icon section
    ${If} $InstallStartMenuIcon == "1"
        ${SelectSection} ${SecStartMenuIcon}
    ${Else}
        ${UnselectSection} ${SecStartMenuIcon}
    ${EndIf}

    ; handle desktop icon section
    ${If} $InstallDesktopIcon == "1"
        ${SelectSection} ${SecDesktopIcon}
    ${Else}
        ${UnselectSection} ${SecDesktopIcon}
    ${EndIf}

    ; handle quick launch icon section
    ${If} $InstallQuickLaunchIcon == "1"
        ${SelectSection} ${SecQuickLaunchIcon}
    ${Else}
        ${UnselectSection} ${SecQuickLaunchIcon}
    ${EndIf}

    pop $1
    pop $0

!macroend

;-----------------------------------------------------------

!define CollectFromRegistry "!insertmacro CollectFromRegistry"
!define ParseCommandLine    "!insertmacro ParseCommandLine"
!define PresetSections      "!insertmacro PresetSections"
!define PreserveInRegistry  "!insertmacro PreserveInRegistry"

;-----------------------------------------------------------

!endif /* _PRESET_SECTIONS_NSH_ */
