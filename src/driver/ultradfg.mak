!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=..\bin
INTDIR=..\obj\driver

ALL : "$(OUTDIR)\ultradfg.sys"


CLEAN :
	-@erase "$(INTDIR)\analyse.obj"
	-@erase "$(INTDIR)\defrag.obj"
	-@erase "$(INTDIR)\int64.obj"
	-@erase "$(INTDIR)\io.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\ultradfg.obj"
	-@erase "$(INTDIR)\ultradfg.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ultradfg.sys"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)" /I "$(DDKINCDIR)\ddk" /I "$(DDKINCDIR)\ndk" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "NATIVE" /Fp"$(INTDIR)\ultradfg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ultradfg.res" /d "NDEBUG" 
	
LINK32=link.exe
LINK32_FLAGS=ntoskrnl.lib hal.lib /nologo /base:"0x10000" /entry:"DriverEntry" /incremental:no /pdb:"$(OUTDIR)\ultradfg.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ultradfg.sys" /driver /align:32 /subsystem:native 
LINK32_OBJS= \
	"$(INTDIR)\analyse.obj" \
	"$(INTDIR)\defrag.obj" \
	"$(INTDIR)\int64.obj" \
	"$(INTDIR)\io.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\ultradfg.obj" \
	"$(INTDIR)\ultradfg.res"

"$(OUTDIR)\ultradfg.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\ultradfg.rc

"$(INTDIR)\ultradfg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)

