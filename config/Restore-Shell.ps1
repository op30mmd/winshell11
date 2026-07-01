#Requires -RunAsAdministrator

Write-Host "=== Restoring Original Shell ===" -ForegroundColor Cyan

# 1. Disable Shell Launcher via WMI
try {
    $sl = Get-WmiObject -Namespace "root\standardcimv2\embedded" -Class WESL_UserSetting -ErrorAction SilentlyContinue
    if ($sl) {
        $null = $sl.Disable()
        Write-Host "  Shell Launcher disabled." -ForegroundColor Green
    }
} catch {
    Write-Warning "  Could not disable Shell Launcher: $_"
}

# 2. Restore registry-based shell
$winlogonPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
$backupPath = "HKLM:\SOFTWARE\CustomShell"
try {
    $origShell = (Get-ItemProperty -Path $backupPath -Name "OriginalShell" -ErrorAction SilentlyContinue).OriginalShell
    if (-not $origShell) {
        $origShell = "explorer.exe"
    }
    Set-ItemProperty -Path $winlogonPath -Name "Shell" -Value $origShell -Force
    Write-Host "  Shell restored to: $origShell" -ForegroundColor Green
} catch {
    Write-Warning "  Could not restore shell: $_"
    Set-ItemProperty -Path $winlogonPath -Name "Shell" -Value "explorer.exe" -Force
    Write-Host "  Shell set to explorer.exe (default)" -ForegroundColor Yellow
}

# 3. Remove Shell Launcher XML config
$configPath = "$env:ProgramData\Microsoft\ShellLauncher\ShellLauncherConfiguration.xml"
if (Test-Path -LiteralPath $configPath) {
    Remove-Item -LiteralPath $configPath -Force
    Write-Host "  Shell Launcher config removed." -ForegroundColor Green
}

Write-Host "`nOriginal shell restored." -ForegroundColor Green
Write-Host "Restart your PC for changes to take effect." -ForegroundColor Yellow
