!IF "$(CFG)" == ""
CFG=winx - Win32 Debug
!MESSAGE No configuration specified. Defaulting to winx - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "winx - Win32 Release" && "$(CFG)" != "winx - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=./bin
INTDIR=./obj

ALL : "$(OUTDIR)\winx.dll"


CLEAN :
	-@erase "$(INTDIR)\winx.obj"
	-@erase "$(INTDIR)\winx.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\winx.dll"
	-@erase "$(OUTDIR)\winx.exp"
	-@erase "$(OUTDIR)\winx.lib"
!IF "$(CFG)" != "winx - Win32 Release"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\winx.ilk"
	-@erase "$(OUTDIR)\winx.pdb"
!ENDIF

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

!IF "$(CFG)" == "winx - Win32 Release"
CPP_PROJ=/nologo /Gd /MT /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "winx_EXPORTS" /Fp"$(INTDIR)\winx.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winx.res" /d "NDEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:no /pdb:"$(OUTDIR)\winx.dll.pdb" /machine:I386 /nodefaultlib /def:".\winx.def" /out:"$(OUTDIR)\winx.dll" /implib:"$(OUTDIR)\winx.lib" 
!ELSE
CPP_PROJ=/nologo /Gd /MTd /W3 /Gm /GX /ZI /Od /I "$(DDKINCDIR)\ddk" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "winx_EXPORTS" /Fp"$(INTDIR)\winx.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winx.res" /d "_DEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:yes /pdb:"$(OUTDIR)\winx.dll.pdb" /debug /machine:I386 /nodefaultlib /def:".\winx.def" /out:"$(OUTDIR)\winx.dll" /implib:"$(OUTDIR)\winx.lib" /pdbtype:sept 
!ENDIF

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe

LINK32=link.exe
DEF_FILE= \
	".\winx.def"
LINK32_OBJS= \
	"$(INTDIR)\winx.obj" \
	"$(INTDIR)\winx.res"

"$(OUTDIR)\winx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\winx.rc

"$(INTDIR)\winx.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
