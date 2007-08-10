!IF "$(CFG)" == ""
CFG=Defrag - Win32 Release
!MESSAGE No configuration specified. Defaulting to Defrag - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Defrag - Win32 Release" && "$(CFG)" != "Defrag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/console

ALL : "$(OUTDIR)\Defrag.exe"


CLEAN :
	-@erase "$(INTDIR)\defrag.obj"
	-@erase "$(INTDIR)\defrag.res"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Defrag.exe"
!IF  "$(CFG)" != "Defrag - Win32 Release"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Defrag.ilk"
	-@erase "$(OUTDIR)\Defrag.pdb"
!ENDIF

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!IF  "$(CFG)" == "Defrag - Win32 Release"
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Defrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag.res" /d "NDEBUG" 
LINK32_FLAGS=kernel32.lib advapi32.lib msvcrt.lib ntdll.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\Defrag.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Defrag.exe" 
!ELSE
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Defrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\defrag.res" /d "_DEBUG" 
LINK32_FLAGS=kernel32.lib advapi32.lib msvcrt.lib ntdll.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\Defrag.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Defrag.exe" /pdbtype:sept 
!ENDIF

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
	
LINK32=link.exe
LINK32_OBJS= \
	"$(INTDIR)\defrag.obj" \
	"$(INTDIR)\defrag.res" \
	"$(INTDIR)\misc.obj"

"$(OUTDIR)\Defrag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<


SOURCE=.\defrag.rc

"$(INTDIR)\defrag.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=..\Shared\misc.c

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)
