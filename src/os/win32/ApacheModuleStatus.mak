# Microsoft Developer Studio Generated NMAKE File, Based on ApacheModuleStatus.dsp
!IF "$(CFG)" == ""
CFG=ApacheModuleStatus - Win32 Release
!MESSAGE No configuration specified. Defaulting to ApacheModuleStatus - Win32\
 Release.
!ENDIF 

!IF "$(CFG)" != "ApacheModuleStatus - Win32 Release" && "$(CFG)" !=\
 "ApacheModuleStatus - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ApacheModuleStatus.mak"\
 CFG="ApacheModuleStatus - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ApacheModuleStatus - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ApacheModuleStatus - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ApacheModuleStatus - Win32 Release"

OUTDIR=.\ApacheModuleStatusR
INTDIR=.\ApacheModuleStatusR
# Begin Custom Macros
OutDir=.\.\ApacheModuleStatusR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ApacheModuleStatus.dll"

!ELSE 

ALL : "$(OUTDIR)\ApacheModuleStatus.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mod_status.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\ApacheModuleStatus.dll"
	-@erase "$(OUTDIR)\ApacheModuleStatus.exp"
	-@erase "$(OUTDIR)\ApacheModuleStatus.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\regex" /I "..\..\core" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\ApacheModuleStatus.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ApacheModuleStatusR/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ApacheModuleStatus.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\CoreR\ApacheCore.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\ApacheModuleStatus.pdb" /machine:I386\
 /out:"$(OUTDIR)\ApacheModuleStatus.dll"\
 /implib:"$(OUTDIR)\ApacheModuleStatus.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mod_status.obj"

"$(OUTDIR)\ApacheModuleStatus.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ApacheModuleStatus - Win32 Debug"

OUTDIR=.\ApacheModuleStatusD
INTDIR=.\ApacheModuleStatusD
# Begin Custom Macros
OutDir=.\.\ApacheModuleStatusD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ApacheModuleStatus.dll"

!ELSE 

ALL : "$(OUTDIR)\ApacheModuleStatus.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mod_status.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\ApacheModuleStatus.dll"
	-@erase "$(OUTDIR)\ApacheModuleStatus.exp"
	-@erase "$(OUTDIR)\ApacheModuleStatus.ilk"
	-@erase "$(OUTDIR)\ApacheModuleStatus.lib"
	-@erase "$(OUTDIR)\ApacheModuleStatus.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\regex" /I "..\..\core" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\ApacheModuleStatus.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ApacheModuleStatusD/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ApacheModuleStatus.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\CoreD\ApacheCore.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\ApacheModuleStatus.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\ApacheModuleStatus.dll"\
 /implib:"$(OUTDIR)\ApacheModuleStatus.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mod_status.obj"

"$(OUTDIR)\ApacheModuleStatus.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(CFG)" == "ApacheModuleStatus - Win32 Release" || "$(CFG)" ==\
 "ApacheModuleStatus - Win32 Debug"
SOURCE=..\..\modules\standard\mod_status.c

!IF  "$(CFG)" == "ApacheModuleStatus - Win32 Release"

DEP_CPP_MOD_S=\
	"..\..\core\alloc.h"\
	"..\..\core\buff.h"\
	"..\..\core\conf.h"\
	"..\..\core\http_config.h"\
	"..\..\core\http_core.h"\
	"..\..\core\http_log.h"\
	"..\..\core\http_main.h"\
	"..\..\core\http_protocol.h"\
	"..\..\core\httpd.h"\
	"..\..\core\scoreboard.h"\
	"..\..\core\util_script.h"\
	"..\..\regex\regex.h"\
	".\readdir.h"\
	

"$(INTDIR)\mod_status.obj" : $(SOURCE) $(DEP_CPP_MOD_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ApacheModuleStatus - Win32 Debug"


"$(INTDIR)\mod_status.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

