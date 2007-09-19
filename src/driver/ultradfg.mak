!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=..\bin
INTDIR=..\obj\driver

ALL :
!IF "$(NT4_TARGET)" == "true"
	"$(OUTDIR)\ultradfg_nt4.sys"
!ELSE
	"$(OUTDIR)\ultradfg.sys"
!ENDIF

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
	-@erase "$(OUTDIR)\ultradfg_nt4.sys"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
!IF "$(NT4_TARGET)" == "true"
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)" /I "$(DDKINCDIR)\ddk" /I "$(DDKINCDIR)\ndk" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "NATIVE" /D "NT4_TARGET" /Fp"$(INTDIR)\ultradfg_nt4.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ELSE
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)" /I "$(DDKINCDIR)\ddk" /I "$(DDKINCDIR)\ndk" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "NATIVE" /Fp"$(INTDIR)\ultradfg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
!IF "$(NT4_TARGET)" == "true"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ultradfg.res" /d "NDEBUG" 
!ELSE
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ultradfg.res" /d "NDEBUG" /d "NT4_TARGET"
!ENDIF
	
LINK32=link.exe
!IF "$(NT4_TARGET)" == "true"
LINK32_FLAGS=ntoskrnl.lib hal.lib /nologo /base:"0x10000" /entry:"DriverEntry" /incremental:no /pdb:"$(OUTDIR)\ultradfg_nt4.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ultradfg_nt4.sys" /driver /align:32 /subsystem:native 
!ELSE
LINK32_FLAGS=ntoskrnl.lib hal.lib /nologo /base:"0x10000" /entry:"DriverEntry" /incremental:no /pdb:"$(OUTDIR)\ultradfg.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ultradfg.sys" /driver /align:32 /subsystem:native 
!ENDIF
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

!IF "$(NT4_TARGET)" == "true"
"$(OUTDIR)\ultradfg_nt4.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
!ELSE
"$(OUTDIR)\ultradfg.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
!ENDIF

SOURCE=.\ultradfg.rc

"$(INTDIR)\ultradfg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)

