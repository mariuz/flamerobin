# Microsoft Developer Studio Project File - Name="flamerobin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=flamerobin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin.mak" CFG="flamerobin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "flamerobin - Win32 DLL Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Release" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Debug" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25u.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxmsw25u_html.lib wxmsw25u_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25u.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxmsw25u_html.lib wxmsw25u_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25ud.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxmsw25ud_html.lib wxmsw25ud_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25ud.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxmsw25ud_html.lib wxmsw25ud_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25.lib wxmsw25_adv.lib wxmsw25_core.lib wxmsw25_html.lib wxmsw25_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25.lib wxmsw25_adv.lib wxmsw25_core.lib wxmsw25_html.lib wxmsw25_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "WXUSINGDLL" /D "WXUSINGDLL" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "WXUSINGDLL" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25d.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxmsw25d_html.lib wxmsw25d_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25d.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxmsw25d_html.lib wxmsw25d_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_dll" /libpath:"$(WXDIR)\lib\vc_dll" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /i "$(WXDIR)\lib\vc_lib\msw" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /i "$(WXDIR)\lib\vc_lib\msw" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25u.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxmsw25u_html.lib wxmsw25u_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25u.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxmsw25u_html.lib wxmsw25u_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /i "$(WXDIR)\lib\vc_lib\mswd" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /i "$(WXDIR)\lib\vc_lib\mswd" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25ud.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxmsw25ud_html.lib wxmsw25ud_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25ud.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxmsw25ud_html.lib wxmsw25ud_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /FdRelease\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\lib\vc_lib\msw" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\lib\vc_lib\msw" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25.lib wxmsw25_adv.lib wxmsw25_core.lib wxmsw25_html.lib wxmsw25_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25.lib wxmsw25_adv.lib wxmsw25_core.lib wxmsw25_html.lib wxmsw25_stc.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib ibpp.lib /nologo /machine:i386 /out:"Release\flamerobin.exe" /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /Zi /Gm /GZ /FdDebug\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "..\src" /I "..\res\" /I "$(IBPP)" /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "STRICT" /D "WIN32" /D WINVER=0x400 /D wxUSE_GUI=1 /D "__WXMSW__" /D "__WIN32__" /D "__WIN95__" /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /i "$(WXDIR)\lib\vc_lib\mswd" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /i "$(WXDIR)\lib\vc_lib\mswd" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "STRICT" /d "WIN32" /d WINVER=0x400 /d wxUSE_GUI=1 /d "__WXMSW__" /d "__WIN32__" /d "__WIN95__" /d "WIN32_LEAN_AND_MEAN" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "..\src" /i "..\res\" /i "$(IBPP)" /i "$(WXDIR)\include" /i $(WXDIR)\contrib\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25d.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxmsw25d_html.lib wxmsw25d_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib wxbase25d.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxmsw25d_html.lib wxmsw25d_stc.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib ibpp.lib /nologo /machine:i386 /out:"Debug\flamerobin.exe" /debug /libpath:"$(WXDIR)\lib\vc_lib" /libpath:"$(WXDIR)\lib\vc_lib" /nologo /subsystem:windows /machine:I386 /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ENDIF

# Begin Target

# Name "flamerobin - Win32 DLL Unicode Release"
# Name "flamerobin - Win32 DLL Unicode Debug"
# Name "flamerobin - Win32 DLL Release"
# Name "flamerobin - Win32 DLL Debug"
# Name "flamerobin - Win32 Unicode Release"
# Name "flamerobin - Win32 Unicode Debug"
# Name "flamerobin - Win32 Release"
# Name "flamerobin - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\src\gui\BackupFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\BackupRestoreBaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\BaseDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\BaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\DatabaseRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\ExecuteSqlFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\ExecuteSqlFrameImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\FieldPropertiesFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\FieldPropertiesFrameImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\MainFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\MainFrameImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\MetadataItemPropertiesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\MetadataItemPropertiesDialogImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\MultilineEnterDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\ReorderFieldsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\ReorderFieldsDialogImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\RestoreFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\ServerRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\addconstrainthandler.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\column.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\config.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\constraints.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\converters.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\database.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\dberror.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\domain.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\exception.cpp
# End Source File
# Begin Source File

SOURCE=.\..\res\flamerobin.rc
# End Source File
# Begin Source File

SOURCE=.\..\src\frutils.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\function.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\generator.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\images.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\main.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\metadataitem.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\metadataitemwithcolumns.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\myDataGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\myTreeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\objectdescriptionhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\observer.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\procedure.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\role.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\root.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\server.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\styleguide.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\styleguidemsw.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\subject.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\table.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\treeitem.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\trigger.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\ugly.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\urihandler.cpp
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\view.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\src\gui\BackupFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\BackupRestoreBaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\BaseDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\BaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\DatabaseRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\ExecuteSqlFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\FieldPropertiesFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\MainFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\MetadataItemPropertiesDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\MultilineEnterDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\ReorderFieldsDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\RestoreFrame.h
# End Source File
# Begin Source File

SOURCE=.\..\src\gui\ServerRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\collection.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\column.h
# End Source File
# Begin Source File

SOURCE=.\..\src\config.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\constraints.h
# End Source File
# Begin Source File

SOURCE=.\..\src\converters.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\database.h
# End Source File
# Begin Source File

SOURCE=.\..\src\dberror.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\domain.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\exception.h
# End Source File
# Begin Source File

SOURCE=.\..\src\frutils.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\function.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\generator.h
# End Source File
# Begin Source File

SOURCE=.\..\src\images.h
# End Source File
# Begin Source File

SOURCE=.\..\src\main.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\metadataitem.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\metadataitemwithcolumns.h
# End Source File
# Begin Source File

SOURCE=.\..\src\myDataGrid.h
# End Source File
# Begin Source File

SOURCE=.\..\src\myTreeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\..\src\observer.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\parameter.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\procedure.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\role.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\root.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\server.h
# End Source File
# Begin Source File

SOURCE=.\..\src\styleguide.h
# End Source File
# Begin Source File

SOURCE=.\..\src\subject.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\table.h
# End Source File
# Begin Source File

SOURCE=.\..\src\treeitem.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\trigger.h
# End Source File
# Begin Source File

SOURCE=.\..\src\ugly.h
# End Source File
# Begin Source File

SOURCE=.\..\src\urihandler.h
# End Source File
# Begin Source File

SOURCE=.\..\src\metadata\view.h
# End Source File
# End Group
# End Target
# End Project

