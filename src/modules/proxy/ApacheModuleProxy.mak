# Microsoft Developer Studio Generated NMAKE File, Based on ApacheModuleProxy.dsp
!IF "$(CFG)" == ""
CFG=ApacheModuleProxy - Win32 Release
!MESSAGE No configuration specified. Defaulting to ApacheModuleProxy - Win32\
 Release.
!ENDIF 

!IF "$(CFG)" != "ApacheModuleProxy - Win32 Release" && "$(CFG)" !=\
 "ApacheModuleProxy - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ApacheModuleProxy.mak"\
 CFG="ApacheModuleProxy - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ApacheModuleProxy - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ApacheModuleProxy - Win32 Debug" (based on\
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

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ApacheModuleProxy.dll"

!ELSE 

ALL : "ApacheCore - Win32 Release" "$(OUTDIR)\ApacheModuleProxy.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ApacheCore - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\mod_proxy.obj"
	-@erase "$(INTDIR)\proxy_cache.obj"
	-@erase "$(INTDIR)\proxy_connect.obj"
	-@erase "$(INTDIR)\proxy_ftp.obj"
	-@erase "$(INTDIR)\proxy_http.obj"
	-@erase "$(INTDIR)\proxy_util.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\ApacheModuleProxy.dll"
	-@erase "$(OUTDIR)\ApacheModuleProxy.exp"
	-@erase "$(OUTDIR)\ApacheModuleProxy.lib"
	-@erase "$(OUTDIR)\ApacheModuleProxy.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "SHARED_MODULE" /D "WIN32_LEAN_AND_MEAN" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ApacheModuleProxy.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\CoreR\ApacheCore.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ws2_32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\ApacheModuleProxy.pdb"\
 /map:"$(INTDIR)\ApacheModuleProxy.map" /machine:I386\
 /out:"$(OUTDIR)\ApacheModuleProxy.dll"\
 /implib:"$(OUTDIR)\ApacheModuleProxy.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mod_proxy.obj" \
	"$(INTDIR)\proxy_cache.obj" \
	"$(INTDIR)\proxy_connect.obj" \
	"$(INTDIR)\proxy_ftp.obj" \
	"$(INTDIR)\proxy_http.obj" \
	"$(INTDIR)\proxy_util.obj" \
	"..\..\CoreR\ApacheCore.lib"

"$(OUTDIR)\ApacheModuleProxy.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ApacheModuleProxy.dll"

!ELSE 

ALL : "ApacheCore - Win32 Debug" "$(OUTDIR)\ApacheModuleProxy.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ApacheCore - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\mod_proxy.obj"
	-@erase "$(INTDIR)\proxy_cache.obj"
	-@erase "$(INTDIR)\proxy_connect.obj"
	-@erase "$(INTDIR)\proxy_ftp.obj"
	-@erase "$(INTDIR)\proxy_http.obj"
	-@erase "$(INTDIR)\proxy_util.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\ApacheModuleProxy.dll"
	-@erase "$(OUTDIR)\ApacheModuleProxy.exp"
	-@erase "$(OUTDIR)\ApacheModuleProxy.lib"
	-@erase "$(OUTDIR)\ApacheModuleProxy.map"
	-@erase "$(OUTDIR)\ApacheModuleProxy.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\include" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "SHARED_MODULE" /D "WIN32_LEAN_AND_MEAN"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ApacheModuleProxy.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\CoreD\ApacheCore.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ws2_32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\ApacheModuleProxy.pdb"\
 /map:"$(INTDIR)\ApacheModuleProxy.map" /debug /machine:I386\
 /out:"$(OUTDIR)\ApacheModuleProxy.dll"\
 /implib:"$(OUTDIR)\ApacheModuleProxy.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mod_proxy.obj" \
	"$(INTDIR)\proxy_cache.obj" \
	"$(INTDIR)\proxy_connect.obj" \
	"$(INTDIR)\proxy_ftp.obj" \
	"$(INTDIR)\proxy_http.obj" \
	"$(INTDIR)\proxy_util.obj" \
	"..\..\CoreD\ApacheCore.lib"

"$(OUTDIR)\ApacheModuleProxy.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "ApacheModuleProxy - Win32 Release" || "$(CFG)" ==\
 "ApacheModuleProxy - Win32 Debug"
SOURCE=.\mod_proxy.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_MOD_P=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\http_request.h"\
	"..\..\include\http_vhost.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\mod_proxy.obj" : $(SOURCE) $(DEP_CPP_MOD_P) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_MOD_P=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\http_request.h"\
	"..\..\include\http_vhost.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\mod_proxy.obj" : $(SOURCE) $(DEP_CPP_MOD_P) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy_cache.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_PROXY=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_md5.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_conf_globals.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\multithread.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_cache.obj" : $(SOURCE) $(DEP_CPP_PROXY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_PROXY=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_md5.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_conf_globals.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\multithread.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_cache.obj" : $(SOURCE) $(DEP_CPP_PROXY) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy_connect.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_PROXY_=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_connect.obj" : $(SOURCE) $(DEP_CPP_PROXY_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_PROXY_=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_connect.obj" : $(SOURCE) $(DEP_CPP_PROXY_) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy_ftp.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_PROXY_F=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_core.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_ftp.obj" : $(SOURCE) $(DEP_CPP_PROXY_F) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_PROXY_F=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_core.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_ftp.obj" : $(SOURCE) $(DEP_CPP_PROXY_F) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy_http.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_PROXY_H=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_core.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_http.obj" : $(SOURCE) $(DEP_CPP_PROXY_H) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_PROXY_H=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_core.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_http.obj" : $(SOURCE) $(DEP_CPP_PROXY_H) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy_util.c

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

DEP_CPP_PROXY_U=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_md5.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\multithread.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_util.obj" : $(SOURCE) $(DEP_CPP_PROXY_U) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

DEP_CPP_PROXY_U=\
	"..\..\include\ap_alloc.h"\
	"..\..\include\ap.h"\
	"..\..\include\ap_config.h"\
	"..\..\include\ap_ctype.h"\
	"..\..\include\ap_md5.h"\
	"..\..\include\ap_mmn.h"\
	"..\..\include\buff.h"\
	"..\..\include\explain.h"\
	"..\..\include\hsregex.h"\
	"..\..\include\http_config.h"\
	"..\..\include\http_log.h"\
	"..\..\include\http_main.h"\
	"..\..\include\http_protocol.h"\
	"..\..\include\httpd.h"\
	"..\..\include\multithread.h"\
	"..\..\include\util_date.h"\
	"..\..\include\util_uri.h"\
	"..\..\os\win32\os.h"\
	"..\..\os\win32\readdir.h"\
	".\mod_proxy.h"\
	

"$(INTDIR)\proxy_util.obj" : $(SOURCE) $(DEP_CPP_PROXY_U) "$(INTDIR)"


!ENDIF 

!IF  "$(CFG)" == "ApacheModuleProxy - Win32 Release"

"ApacheCore - Win32 Release" : 
   cd "\live\apache-1.3\src"
   $(MAKE) /$(MAKEFLAGS) /F ".\ApacheCore.mak" CFG="ApacheCore - Win32 Release"\
 
   cd ".\modules\proxy"

"ApacheCore - Win32 ReleaseCLEAN" : 
   cd "\live\apache-1.3\src"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\ApacheCore.mak"\
 CFG="ApacheCore - Win32 Release" RECURSE=1 
   cd ".\modules\proxy"

!ELSEIF  "$(CFG)" == "ApacheModuleProxy - Win32 Debug"

"ApacheCore - Win32 Debug" : 
   cd "\live\apache-1.3\src"
   $(MAKE) /$(MAKEFLAGS) /F ".\ApacheCore.mak" CFG="ApacheCore - Win32 Debug" 
   cd ".\modules\proxy"

"ApacheCore - Win32 DebugCLEAN" : 
   cd "\live\apache-1.3\src"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\ApacheCore.mak"\
 CFG="ApacheCore - Win32 Debug" RECURSE=1 
   cd ".\modules\proxy"

!ENDIF 


!ENDIF 

