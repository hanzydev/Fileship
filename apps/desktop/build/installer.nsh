!macro customUnInstall
    ${ifNot} ${isUpdated}
        nsExec::Exec '"$INSTDIR\resources\harbor\fileship-harbor.exe" --unregister-all'
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipUpload"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborCopyLink"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborOpen"
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipHarborShow"
        DeleteRegKey HKCU "Software\Classes\CLSID\{9FDB7CD2-C34C-4340-A839-34E67C3335AD}"
        DeleteRegKey HKCU "Software\Classes\CLSID\{5904EFEC-D576-4002-9F5A-DEEB56334DF9}"
        DeleteRegKey HKCU "Software\Classes\CLSID\{5C54A548-3A15-4569-B3CC-9D1472CC8672}"
        Delete "$LOCALAPPDATA\Fileship\harbor.json"
    ${endIf}
!macroend
