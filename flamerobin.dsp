# Microsoft Developer Studio Project File - Name="flamerobin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=flamerobin - Win32 Debug Dynamic
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin.mak" CFG="flamerobin - Win32 Debug Dynamic"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "flamerobin - Win32 DLL Unicode Release Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Unicode Release Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Unicode Debug Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Unicode Debug Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Release Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Release Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Debug Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Debug Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Release Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Release Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Debug Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Unicode Debug Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Release Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Release Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Debug Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 Debug Dynamic" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcus"
# PROP BASE Intermediate_Dir "vcus\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcus"
# PROP Intermediate_Dir "vcus\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcus\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcus\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcus\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcus\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcu"
# PROP BASE Intermediate_Dir "vcu\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcu"
# PROP Intermediate_Dir "vcu\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcu\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcu\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcu\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcu\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcusd"
# PROP BASE Intermediate_Dir "vcusd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcusd"
# PROP Intermediate_Dir "vcusd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcusd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcusd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcusd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcusd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Unicode Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcud"
# PROP BASE Intermediate_Dir "vcud\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcud"
# PROP Intermediate_Dir "vcud\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcud\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcud\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcud\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcud\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcs"
# PROP BASE Intermediate_Dir "vcs\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcs"
# PROP Intermediate_Dir "vcs\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcs\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcs\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcs\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcs\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc"
# PROP BASE Intermediate_Dir "vc\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc"
# PROP Intermediate_Dir "vc\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvc\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvc\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_dll\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vc\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vc\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcsd"
# PROP BASE Intermediate_Dir "vcsd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcsd"
# PROP Intermediate_Dir "vcsd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcsd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcsd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcsd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcsd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcd"
# PROP BASE Intermediate_Dir "vcd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcd"
# PROP Intermediate_Dir "vcd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_dll\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcus"
# PROP BASE Intermediate_Dir "vcus\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcus"
# PROP Intermediate_Dir "vcus\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcus\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcus\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcus\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcus\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcu"
# PROP BASE Intermediate_Dir "vcu\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcu"
# PROP Intermediate_Dir "vcu\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcu\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcu\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcu\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25u_stc.lib wxmsw25u_html.lib wxmsw25u_adv.lib wxmsw25u_core.lib wxbase25u.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcu\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcusd"
# PROP BASE Intermediate_Dir "vcusd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcusd"
# PROP Intermediate_Dir "vcusd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcusd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcusd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcusd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcusd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Unicode Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcud"
# PROP BASE Intermediate_Dir "vcud\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcud"
# PROP Intermediate_Dir "vcud\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcud\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcud\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_UNICODE" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_UNICODE" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcud\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25ud_stc.lib wxmsw25ud_html.lib wxmsw25ud_adv.lib wxmsw25ud_core.lib wxbase25ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcud\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcs"
# PROP BASE Intermediate_Dir "vcs\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcs"
# PROP Intermediate_Dir "vcs\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcs\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvcs\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcs\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcs\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc"
# PROP BASE Intermediate_Dir "vc\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc"
# PROP Intermediate_Dir "vc\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvc\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Fdvc\flamerobin.pdb /O1 /I "$(WXDIR)\lib\vc_lib\msw" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\msw" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vc\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"
# ADD LINK32 wxmsw25_stc.lib wxmsw25_html.lib wxmsw25_adv.lib wxmsw25_core.lib wxbase25.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vc\flamerobin.exe" /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\release\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcsd"
# PROP BASE Intermediate_Dir "vcsd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcsd"
# PROP Intermediate_Dir "vcsd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcsd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcsd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcsd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcsd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcd"
# PROP BASE Intermediate_Dir "vcd\flamerobin"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcd"
# PROP Intermediate_Dir "vcd\flamerobin"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /I "$(WXDIR)\include" /I "$(WXDIR)\contrib\include" /Zi /Gm /GZ /Fdvcd\flamerobin.pdb /Od /I "$(WXDIR)\lib\vc_lib\mswd" /GR /GX /YX /W4 /I "src" /I "res" /I "$(IBPP)" /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "__WXDEBUG__" /D "_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
# ADD RSC /l 0x405 /i "$(WXDIR)\include" /i "$(WXDIR)\contrib\include" /d "__WXDEBUG__" /d "_DEBUG" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswd" /d "_WINDOWS" /d "IBPP_MSVC" /d "IBPP_WINDOWS" /i "src" /i "res" /i $(IBPP)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"
# ADD LINK32 wxmsw25d_stc.lib wxmsw25d_html.lib wxmsw25d_adv.lib wxmsw25d_core.lib wxbase25d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib ibpp.lib /nologo /machine:i386 /out:"vcd\flamerobin.exe" /debug /nologo /subsystem:windows /machine:I386 /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows /libpath:"$(IBPP)\debug\win32_msvc"

!ENDIF

# Begin Target

# Name "flamerobin - Win32 DLL Unicode Release Static"
# Name "flamerobin - Win32 DLL Unicode Release Dynamic"
# Name "flamerobin - Win32 DLL Unicode Debug Static"
# Name "flamerobin - Win32 DLL Unicode Debug Dynamic"
# Name "flamerobin - Win32 DLL Release Static"
# Name "flamerobin - Win32 DLL Release Dynamic"
# Name "flamerobin - Win32 DLL Debug Static"
# Name "flamerobin - Win32 DLL Debug Dynamic"
# Name "flamerobin - Win32 Unicode Release Static"
# Name "flamerobin - Win32 Unicode Release Dynamic"
# Name "flamerobin - Win32 Unicode Debug Static"
# Name "flamerobin - Win32 Unicode Debug Dynamic"
# Name "flamerobin - Win32 Release Static"
# Name "flamerobin - Win32 Release Dynamic"
# Name "flamerobin - Win32 Debug Static"
# Name "flamerobin - Win32 Debug Dynamic"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gui\BackupFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupRestoreBaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BaseDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\DatabaseRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSqlFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FieldPropertiesFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FieldPropertiesFrameImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MainFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MainFrameImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MetadataItemPropertiesFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MultilineEnterDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\OptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ReorderFieldsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\RestoreFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServerRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\addconstrainthandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\column.cpp
# End Source File
# Begin Source File

SOURCE=.\src\config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\constraints.cpp
# End Source File
# Begin Source File

SOURCE=.\src\converters.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\database.cpp
# End Source File
# Begin Source File

SOURCE=.\src\dberror.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\domain.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\exception.cpp
# End Source File
# Begin Source File

SOURCE=.\res\flamerobin.rc
# End Source File
# Begin Source File

SOURCE=.\src\frDataGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\src\frDataGridCells.cpp
# End Source File
# Begin Source File

SOURCE=.\src\frDataGridTable.cpp
# End Source File
# Begin Source File

SOURCE=.\src\frutils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\function.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\generator.cpp
# End Source File
# Begin Source File

SOURCE=.\src\images.cpp
# End Source File
# Begin Source File

SOURCE=.\src\main.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitemwithcolumns.cpp
# End Source File
# Begin Source File

SOURCE=.\src\myDataGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\src\myTreeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\objectdescriptionhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\observer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\procedure.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\role.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\root.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\server.cpp
# End Source File
# Begin Source File

SOURCE=.\src\simpleparser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\styleguide.cpp
# End Source File
# Begin Source File

SOURCE=.\src\styleguidemsw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\subject.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\table.cpp
# End Source File
# Begin Source File

SOURCE=.\src\treeitem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\trigger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ugly.cpp
# End Source File
# Begin Source File

SOURCE=.\src\urihandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\view.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gui\BackupFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupRestoreBaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\BaseDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\BaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\DatabaseRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSqlFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\FieldPropertiesFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\MainFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\MetadataItemPropertiesFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\MultilineEnterDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\OptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ReorderFieldsDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\RestoreFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServerRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\collection.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\column.h
# End Source File
# Begin Source File

SOURCE=.\src\config.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\constraints.h
# End Source File
# Begin Source File

SOURCE=.\src\converters.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\database.h
# End Source File
# Begin Source File

SOURCE=.\src\dberror.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\domain.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\exception.h
# End Source File
# Begin Source File

SOURCE=.\src\frDataGrid.h
# End Source File
# Begin Source File

SOURCE=.\src\frDataGridCells.h
# End Source File
# Begin Source File

SOURCE=.\src\frDataGridTable.h
# End Source File
# Begin Source File

SOURCE=.\src\frutils.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\function.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\generator.h
# End Source File
# Begin Source File

SOURCE=.\src\images.h
# End Source File
# Begin Source File

SOURCE=.\src\main.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitem.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitemwithcolumns.h
# End Source File
# Begin Source File

SOURCE=.\src\myDataGrid.h
# End Source File
# Begin Source File

SOURCE=.\src\myTreeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\src\observer.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\parameter.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\procedure.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\role.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\root.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\server.h
# End Source File
# Begin Source File

SOURCE=.\src\simpleparser.h
# End Source File
# Begin Source File

SOURCE=.\src\styleguide.h
# End Source File
# Begin Source File

SOURCE=.\src\subject.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\table.h
# End Source File
# Begin Source File

SOURCE=.\src\treeitem.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\trigger.h
# End Source File
# Begin Source File

SOURCE=.\src\ugly.h
# End Source File
# Begin Source File

SOURCE=.\src\urihandler.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\view.h
# End Source File
# End Group
# End Target
# End Project

