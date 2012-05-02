# Microsoft Developer Studio Project File - Name="dspserver" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=dspserver - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dspserver.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dspserver.mak" CFG="dspserver - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dspserver - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "dspserver - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dspserver - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "C:\scada\bin"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(QTDIR)\include" /I "$(QTDIR)\mkspecs\win32-msvc" /I "." /I "..\gigabase" /I "..\dispatcher\client" /I "..\dispatcher\server" /I "..\trace" /I "..\utilities" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "QT_THREAD_SUPPORT" /D "USE_LOCALE_SETTINGS" /D "SECURE_SERVER" /D "QT_DLL" /D "QT_H_CPP" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 advapi32.lib user32.lib snmpapi.lib utilities.lib wsock32.lib qt-mtnc$(QTVER).lib dbase.lib /nologo /subsystem:console /pdb:"Release/dspserver.pdb" /debug /machine:I386 /libpath:"$(QTDIR)\lib" /libpath:"c:\scada\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "dspserver - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "c:\scada\bin"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Zi /Od /I "$(QTDIR)\include" /I "$(QTDIR)\mkspecs\win32-msvc" /I "." /I "..\gigabase" /I "..\dispatcher\client" /I "..\dispatcher\server" /I "..\trace" /I "..\utilities" /D "_CONSOLE" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "QT_THREAD_SUPPORT" /D "USE_LOCALE_SETTINGS" /D "QT_DLL" /D "QT_H_CPP" /D "SECURE_SERVER" /c
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib user32.lib snmpapi.lib utilitiesd.lib dbased.lib wsock32.lib qt-mtnc$(QTVER).lib /nologo /subsystem:console /incremental:no /pdb:"Debug/dspserver.pdb" /debug /machine:I386 /pdbtype:sept /libpath:"$(QTDIR)\lib" /libpath:"c:\scada\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "dspserver - Win32 Release"
# Name "dspserver - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "dispatcher"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dispatcher\client\cli_dispatcher.h
# End Source File
# Begin Source File

SOURCE=..\dispatcher\client\cliproto_dispatcher.h
# End Source File
# Begin Source File

SOURCE=..\dispatcher\server\dispatcher.cpp
# End Source File
# Begin Source File

SOURCE=..\dispatcher\server\dispatcher.h
# End Source File
# Begin Source File

SOURCE=..\dispatcher\server\server_dispatcher.cpp
# End Source File
# Begin Source File

SOURCE=..\dispatcher\server\server_dispatcher.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# End Target
# End Project
