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
* Universal code for both main and micro edition installers.
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

  ${If} $AtLeastXP == "1"
    WriteRegStr HKCR "Drive\shell\udefrag" "" "[--- &Ultra Defragmenter ---]"
    ; Without $SYSDIR because x64 system applies registry redirection for HKCR before writing.
    ; When we are using $SYSDIR Windows always converts them to C:\WINDOWS\SysWow64.
    WriteRegStr HKCR "Drive\shell\udefrag\command" "" "udctxhandler.cmd $\"%1$\""
  ${Else}
    DeleteRegKey HKCR "Drive\shell\udefrag"
  ${EndIf}

  WriteRegStr HKCR "Folder\shell\udefrag" "" "[--- &Ultra Defragmenter ---]"
  WriteRegStr HKCR "Folder\shell\udefrag\command" "" "udctxhandler.cmd $\"%1$\""

  WriteRegStr HKCR "*\shell\udefrag" "" "[--- &Ultra Defragmenter ---]"
  WriteRegStr HKCR "*\shell\udefrag\command" "" "udctxhandler.cmd $\"%1$\""

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
  WriteRegStr HKCR "LuaReport\DefaultIcon" "" "lua5.1a_gui.exe,1"
  WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
  WriteRegStr HKCR "LuaReport\shell\view\command" "" "lua5.1a_gui.exe $INSTDIR\scripts\udreportcnv.lua %1 $WINDIR -v"

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

  RMDir /r "$SYSDIR\UltraDefrag"
  Delete "$SYSDIR\udefrag-gui-dbg.cmd"
  Delete "$SYSDIR\udefrag-gui.exe"
  Delete "$SYSDIR\udefrag-gui.cmd"
  Delete "$SYSDIR\ultradefrag.exe"
  Delete "$SYSDIR\udefrag-gui-config.exe"
  Delete "$SYSDIR\udefrag-scheduler.exe"
  Delete "$SYSDIR\ud-config.cmd"

  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\presets"
  RMDir /r "$INSTDIR\logs"
  RMDir /r "$INSTDIR\portable_${ULTRADFGARCH}_package"

  Delete "$INSTDIR\scripts\udctxhandler.lua"
  Delete "$INSTDIR\dfrg.exe"
  Delete "$INSTDIR\INSTALL.TXT"
  Delete "$INSTDIR\FAQ.TXT"
  Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
  Delete "$INSTDIR\boot_on.cmd"
  Delete "$INSTDIR\boot_off.cmd"
  Delete "$INSTDIR\ud_i18n.dll"

  Delete "$INSTDIR\udefrag-scheduler.exe"
  Delete "$INSTDIR\ud_scheduler_i18n.lng"

  ; remove shortcuts of any previous version of the program
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\DASoft"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\FAQ.lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\User manual.url"
  Delete "$SMPROGRAMS\UltraDefrag\UltraDefrag (Debug mode).lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Portable package.lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Scheduler.NET.lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Scheduler.lnk"

  RMDir /r "$SMPROGRAMS\UltraDefrag\Boot time options"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Preferences"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Debugging information"
  
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

  ; 1. install default boot time script if it is not installed yet
  SetOutPath "$SYSDIR"
  ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
    File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
  ${EndUnless}
  
  ; 2. install GUI related config files
!ifndef MICRO_EDITION
  ; A. install default CSS for file fragmentation reports
  SetOutPath "$INSTDIR\scripts"
  ${Unless} ${FileExists} "$INSTDIR\scripts\udreport.css"
    File "${ROOTDIR}\src\scripts\udreport.css"
  ${EndUnless}
  ; B. install default report options
  SetOutPath "$INSTDIR\options"
  ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua"
    File "${ROOTDIR}\src\scripts\udreportopts.lua"
  ${Else}
    ${Unless} ${FileExists} "$INSTDIR\options\udreportopts.lua.old"
      ; this sequence ensures that we have the file in recent format
      Rename "$INSTDIR\options\udreportopts.lua" "$INSTDIR\options\udreportopts.lua.old"
      File "${ROOTDIR}\src\scripts\udreportopts.lua"
    ${EndUnless}
  ${EndUnless}
  ; C. write default GUI settings to guiopts.lua file
  SetOutPath "$INSTDIR"
  ; the options subdirectory must be already existing here
  ExecWait '"$INSTDIR\ultradefrag.exe" --setup'
!endif

!macroend

;-----------------------------------------

!macro UninstallTheProgram

  ${DisableX64FSRedirection}
  StrCpy $INSTDIR "$WINDIR\UltraDefrag"
  
!ifndef MICRO_EDITION
  DetailPrint "Remove shortcuts..."
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\UltraDefrag"
  Delete "$DESKTOP\UltraDefrag.lnk"
  Delete "$QUICKLAUNCH\UltraDefrag.lnk"
!endif

  DetailPrint "Deregister boot time defragmenter..."
  ${Unless} ${Silent}
  ExecWait '"$SYSDIR\bootexctrl.exe" /u defrag_native'
  ${Else}
  ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'
  ${EndUnless}

  DetailPrint "Remove installation directory..."
  ; safe, because installation directory is predefined
  RMDir /r $INSTDIR

  DetailPrint "Cleanup system directory..."
  Delete "$SYSDIR\boot-config.cmd"
  Delete "$SYSDIR\boot-off.cmd"
  Delete "$SYSDIR\boot-on.cmd"
  Delete "$SYSDIR\ud-boot-time.cmd"
  Delete "$SYSDIR\ud-help.cmd"
  Delete "$SYSDIR\udctxhandler.cmd"

  Delete "$SYSDIR\bootexctrl.exe"
  Delete "$SYSDIR\defrag_native.exe"
  Delete "$SYSDIR\hibernate4win.exe"
  Delete "$SYSDIR\udefrag.exe"

  Delete "$SYSDIR\udefrag.dll"
  Delete "$SYSDIR\udefrag-kernel.dll"
  Delete "$SYSDIR\zenwinx.dll"

!ifndef MICRO_EDITION
  Delete "$SYSDIR\lua5.1a.dll"
  Delete "$SYSDIR\lua5.1a.exe"
  Delete "$SYSDIR\lua5.1a_gui.exe"
  Delete "$SYSDIR\wgx.dll"
!endif

  DetailPrint "Cleanup registry..."
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
  DeleteRegKey HKLM "Software\UltraDefrag"

  DetailPrint "Uninstall the context menu handler..."
  DeleteRegKey HKCR "Drive\shell\udefrag"
  DeleteRegKey HKCR "Folder\shell\udefrag"
  DeleteRegKey HKCR "*\shell\udefrag"

!ifndef MICRO_EDITION
  DetailPrint "Deregister .luar file extension..."
  DeleteRegKey HKCR "LuaReport"
  DeleteRegKey HKCR ".luar"
!endif
  ${EnableX64FSRedirection}

!macroend

;-----------------------------------------

!macro WriteTheUninstaller

  push $R0

  DetailPrint "Write the uninstall keys..."
  SetOutPath "$INSTDIR"
  StrCpy $R0 "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
!ifdef MICRO_EDITION
  WriteRegStr   HKLM $R0 "DisplayName"     "Ultra Defragmenter Micro Edition"
!else
  WriteRegStr   HKLM $R0 "DisplayName"     "Ultra Defragmenter"
!endif
  WriteRegStr   HKLM $R0 "DisplayVersion"  "${ULTRADFGVER}"
  WriteRegStr   HKLM $R0 "Publisher"       "UltraDefrag Development Team"
  WriteRegStr   HKLM $R0 "URLInfoAbout"    "http://ultradefrag.sourceforge.net/"
  WriteRegStr   HKLM $R0 "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM $R0 "DisplayIcon"     '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM $R0 "InstallLocation" '"$INSTDIR"'
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

!define CheckWinVersion "!insertmacro CheckWinVersion"
!define SetContextMenuHandler "!insertmacro SetContextMenuHandler"
!define RemoveObsoleteFiles "!insertmacro RemoveObsoleteFiles"
!define InstallConfigFiles "!insertmacro InstallConfigFiles"
!define RegisterFileExtensions "!insertmacro RegisterFileExtensions"
!define InstallNativeDefragmenter "!insertmacro InstallNativeDefragmenter"
!define UninstallTheProgram "!insertmacro UninstallTheProgram"
!define WriteTheUninstaller "!insertmacro WriteTheUninstaller"
!define UpdateUninstallSizeValue "!insertmacro UpdateUninstallSizeValue"
