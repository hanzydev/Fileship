!macro customCheckAppRunning
    nsExec::Exec `"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process -Filter \"Name='fileship-host.exe' or Name='fileship-harbor.exe' or Name='Fileship.exe'\" | Where-Object { $$_.ExecutablePath -like '$INSTDIR\*' } | ForEach-Object { Stop-Process -Id $$_.ProcessId -Force -ErrorAction SilentlyContinue }"`
    Pop $0
    Sleep 500
!macroend

!macro customUnInstall
    ${ifNot} ${isUpdated}
        nsExec::Exec '"$INSTDIR\resources\harbor\fileship-harbor.exe" --unregister-all'
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipUpload"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborCopyLink"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborOpen"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborShow"
        DeleteRegKey HKCU "Software\Classes\Directory\shell\FileshipHarborFolderLink"
        DeleteRegKey HKCU "Software\Classes\Directory\shell\FileshipHarborInboxLink"
        DeleteRegKey HKCU "Software\Classes\CLSID\{9FDB7CD2-C34C-4340-A839-34E67C3335AD}"
        DeleteRegKey HKCU "Software\Classes\CLSID\{5904EFEC-D576-4002-9F5A-DEEB56334DF9}"
        DeleteRegKey HKCU "Software\Classes\CLSID\{5C54A548-3A15-4569-B3CC-9D1472CC8672}"
        Delete "$LOCALAPPDATA\Fileship\harbor.json"
        DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Fileship"
    ${endIf}
!macroend
