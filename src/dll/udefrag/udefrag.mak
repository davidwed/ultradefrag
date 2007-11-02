!IF "$(CFG)" == ""
CFG=udefrag - Win32 Debug
!MESSAGE No configuration specified. Defaulting to udefrag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "udefrag - Win32 Release" && "$(CFG)" != "udefrag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../../bin
INTDIR=../../obj/dll/udefrag.dll

ALL : "$(OUTDIR)\udefrag.dll"


CLEAN :
	-@erase "$(INTDIR)\udefrag.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\stdio.obj"
	-@erase "$(INTDIR)\keytrans.obj"
	-@erase "$(INTDIR)\sfuncs.obj"
	-@erase "$(INTDIR)\udefrag.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\udefrag.dll"
	-@erase "$(OUTDIR)\udefrag.exp"
	-@erase "$(OUTDIR)\udefrag.lib"
!IF "$(CFG)" != "udefrag - Win32 Release"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\udefrag.dll.ilk"
	-@erase "$(OUTDIR)\udefrag.dll.pdb"
!ENDIF

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

!IF "$(CFG)" == "udefrag - Win32 Release"
CPP_PROJ=/nologo /Gd /MT /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "UDEFRAG_EXPORTS" /Fp"$(INTDIR)\udefrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\udefrag.res" /d "NDEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:no /pdb:"$(OUTDIR)\udefrag.dll.pdb" /machine:I386 /nodefaultlib /def:".\udefrag.def" /out:"$(OUTDIR)\udefrag.dll" /implib:"$(OUTDIR)\udefrag.lib" 
!ELSE
CPP_PROJ=/nologo /Gd /MTd /W3 /Gm /GX /ZI /Od /I "$(DDKINCDIR)\ddk" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "UDEFRAG_EXPORTS" /Fp"$(INTDIR)\udefrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\udefrag.res" /d "_DEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:yes /pdb:"$(OUTDIR)\udefrag.dll.pdb" /debug /machine:I386 /nodefaultlib /def:".\udefrag.def" /out:"$(OUTDIR)\udefrag.dll" /implib:"$(OUTDIR)\udefrag.lib" /pdbtype:sept 
!ENDIF

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe

LINK32=link.exe
DEF_FILE= \
	".\udefrag.def"
LINK32_OBJS= \
	"$(INTDIR)\udefrag.obj" \
	"$(INTDIR)\settings.obj" \
	"$(INTDIR)\stdio.obj" \
	"$(INTDIR)\keytrans.obj" \
	"$(INTDIR)\sfuncs.obj" \
	"$(INTDIR)\udefrag.res"

"$(OUTDIR)\udefrag.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\udefrag.rc

"$(INTDIR)\udefrag.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
