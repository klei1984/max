# Section
SectionEnd

!include "nsDialogs.nsh"

!define SetLangString "!insertmacro __SetLangString "
!macro __SetLangString id language string
	LangString GameDataDir_${id} ${LANG_${language}} "${string}"
!macroend

!define GetLangString "!insertmacro __GetLangString "
!macro __GetLangString id result
	Push $R0

	StrCpy $R0 $(GameDataDir_${id})
	${If} $R0 == ""
		StrCpy $R0 ${GameDataDirString_${id}}
	${EndIf}

	Push $R0
	Exch
	Pop $R0
	Pop ${result}
!macroend

!define GameDataDirString_Title `"Configure Game Settings"`
!define GameDataDirString_Description `"Select location of original game assets."`
!define GameDataDirString_GameDataDir `"Game Data Folder"`
!define GameDataDirString_GddFound `"Original M.A.X. game assets found."`
!define GameDataDirString_Help `"Please select the CD-ROM drive in case of an original M.A.X. CD-ROM, or the root folder of an existing M.A.X. full installation. $(^ClickInstall)"`

${SetLangString} Title ENGLISH ${GameDataDirString_Title}
${SetLangString} Description ENGLISH ${GameDataDirString_Description}
${SetLangString} GameDataDir ENGLISH ${GameDataDirString_GameDataDir}
${SetLangString} GddFound ENGLISH ${GameDataDirString_GddFound}
${SetLangString} Help ENGLISH ${GameDataDirString_Help}

${SetLangString} Title HUNGARIAN "Játékbeállítások konfigurálása"
${SetLangString} Description HUNGARIAN "Válassza ki az eredeti játékelemek mappáját."
${SetLangString} GameDataDir HUNGARIAN "Eredeti játékelemek helye"
${SetLangString} GddFound HUNGARIAN "Megvannak az eredeti M.A.X. játékelemek."
${SetLangString} Help HUNGARIAN "Eredeti M.A.X. CD-ROM esetén válassza ki a CD-ROM meghajtót. Telepített eredeti M.A.X. játék esetén válassza ki a meglévő telepítés gyökérmappáját. $(^ClickInstall)"

Var GameDataDir_EditControlHandle
Var GameDataDir_GameDataPath

Function GameDataDirVerifyAssets
	Push $R0 # install (next) button handle
	Push $R1 # path

	# discard passed control handle
	Exch 2
	Pop $R0

	# get button handle
	GetDlgItem $R0 $HWNDPARENT 1

	# get path from edit control
	${NSD_GetText} $GameDataDir_EditControlHandle $R1

	# verify assets
	${if} ${FileExists} "$R1\MAX.RES"
		EnableWindow $R0 1
		GetFullPathName $GameDataDir_GameDataPath "$R1"
	${ElseIf} ${FileExists} "$R1\MAX\MAX.RES"
		EnableWindow $R0 1
		GetFullPathName $GameDataDir_GameDataPath "$R1\MAX"
	${else}
		EnableWindow $R0 0
		StrCpy $GameDataDir_GameDataPath ""
	${endif}

	Pop $R1
	Pop $R0
FunctionEnd

Function GameDataDirBrowseButtonOnClick
	Push $R0 # path

	# discard passed control handle
	Exch
	Pop $R0

	# get path
	${NSD_GetText} $GameDataDir_EditControlHandle $R0

	# set initial path for dialog
	nsDialogs::SelectFolderDialog " " $R0
	Pop $R0

	# update edit control with valid selection
	${IfNot} $R0 == error
		${NSD_SetText} $GameDataDir_EditControlHandle "$R0"
	${EndIf}

	Pop $R0
FunctionEnd

Function GameDataDirPageCreate
	Push $R0 # Dialog handle
	Push $R1 # captions
	Push $R2 # captions
	Push $R3 # unused control handles
	Push $R4 # button handle

	# create a new page
	nsDialogs::Create 1018
	Pop $R0

	${If} $R0 == error
		Abort
	${EndIf}

	${GetLangString} Title $R1
	${GetLangString} Description $R2

	# set title and description fields
	!insertmacro MUI_HEADER_TEXT "$R1" "$R2"

	# set help text
	${GetLangString} Help $R1
	${NSD_CreateLabel} 0 0 100% 30u $R1

	# create group box for file dialog
	${GetLangString} GameDataDir $R1

	${NSD_CreateGroupBox} 0u 71u 100% 34u "$R1"
	Pop $R2

	# create edit control
	${NSD_CreateDirRequest} 10u 85u 210u 12u ""
	Pop $GameDataDir_EditControlHandle

	${NSD_OnChange} $GameDataDir_EditControlHandle GameDataDirVerifyAssets
	System::Call shlwapi::SHAutoComplete($GameDataDir_EditControlHandle, 1)

	# create browse button
	${NSD_CreateBrowseButton} 228u 83u 60u 15u "$(^BrowseBtn)"
	Pop $R4

	${NSD_OnClick} $R4 GameDataDirBrowseButtonOnClick

	# simulate directory request on change event
	Push ""
	Call GameDataDirVerifyAssets

	# render present page
	nsDialogs::Show

	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Section "" Section_SetupUserPrefsPath
	Push $R0 # context
	Push $R1 # path

	# save context
	Push "false"
	Pop $R0

	IfShellVarContextAll 0 +4
		SetShellVarContext current
		Push "true"
		Pop $R0

	# create user PrefsPath directory
	${IfNot} ${FileExists} "$APPDATA\Interplay\MAX\*.*"
		CreateDirectory "$APPDATA\Interplay\MAX\"
	${EndIf}

	# move settings.ini to user PrefsPath directory
	${IfNot} ${FileExists} "$APPDATA\Interplay\MAX\settings.ini"
		CopyFiles /SILENT /FILESONLY "$INSTDIR\settings.ini" "$APPDATA\Interplay\MAX\"
	${EndIf}

	# configure game data directory
	${If} ${FileExists} "$GameDataDir_GameDataPath\*.*"
		WriteINIStr "$APPDATA\Interplay\MAX\settings.ini" "SETUP" "game_data" "$GameDataDir_GameDataPath"
		FlushINI "$APPDATA\Interplay\MAX\settings.ini"
	${EndIf}

	# copy saved game files to user PrefsPath directory
	${If} ${FileExists} "$GameDataDir_GameDataPath\*.*"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.DTA" "$APPDATA\Interplay\MAX\"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.HOT" "$APPDATA\Interplay\MAX\"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.MLT" "$APPDATA\Interplay\MAX\"
	${EndIf}

	# restore context
	${If} $R0 == "true"
		SetShellVarContext all
	${EndIf}

	Pop $R1
	Pop $R0
# SectionEnd
