; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------
;Include Modern UI
!include "MUI2.nsh"

!define Project dwindow
!define SHCNE_ASSOCCHANGED 0x8000000
!define SHCNF_IDLIST 0
!include "FileAssoc.nsi"

;--------------------------------
;General

SetCompressor /SOLID lzma

; The name of the installer
Name "3DVPlayer"
Icon "..\dwindow\ico\icoVSTAR.ico"

; The file to write
OutFile "dwindow_setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\3DVPlayer

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\3DVPlayer" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\3DVPlayer" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------

; Pages
;  !insertmacro MUI_PAGE_LICENSE "License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;Page components
;Page directory
;Page instfiles

;UninstPage uninstConfirm
;UninstPage instfiles

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" ;first language is the default language
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SerbianLatin"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Uzbek"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Afrikaans"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Esperanto"
  
;--------------------------------
; The stuff to install

LangString MAINPROGRAM_LANG ${LANG_ENGLISH} "3DVPlayer Program(required)"
LangString MAINPROGRAM_LANG ${LANG_SIMPCHINESE} "3DVPlayer主程序(必选)"
Section $(MAINPROGRAM_LANG)

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "StereoPlayer.exe"
  File "reset.exe"
  File "ErrorReport.exe"
  File "dwindow.ini"
  File "alpha.raw"
  File "logo.raw"
  File "Fonts.conf"
  File "MediaInfo.dll"
  File "DevIL.dll"
  File "detoured.dll"
  File "IntelWiDiExtensions.dll"
  SetOutPath $INSTDIR\codec
  File /r "codec\*"
  SetOutPath $INSTDIR\skin
  File "skin\*"
  SetOutPath $INSTDIR\Language
  File /r "Language\*"
  SetOutPath $INSTDIR
  SetOutPath $INSTDIR\UI
  File /r "UI\*"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\3DVPlayer "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\3DVPlayer" "DisplayName" "3DVPlayer"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\3DVPlayer" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\3DVPlayer" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\3DVPlayer" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
LangString STARTMENU_LANG ${LANG_ENGLISH} "Start Menu Shortcuts"
LangString STARTMENU_LANG ${LANG_SIMPCHINESE} "开始菜单快捷方式"
Section $(STARTMENU_LANG)

  CreateDirectory "$SMPROGRAMS\3DVPlayer"
  CreateShortCut "$SMPROGRAMS\3DVPlayer\3DVPlayer.lnk" "$INSTDIR\StereoPlayer.exe" "" "$INSTDIR\StereoPlayer.exe" 0
  CreateShortCut "$SMPROGRAMS\3DVPlayer\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

; Optional section (can be disabled by the user)
LangString DESKTOP_LANG ${LANG_ENGLISH} "Desktop Shortcuts"
LangString DESKTOP_LANG ${LANG_SIMPCHINESE} "桌面快捷方式"
Section $(DESKTOP_LANG)

  CreateShortCut "$DESKTOP\3DVPlayer.lnk" "$INSTDIR\StereoPlayer.exe" "" "$INSTDIR\StereoPlayer.exe" 0
  
SectionEnd

LangString ASSOC_LANG ${LANG_ENGLISH} "File association"
LangString ASSOC_LANG ${LANG_SIMPCHINESE} "文件关联"
SectionGroup /e $(ASSOC_LANG)

Section "3dv"
!insertmacro Assoc 3dv "3dv" "3dv file" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mkv3d"
!insertmacro Assoc mkv3d "mkv3d" "3D MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
!insertmacro Assoc mk3d "mk3d" "3D MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mkv"
!insertmacro Assoc mkv "mkv" "MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mp4"
!insertmacro Assoc mp4 "mp4" "MP4 File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mkv3d"
!insertmacro Assoc mkv3d "mkv3d" "3D MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
!insertmacro Assoc mk3d "mk3d" "3D MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "ssif"
!insertmacro Assoc ssif "ssif" "SSIF File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mpls"
!insertmacro Assoc mpls "mpls" "MPLS Playlist" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "ts/m2ts"
!insertmacro Assoc ts "ts" "Transport Stream File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
!insertmacro Assoc m2ts "m2ts" "Transport Stream File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "rmvb"
!insertmacro Assoc rmvb "rmvb" "RMVB File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
!insertmacro Assoc rm "rm" "RMVB File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "avi"
!insertmacro Assoc avi "avi" "AVI File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

SectionGroupEnd

Section "-Refresh File Icon"
System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'
SectionEnd


;--------------------------------
; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\3DVPlayer"
  DeleteRegKey HKCU "SOFTWARE\DWindow"

  ; Remove files and uninstaller
  Delete $INSTDIR\codec\*.dll
  Delete $INSTDIR\codec\*.ax
  Delete $INSTDIR\dwindow.exe
  Delete $INSTDIR\StereoPlayer.exe
  Delete $INSTDIR\reset.exe
  Delete $INSTDIR\detoured.dll
  Delete $INSTDIR\kill_ssaver.dll
  Delete $INSTDIR\launcher.exe
  Delete $INSTDIR\regsvr.exe
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\xvidcore.dll
  Delete $INSTDIR\alpha.raw
  Delete $INSTDIR\logo.raw
  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\fonts.conf
  Delete $INSTDIR\dxva2.dll
  Delete $INSTDIR\*.ini
  Delete $INSTDIR\Language\*.*

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\3DVPlayer\*.*"
  RMDir "$SMPROGRAMS\3DVPlayer"
  Delete "$DESKTOP\3DVPlayer.lnk"

  ; Remove directories used
  RMDir /r "$SMPROGRAMS\3DVPlayer"
  RMDir /r "$INSTDIR\codec"
  RMDir /r "$INSTDIR\Language"
  RMDir /r "$INSTDIR"
  RMDir /r "$INSTDIR\UI"

  ; Revese file association
  !insertmacro UnAssoc mkv
  !insertmacro UnAssoc mkv3d
  !insertmacro UnAssoc mk3d
  !insertmacro UnAssoc ssif
  !insertmacro UnAssoc mpls
  !insertmacro UnAssoc 3dv
  !insertmacro UnAssoc mp4
  !insertmacro UnAssoc rm
  !insertmacro UnAssoc rmvb
  !insertmacro UnAssoc ts
  !insertmacro UnAssoc m2ts
  !insertmacro UnAssoc avi

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

SectionEnd
