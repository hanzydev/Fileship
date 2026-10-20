!macro customUnInstall
    ${ifNot} ${isUpdated}
        nsExec::Exec '"$INSTDIR\resources\drive\fileship-drive.exe" --unregister-all'
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipUpload"
        DeleteRegKey HKCU "Software\Classes\CLSID\{9FDB7CD2-C34C-4340-A839-34E67C3335AD}"
        Delete "$LOCALAPPDATA\Fileship\harbor.json"
    ${endIf}
!macroend
