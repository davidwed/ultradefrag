!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/gui

ALL : "$(OUTDIR)\inst_helper.exe"


CLEAN :
	-@erase "$(INTDIR)\inst_helper.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\inst_helper.exe"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\inst_helper.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib msvcrt.lib ntdll.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\inst_helper.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\inst_helper.exe" 

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_OBJS= "$(INTDIR)\inst_helper.obj"

"$(OUTDIR)\inst_helper.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
