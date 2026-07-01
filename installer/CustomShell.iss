[Setup]
AppName=CustomShell
AppVersion=1.0
DefaultDirName={commonpf}\CustomShell
DefaultGroupName=CustomShell
OutputBaseFilename=CustomShellInstaller
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin

[Files]
Source: "..\dist\shellhost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\shellwatchdog.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\config\shell.config.json.sample"; DestDir: "{commonappdata}\CustomShell"; DestName: "shell.config.json"; Flags: ignoreversion
; Shell Launcher v2 configuration
Source: "..\config\ShellLauncherConfiguration.xml"; DestDir: "{commonappdata}\Microsoft\ShellLauncher"; Flags: ignoreversion
Source: "..\config\Enable-ShellLauncher.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\CustomShell"; Filename: "{app}\shellhost.exe"

[Run]
Filename: "powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\Enable-ShellLauncher.ps1"""; StatusMsg: "Configuring Shell Launcher v2..."; Flags: runhidden runascurrentuser; Description: "Register CustomShell via Shell Launcher v2 (IoT Enterprise LTSC)"
Filename: "{app}\shellhost.exe"; Description: "Launch CustomShell now"; Flags: nowait postinstall skipifsilent
