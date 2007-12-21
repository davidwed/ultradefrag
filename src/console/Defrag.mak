!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/console

ALL : "$(OUTDIR)\udefrag.exe"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Defrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag.res" /d "NDEBUG" 
LINK32_FLAGS=kernel32.lib advapi32.lib msvcrt.lib ntdll.lib ..\lib\udefrag.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\Defrag.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\udefrag.exe" 

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
	
LINK32=link.exe
LINK32_OBJS= \
	"$(INTDIR)\defrag.obj" \
	"$(INTDIR)\defrag.res"

"$(OUTDIR)\udefrag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<


SOURCE=.\defrag.rc

"$(INTDIR)\defrag.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
