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
!define GameDataDirString_Help `"Please select the CD-ROM drive in case of an original M.A.X. CD-ROM, or the root folder of an existing M.A.X. full installation. $(^ClickInstall)"`
!define GameDataDirString_Portable `"Save all user data into the same folder that contains the game executable."`

${SetLangString} Title ENGLISH ${GameDataDirString_Title}
${SetLangString} Description ENGLISH ${GameDataDirString_Description}
${SetLangString} GameDataDir ENGLISH ${GameDataDirString_GameDataDir}
${SetLangString} Help ENGLISH ${GameDataDirString_Help}
${SetLangString} Portable ENGLISH ${GameDataDirString_Portable}

${SetLangString} Title GERMAN "Spieleinstellungen konfigurieren"
${SetLangString} Description GERMAN "Wählen Sie den Speicherort der Originalspiel-Dateien."
${SetLangString} GameDataDir GERMAN "Ordner für Spieldaten"
${SetLangString} Help GERMAN "Bitte wählen Sie das CD-ROM-Laufwerk bei einer originalen M.A.X. CD-ROM aus, oder das Stammverzeichnis einer bestehenden M.A.X. Vollinstallation. $(^ClickInstall)"
${SetLangString} Portable GERMAN "Speichern Sie alle Benutzerdaten im selben Ordner wie die Spielanwendung."

${SetLangString} Title FRENCH "Configurer les paramètres du jeu"
${SetLangString} Description FRENCH "Sélectionnez l'emplacement des fichiers originaux du jeu."
${SetLangString} GameDataDir FRENCH "Dossier des données du jeu"
${SetLangString} Help FRENCH "Veuillez sélectionner le lecteur CD-ROM si vous avez un CD-ROM original de M.A.X., ou le dossier racine d'une installation complète existante de M.A.X. $(^ClickInstall)"
${SetLangString} Portable FRENCH "Enregistrer toutes les données utilisateur dans le même dossier que l’exécutable du jeu."

${SetLangString} Title SPANISH "Configurar ajustes del juego"
${SetLangString} Description SPANISH "Seleccione la ubicación de los archivos originales del juego."
${SetLangString} GameDataDir SPANISH "Carpeta de datos del juego"
${SetLangString} Help SPANISH "Por favor, seleccione la unidad de CD-ROM si tiene el CD-ROM original de M.A.X., o la carpeta raíz de una instalación completa existente de M.A.X. $(^ClickInstall)"
${SetLangString} Portable SPANISH "Guardar todos los datos de usuario en la misma carpeta que contiene el ejecutable del juego."

${SetLangString} Title ITALIAN "Configura le impostazioni di gioco"
${SetLangString} Description ITALIAN "Seleziona la posizione dei file originali del gioco."
${SetLangString} GameDataDir ITALIAN "Cartella dei dati di gioco"
${SetLangString} Help ITALIAN "Seleziona l'unità CD-ROM in caso di CD-ROM originale di M.A.X., o la cartella principale di un’installazione completa di M.A.X. $(^ClickInstall)"
${SetLangString} Portable ITALIAN "Salva tutti i dati utente nella stessa cartella che contiene l'eseguibile del gioco."

${SetLangString} Title POLISH "Konfiguruj ustawienia gry"
${SetLangString} Description POLISH "Wybierz lokalizację oryginalnych plików gry."
${SetLangString} GameDataDir POLISH "Folder danych gry"
${SetLangString} Help POLISH "Wybierz napęd CD-ROM w przypadku oryginalnego CD-ROM M.A.X., lub folder główny istniejącej pełnej instalacji M.A.X. $(^ClickInstall)"
${SetLangString} Portable POLISH "Zapisz wszystkie dane użytkownika w tym samym folderze co plik wykonywalny gry."

${SetLangString} Title RUSSIAN "Настройка параметров игры"
${SetLangString} Description RUSSIAN "Выберите расположение оригинальных файлов игры."
${SetLangString} GameDataDir RUSSIAN "Папка игровых данных"
${SetLangString} Help RUSSIAN "Пожалуйста, выберите привод CD-ROM для оригинального диска M.A.X., или корневую папку существующей полной установки M.A.X. $(^ClickInstall)"
${SetLangString} Portable RUSSIAN "Сохранить все пользовательские данные в той же папке, что и исполняемый файл игры."

${SetLangString} Title HUNGARIAN "Játékbeállítások konfigurálása"
${SetLangString} Description HUNGARIAN "Válassza ki az eredeti játékelemek mappáját."
${SetLangString} GameDataDir HUNGARIAN "Eredeti játékelemek helye"
${SetLangString} Help HUNGARIAN "Eredeti M.A.X. CD-ROM esetén válassza ki a CD-ROM meghajtót. Telepített eredeti M.A.X. játék esetén válassza ki a meglévő telepítés gyökérmappáját. $(^ClickInstall)"
${SetLangString} Portable HUNGARIAN "Mentse az összes felhasználói adatot ugyanabba a mappába, amely a játék futtatható állományát tartalmazza."

Var GameDataDir_EditControlHandle
Var GameDataDir_CheckBoxHandle
Var GameDataDir_GameDataPath
Var GameDataDir_IsPortableMode

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

	# set portable text
	${GetLangString} Portable $R1

	# create checkbox
	${NSD_CreateCheckbox} 0u 110u 100% 20u "$R1"
	Pop $GameDataDir_CheckBoxHandle

	${NSD_Uncheck} $GameDataDir_CheckBoxHandle

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

Function GameDataDirPageLeave
	Push $R0 # checkbox state

	# save portable mode state
	${NSD_GetState} $GameDataDir_CheckBoxHandle $R0

	${If} $R0 == ${BST_CHECKED}
		Push "true"
	${Else}
		Push "false"
	${EndIf}

	Pop $GameDataDir_IsPortableMode

	Pop $R0
FunctionEnd

Section "" Section_SetupUserPrefsPath
	Push $R0 # context
	Push $R1 # path
	Push $R2 # user data path

	# save context
	Push "false"
	Pop $R0

	IfShellVarContextAll 0 +4
		SetShellVarContext current
		Push "true"
		Pop $R0

	# non portable mode
	${IfNot} $GameDataDir_IsPortableMode == "true"
		# store user data directory path
		Push "$APPDATA\M.A.X. Port"
		Pop $R2

		# create user PrefsPath directory
		${IfNot} ${FileExists} "$R2\*.*"
			CreateDirectory "$R2\"
		${EndIf}

		# move settings.ini to user PrefsPath directory
		${IfNot} ${FileExists} "$R2\settings.ini"
			CopyFiles /SILENT /FILESONLY "$INSTDIR\settings.ini" "$R2\"
		${EndIf}

		# remove unwanted configuration files
		Delete "$INSTDIR\settings.ini"
		Delete "$INSTDIR\.portable"
	${Else}
		# store user data directory path
		Push "$INSTDIR"
		Pop $R2
	${EndIf}

	# configure game data directory
	${If} ${FileExists} "$GameDataDir_GameDataPath\*.*"
		WriteINIStr "$R2\settings.ini" "SETUP" "game_data" "$GameDataDir_GameDataPath"
		FlushINI "$R2\settings.ini"
	${EndIf}

	# copy saved game files to user PrefsPath directory
	${If} ${FileExists} "$GameDataDir_GameDataPath\*.*"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.DTA" "$R2\"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.HOT" "$R2\"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.MLT" "$R2\"
		CopyFiles /SILENT /FILESONLY "$GameDataDir_GameDataPath\*.BAK" "$R2\"
	${EndIf}

	# restore context
	${If} $R0 == "true"
		SetShellVarContext all
	${EndIf}

	Pop $R2
	Pop $R1
	Pop $R0
# SectionEnd
