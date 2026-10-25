Unicode true
RequestExecutionLevel user
SetCompressor /SOLID zlib
SetDatablockOptimize on

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

Name "Vibmory ${APP_VERSION}"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\Vibmory"
InstallDirRegKey HKCU "Software\Vibmory\Installer" "InstallDir"
BrandingText "Vibmory"
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "Vibmory"
VIAddVersionKey /LANG=1033 "FileDescription" "Vibmory per-user installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright (c) 2021-2026 Mohammad Mokhalled and contributors"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\Vibmory.exe"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${APP_DIR}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "Vibmory requires 64-bit Windows."
        SetErrorLevel 1633
        Abort
    ${EndIf}
    ${IfNot} ${AtLeastBuild} 17763
        MessageBox MB_OK|MB_ICONSTOP "Vibmory requires Windows 10 version 1809 or newer."
        SetErrorLevel 1633
        Abort
    ${EndIf}
    SetRegView 64
    SetShellVarContext current
FunctionEnd

Function un.onInit
    SetRegView 64
    SetShellVarContext current
FunctionEnd

; Opening the installed executable for writing fails while it is mapped by a
; running application. This check happens before removing any installed files.
!macro CHECK_CLOSED NAME
Function ${NAME}
retry:
    IfFileExists "$INSTDIR\Vibmory.exe" 0 done
    System::Call 'kernel32::CreateFileW(w "$INSTDIR\Vibmory.exe", i 0x40000000, i 1, p 0, i 3, i 0, p 0)p.r0'
    ${If} $0 != -1
        System::Call 'kernel32::CloseHandle(p r0)'
        Goto done
    ${EndIf}
    IfSilent fail
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Close Vibmory before continuing. If it is already closed, check that you can write to the installation folder." IDRETRY retry
fail:
    SetErrorLevel 5
    Abort
done:
FunctionEnd
!macroend
!insertmacro CHECK_CLOSED CheckClosed
!insertmacro CHECK_CLOSED un.CheckClosed

Section "Vibmory" SEC_APPLICATION
    GetFullPathName $INSTDIR $INSTDIR
    ReadRegStr $0 HKCU "Software\Vibmory\Installer" "InstallDir"
    ${If} $0 != ""
        GetFullPathName $0 $0
        ${If} $0 != $INSTDIR
            IfSilent different_location
            MessageBox MB_OK|MB_ICONSTOP "Vibmory is installed in $0. Upgrade in that folder, or uninstall it before choosing another location."
different_location:
            SetErrorLevel 5
            Abort
        ${EndIf}
    ${EndIf}
    Call CheckClosed
    IfFileExists "$INSTDIR\Uninstall.exe" 0 install_files
    ExecWait '"$INSTDIR\Uninstall.exe" /S _?=$INSTDIR' $1
    ${If} $1 != 0
        SetErrorLevel $1
        Abort
    ${EndIf}
    Delete "$INSTDIR\Uninstall.exe"
install_files:
    SetOutPath "$INSTDIR"
    SetOverwrite on
    ClearErrors
    File /r "${APP_DIR}\*"
    IfErrors failed
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    IfErrors failed
    WriteRegStr HKCU "Software\Vibmory\Installer" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "DisplayName" "Vibmory"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "Publisher" "Vibmory"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "DisplayIcon" "$INSTDIR\Vibmory.exe,0"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory" "NoRepair" 1
    CreateDirectory "$SMPROGRAMS\Vibmory"
    CreateShortcut "$SMPROGRAMS\Vibmory\Vibmory.lnk" "$INSTDIR\Vibmory.exe"
    CreateShortcut "$SMPROGRAMS\Vibmory\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    SetErrorLevel 0
    Goto done
failed:
    SetErrorLevel 5
    Abort
done:
SectionEnd

Section "Uninstall"
    Call un.CheckClosed
    ClearErrors
    ; The generated list deletes only files shipped by this version. Empty
    ; directory removal preserves projects or other user files in the folder.
    !include "${UNINSTALL_FILES}"
    IfErrors failed
    Delete "$SMPROGRAMS\Vibmory\Vibmory.lnk"
    Delete "$SMPROGRAMS\Vibmory\Uninstall.lnk"
    RMDir "$SMPROGRAMS\Vibmory"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory"
    DeleteRegKey HKCU "Software\Vibmory\Installer"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    SetErrorLevel 0
    Goto done
failed:
    SetErrorLevel 5
    Abort
done:
SectionEnd
