$shellPath = "C:\Program Files\CustomShell\shellhost.exe"
$everyoneSid = "S-1-1-0"

try {
    $sl = Get-CimInstance -Namespace "root\standardcimv2\embedded" -ClassName WESL_UserSetting -ErrorAction Stop
    if (-not $sl) {
        Write-Host "Creating WESL_UserSetting instance..."
        $sl = New-CimInstance -Namespace "root\standardcimv2\embedded" -ClassName WESL_UserSetting -ErrorAction Stop
    }
    Invoke-CimMethod -InputObject $sl -MethodName Enable | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetCustomShell -Arguments @{ Sid = $everyoneSid; Shell = $shellPath } | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetShellAction -Arguments @{ Sid = $everyoneSid; Action = "RestartShell"; ReturnCode = 0 } | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetShellAction -Arguments @{ Sid = $everyoneSid; Action = "RestartShell"; ReturnCode = 1 } | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetShellAction -Arguments @{ Sid = $everyoneSid; Action = "RestartShell"; ReturnCode = 2 } | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetShellAction -Arguments @{ Sid = $everyoneSid; Action = "RestartShell"; ReturnCode = -1 } | Out-Null
    Invoke-CimMethod -InputObject $sl -MethodName SetDefaultShellAction -Arguments @{ Sid = $everyoneSid; Action = "RestartShell" } | Out-Null
    Write-Host "WMI configuration applied successfully." -ForegroundColor Green
} catch {
    Write-Host "WMI error: $_" -ForegroundColor Red
}
