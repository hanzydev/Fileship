!macro customUnInstall
    ${ifNot} ${isUpdated}
        nsExec::Exec '"$INSTDIR\resources\drive\fileship-drive.exe" --unregister-all'
    ${endIf}
!macroend
