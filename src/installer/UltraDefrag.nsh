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

!macro UD_CheckWinVersion

  ${Unless} ${IsNT}
  ${OrUnless} ${AtLeastWinNT4}
    MessageBox MB_OK|MB_ICONEXCLAMATION \
     "On Windows 9x and NT 3.x this program is absolutely useless!" \
     /SD IDOK
    Abort
  ${EndUnless}

  /* this idea was suggested by bender647 at users.sourceforge.net */
  push $R0
  ClearErrors
  ReadEnvStr $R0 "PROCESSOR_ARCHITECTURE"
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

!define UD_CheckWinVersion "!insertmacro UD_CheckWinVersion"
