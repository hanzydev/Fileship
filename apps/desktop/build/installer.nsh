!macro customUnInstall
    ${ifNot} ${isUpdated}
        nsExec::Exec '"$INSTDIR\resources\drive\fileship-drive.exe" --unregister-all'
        DeleteRegKey HKCU "Software\Classes\*\shell\FileshipUpload"
    ${endIf}
!macroend
