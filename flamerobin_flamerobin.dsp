# Microsoft Developer Studio Project File - Name="flamerobin_flamerobin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=flamerobin - Win32 Debug Dynamic
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin_flamerobin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin_flamerobin.mak" CFG="flamerobin - Win32 Debug Dynamic"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "flamerobin - Win32 DLL Release Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Release Dynamic" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Debug Static" (based on "Win32 (x86) Application")
!MESSAGE "flamerobin - Win32 DLL Debug Dynamic" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "flamerobin - Win32 DLL Release Static"

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
# ADD BASE CPP /nologo /FD /MT /Fdvcus\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcus\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcus\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcus\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcus\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcus\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows
# ADD LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcus\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcus\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Release Dynamic"

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
# ADD BASE CPP /nologo /FD /MD /Fdvcu\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcu\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvcu\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcu\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcu\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcu\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows
# ADD LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcu\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcu\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Debug Static"

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
# ADD BASE CPP /nologo /FD /MTd /Zi /Fdvcusd\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcusd\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Fdvcusd\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcusd\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcusd\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcusd\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows
# ADD LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcusd\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcusd\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 DLL Debug Dynamic"

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
# ADD BASE CPP /nologo /FD /MDd /Zi /Fdvcud\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcud\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Fdvcud\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_dll\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcud\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_dll\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcud\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcud\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows
# ADD LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcud\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcud\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_dll" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Release Static"

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
# ADD BASE CPP /nologo /FD /MT /Fdvcus\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcus\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcus\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcus\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcus\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcus\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows
# ADD LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcus\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcus\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Release Dynamic"

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
# ADD BASE CPP /nologo /FD /MD /Fdvcu\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcu\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvcu\flamerobin.pdb /O1 /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswu" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcu\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswu" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcu\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcu\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows
# ADD LINK32 wxmsw32u_aui.lib wxmsw32u_stc.lib wxmsw32u_html.lib wxmsw32u_adv.lib wxmsw32u_core.lib wxbase32u_xml.lib wxbase32u.lib wxscintilla.lib wxexpat.lib wxtiff.lib wxjpeg.lib wxpng.lib wxzlib.lib wxregexu.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcu\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /pdb:"vcu\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Debug Static"

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
# ADD BASE CPP /nologo /FD /MTd /Zi /Fdvcusd\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcusd\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Fdvcusd\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcusd\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcusd\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcusd\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows
# ADD LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcusd\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcusd\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows

!ELSEIF  "$(CFG)" == "flamerobin - Win32 Debug Dynamic"

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
# ADD BASE CPP /nologo /FD /MDd /Zi /Fdvcud\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcud\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Fdvcud\flamerobin.pdb /Od /Gm /GR /EHsc /W4 /I "$(WXDIR)\lib\vc_lib\mswud" /I "$(WXDIR)\contrib\include" /I "$(WXDIR)\include" /Yu"wx/wxprec.h" /Fp"vcud\flamerobin.pch" /I "." /I ".\src" /I ".\src\ibpp" /I ".\res" /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_DEBUG" /D "__WXDEBUG__" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D WINVER=0x400 /D "WIN32" /D "__WIN32__" /D "__WIN95__" /D "STRICT" /D "__WXMSW__" /D wxUSE_GUI=1 /D wxUSE_REGEX=1 /D wxUSE_UNICODE=1 /D "WIN32_LEAN_AND_MEAN" /D "_WINDOWS" /D "IBPP_WINDOWS" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
# ADD RSC /l 0x409 /d "_DEBUG" /d "__WXDEBUG__" /d "_DEBUG" /d "_WINDOWS" /d "__WINDOWS__" /d WINVER=0x400 /d "WIN32" /d "__WIN32__" /d "__WIN95__" /d "STRICT" /d "__WXMSW__" /d wxUSE_GUI=1 /d wxUSE_REGEX=1 /d wxUSE_UNICODE=1 /d "WIN32_LEAN_AND_MEAN" /i "$(WXDIR)\lib\vc_lib\mswud" /i "$(WXDIR)\contrib\include" /i "$(WXDIR)\include" /d "_WINDOWS" /d "IBPP_WINDOWS" /i "." /i ".\src" /i ".\src\ibpp" /i .\res
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcud\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcud\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows
# ADD LINK32 wxmsw32ud_aui.lib wxmsw32ud_stc.lib wxmsw32ud_html.lib wxmsw32ud_adv.lib wxmsw32ud_core.lib wxbase32ud_xml.lib wxbase32ud.lib wxscintillad.lib wxexpatd.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexud.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib rpcrt4.lib wsock32.lib vcud\ibpp.lib /nologo /machine:i386 /out:"flamerobin.exe" /debug /pdb:"vcud\flamerobin.pdb" /nologo /subsystem:windows /libpath:"$(WXDIR)\lib\vc_lib" /subsystem:windows

!ENDIF

# Begin Target

# Name "flamerobin - Win32 DLL Release Static"
# Name "flamerobin - Win32 DLL Release Dynamic"
# Name "flamerobin - Win32 DLL Debug Static"
# Name "flamerobin - Win32 DLL Debug Dynamic"
# Name "flamerobin - Win32 Release Static"
# Name "flamerobin - Win32 Release Dynamic"
# Name "flamerobin - Win32 Debug Static"
# Name "flamerobin - Win32 Debug Dynamic"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gui\AboutBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\AdvancedMessageDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\AdvancedSearchFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\ArtProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupRestoreBaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\BaseDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\BaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\CharacterSet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\CodeTemplateProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\Collation.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\CommandManager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ConfdefTemplateProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\config\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ContextMenuMetadataItemVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\ControlUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\CreateDDLVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\CreateIndexDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DBHTreeControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\DataGeneratorFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridRowBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridRows.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridTable.cpp
# End Source File
# Begin Source File

SOURCE=.\src\config\DatabaseConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\DatabaseRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DndTextControls.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\EditBlobDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\EventWatcherFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSql.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSqlFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\FRDecimal.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\FRError.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\FRInt128.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\FRLayoutConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\FRStyle.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\FieldPropertiesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\FindDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\GUIURIHandlerHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\HtmlHeaderMetadataItemVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\HtmlTemplateProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\Identifier.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\IncompleteStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\Index.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\InsertDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\InsertParametersDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\config\LocalSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\LogTextControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MainFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MasterPassword.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemCreateStatementVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemDescriptionVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MetadataItemPropertiesFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemURIHandlerHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemVisitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\engine\MetadataLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataTemplateCmdHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataTemplateManager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\MultiStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MultilineEnterDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\Observer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\PreferencesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\PreferencesDialogSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\PrintableHtmlWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\PrivilegesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\ProgressIndicator.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ReorderFieldsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\RestoreFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\SelectStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServerRegistrationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServiceBaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ShutdownFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\ShutdownStartupBaseFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\SimpleHtmlFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\SqlStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\SqlTokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\StartupFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sql\StatementBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\StatementHistoryDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\StringUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\StyleGuide.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\msw\StyleGuideMSW.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\Subject.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\TemplateProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\TextControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\URIProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\User.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\UserDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\UsernamePasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\core\Visitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\addconstrainthandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\column.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\constraints.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\database.cpp
# End Source File
# Begin Source File

SOURCE=.\src\databasehandler.cpp
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

SOURCE=.\src\frprec.cpp
# ADD BASE CPP /Yc"wx/wxprec.h"
# ADD CPP /Yc"wx/wxprec.h"
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

SOURCE=.\src\logger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\main.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\objectdescriptionhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\package.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\privilege.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\procedure.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\relation.cpp
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

SOURCE=.\src\statementHistory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\table.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\trigger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\metadata\view.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gui\AboutBox.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\AdvancedMessageDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\AdvancedSearchFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\core\ArtProvider.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\BackupRestoreBaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\BaseDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\BaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\CharacterSet.h
# End Source File
# Begin Source File

SOURCE=.\src\core\CodeTemplateProcessor.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\Collation.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\CommandIds.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\CommandManager.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ConfdefTemplateProcessor.h
# End Source File
# Begin Source File

SOURCE=.\src\config\Config.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ContextMenuMetadataItemVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\ControlUtils.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\CreateDDLVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\CreateIndexDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DBHTreeControl.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\DataGeneratorFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGrid.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridRowBuffer.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridRows.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DataGridTable.h
# End Source File
# Begin Source File

SOURCE=.\src\config\DatabaseConfig.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\DatabaseRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\DndTextControls.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\EditBlobDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\EventWatcherFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSql.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ExecuteSqlFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\core\FRDecimal.h
# End Source File
# Begin Source File

SOURCE=.\src\core\FRError.h
# End Source File
# Begin Source File

SOURCE=.\src\core\FRInt128.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\FRLayoutConfig.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\FRStyle.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\FieldPropertiesDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\FindDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\GUIURIHandlerHelper.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\HtmlHeaderMetadataItemVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\HtmlTemplateProcessor.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\Identifier.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\IncompleteStatement.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\Index.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\InsertDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\InsertParametersDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\Isaac.h
# End Source File
# Begin Source File

SOURCE=.\src\config\LocalSettings.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\LogTextControl.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\MainFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\MasterPassword.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataClasses.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemCreateStatementVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemDescriptionVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\MetadataItemPropertiesFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemURIHandlerHelper.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataItemVisitor.h
# End Source File
# Begin Source File

SOURCE=.\src\engine\MetadataLoader.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\MetadataTemplateManager.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\MultiStatement.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\MultilineEnterDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\core\ObjectWithHandle.h
# End Source File
# Begin Source File

SOURCE=.\src\core\Observer.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\PreferencesDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\PrintableHtmlWindow.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\PrivilegesDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\core\ProcessableObject.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ProgressDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\core\ProgressIndicator.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ReorderFieldsDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\RestoreFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\SelectStatement.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServerRegistrationDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ServiceBaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ShutdownFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\ShutdownStartupBaseFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\SimpleHtmlFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\SqlStatement.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\SqlTokenizer.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\StartupFrame.h
# End Source File
# Begin Source File

SOURCE=.\src\sql\StatementBuilder.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\StatementHistoryDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\core\StringUtils.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\StyleGuide.h
# End Source File
# Begin Source File

SOURCE=.\src\core\Subject.h
# End Source File
# Begin Source File

SOURCE=.\src\core\TemplateProcessor.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\controls\TextControl.h
# End Source File
# Begin Source File

SOURCE=.\src\core\URIProcessor.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\User.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\UserDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\gui\UsernamePasswordDialog.h
# End Source File
# Begin Source File

SOURCE=.\src\core\Visitor.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\collection.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\column.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\constraints.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\database.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\domain.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\exception.h
# End Source File
# Begin Source File

SOURCE=.\src\frutils.h
# End Source File
# Begin Source File

SOURCE=.\src\frversion.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\function.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\generator.h
# End Source File
# Begin Source File

SOURCE=.\src\logger.h
# End Source File
# Begin Source File

SOURCE=.\src\main.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\metadataitem.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\package.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\parameter.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\privilege.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\procedure.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\relation.h
# End Source File
# Begin Source File

SOURCE=.\src\revisioninfo.h
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

SOURCE=.\src\statementHistory.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\table.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\trigger.h
# End Source File
# Begin Source File

SOURCE=.\src\metadata\view.h
# End Source File
# End Group
# End Target
# End Project

