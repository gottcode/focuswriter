;--------------------------------
;Definitions

!define APPNAME "FocusWriter"
!define VERSIONMAJOR 1
!define VERSIONMINOR 5
!define VERSIONPATCH 0
!define APPVERSION "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONPATCH}"
!define ABOUTURL "https://gottcode.org/focuswriter/"

;--------------------------------
;Includes

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "TextFunc.nsh"

;--------------------------------
;General

;Use highest compression
SetCompressor /SOLID /FINAL lzma

;Name and file
Name "${APPNAME}"
OutFile "${APPNAME}_${APPVERSION}.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""

;Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Variables

Var StartMenuFolder

;--------------------------------
;Interface Settings

!define MUI_ICON "focuswriter.ico"
!define MUI_UNICON "focuswriter.ico"
!define MUI_ABORTWARNING
!define MUI_LANGDLL_ALLLANGUAGES

;--------------------------------
;Language Selection Dialog Settings

;Remember the installer language
!define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
!define MUI_LANGDLL_REGISTRY_KEY "Software\${APPNAME}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Start Menu Folder Page Settings

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${APPNAME}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

;--------------------------------
;Finish Page Settings

!define MUI_FINISHPAGE_RUN "$INSTDIR\FocusWriter.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\ReadMe.txt"

;--------------------------------
;Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

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
;Reserve Files

!insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Functions

Function .onInit

	!insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

Function ConvertUnixNewLines

	Exch $R0 ;file #1 path
	Push $R1 ;file #1 handle
	Push $R2 ;file #2 path
	Push $R3 ;file #2 handle
	Push $R4 ;data
	Push $R5

	FileOpen $R1 $R0 r
	GetTempFileName $R2
	FileOpen $R3 $R2 w

	loopRead:
		ClearErrors
		FileRead $R1 $R4
		IfErrors doneRead

		StrCpy $R5 $R4 1 -1
		StrCmp $R5 $\n 0 +4
		StrCpy $R5 $R4 1 -2
		StrCmp $R5 $\r +3
		StrCpy $R4 $R4 -1
		StrCpy $R4 "$R4$\r$\n"

		FileWrite $R3 $R4

	Goto loopRead
	doneRead:

	FileClose $R3
	FileClose $R1

	SetDetailsPrint none
	Delete $R0
	Rename $R2 $R0
	SetDetailsPrint both

	Pop $R5
	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0

FunctionEnd

;--------------------------------
;Installer Section

Section "install"

	;Copy files
	SetOutPath $INSTDIR
	File ..\..\release\FocusWriter.exe
	File ..\symbols\symbols900.dat
	File $%QTDIR%\bin\iconv.dll
	File $%QTDIR%\bin\libgcc_s_sjlj-1.dll
	File $%QTDIR%\bin\libGLESv2.dll
	File $%QTDIR%\bin\libglib-2.0-0.dll
	File $%QTDIR%\bin\libharfbuzz-0.dll
	File $%QTDIR%\bin\libintl-8.dll
	File $%QTDIR%\bin\libpcre-1.dll
	File $%QTDIR%\bin\libpcre16-0.dll
	File $%QTDIR%\bin\libpng16-16.dll
	File $%QTDIR%\bin\libjpeg-62.dll
	File $%QTDIR%\bin\libstdc++-6.dll
	File $%QTDIR%\bin\libtiff-5.dll
	File $%QTDIR%\bin\libwebp-6.dll
	File $%QTDIR%\bin\libwinpthread-1.dll
	File $%QTDIR%\bin\zlib1.dll
	File $%QTDIR%\bin\Qt5Core.dll
	File $%QTDIR%\bin\Qt5Gui.dll
	File $%QTDIR%\bin\Qt5Multimedia.dll
	File $%QTDIR%\bin\Qt5Network.dll
	File $%QTDIR%\bin\Qt5PrintSupport.dll
	File $%QTDIR%\bin\Qt5Svg.dll
	File $%QTDIR%\bin\Qt5Widgets.dll
	File $%QTDIR%\bin\Qt5WinExtras.dll

	SetOutPath $INSTDIR\audio
	File $%QTDIR%\lib\qt5\plugins\audio\*.dll

	SetOutPath $INSTDIR\bearer
	File $%QTDIR%\lib\qt5\plugins\bearer\*.dll

	SetOutPath $INSTDIR\imageformats
	File $%QTDIR%\lib\qt5\plugins\imageformats\*.dll

	SetOutPath $INSTDIR\mediaservice
	File $%QTDIR%\lib\qt5\plugins\mediaservice\*.dll

	SetOutPath $INSTDIR\platforms
	File $%QTDIR%\lib\qt5\plugins\platforms\*.dll

	SetOutPath $INSTDIR\printsupport
	File $%QTDIR%\lib\qt5\plugins\printsupport\*.dll

	SetOutPath $INSTDIR\dictionaries
	File dicts\*.aff
	File dicts\*.dic
	File dicts\*.dll
	SetOutPath $INSTDIR\dictionaries\2\mor-standard
	File dicts\2\mor-standard\*

	SetOutPath $INSTDIR\icons\hicolor
	File ..\images\icons\oxygen\hicolor\index.theme
	SetOutPath $INSTDIR\icons\hicolor\16
	File ..\images\icons\oxygen\hicolor\16\*
	SetOutPath $INSTDIR\icons\hicolor\22
	File ..\images\icons\oxygen\hicolor\22\*
	SetOutPath $INSTDIR\icons\hicolor\32
	File ..\images\icons\oxygen\hicolor\32\*
	SetOutPath $INSTDIR\icons\hicolor\48
	File ..\images\icons\oxygen\hicolor\48\*
	SetOutPath $INSTDIR\icons\hicolor\64
	File ..\images\icons\oxygen\hicolor\64\*

	SetOutPath $INSTDIR\sounds
	File ..\sounds\*.wav

	SetOutPath $INSTDIR\themes
	File ..\themes\*.theme
	SetOutPath $INSTDIR\themes\images
	File ..\themes\images\*

	SetOutPath $INSTDIR\translations
	File ..\..\translations\*.qm
	File $%QTDIR%\share\qt5\translations\qt_*.qm
	File $%QTDIR%\share\qt5\translations\qtbase_*.qm
	File $%QTDIR%\share\qt5\translations\qtmultimedia_*.qm

	;Create ReadMe file
	SetOutPath $INSTDIR
	File /oname=ReadMe.txt ..\..\README
	FileOpen $4 "ReadMe.txt" a
	FileSeek $4 0 END
	FileWrite $4 "$\n$\nCredits$\n=======$\n$\n"
	FileClose $4
	File ..\..\CREDITS
	${FileJoin} "ReadMe.txt" "CREDITS" "ReadMe.txt"
	Delete $INSTDIR\CREDITS
	FileOpen $4 "ReadMe.txt" a
	FileSeek $4 0 END
	FileWrite $4 "$\n$\nNews$\n====$\n$\n"
	FileClose $4
	File ..\..\NEWS
	${FileJoin} "ReadMe.txt" "NEWS" "ReadMe.txt"
	Delete $INSTDIR\NEWS
	Push $INSTDIR\ReadMe.txt
	Call ConvertUnixNewLines

	;Registry information for add/remove programs
	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "Graeme Gott"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$\"$INSTDIR\FocusWriter.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "$\"${ABOUTURL}$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${APPVERSION}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMajor" ${VERSIONMAJOR}
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMinor" ${VERSIONMINOR}
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "EstimatedSize" "$0"

	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"

	;Create shortcut
	SetShellVarContext all
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME}.lnk" "$INSTDIR\FocusWriter.exe"
	!insertmacro MUI_STARTMENU_WRITE_END
	SetShellVarContext current

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE

FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	; Remove from registry
	DeleteRegKey HKLM "Software\${APPNAME}"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

	;Remove files
	Delete $INSTDIR\FocusWriter.exe
	Delete $INSTDIR\symbols*.dat
	Delete $INSTDIR\ReadMe.txt
	Delete $INSTDIR\*.dll
	Delete $INSTDIR\audio\*.dll
	Delete $INSTDIR\bearer\*.dll
	Delete $INSTDIR\dictionaries\*\*\*
	Delete $INSTDIR\icons\hicolor\*\*
	Delete $INSTDIR\imageformats\*.dll
	Delete $INSTDIR\mediaservice\*.dll
	Delete $INSTDIR\platforms\*.dll
	Delete $INSTDIR\printsupport\*.dll
	Delete $INSTDIR\sounds\*.wav
	Delete $INSTDIR\themes\*\*
	Delete $INSTDIR\translations\*.qm
	Delete $INSTDIR\Uninstall.exe

	;Remove directories
	RMDir /r $INSTDIR\dictionaries
	RMDir /r $INSTDIR\icons
	RMDir /r $INSTDIR\themes
	RMDir $INSTDIR\audio
	RMDir $INSTDIR\bearer
	RMDir $INSTDIR\imageformats
	RMDir $INSTDIR\mediaservice
	RMDir $INSTDIR\platforms
	RMDir $INSTDIR\printsupport
	RMDir $INSTDIR\sounds
	RMDir $INSTDIR\translations
	RMDir $INSTDIR

	;Remove shortcut
	SetShellVarContext all
	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
	Delete "$SMPROGRAMS\$StartMenuFolder\${APPNAME}.lnk"
	RMDir "$SMPROGRAMS\$StartMenuFolder"
	SetShellVarContext current

SectionEnd
