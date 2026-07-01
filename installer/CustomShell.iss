[Setup]
AppName=CustomShell
AppVersion=1.0
DefaultDirName={pf}\CustomShell
DefaultGroupName=CustomShell
OutputBaseFilename=CustomShellInstaller
Compression=lzma
SolidCompression=yes

[Files]
Source: "..\dist\shellhost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\shellwatchdog.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\config\shell.config.json.sample"; DestDir: "{commonappdata}\CustomShell"; DestName: "shell.config.json"; Flags: ignoreversion

[Icons]
Name: "{group}\CustomShell"; Filename: "{app}\shellhost.exe"

[Run]
Filename: "{app}\shellhost.exe"; Description: "Launch CustomShell"; Flags: nowait postinstall skipifsilent
