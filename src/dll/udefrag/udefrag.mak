!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../../bin
INTDIR=../../obj/dll/udefrag.dll

ALL : "$(OUTDIR)\udefrag.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /Gd /MT /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "UDEFRAG_EXPORTS" /Fp"$(INTDIR)\udefrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\udefrag.res" /d "NDEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:no /pdb:"$(OUTDIR)\udefrag.dll.pdb" /machine:I386 /nodefaultlib /def:".\udefrag.def" /out:"$(OUTDIR)\udefrag.dll" /implib:"$(OUTDIR)\udefrag.lib" 

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
	"$(INTDIR)\cfuncs.obj" \
	"$(INTDIR)\sfuncs.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\udefrag.res"

"$(OUTDIR)\udefrag.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\udefrag.rc

"$(INTDIR)\udefrag.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
