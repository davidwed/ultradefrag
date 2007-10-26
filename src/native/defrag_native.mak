!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/native

!IF "$(NT4_TARGET)" == "true"
ALL : "$(OUTDIR)\defrag_native_nt4.exe"
!ELSE
ALL : "$(OUTDIR)\defrag_native.exe"
!ENDIF

CLEAN :
	-@erase "$(INTDIR)\defrag_native.obj"
	-@erase "$(INTDIR)\defrag_native.res"
	-@erase "$(INTDIR)\keytrans.obj"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\stdio.obj"
	-@erase "$(INTDIR)\sys.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\defrag_native.exe"
	-@erase "$(OUTDIR)\defrag_native_nt4.exe"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe

!IF  "$(CFG)" != "WinDDK"

!IF "$(NT4_TARGET)" == "true"
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "NT4_TARGET" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\defrag_native_nt4.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ELSE
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "NDEBUG" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\defrag_native.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ENDIF

!ELSE

!IF "$(NT4_TARGET)" == "true"
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "USE_WINDDK" /D "X86_WINDDK_BUILD" /D "NT4_TARGET" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\defrag_native_nt4.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ELSE
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "USE_WINDDK" /D "X86_WINDDK_BUILD" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\defrag_native.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
!ENDIF

!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
!IF "$(NT4_TARGET)" == "true"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag_native.res" /d "NDEBUG" /d "NT4_TARGET"
!ELSE
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag_native.res" /d "NDEBUG" 
!ENDIF

LINK32=link.exe
!IF "$(NT4_TARGET)" == "true"
LINK32_FLAGS=ntdll.lib ..\lib\udefrag.lib /nologo /entry:"NtProcessStartup" /incremental:no /pdb:"$(OUTDIR)\defrag_native_nt4.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\defrag_native_nt4.exe" /subsystem:native 
!ELSE
LINK32_FLAGS=ntdll.lib ..\lib\udefrag.lib /nologo /entry:"NtProcessStartup" /incremental:no /pdb:"$(OUTDIR)\defrag_native.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\defrag_native.exe" /subsystem:native 
!ENDIF
LINK32_OBJS= \
	"$(INTDIR)\defrag_native.obj" \
	"$(INTDIR)\defrag_native.res" \
	"$(INTDIR)\keytrans.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\stdio.obj" \
	"$(INTDIR)\sys.obj" \
	"$(INTDIR)\misc.obj"

!IF "$(NT4_TARGET)" == "true"
"$(OUTDIR)\defrag_native_nt4.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
!ELSE
"$(OUTDIR)\defrag_native.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
!ENDIF

SOURCE=.\defrag_native.rc

"$(INTDIR)\defrag_native.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=..\Shared\misc.c

"$(INTDIR)\misc.obj"	"$(INTDIR)\misc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)
