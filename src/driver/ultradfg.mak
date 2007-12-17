!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=..\bin
INTDIR=..\obj\driver

!IF "$(NT4_TARGET)" == "true"
ALL: "$(OUTDIR)\ultradfg_nt4.sys"
!ELSE
ALL: "$(OUTDIR)\ultradfg.sys"
!ENDIF

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ultradfg.res" /d "NDEBUG" /d "NT4_TARGET"
!ELSE
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ultradfg.res" /d "NDEBUG" 
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

