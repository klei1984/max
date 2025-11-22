; Section
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
	Push $R1 # Path

	; Discard passed control handle
	Exch 2
	Pop $R0

	; Get button handle
	GetDlgItem $R0 $HWNDPARENT 1

	; Get path from edit control
	${NSD_GetText} $GameDataDir_EditControlHandle $R1

	; Verify assets
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
	Push $R0 # Path

	; Discard passed control handle
	Exch
	Pop $R0

	; Get path
	${NSD_GetText} $GameDataDir_EditControlHandle $R0

	; Set initial path for dialog
	nsDialogs::SelectFolderDialog " " $R0
	Pop $R0

	; Update edit control with valid selection
	${IfNot} $R0 == error
		${NSD_SetText} $GameDataDir_EditControlHandle "$R0"
	${EndIf}

	Pop $R0
FunctionEnd

Function GameDataDirPageCreate
	Push $R0 # Dialog handle
	Push $R1 # captions
	Push $R2 # captions
	Push $R3 # Unused control handles
	Push $R4 # Button handle

	; Create a new page
	nsDialogs::Create 1018
	Pop $R0

	${If} $R0 == error
		Abort
	${EndIf}

	${GetLangString} Title $R1
	${GetLangString} Description $R2

	; Set title and description fields
	!insertmacro MUI_HEADER_TEXT "$R1" "$R2"

	; Set help text
	${GetLangString} Help $R1
	${NSD_CreateLabel} 0 0 100% 30u $R1

	; Create group box for file dialog
	${GetLangString} GameDataDir $R1

	${NSD_CreateGroupBox} 0u 71u 100% 34u "$R1"
	Pop $R2

	; Create edit control
	${NSD_CreateDirRequest} 10u 85u 210u 12u ""
	Pop $GameDataDir_EditControlHandle

	${NSD_OnChange} $GameDataDir_EditControlHandle GameDataDirVerifyAssets
	System::Call shlwapi::SHAutoComplete($GameDataDir_EditControlHandle, 1)

	; Create browse button
	${NSD_CreateBrowseButton} 228u 83u 60u 15u "$(^BrowseBtn)"
	Pop $R4

	${NSD_OnClick} $R4 GameDataDirBrowseButtonOnClick

	; Set portable text
	${GetLangString} Portable $R1

	; Create checkbox
	${NSD_CreateCheckbox} 0u 110u 100% 20u "$R1"
	Pop $GameDataDir_CheckBoxHandle

	${NSD_Uncheck} $GameDataDir_CheckBoxHandle

	; Simulate directory request on change event
	Push ""
	Call GameDataDirVerifyAssets

	; Render present page
	nsDialogs::Show

	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Function GameDataDirPageLeave
	Push $R0 # Checkbox state

	; Save portable mode state
	${NSD_GetState} $GameDataDir_CheckBoxHandle $R0

	${If} $R0 == ${BST_CHECKED}
		Push "true"
	${Else}
		Push "false"
	${EndIf}

	Pop $GameDataDir_IsPortableMode

	Pop $R0
FunctionEnd

Function UpdateJsonGameDataPath
	; Stack on entry: settings_file_path
	Exch $R6 ; $R6 = settings file path, stack now has old $R6
	Push $R0 # Input file handle
	Push $R1 # Output file handle
	Push $R2 # Line buffer
	Push $R3 # Game_data path with escaped backslashes
	Push $R4 # Character position counter
	Push $R5 # Temp file path
	Push $R7 # Temp character
	Push $R8 # Substring test
	Push $R9 # Match counter for debugging

	; Escape backslashes in game data path for JSON (replace \\ with \\\\)
	StrCpy $R3 ""
	StrCpy $R4 0

	escape_loop:
		StrCpy $R7 "$GameDataDir_GameDataPath" 1 $R4
		StrCmp $R7 "" escape_done
		StrCmp $R7 "\" 0 +3
			StrCpy $R3 "$R3\\"
			Goto escape_next
		StrCpy $R3 "$R3$R7"
		escape_next:
		IntOp $R4 $R4 + 1
		Goto escape_loop

	escape_done:

	; Check if input file exists
	IfFileExists "$R6" file_exists file_not_found

	file_not_found:
		Goto cleanup

	file_exists:
	; Open input file for reading (mode 'r' = read, binary mode to preserve exact content)
	ClearErrors
	FileOpen $R0 "$R6" r
	${If} ${Errors}
		; Try alternative: open for read-write which sometimes works better on Windows
		ClearErrors
		FileOpen $R0 "$R6" a
		${If} ${Errors}
			Goto file_open_error
		${EndIf}
		FileSeek $R0 0 SET  # Go back to beginning
	${EndIf}

	; Create temporary output file
	GetTempFileName $R5
	FileOpen $R1 "$R5" w
	IfErrors temp_open_error

	; Initialize match counter
	StrCpy $R9 0

	; Process each line
	read_loop:
		ClearErrors
		FileRead $R0 $R2
		IfErrors read_done

		; Check if line contains "game_data" key using StrStr
		; StrStr returns the haystack starting from the needle if found, empty string if not found
		Push $R2
		Push '"game_data"'
		Call StrStr
		Pop $R8

		; If $R8 is empty, the substring was not found
		StrCmp $R8 "" write_line

		; Additionally check we're not in the middle of another key name
		; The line should have "game_data" not as part of another word
		; A proper match would have whitespace or quote before "game_data"
		Push $R2
		Push "$\t"
		Call StrStr
		Pop $R7
		StrCmp $R7 "" check_space_prefix

		; Check if tab+quote+game_data exists
		Push $R7
		Push '	"game_data"'
		Call StrStr
		Pop $R7
		StrCmp $R7 "" write_line  # Not the right pattern, skip
		Goto found_game_data

		check_space_prefix:
			; Check for space prefix pattern
			Push $R2
			Push ' "game_data"'
			Call StrStr
			Pop $R7
			StrCmp $R7 "" write_line  # Not found with proper prefix

		found_game_data:
		; Found game_data line with proper formatting
		IntOp $R9 $R9 + 1

		; This line contains game_data, find the part before the colon
		StrCpy $R4 0
		StrCpy $R8 ""

		find_colon:
			StrCpy $R7 $R2 1 $R4
			StrCmp $R7 "" write_line
			StrCmp $R7 ":" found_colon
			StrCpy $R8 "$R8$R7"
			IntOp $R4 $R4 + 1
			Goto find_colon

		found_colon:
			; Write modified line with proper JSON formatting (preserving indentation)
			FileWrite $R1 '$R8: "$R3",$\r$\n'
			Goto read_loop

		write_line:
			; Write unmodified line
			FileWrite $R1 $R2
			Goto read_loop

	read_done:
		FileClose $R0
		FileClose $R1

		; Replace original file with modified version
		Delete "$R6"
		Rename "$R5" "$R6"
		IfErrors rename_error
		Goto cleanup

	file_open_error:
		Goto cleanup

	temp_open_error:
		FileClose $R0
		Goto cleanup

	rename_error:

	cleanup:
	Pop $R9
	Pop $R8
	Pop $R7
	Pop $R6
	Pop $R5
	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Function CopyFilesNoClobber
	; Stack on entry (top first): destination_dir, source_pattern
	; Calling convention: Push source_pattern, Push destination_dir, Call CopyFilesNoClobber
	Exch $R0 ; $R0 = destination dir (top of stack), stack now has old $R0
	Exch ; Swap, stack now: destination_dir, old $R0
	Exch $R1 ; $R1 = source pattern (second on stack), stack now has old $R1, old $R0
	Push $R2 ; Search handle
	Push $R3 ; Current filename
	Push $R4 ; Source directory
	Push $R5 ; Destination file path
	Push $R6 ; Source file path / temp

	; Extract source directory by finding last backslash
	StrCpy $R4 ""
	StrLen $R6 $R1
	IntOp $R6 $R6 - 1
	
find_last_backslash:
	IntCmp $R6 -1 copy_loop_start
	StrCpy $R5 $R1 1 $R6
	StrCmp $R5 "\" found_backslash
	IntOp $R6 $R6 - 1
	Goto find_last_backslash
	
found_backslash:
	IntOp $R6 $R6 + 1
	StrCpy $R4 $R1 $R6

copy_loop_start:
	; Start finding files matching the pattern
	FindFirst $R2 $R3 $R1
	
	; Check if search handle is valid
	${If} $R2 == ""
		Goto copy_done
	${EndIf}
	${If} $R2 == "0"
		Goto copy_done
	${EndIf}
	
	; Check if any files were found
	StrCmp $R3 "" copy_done
	
copy_loop:
	; Skip "." and ".." entries
	StrCmp $R3 "." find_next
	StrCmp $R3 ".." find_next
	
	; Build full source and destination paths
	StrCpy $R6 "$R4$R3"
	StrCpy $R5 "$R0$R3"
	
	; Check if destination file already exists
	IfFileExists "$R5" find_next
	
	; Copy file (only if it doesn't exist)
	CopyFiles /SILENT "$R6" "$R5"
	
find_next:
	FindNext $R2 $R3
	StrCmp $R3 "" copy_done
	Goto copy_loop
	
copy_done:
	; Check if handle is valid before closing
	${If} $R2 != ""
	${AndIf} $R2 != "0"
		FindClose $R2
	${EndIf}
	
	Pop $R6
	Pop $R5
	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Section "" Section_SetupUserPrefsPath
	Push $R0 # Context
	Push $R1 # Unused
	Push $R2 # User data path

	; Save context
	Push "false"
	Pop $R0

	IfShellVarContextAll 0 +4
		SetShellVarContext current
		Push "true"
		Pop $R0

	; Non portable mode
	${IfNot} $GameDataDir_IsPortableMode == "true"
		; Store user data directory path
		Push "$APPDATA\M.A.X. Port"
		Pop $R2

		; Create user PrefsPath directory
		${IfNot} ${FileExists} "$R2\*.*"
			CreateDirectory "$R2\"
		${EndIf}

		; Move settings.json to user PrefsPath directory
		${IfNot} ${FileExists} "$R2\settings.json"
			CopyFiles /SILENT /FILESONLY "$INSTDIR\settings.json" "$R2\"
		${EndIf}

		; Remove unwanted configuration files
		Delete "$INSTDIR\settings.json"
		Delete "$INSTDIR\.portable"
	${Else}
		; Store user data directory path
		Push "$INSTDIR"
		Pop $R2
	${EndIf}

	; Configure game data directory
	${If} ${FileExists} "$GameDataDir_GameDataPath\*.*"
		; Update configuration file
		Push "$R2\settings.json"
		Call UpdateJsonGameDataPath

		; Copy saved game files to user PrefsPath directory (without overwriting existing files)
		${IfNot} $GameDataDir_GameDataPath == $R2
			Push "$GameDataDir_GameDataPath\*.DTA"
			Push "$R2\"
			Call CopyFilesNoClobber

			Push "$GameDataDir_GameDataPath\*.HOT"
			Push "$R2\"
			Call CopyFilesNoClobber

			Push "$GameDataDir_GameDataPath\*.MLT"
			Push "$R2\"
			Call CopyFilesNoClobber

			Push "$GameDataDir_GameDataPath\*.BAK"
			Push "$R2\"
			Call CopyFilesNoClobber
		${EndIf}
	${EndIf}

	; Restore context
	${If} $R0 == "true"
		SetShellVarContext all
	${EndIf}

	Pop $R2
	Pop $R1
	Pop $R0
; SectionEnd
