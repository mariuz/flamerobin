# Microsoft Developer Studio Project File - Name="flamerobin_ibpp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ibpp - Win32 Debug Dynamic
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin_ibpp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flamerobin_ibpp.mak" CFG="ibpp - Win32 Debug Dynamic"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ibpp - Win32 DLL Unicode Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Unicode Release Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Unicode Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Unicode Debug Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Release Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 DLL Debug Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Unicode Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Unicode Release Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Unicode Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Unicode Debug Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Release Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "ibpp - Win32 Debug Dynamic" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ibpp - Win32 DLL Unicode Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcus"
# PROP BASE Intermediate_Dir "vcus\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcus"
# PROP Intermediate_Dir "vcus\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /Fdvcus\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcus\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcus\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcus\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcus\ibpp.lib"
# ADD LIB32 /nologo /out:"vcus\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Unicode Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcu"
# PROP BASE Intermediate_Dir "vcu\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcu"
# PROP Intermediate_Dir "vcu\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /Fdvcu\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcu\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvcu\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcu\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcu\ibpp.lib"
# ADD LIB32 /nologo /out:"vcu\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Unicode Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcusd"
# PROP BASE Intermediate_Dir "vcusd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcusd"
# PROP Intermediate_Dir "vcusd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcusd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcusd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcusd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcusd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcusd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcusd\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Unicode Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcud"
# PROP BASE Intermediate_Dir "vcud\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcud"
# PROP Intermediate_Dir "vcud\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcud\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcud\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcud\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcud\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcud\ibpp.lib"
# ADD LIB32 /nologo /out:"vcud\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcs"
# PROP BASE Intermediate_Dir "vcs\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcs"
# PROP Intermediate_Dir "vcs\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /Fdvcs\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcs\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcs\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcs\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcs\ibpp.lib"
# ADD LIB32 /nologo /out:"vcs\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc"
# PROP BASE Intermediate_Dir "vc\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc"
# PROP Intermediate_Dir "vc\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /Fdvc\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vc\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvc\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vc\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vc\ibpp.lib"
# ADD LIB32 /nologo /out:"vc\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcsd"
# PROP BASE Intermediate_Dir "vcsd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcsd"
# PROP Intermediate_Dir "vcsd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcsd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcsd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcsd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcsd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcsd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcsd\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 DLL Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcd"
# PROP BASE Intermediate_Dir "vcd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcd"
# PROP Intermediate_Dir "vcd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcd\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Unicode Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcus"
# PROP BASE Intermediate_Dir "vcus\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcus"
# PROP Intermediate_Dir "vcus\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /Fdvcus\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcus\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcus\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcus\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcus\ibpp.lib"
# ADD LIB32 /nologo /out:"vcus\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Unicode Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcu"
# PROP BASE Intermediate_Dir "vcu\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcu"
# PROP Intermediate_Dir "vcu\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /Fdvcu\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcu\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvcu\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcu\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcu\ibpp.lib"
# ADD LIB32 /nologo /out:"vcu\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Unicode Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcusd"
# PROP BASE Intermediate_Dir "vcusd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcusd"
# PROP Intermediate_Dir "vcusd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcusd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcusd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcusd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcusd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcusd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcusd\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Unicode Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcud"
# PROP BASE Intermediate_Dir "vcud\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcud"
# PROP Intermediate_Dir "vcud\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcud\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcud\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcud\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcud\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcud\ibpp.lib"
# ADD LIB32 /nologo /out:"vcud\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcs"
# PROP BASE Intermediate_Dir "vcs\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcs"
# PROP Intermediate_Dir "vcs\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MT /Fdvcs\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcs\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MT /Fdvcs\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcs\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcs\ibpp.lib"
# ADD LIB32 /nologo /out:"vcs\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc"
# PROP BASE Intermediate_Dir "vc\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc"
# PROP Intermediate_Dir "vc\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /Fdvc\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vc\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MD /Fdvc\ibpp.pdb /O1 /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vc\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vc\ibpp.lib"
# ADD LIB32 /nologo /out:"vc\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcsd"
# PROP BASE Intermediate_Dir "vcsd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcsd"
# PROP Intermediate_Dir "vcsd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcsd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcsd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MTd /Zi /Gm /GZ /Fdvcsd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcsd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcsd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcsd\ibpp.lib"

!ELSEIF  "$(CFG)" == "ibpp - Win32 Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcd"
# PROP BASE Intermediate_Dir "vcd\ibpp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcd"
# PROP Intermediate_Dir "vcd\ibpp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD CPP /nologo /FD /MDd /Zi /Gm /GZ /Fdvcd\ibpp.pdb /Od /GR /EHsc /W4 /Yu"ibpp.h" /Fp"vcd\ibpp.pch" /I ".\src\ibpp" /I ".\src\ibpp\fbheaders" /D "WIN32" /D "_LIB" /D "_DEBUG" /D "IBPP_MSVC" /D "IBPP_WINDOWS" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcd\ibpp.lib"
# ADD LIB32 /nologo /out:"vcd\ibpp.lib"

!ENDIF

# Begin Target

# Name "ibpp - Win32 DLL Unicode Release Static"
# Name "ibpp - Win32 DLL Unicode Release Dynamic"
# Name "ibpp - Win32 DLL Unicode Debug Static"
# Name "ibpp - Win32 DLL Unicode Debug Dynamic"
# Name "ibpp - Win32 DLL Release Static"
# Name "ibpp - Win32 DLL Release Dynamic"
# Name "ibpp - Win32 DLL Debug Static"
# Name "ibpp - Win32 DLL Debug Dynamic"
# Name "ibpp - Win32 Unicode Release Static"
# Name "ibpp - Win32 Unicode Release Dynamic"
# Name "ibpp - Win32 Unicode Debug Static"
# Name "ibpp - Win32 Unicode Debug Dynamic"
# Name "ibpp - Win32 Release Static"
# Name "ibpp - Win32 Release Dynamic"
# Name "ibpp - Win32 Debug Static"
# Name "ibpp - Win32 Debug Dynamic"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ibpp\_dpb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_epb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_ibs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_rb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_spb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_tpb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\array.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\blob.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\database.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\date.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\dbkey.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\exception.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\ibpp.cpp
# ADD BASE CPP /Yc"ibpp.h"
# ADD CPP /Yc"ibpp.h"
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\row.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\service.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\statement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\time.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\transaction.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ibpp\_internals.h
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\_msvcwarnings.h
# End Source File
# Begin Source File

SOURCE=.\src\ibpp\ibpp.h
# End Source File
# End Group
# End Target
# End Project

