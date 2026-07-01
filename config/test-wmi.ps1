$namespace = "root\standardcimv2\embedded"
Write-Host "Checking WMI namespace: $namespace"

try {
    $sl = Get-WmiObject -Namespace $namespace -Class WESL_UserSetting -ErrorAction Stop
    if ($sl -eq $null) {
        Write-Host "WESL_UserSetting: NULL (no instance returned)" -ForegroundColor Red
    } else {
        Write-Host "WESL_UserSetting: FOUND" -ForegroundColor Green
        Write-Host "  IsEnabled: $($sl.IsEnabled)"
        Write-Host "  Trying Enable..."
        $sl.Enable() | Out-Null
        Write-Host "  Enable() succeeded" -ForegroundColor Green
    }
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
}
