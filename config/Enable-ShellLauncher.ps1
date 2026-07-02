#Requires -RunAsAdministrator

$shellPath = "C:\Program Files\CustomShell\WinUIShell.exe"
$configDir = "$env:ProgramData\Microsoft\ShellLauncher"
$configPath = "$configDir\ShellLauncherConfiguration.xml"
$everyoneSid = "S-1-1-0"

Write-Host "=== CustomShell Boot Setup ===" -ForegroundColor Cyan

# 1. Deploy Shell Launcher v2 XML configuration
Write-Host "[1/4] Deploying Shell Launcher v2 XML config..." -ForegroundColor Cyan
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
          <ReturnCodeAction ReturnCode="-1" Action="RestartShell" />
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
Set-Content -Path $configPath -Value $configXml -Force
Write-Host "  Config placed at: $configPath" -ForegroundColor Green

# 2. Enable Shell Launcher v2 feature
Write-Host "[2/4] Enabling Shell Launcher feature..." -ForegroundColor Cyan
$shellLauncherAvailable = $false
try {
    $feature = Get-WindowsOptionalFeature -Online -FeatureName "Client-EmbeddedShellLauncher" -ErrorAction Stop
    if ($feature.State -eq "Enabled") {
        Write-Host "  Shell Launcher feature already enabled." -ForegroundColor Green
        $shellLauncherAvailable = $true
    } else {
        Enable-WindowsOptionalFeature -Online -FeatureName "Client-EmbeddedShellLauncher" -All -NoRestart -ErrorAction Stop | Out-Null
        Write-Host "  Shell Launcher feature enabled." -ForegroundColor Green
        $shellLauncherAvailable = $true
    }
} catch {
    Write-Warning "  Could not enable Shell Launcher feature: $_"
}

# 3. WMI activation (optional - v2 uses XML config at boot)
Write-Host "[3/4] Activating via WMI..." -ForegroundColor Cyan
try {
    $sl = Get-WmiObject -Namespace "root\standardcimv2\embedded" -Class WESL_UserSetting -ErrorAction SilentlyContinue
    if ($sl) {
        $null = $sl.Enable()
        $null = $sl.SetCustomShell($everyoneSid, $shellPath)
        $null = $sl.SetShellAction($everyoneSid, 0, "RestartShell")
        $null = $sl.SetShellAction($everyoneSid, 1, "RestartShell")
        $null = $sl.SetShellAction($everyoneSid, 2, "RestartShell")
        $null = $sl.SetShellAction($everyoneSid, -1, "RestartShell")
        $null = $sl.SetDefaultShellAction($everyoneSid, "RestartShell")
        Write-Host "  WMI configuration applied." -ForegroundColor Green
    } else {
        Write-Host "  WMI class not available (v2 uses XML at boot)" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  WMI step skipped (v2 XML config is sufficient)" -ForegroundColor Yellow
}

# 4. Registry fallback
Write-Host "[4/4] Setting registry fallback..." -ForegroundColor Cyan
$winlogonPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
try {
    $origShell = (Get-ItemProperty -Path $winlogonPath -Name "Shell" -ErrorAction SilentlyContinue).Shell
    if (-not $origShell -or $origShell -eq "") {
        $origShell = "explorer.exe"
    }
    $backupPath = "HKLM:\SOFTWARE\CustomShell"
    $null = New-Item -ItemType Directory -Path $backupPath -Force
    Set-ItemProperty -Path $backupPath -Name "OriginalShell" -Value $origShell -Force
    Write-Host "  Original shell backed up: $origShell" -ForegroundColor Green

    Set-ItemProperty -Path $winlogonPath -Name "Shell" -Value $shellPath -Force
    Write-Host "  Shell set in registry: $shellPath" -ForegroundColor Green
} catch {
    Write-Warning "  Registry fallback failed: $_"
}

# Summary
Write-Host ""
if ($shellLauncherAvailable) {
    Write-Host "Shell Launcher v2: AVAILABLE" -ForegroundColor Green
    Write-Host "  Feature enabled, XML config deployed." -ForegroundColor Green
} else {
    Write-Host "Shell Launcher v2: NOT AVAILABLE" -ForegroundColor Yellow
    Write-Host "  Registry fallback will launch the shell." -ForegroundColor Yellow
}
Write-Host "Registry fallback: SET" -ForegroundColor Green
Write-Host ""
Write-Host "IMPORTANT: Restart your PC for changes to take effect." -ForegroundColor Yellow
Write-Host "To restore the original shell, run: Restore-Shell.ps1" -ForegroundColor Cyan
