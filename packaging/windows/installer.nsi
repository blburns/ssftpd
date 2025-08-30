; Simple-Secure FTP Daemon Windows Installer
; NSIS Installer Script

!define PRODUCT_NAME "Simple-Secure FTP Daemon"
!define PRODUCT_VERSION "0.1.0"
!define PRODUCT_PUBLISHER "ssftpd Team"
!define PRODUCT_WEB_SITE "https://github.com/ssftpd/ssftpd"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\ssftpd.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "LICENSE"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "ssftpd-${PRODUCT_VERSION}-windows-x64.exe"
InstallDir "$PROGRAMFILES\ssftpd"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainApplication" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  
  ; Main executable
  File "ssftpd.exe"
  File "ssftpd-site.exe"
  File "ssftpd-module.exe"
  
  ; Libraries
  File "*.dll"
  
  ; Configuration files
  SetOutPath "$INSTDIR\etc\deployment"
  File /r "deployment\*"
  
  ; Service files
  SetOutPath "$INSTDIR\etc"
  File /r "etc\windows\*"
  
  ; Documentation
  SetOutPath "$INSTDIR\docs"
  File /r "docs\*"
  
  ; Create directories
  CreateDirectory "$INSTDIR\logs"
  CreateDirectory "$INSTDIR\var"
  CreateDirectory "$INSTDIR\ssl"
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\ssftpd"
  CreateShortCut "$SMPROGRAMS\ssftpd\ssftpd.lnk" "$INSTDIR\ssftpd.exe"
  CreateShortCut "$SMPROGRAMS\ssftpd\Uninstall.lnk" "$INSTDIR\uninst.exe"
  CreateShortCut "$DESKTOP\ssftpd.lnk" "$INSTDIR\ssftpd.exe"
SectionEnd

Section "ServiceInstallation" SEC02
  ; Install Windows service using NSSM
  SetOutPath "$INSTDIR"
  File "nssm.exe"
  
  ; Check if running as Administrator
  UserInfo::GetAccountType
  Pop $0
  StrCmp $0 1 admin_ok
  MessageBox MB_OK|MB_ICONSTOP "This installer must be run as Administrator to install the Windows service."
  Goto admin_fail
  
  admin_ok:
    ; Install service
    ExecWait '"$INSTDIR\nssm.exe" install ssftpd "$INSTDIR\ssftpd.exe" --config "$INSTDIR\etc\deployment\ssftpd.conf"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd DisplayName "Simple-Secure FTP Daemon"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd Description "Secure, configurable FTP server with modular architecture"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd AppDirectory "$INSTDIR"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd AppStdout "$INSTDIR\logs\ssftpd-service.log"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd AppStderr "$INSTDIR\logs\ssftpd-service-error.log"'
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd Start SERVICE_AUTO_START'
    
    ; Set environment variables
    ExecWait '"$INSTDIR\nssm.exe" set ssftpd AppEnvironmentExtra SSFTPD_HOME="$INSTDIR" SSFTPD_CONFIG="$INSTDIR\etc\deployment" SSFTPD_LOGS="$INSTDIR\logs"'
    
    ; Start service
    ExecWait 'net start ssftpd'
    
    Goto admin_done
  
  admin_fail:
    MessageBox MB_OK|MB_ICONINFORMATION "Service installation skipped. You can install it later by running the installer as Administrator."
  
  admin_done:
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\ssftpd\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\ssftpd.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ssftpd.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Uninstaller
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ; Stop and remove service
  ExecWait 'net stop ssftpd'
  ExecWait '"$INSTDIR\nssm.exe" remove ssftpd confirm'
  
  ; Remove shortcuts
  Delete "$SMPROGRAMS\ssftpd\Uninstall.lnk"
  Delete "$SMPROGRAMS\ssftpd\Website.lnk"
  Delete "$SMPROGRAMS\ssftpd\ssftpd.lnk"
  Delete "$DESKTOP\ssftpd.lnk"
  
  ; Remove directories
  RMDir "$SMPROGRAMS\ssftpd"
  RMDir /r "$INSTDIR\etc"
  RMDir /r "$INSTDIR\docs"
  RMDir /r "$INSTDIR\logs"
  RMDir /r "$INSTDIR\var"
  RMDir /r "$INSTDIR\ssl"
  
  ; Remove files
  Delete "$INSTDIR\ssftpd.exe"
  Delete "$INSTDIR\ssftpd-site.exe"
  Delete "$INSTDIR\ssftpd-module.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\nssm.exe"
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  
  ; Remove installation directory
  RMDir "$INSTDIR"
  
  ; Remove registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
SectionEnd
