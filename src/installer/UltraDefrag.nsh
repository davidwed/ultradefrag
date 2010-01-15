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
* Universal code for both main and micro edition installers.
*/

;!define INCLUDE_KERNEL_MODE_DRIVER

Var AtLeastXP

!macro CheckWinVersion

  ${Unless} ${IsNT}
  ${OrUnless} ${AtLeastWinNT4}
    MessageBox MB_OK|MB_ICONEXCLAMATION \
     "On Windows 9x and NT 3.x this program is absolutely useless!" \
     /SD IDOK
    Abort
  ${EndUnless}
  
  ${If} ${AtLeastWinXP}
    StrCpy $AtLeastXP 1
  ${Else}
    StrCpy $AtLeastXP 0
  ${EndIf}

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

!ifdef INCLUDE_KERNEL_MODE_DRIVER
;-----------------------------------------

Function WriteDriverAndDbgSettings

  push $R2
  push $R3
  ; write settings only if control set exists
  StrCpy $R3 0
  EnumRegKey $R2 HKLM $R0 $R3
  ${If} $R2 != ""
  ${OrIf} $R0 == "SYSTEM\CurrentControlSet"
    WriteRegStr HKLM "$R0\Services\ultradfg" "DisplayName" "ultradfg"
    WriteRegDWORD HKLM "$R0\Services\ultradfg" "ErrorControl" 0x0
    WriteRegExpandStr HKLM "$R0\Services\ultradfg" "ImagePath" "System32\DRIVERS\ultradfg.sys"
    WriteRegDWORD HKLM "$R0\Services\ultradfg" "Start" 0x3
    WriteRegDWORD HKLM "$R0\Services\ultradfg" "Type" 0x1

    ; not acceptable by the running server systems
    ;;;WriteRegDWORD HKLM "$R0\Control\CrashControl" "AutoReboot" 0x0
  ${EndIf}
  pop $R3
  pop $R2

FunctionEnd

;-----------------------------------------

!macro WriteDriverAndDbgSettingsMacro

  push $R0
  StrCpy $R0 "SYSTEM\CurrentControlSet"
  call WriteDriverAndDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet001"
  call WriteDriverAndDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet002"
  call WriteDriverAndDbgSettings
  StrCpy $R0 "SYSTEM\ControlSet003"
  call WriteDriverAndDbgSettings
  pop $R0

!macroend

;-----------------------------------------
!define WriteDriverAndDbgSettings "!insertmacro WriteDriverAndDbgSettingsMacro"
!endif /* INCLUDE_KERNEL_MODE_DRIVER */

!macro RemoveObsoleteFilesMacro

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

  ; remove shortcuts of any previous version of the program
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\DASoft"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\FAQ.lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Documentation\User manual.url"
  Delete "$SMPROGRAMS\UltraDefrag\UltraDefrag (Debug mode).lnk"
  Delete "$SMPROGRAMS\UltraDefrag\Portable package.lnk"

  RMDir /r "$SMPROGRAMS\UltraDefrag\Boot time options"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Preferences"
  RMDir /r "$SMPROGRAMS\UltraDefrag\Debugging information"
  
!ifndef INCLUDE_KERNEL_MODE_DRIVER
  Delete "$SYSDIR\Drivers\ultradfg.sys"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet001\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet002\Enum\Root\LEGACY_ULTRADFG"
  DeleteRegKey HKLM "SYSTEM\ControlSet003\Enum\Root\LEGACY_ULTRADFG"
!endif

!macroend

;-----------------------------------------

!define CheckWinVersion "!insertmacro CheckWinVersion"
!define SetContextMenuHandler "!insertmacro SetContextMenuHandler"
!define RemoveObsoleteFiles "!insertmacro RemoveObsoleteFilesMacro"
