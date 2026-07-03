[Setup]
AppName=CustomShell
AppVersion=1.0
DefaultDirName={commonpf}\CustomShell
DefaultGroupName=CustomShell
OutputBaseFilename=CustomShellInstaller
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
DisableProgramGroupPage=yes
UninstallDisplayName=CustomShell

[Files]
Source: "..\src\WinUIShell\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64\WinUIShell.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\WinUIShell\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\config\shell.config.json.sample"; DestDir: "{commonappdata}\CustomShell"; DestName: "shell.config.json"; Flags: ignoreversion
; Shell Launcher v2 configuration
Source: "..\config\ShellLauncherConfiguration.xml"; DestDir: "{commonappdata}\Microsoft\ShellLauncher"; DestName: "ShellLauncherConfiguration.xml"; Flags: ignoreversion
Source: "..\config\Enable-ShellLauncher.ps1"; DestDir: "{app}"; Flags: ignoreversion
; Recovery script
Source: "..\config\Restore-Shell.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; Register shell at boot via Shell Launcher v2 + registry fallback
Filename: "powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\Enable-ShellLauncher.ps1"""; StatusMsg: "Configuring boot shell..."; Flags: runhidden runascurrentuser
; Launch shell immediately
Filename: "{app}\WinUIShell.exe"; Description: "Launch CustomShell now"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Restore original shell
Filename: "powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\Restore-Shell.ps1"""; RunOnceId: "RestoreShell"; Flags: runhidden runascurrentuser

[UninstallDelete]
Type: files; Name: "{app}\*"
Type: dirifempty; Name: "{app}"
Type: files; Name: "{commonappdata}\CustomShell\*"
Type: dirifempty; Name: "{commonappdata}\CustomShell"
