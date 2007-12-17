!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=./bin
INTDIR=./obj

ALL : "$(OUTDIR)\zenwinx.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /Gd /MT /W3 /GX /O2 /I "$(DDKINCDIR)\ddk" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_USRDLL" /D "zenwinx_EXPORTS" /Fp"$(INTDIR)\zenwinx.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\zenwinx.res" /d "NDEBUG" 
LINK32_FLAGS=ntdll.lib /nologo /entry:"DllMain" /subsystem:console /dll /incremental:no /pdb:"$(OUTDIR)\zenwinx.dll.pdb" /machine:I386 /nodefaultlib /def:".\zenwinx.def" /out:"$(OUTDIR)\zenwinx.dll" /implib:"$(OUTDIR)\zenwinx.lib" 

CPP=cl.exe

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe

LINK32=link.exe
DEF_FILE= \
	".\zenwinx.def"
LINK32_OBJS= \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\keytrans.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\messages.obj" \
	"$(INTDIR)\stdio.obj" \
	"$(INTDIR)\zenwinx.obj" \
	"$(INTDIR)\zenwinx.res"

"$(OUTDIR)\zenwinx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=.\zenwinx.rc

"$(INTDIR)\zenwinx.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)
