# Microsoft Developer Studio Project File - Name="qui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=qui - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "qui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "qui.mak" CFG="qui - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "qui - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "qui - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "qui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../../../lib"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\shared" /I "$(QTDIR)\include" /I "." /I "$(QTDIR)\mkspecs\win32-msvc" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /D "QT_INTERNAL_XML" /D Q_TEMPLATE_EXTERN=extern /D "RESOURCE" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /FD -Zm200 /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo /out:"c:\scada\lib\qui.lib"

!ELSEIF  "$(CFG)" == "qui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\shared" /I "$(QTDIR)\include" /I "." /I "$(QTDIR)\mkspecs\win32-msvc" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /D "QT_INTERNAL_XML" /D Q_TEMPLATE_EXTERN=extern /D "RESOURCE" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /FD /GZ -Zm200 /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo /out:"c:\scada\lib\qui.lib"

!ENDIF 

# Begin Target

# Name "qui - Win32 Release"
# Name "qui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\designer\database.cpp
# End Source File
# Begin Source File

SOURCE=..\designer\database2.h

!IF  "$(CFG)" == "qui - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - MOCing database2.h...
InputDir=\scada\src\hmi_designer\designer\designer
InputPath=..\designer\database2.h
InputName=database2

"$(InputDir)\moc_$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"%qtdir%\bin\moc.exe" "$(InputDir)\$(InputName).h" -o "$(InputDir)\moc_$(InputName).cpp"

# End Custom Build

!ELSEIF  "$(CFG)" == "qui - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - MOCing database2.h...
InputDir=\scada\src\hmi_designer\designer\designer
InputPath=..\designer\database2.h
InputName=database2

"$(InputDir)\moc_$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"%qtdir%\bin\moc.exe" "$(InputDir)\$(InputName).h" -o "$(InputDir)\moc_$(InputName).cpp"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared\domtool.cpp
# End Source File
# Begin Source File

SOURCE=..\designer\moc_database2.cpp
# End Source File
# Begin Source File

SOURCE=qwidgetfactory.cpp
# End Source File
# Begin Source File

SOURCE=..\shared\uib.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\shared\domtool.h
# End Source File
# Begin Source File

SOURCE=..\shared\uib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Generated"

# PROP Default_Filter "moc"
# End Group
# End Target
# End Project
