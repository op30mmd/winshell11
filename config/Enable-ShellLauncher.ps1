#Requires -RunAsAdministrator

$featureName = "Client-EmbeddedShellLauncher"
$shellPath = "C:\Program Files\CustomShell\shellhost.exe"
$namespace = "root\standardcimv2\embedded"
$everyoneSid = "S-1-1-0"

Write-Host "=== Shell Launcher v2 Setup ===" -ForegroundColor Cyan

Write-Host "[1/3] Enabling Shell Launcher feature..." -ForegroundColor Cyan
try {
    Enable-WindowsOptionalFeature -Online -FeatureName $featureName -All -ErrorAction Stop
    Write-Host "  Feature enabled." -ForegroundColor Green
} catch {
    Write-Warning "  Could not enable feature (may already be enabled): $_"
}

Write-Host "[2/3] Deploying Shell Launcher configuration file..." -ForegroundColor Cyan
$configDir = "$env:ProgramData\Microsoft\ShellLauncher"
$null = New-Item -ItemType Directory -Path $configDir -Force
$configXml = @"
<?xml version="1.0" encoding="utf-8"?>
<ShellConfiguration xmlns="http://schemas.microsoft.com/ShellLauncher/2019/Configuration">
  <Profiles>
    <Profile ID="{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}">
      <Shell Shell="$shellPath">
        <ReturnCodeActions>
          <ReturnCodeAction ReturnCode="0" Action="RestartShell" />
          <ReturnCodeAction ReturnCode="1" Action="RestartShell" />
          <ReturnCodeAction ReturnCode="2" Action="RestartShell" />
        </ReturnCodeActions>
        <DefaultAction>RestartShell</DefaultAction>
      </Shell>
    </Profile>
  </Profiles>
  <Configurations>
    <Configuration>
      <Profile ID="{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}" />
      <DefaultProfile />
    </Configuration>
  </Configurations>
</ShellConfiguration>
"@
$configPath = "$configDir\ShellLauncherConfiguration.xml"
Set-Content -Path $configPath -Value $configXml -Force
Write-Host "  Config placed at: $configPath" -ForegroundColor Green

Write-Host "[3/3] Applying Shell Launcher via WMI..." -ForegroundColor Cyan
try {
    $sl = Get-WmiObject -Namespace $namespace -Class WESL_UserSetting -ErrorAction Stop

    $null = $sl.Enable()
    Write-Host "  Shell Launcher enabled." -ForegroundColor Green

    $null = $sl.SetCustomShell($everyoneSid, $shellPath)
    Write-Host "  Custom shell set for Everyone group." -ForegroundColor Green

    $null = $sl.SetShellAction($everyoneSid, 0, "RestartShell")
    $null = $sl.SetShellAction($everyoneSid, 1, "RestartShell")
    $null = $sl.SetShellAction($everyoneSid, 2, "RestartShell")
    $null = $sl.SetDefaultShellAction($everyoneSid, "RestartShell")
    Write-Host "  Shell restart actions configured." -ForegroundColor Green

    Write-Host "`nShell Launcher v2 configured successfully!" -ForegroundColor Green
    Write-Host "Shell: $shellPath" -ForegroundColor Green
    Write-Host "Target: Everyone group" -ForegroundColor Green
    Write-Host "`nIMPORTANT: Restart your PC for changes to take effect." -ForegroundColor Yellow
} catch {
    Write-Warning "  WMI configuration failed: $_"
    Write-Host "  The XML config file has been placed at: $configPath" -ForegroundColor Yellow
    Write-Host "  You can apply it later using: Set-AssignedAccess -AppName '$shellPath'" -ForegroundColor Yellow
}
