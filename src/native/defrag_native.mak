!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/native

ALL : "$(OUTDIR)\defrag_native.exe"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\defrag_native.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag_native.res" /d "NDEBUG" 

LINK32=link.exe
LINK32_FLAGS=ntdll.lib ..\lib\udefrag.lib ..\lib\zenwinx.lib /nologo /entry:"NtProcessStartup" /incremental:no /pdb:"$(OUTDIR)\defrag_native.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\defrag_native.exe" /subsystem:native 
LINK32_OBJS= \
	"$(INTDIR)\defrag_native.obj" \
	"$(INTDIR)\defrag_native.res"

"$(OUTDIR)\defrag_native.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\defrag_native.rc

"$(INTDIR)\defrag_native.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
