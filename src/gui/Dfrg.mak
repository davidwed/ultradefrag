!IF "$(CFG)" == ""
CFG=Dfrg - Win32 Release
!MESSAGE No configuration specified. Defaulting to Dfrg - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Dfrg - Win32 Release" && "$(CFG)" != "Dfrg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=../bin
INTDIR=../obj/gui

ALL : "$(OUTDIR)\Dfrg.exe"


CLEAN :
	-@erase "$(INTDIR)\dfrg.res"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\map.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Dfrg.exe"
!IF  "$(CFG)" != "Dfrg - Win32 Release"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Dfrg.ilk"
	-@erase "$(OUTDIR)\Dfrg.pdb"
!ENDIF

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!IF  "$(CFG)" == "Dfrg - Win32 Release"
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Dfrg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dfrg.res" /d "NDEBUG" 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib msvcrt.lib ntdll.lib comdlg32.lib ..\lib\udefrag.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Dfrg.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Dfrg.exe" 
!ELSE
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "$(DDKINCDIR)\ddk" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Dfrg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dfrg.res" /d "_DEBUG" 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib msvcrt.lib ntdll.lib comdlg32.lib ..\lib\udefrag.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\Dfrg.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Dfrg.exe" /pdbtype:sept 
!ENDIF

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe

LINK32=link.exe
LINK32_OBJS= \
	"$(INTDIR)\dfrg.res" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\map.obj" \
	"$(INTDIR)\settings.obj"

"$(OUTDIR)\Dfrg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\dfrg.rc

"$(INTDIR)\dfrg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
