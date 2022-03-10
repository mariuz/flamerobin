;Copyright (c) 2004-2022 The FlameRobin Development Team
;
;Permission is hereby granted, free of charge, to any person obtaining
;a copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:
;
;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.
;
;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;

;#define DEBUG

#include "..\..\src\frversion.h"
#define FR_VERSION_STRING Str(FR_VERSION_MAJOR) + "." + Str(FR_VERSION_MINOR) + "." + Str(FR_VERSION_RLS)

#if (FR_VERSION_RLS % 2 == 1)
  #if defined(FR_GIT_HASH)
    #define FR_VERSION_STRING FR_VERSION_STRING + "." + FR_GIT_HASH
  #endif
#endif

[Setup]
#ifdef X64VERSION
AppName=FlameRobin (x64)
AppVerName=FlameRobin {#FR_VERSION_STRING} (x64)
#else
AppName=FlameRobin
AppVerName=FlameRobin {#FR_VERSION_STRING}
#endif
AppPublisher=The FlameRobin Project
AppPublisherURL=http://www.flamerobin.org
AppSupportURL=http://www.flamerobin.org
AppUpdatesURL=http://www.flamerobin.org
#ifdef X64VERSION
DefaultDirName={pf}\FlameRobin (x64)
DefaultGroupName=FlameRobin (x64)
#else
DefaultDirName={pf}\FlameRobin
DefaultGroupName=FlameRobin
#endif
AllowNoIcons=true
LicenseFile=..\..\docs-src\fr_license.txt
#ifdef X64VERSION
InfoBeforeFile=info_before_win64.rtf
#endif
InfoAfterFile=
MinVersion=0,5.0.2195
#ifdef DEBUG
Compression=lzma/ultra
#ifdef X64VERSION
OutputBaseFilename=flamerobin-{#FR_VERSION_STRING}-setup-x64-debug
#else
OutputBaseFilename=flamerobin-{#FR_VERSION_STRING}-setup-debug
#endif
#else
Compression=lzma
#ifdef X64VERSION
OutputBaseFilename=flamerobin-{#FR_VERSION_STRING}-setup-x64
#else
OutputBaseFilename=flamerobin-{#FR_VERSION_STRING}-setup
#endif
#endif
SolidCompression=true
OutputDir=.\output
InternalCompressLevel=ultra
ShowLanguageDialog=yes
PrivilegesRequired=none
#ifdef X64VERSION
ArchitecturesAllowed=x64
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
; On all other architectures it will install in "32-bit mode".
ArchitecturesInstallIn64BitMode=x64
#endif

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}

[Files]
#ifdef DEBUG
#ifdef X64VERSION
Source: ..\..\vcud\flamerobin.exe; DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode
#else
Source: ..\..\vcud\flamerobin.exe; DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode
#endif
#else
#ifdef X64VERSION
Source: ..\..\vcu\flamerobin.exe; DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode
#else
Source: ..\..\vcu\flamerobin.exe; DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode
#endif
#endif
Source: ..\..\docs\*.*; Excludes: flamerobin.1; DestDir: {app}\docs; Flags: ignoreversion
Source: ..\..\conf-defs\*.*; DestDir: {app}\conf-defs; Flags: ignoreversion
Source: ..\..\code-templates\*.*; DestDir: {app}\code-templates; Flags: ignoreversion
Source: ..\..\html-templates\*.*; DestDir: {app}\html-templates; Flags: ignoreversion
Source: ..\..\sys-templates\*.*; DestDir: {app}\sys-templates; Flags: ignoreversion
#ifndef X64VERSION
;Source: ..\..\res\system32\msvcr71.dll; DestDir: {app}
;Source: ..\..\res\system32\msvcp71.dll; DestDir: {app}
#endif

[INI]
Filename: {app}\flamerobin.url; Section: InternetShortcut; Key: URL; String: http://www.flamerobin.org

[Icons]
Name: {group}\FlameRobin; Filename: {app}\flamerobin.exe; WorkingDir: {app}
Name: {group}\License; Filename: {app}\docs\fr_license.html
Name: {group}\What's new; Filename: {app}\docs\fr_whatsnew.html
Name: {group}\{cm:ProgramOnTheWeb,FlameRobin}; Filename: {app}\flamerobin.url
Name: {group}\{cm:UninstallProgram,FlameRobin}; Filename: {uninstallexe}
Name: {userdesktop}\FlameRobin; Filename: {app}\flamerobin.exe; Tasks: desktopicon; WorkingDir: {app}; IconIndex: 0
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\FlameRobin; Filename: {app}\flamerobin.exe; Tasks: quicklaunchicon; WorkingDir: {app}; IconIndex: 0

[Run]
Filename: {app}\flamerobin.exe; Description: {cm:LaunchProgram,FlameRobin}; Flags: nowait postinstall skipifsilent
Filename: {app}\docs\fr_whatsnew.html; Description: Show Release Notes; StatusMsg: Showing Release notes; Flags: nowait shellexec postinstall skipifsilent

[UninstallDelete]
Type: files; Name: {app}\flamerobin.url

[_ISTool]
UseAbsolutePaths=false

#expr SaveToFile(AddBackslash(SourcePath) + "FlameRobinSetup_Preprocessed.iss")
