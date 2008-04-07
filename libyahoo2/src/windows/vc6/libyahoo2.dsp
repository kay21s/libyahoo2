# Microsoft Developer Studio Project File - Name="libyahoo2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libyahoo2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libyahoo2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libyahoo2.mak" CFG="libyahoo2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libyahoo2 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libyahoo2 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libyahoo2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".libyahoo2-release"
# PROP BASE Intermediate_Dir ".libyahoo2-release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".libyahoo2-release"
# PROP Intermediate_Dir ".libyahoo2-release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libyahoo2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".libyahoo2-debug"
# PROP BASE Intermediate_Dir ".libyahoo2-debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libyahoo2-debug"
# PROP Intermediate_Dir ".libyahoo2-debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:".libyahoo2-debug\libyahoo2d.lib"

!ENDIF 

# Begin Target

# Name "libyahoo2 - Win32 Release"
# Name "libyahoo2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\libyahoo2.c
# End Source File
# Begin Source File

SOURCE=..\..\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\sha1.c
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_fn.c
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_httplib.c
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_list.c
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\config.h
# End Source File
# Begin Source File

SOURCE=..\..\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo2.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo2_callbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo2_types.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_debug.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_fn.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_httplib.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_list.h
# End Source File
# Begin Source File

SOURCE=..\..\yahoo_util.h
# End Source File
# End Group
# End Target
# End Project
