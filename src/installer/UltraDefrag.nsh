/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Universal code for both regular and micro edition installers.
*/

!ifndef _ULTRA_DEFRAG_NSH_
!define _ULTRA_DEFRAG_NSH_

/*
* 1. ${DisableX64FSRedirection} is required before
*    all macros except CheckWinVersion and UninstallTheProgram.
* 2. Most macros require the $INSTDIR variable
*    to be set and plugins directory to be initialized.
*    It is safe to do both initialization actions
*    in the .onInit function.
*/

Var AtLeastXP

!macro CheckWinVersion

  ${If} ${AtLeastWinXP}
    StrCpy $AtLeastXP 1
  ${Else}
    StrCpy $AtLeastXP 0
  ${EndIf}

  ${Unless} ${IsNT}
  ${OrUnless} ${AtLeastWinNT4}
    MessageBox MB_OK|MB_ICONEXCLAMATION \
     "On Windows 9x and NT 3.x this program is absolutely useless!$\nIf you are running another system then something is corrupt inside it.$\nTherefore we will try to continue." \
     /SD IDOK
    ;Abort
    /*
    * Sometimes on modern versions of Windows it fails.
    * This problem has been encountered on XP SP3 and Vista.
    * Therefore let's assume that we have at least XP system.
    * A dirty hack, but should work.
    */
    StrCpy $AtLeastXP 1
  ${EndUnless}
  
  /* this idea was suggested by bender647 at users.sourceforge.net */
  push $R0
  ClearErrors
  ;ReadEnvStr $R0 "PROCESSOR_ARCHITECTURE"
  ; On 64-bit systems it always returns 'x86' because the installer
  ; is a 32-bit application and runs on a virtual machine :(((

  ; read the PROCESSOR_ARCHITECTURE variable from registry
  ReadRegStr $R0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
   "PROCESSOR_ARCHITECTURE"

  ${Unless} ${Errors}
    ${If} $R0 == "x86"
    ${AndIf} ${ULTRADFGARCH} != "i386"
      MessageBox MB_OK|MB_ICONEXCLAMATION \
       "This installer cannot be used on 32-bit Windows!$\n \
       Download the i386 version from http://ultradefrag.sourceforge.net/" \
       /SD IDOK
      pop $R0
      Abort
    ${EndIf}
    ${If} $R0 == "amd64"
    ${AndIf} ${ULTRADFGARCH} != "amd64"
      MessageBox MB_OK|MB_ICONEXCLAMATION \
       "This installer cannot be used on x64 version of Windows!$\n \
       Download the amd64 version from http://ultradefrag.sourceforge.net/" \
       /SD IDOK
      pop $R0
      Abort
    ${EndIf}
    ${If} $R0 == "ia64"
    ${AndIf} ${ULTRADFGARCH} != "ia64"
      MessageBox MB_OK|MB_ICONEXCLAMATION \
       "This installer cannot be used on IA-64 version of Windows!$\n \
       Download the ia64 version from http://ultradefrag.sourceforge.net/" \
       /SD IDOK
      pop $R0
      Abort
    ${EndIf}
  ${EndUnless}
  pop $R0

!macroend

;-----------------------------------------

/**
 * @note Disables the x64 file system redirection.
 */
!macro ShowBootSplash

  ${Unless} ${Silent}
    push $R0

    ${EnableX64FSRedirection}
    SetOutPath $PLUGINSDIR
    File "${ROOTDIR}\src\installer\UltraDefrag.bmp"

    advsplash::show 2000 400 0 -1 "$PLUGINSDIR\UltraDefrag"
    pop $R0

    Delete "$PLUGINSDIR\UltraDefrag.bmp"
    ${DisableX64FSRedirection}

    pop $R0
  ${EndUnless}

!macroend

;-----------------------------------------

!macro InstallNativeDefragmenter

  ClearErrors
  Delete "$SYSDIR\defrag_native.exe"
  ${If} ${Errors}
      /*
      * If the native app depends from native DLL's
      * it may cause BSOD in case of inconsistency
      * between their versions. Therefore, we must
      * force upgrade to the latest monolythic native
      * defragmenter.
      */
      MessageBox MB_OK|MB_ICONSTOP \
       "Cannot update $SYSDIR\defrag_native.exe file!" \
       /SD IDOK
      ; try to recover the problem
      ExecWait '"$SYSDIR\bootexctrl.exe" /u defrag_native'
      ; the second attempt, just for safety
      ${EnableX64FSRedirection}
      SetOutPath $PLUGINSDIR
      File "bootexctrl.exe"
      ExecWait '"$PLUGINSDIR\bootexctrl.exe" /u defrag_native'
      ; abort an installation
      Abort
  ${EndIf}
  File "defrag_native.exe"

!macroend

;-----------------------------------------

!macro SetContextMenuHandler

  push $0
  push $1
  push $2
  
  StrCpy $0 "$\"$SYSDIR\udefrag.exe$\" --shellex $\"%1$\""
  StrCpy $1 "$INSTDIR\shellex.ico"
  StrCpy $2 "[--- &Ultra Defragmenter ---]"

  ${If} $AtLeastXP == "1"
    WriteRegStr HKCR "Drive\shell\udefrag"         ""     $2
    WriteRegStr HKCR "Drive\shell\udefrag"         "Icon" $1
    WriteRegStr HKCR "Drive\shell\udefrag\command" ""     $0
  ${Else}
    DeleteRegKey HKCR "Drive\shell\udefrag"
  ${EndIf}

  WriteRegStr HKCR "Folder\shell\udefrag"         ""     $2
  WriteRegStr HKCR "Folder\shell\udefrag"         "Icon" $1
  WriteRegStr HKCR "Folder\shell\udefrag\command" ""     $0

  WriteRegStr HKCR "*\shell\udefrag"         ""     $2
  WriteRegStr HKCR "*\shell\udefrag"         "Icon" $1
  WriteRegStr HKCR "*\shell\udefrag\command" ""     $0
  
  pop $2
  pop $1
  pop $0

!macroend

;-----------------------------------------

!macro RegisterFileExtensions

  push $R0
  push $0

  DetailPrint "Register file extensions..."
  ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
  ; When we are using $SYSDIR Windows always converts them to C:\WINDOWS\SysWow64.

  WriteRegStr HKCR ".luar" "" "LuaReport"
  WriteRegStr HKCR "LuaReport" "" "Lua Report"
  WriteRegStr HKCR "LuaReport\DefaultIcon" "" "$INSTDIR\lua5.1a_gui.exe,1"
  WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
  WriteRegStr HKCR "LuaReport\shell\view\command" "" "$\"$INSTDIR\lua5.1a_gui.exe$\" $\"$INSTDIR\scripts\udreportcnv.lua$\" $\"%1$\" $\"$INSTDIR$\" -v"

  ClearErrors
  ReadRegStr $R0 HKCR ".lua" ""
  ${If} ${Errors}
    WriteRegStr HKCR ".lua" "" "Lua.Script"
    WriteRegStr HKCR "Lua.Script" "" "Lua Script File"
    WriteRegStr HKCR "Lua.Script\shell\Edit" "" "Edit Script"
    WriteRegStr HKCR "Lua.Script\shell\Edit\command" "" "notepad.exe %1"
  ${Else}
    StrCpy $0 $R0
    ClearErrors
    ReadRegStr $R0 HKCR "$0\shell\Edit" ""
    ${If} ${Errors}
      WriteRegStr HKCR "$0\shell\Edit" "" "Edit Script"
      WriteRegStr HKCR "$0\shell\Edit\command" "" "notepad.exe %1"
    ${EndIf}
  ${EndIf}

  ClearErrors
  ReadRegStr $R0 HKCR ".lng" ""
  ${If} ${Errors}
    WriteRegStr HKCR ".lng" "" "LanguagePack"
    WriteRegStr HKCR "LanguagePack" "" "Language Pack"
    WriteRegStr HKCR "LanguagePack\shell\open" "" "Open"
    WriteRegStr HKCR "LanguagePack\DefaultIcon" "" "shell32.dll,0"
    WriteRegStr HKCR "LanguagePack\shell\open\command" "" "notepad.exe %1"
  ${EndIf}
  
  pop $0
  pop $R0

!macroend

;-----------------------------------------

!macro RemoveObsoleteFiles

  ; remove files of previous installations
  DeleteRegKey HKLM "SYSTEM\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Control\UltraDefrag"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Control\UltraDefrag"
  SetRegView 64
  DeleteRegKey HKLM "Software\UltraDefrag"
  SetRegView 32
  DeleteRegKey HKLM "Software\UltraDefrag"

  RMDir /r "$SYSDIR\UltraDefrag"
  Delete "$SYSDIR\udefrag-gui-dbg.cmd"
  Delete "$SYSDIR\udefrag-gui.exe"
  Delete "$SYSDIR\udefrag-gui.cmd"
  Delete "$SYSDIR\ultradefrag.exe"
  Delete "$SYSDIR\udefrag-gui-config.exe"
  Delete "$SYSDIR\udefrag-scheduler.exe"
  Delete "$SYSDIR\ud-config.cmd"
  Delete "$SYSDIR\udefrag-kernel.dll"
  Delete "$SYSDIR\wgx.dll"
  Delete "$SYSDIR\lua5.1a.dll"
  Delete "$SYSDIR\lua5.1a.exe"
  Delete "$SYSDIR\lua5.1a_gui.exe"
  Delete "$SYSDIR\udctxhandler.cmd"
  Delete "$SYSDIR\udctxhandler.vbs"

  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\presets"
  RMDir /r "$INSTDIR\logs"
  RMDir /r "$INSTDIR\portable_${ULTRADFGARCH}_package"
  RMDir /r "$INSTDIR\i18n\gui"
  RMDir /r "$INSTDIR\i18n\gui-config"

  Delete "$INSTDIR\scripts\udctxhandler.lua"
  Delete "$INSTDIR\dfrg.exe"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\FAQ.TXT"
  Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
  Delete "$INSTDIR\boot_on.cmd"
  Delete "$INSTDIR\boot_off.cmd"
  Delete "$INSTDIR\ud_i18n.dll"

  Delete "$INSTDIR\udefrag-scheduler.exe"
  Delete "$INSTDIR\*.lng"
  Delete "$INSTDIR\udefrag-gui-config.exe"
  Delete "$INSTDIR\LanguageSelector.exe"

  ; remove shortcuts of any previous version of the program
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\DASoft"
  RMDir /r "$SMPROGRAMS\UltraDefrag"
  
  Delete "$SYSDIR\Drivers\ultradfg.sys"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Enum\Root\LEGACY_ULTRADFG"

!macroend

;-----------------------------------------

/**
 * This procedure installs all configuration
 * files and upgrades already existing files
 * if they are in obsolete format.
 */
!macro InstallConfigFiles

  ; 1. install boot time script if it is not installed yet
  SetOutPath "$SYSDIR"
  ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
    File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
  ${EndUnless}
  
  ; 2. install report options and style
  ${If} $InstallGUI == "1"
      ; A. install default CSS for file fragmentation reports
      SetOutPath "$INSTDIR\scripts"
      ${If} ${FileExists} "$INSTDIR\scripts\udreport.css"
        ${Unless} ${FileExists} "$INSTDIR\scripts\udreport.css.old"
          ; ensure that user's choice will not be lost
          Rename "$INSTDIR\scripts\udreport.css" "$INSTDIR\scripts\udreport.css.old"
        ${EndUnless}
      ${EndIf}
      File "${ROOTDIR}\src\scripts\udreport.css"
      ; B. install default report options
      SetOutPath "$INSTDIR\options"
      ${If} ${FileExists} "$INSTDIR\options\udreportopts.lua"
        ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua.old"
          ; ensure that user's choice will not be lost
          Rename "$INSTDIR\options\udreportopts.lua" "$INSTDIR\options\udreportopts.lua.old"
        ${EndUnless}
      ${EndIf}
      File "${ROOTDIR}\src\scripts\udreportopts.lua"
  ${Else}
      Delete "$INSTDIR\scripts\udreport.css"
      Delete "$INSTDIR\options\udreportopts.lua"
  ${EndIf}

!macroend

;-----------------------------------------

!macro UninstallTheProgram

  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"
  
  DetailPrint "Remove shortcuts..."
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\UltraDefrag"
  Delete "$SMPROGRAMS\UltraDefrag.lnk"
  Delete "$DESKTOP\UltraDefrag.lnk"
  Delete "$QUICKLAUNCH\UltraDefrag.lnk"

  DetailPrint "Deregister boot time defragmenter..."
  ${Unless} ${Silent}
    ExecWait '"$SYSDIR\bootexctrl.exe" /u defrag_native'
  ${Else}
    ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'
  ${EndUnless}
  Delete "$WINDIR\pending-boot-off"

  DetailPrint "Remove installation directory..."
  ; safe, because installation directory is predefined
  RMDir /r $INSTDIR
  
  DetailPrint "Cleanup system directory..."
  Delete "$SYSDIR\boot-config.cmd"
  Delete "$SYSDIR\boot-off.cmd"
  Delete "$SYSDIR\boot-on.cmd"
  Delete "$SYSDIR\ud-boot-time.cmd"
  Delete "$SYSDIR\ud-help.cmd"
  Delete "$SYSDIR\bootexctrl.exe"
  Delete "$SYSDIR\defrag_native.exe"
  Delete "$SYSDIR\hibernate4win.exe"
  Delete "$SYSDIR\udefrag.exe"
  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\zenwinx.dll"

  DetailPrint "Cleanup registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"

  DetailPrint "Uninstall the context menu handler..."
  DeleteRegKey HKCR "Drive\shell\udefrag"
  DeleteRegKey HKCR "Folder\shell\udefrag"
  DeleteRegKey HKCR "*\shell\udefrag"

  DetailPrint "Deregister .luar file extension..."
  DeleteRegKey HKCR "LuaReport"
  DeleteRegKey HKCR ".luar"
  ${EnableX64FSRedirection}

!macroend

;-----------------------------------------

!macro WriteTheUninstaller

  push $R0

  DetailPrint "Write the uninstall keys..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"

  WriteRegStr   HKLM $R0 "DisplayName"     "Ultra Defragmenter"
  WriteRegStr   HKLM $R0 "DisplayVersion"  "${ULTRADFGVER}"
  WriteRegStr   HKLM $R0 "Publisher"       "UltraDefrag Development Team"
  WriteRegStr   HKLM $R0 "URLInfoAbout"    "http://ultradefrag.sourceforge.net/"
  WriteRegStr   HKLM $R0 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM $R0 "DisplayIcon"     "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM $R0 "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM $R0 "NoModify" 1
  WriteRegDWORD HKLM $R0 "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  ${UpdateUninstallSizeValue}
  
  pop $R0

!macroend

;-----------------------------------------

!macro UpdateUninstallSizeValue

  push $R0
  push $0
  push $1
  push $2

  ; update the uninstall size value
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  WriteRegDWORD HKLM $R0 "EstimatedSize" "$0"
  
  pop $2
  pop $1
  pop $0
  pop $R0

!macroend

;-----------------------------------------

!define CheckWinVersion           "!insertmacro CheckWinVersion"
!define ShowBootSplash            "!insertmacro ShowBootSplash"
!define SetContextMenuHandler     "!insertmacro SetContextMenuHandler"
!define RemoveObsoleteFiles       "!insertmacro RemoveObsoleteFiles"
!define InstallConfigFiles        "!insertmacro InstallConfigFiles"
!define RegisterFileExtensions    "!insertmacro RegisterFileExtensions"
!define InstallNativeDefragmenter "!insertmacro InstallNativeDefragmenter"
!define UninstallTheProgram       "!insertmacro UninstallTheProgram"
!define WriteTheUninstaller       "!insertmacro WriteTheUninstaller"
!define UpdateUninstallSizeValue  "!insertmacro UpdateUninstallSizeValue"

!endif /* _ULTRA_DEFRAG_NSH_ */
