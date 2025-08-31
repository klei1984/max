---
layout: page
title: M.A.X. Scripting API Manual
permalink: /scripting/
custom_css: documentation
---

## Preface

The manual specifies the scripting capabilities of M.A.X. Port for content creators. Use this guide to create and add new maps, missions, and campaigns to M.A.X. Port.

<h3 class="no-toc">Table of Contents</h3>

* TOC
{:toc}

## Scripting Overview

M.A.X. Port uses JSON configuration files [\[1\]](#ref1) and Lua v5.4 scripts as its scripting interface [\[2\]](#ref2). The manual assumes that the reader is familiar with both languages.

In the original MS-DOS versions of M.A.X. training missions, scenarios or stand alone missions, and even the campaign missions are just normal game save files using save file format version 70. In theory this means that one can develop additional missions for the game by simply adding new save files to the game, but unfortunately M.A.X. hard codes mission goals, or victory and loss conditions, and thus the number of supported scenarios, campaign and training missions. Mission descriptions and briefings are loaded from text files that are dedicated to a particular language and such text files are encoded according to a game executable specific MS-DOS code page. Full screen background images for briefing and debriefing screens are randomly selected from a predefined set. Background music for mission selection and mission briefings are fixed. Unit build capability lists that define what units are allowed to be built during a mission are hard coded and unique configurations are limited to a set of training missions.

Maps or `*.WRL` files are even more constrainted as only a fixed set of map files are supported and their file names are hard coded together with the map title, description and world class. Game save files simply store a map index to identify which map to load from the fixed list and all maps must be 112 by 112 tiles in size. M.A.X. uses color palette animations for water caustic effects and similar with fixed timings and color rotation cycles. Map files embed the used tile graphic set creating a lot of redundancy, unnecessarily increasing file size.

The primary goal of M.A.X. Port's scripting interface is to make it possible to support more maps, missions, additional languages, and more versatile mission rules.

## Asset Management

M.A.X. Port distinguishes resources, abstract file system resources and file system files.

### Folder Structure

Each operating system follows its own unique folder structure for application and user specific files. M.A.X. Port defines the following asset search locations:

- `FilePathGameData`: A file system path to original MS-DOS M.A.X. game assets that is user configurable via `settings.ini`. This is the base folder of an original M.A.X. installation or the MAX folder of an original CD-ROM release. Examples: `C:\INTRPLAY\MAX`, `c:\Program Files (x86)\GOG Galaxy\Games\MAX`, `~/.steam/steam/steamapps/common/M.A.X. Mechanized Assault & Exploration/max`, `D:\MAX`. Assets accessed through this parameter are handled as read-only.

- `FilePathGameBase`: The root folder of a M.A.X. Port installation where the game executable and other read-only assets are found like `PATCHES.RES` or language packs like `lang_en-US.ini`. Examples: `C:\Program Files\M.A.X. Port`, `/usr/share/max-port/`. In case of portable installations `FilePathGameBase` and `FilePathGameData` are usually set to the same file system location.

- `FilePathGamePref`: A file system path where users have write access privileges. This is where `settings.ini`, game save files, screenshots, and log files are created. In case of portable installations `FilePathGamePref` is set to `FilePathGameBase` which means that the user must have write access privileges to the latter. Examples: `C:\Users\<user name>\AppData\Roaming\M.A.X. Port`, `$XDG_DATA_HOME/max-port`, `$HOME/.var/app/io.github.max-port/data/max-port`.

Certain files are searched case insensitive by the application. In practice this means that a file name is searched on the file system by converting the provided name to all lower-case or all upper-case glyphs. Best practice rule: create and reference only all lower-case file names.

#### Resources

The game stores resources in `MAX.RES` and `PATCHES.RES`. The list of resources cannot be extended by content creators, the defined resources are hard coded by the application via the ResourceManager software component. `PATCHES.RES` not only holds M.A.X. Port specific new resources, the resource file can be used to override resources defined by `MAX.RES`. M.A.X. Port uses the RES file format of MS-DOS M.A.X. v1.04.

RES files have no deterministic format specification identifiers. M.A.X. 1 and M.A.X. 2 uses different format specifications and their resource files are not interoperable.

#### Abstract File System Resources

RES files can be used as a file system indirection system. In practice this means that the application references a resource ID, and the resource just holds a file name that is found in the file system. A benefitial use case for the indirection layer would be to store all language specific files in the same folder by encoding the language variant into the file name and then simply use the language variant specific file in the resource held by the language specific RES file. This way the game would automatically use the correct voice files or mission briefing and description files without the need to reinstall the original game selecting a different language. Original M.A.X. did not harness this opportunity. The original developers only used the system to rename RIFF `*.WAV` files found on `CD-ROM CD-ICD-082-0` and `CD-ICD-082-EU` to `*.SPW` on later CD-ROM releases like `CD-ICD-082-SP` or `CD-ICD-082-1`.

#### File System Files

There are certain file types that the application directly references without indirection through resources. These include game save files, the color palette PAL file and RES files.

## Lua Interpreter

The application embeds a subset of Lua v5.4 features. Scripts are executed in isolated contexts, each with its own global tables and predefined set of functions. Lua scripts can access various aspects of the game in a read-only fashion via exposed C/C++ APIs.

The ability to load packages is removed. In addition, the following Lua standard libraries are removed: package library, input and output, operating system facilities, debug facilities.

The basic library's `print()` function is redirected to `stdout.txt` which is saved latest on application exit to the folder `FilePathGamePref`. If Lua scripts encounter runtime errors they are aborted. Typically diagnostic messages would be shown to users and they would be saved to `stdout.txt` as well.

Each script runs on a time budget which is expressed in milliseconds. The limit is set to 10 milliseconds.

### Script Contexts

There are specific Lua contexts for specific types of scripts. In other words each type of script runs in its own dedicated context.

| Context Class | Description |
|-------|--------|
| `WINLOSS_CONDITIONS` | Mission victory and loss conditions are evalauted at the end of each turn. Each Mission registers a single script to determine victory and loss conditions. |
| `GAME_RULES` | Builder capability lists and build rules are evaluated by such scripts on demand. Each Mission registers a single script to determine these rules. |
| `GAME_EVENTS` | These scripts can evaluate and signal game events. |
| `GAME_MUSIC`| The music jukebox is controlled by such scripts. Each Mission could optionally register its own jukebox function. |

## Registry System

The registry system exposes dynamic game state to Lua via the `MAX_REGISTRY` global table. `MAX_REGISTRY` holds predefined key and value pairs. The predefined keys are read-only for scripts, only the application is allowed to update their values. Predefined keys update their values just before a script is executed, they are kept always up to date.

```lua
-- save the value of the registry key RedTeamPoints to a local variable for the life cycle of the chunk
local red_team_point = MAX_REGISTRY.RedTeamPoints
```

Scripts are allowed to add new key and value pairs to `MAX_REGISTRY` that are not reserved by predefined key and value pairs. The registry is globally shared between all script contexts. For example a mission can define a `GAME_EVENTS` script that sets a `MAX_REGISTRY` key and value pair that the `WINLOSS_CONDITIONS` script acts upon. The keys are strings. The values could be strings, integer numbers, floating numbers and booleans. To erase a script defined key and value pair assign the key specific value to nil.

```lua
-- create a new key and value pair
MAX_REGISTRY.MyNewKey1 = 1000
MAX_REGISTRY["MyNewKey2"] = 2000

-- read the value of the new key
local var1 = MAX_REGISTRY.MyNewKey2
local var2 = MAX_REGISTRY["MyNewKey1"]

-- delete the previously created key and value pairs
MAX_REGISTRY["MyNewKey1"] = nil
MAX_REGISTRY.MyNewKey2 = nil
```

The `MAX_REGISTRY` is reset on mission initialization time.

The following read-only registry key and value pairs are predefined:

| Key | Value Type | Value Range | Description |
|-------|--------|-------|--------|
| `RedTeamPoints` | `integer number` | 0 .. 2^32 | Team points or eco-sphere rating of `MAX_TEAM.RED`. |
| `GreenTeamPoints` | `integer number` | 0 .. 2^32 | Team points or eco-sphere rating of `MAX_TEAM.GREEN`. |
| `BlueTeamPoints` | `integer number` | 0 .. 2^32 | Team points or eco-sphere rating of `MAX_TEAM.BLUE`. |
| `GrayTeamPoints` | `integer number` | 0 .. 2^32 | Team points or eco-sphere rating of `MAX_TEAM.GRAY`. |
| `TeamScoreVictoryLimit` | `integer number` | -1, 0 .. 2^32 | Score based victory limit or -1 if victory limit is turns based. |
| `TeamTurnVictoryLimit` | `integer number` | -1, 0 .. 2^32 | Turns based victory limit or -1 if victory limit is score based. |
| `TeamTurnValue` | `integer number` | 0 .. 2^31 | Turn counter value of the active team(s). |

### Global Read-Only Tables

The following read-only tables are defined in every script context:

| Table Name | Description |
|-------|--------|
| [`MAX_UNIT`](#max_unit-enumeration) | Resource ID of each unit type. |
| [`MAX_VOICE`](#max_voice-enumeration) | Resource ID of each voice sound. |
| [`MAX_MUSIC`](#max_music-enumeration) | Resource ID of each music track. |
| [`MAX_IMAGE`](#max_image-enumeration) | Resource ID of each full screen background image. |
| [`MAX_ICON`](#max_icon-enumeration) | Resource ID of icons used by the messages section of the reports menu. |
| [`MAX_TEAM`](#max_team-enumeration) | Logical ID of each team. |
| [`MAX_CARGO_TYPE`](#max_cargo_type-enumeration) | Logical ID of each surveyable resource or cargo type. |
| [`MAX_UNIT_CLASS`](#max_unit_class-enumeration) | Logical ID of each unit category. |
| [`MAX_UNIT_ATTRIB`](#max_unit_attrib-enumeration) | Logical ID of each unit attribute. |
| [`MAX_RESEARCH_TOPIC`](#max_research_topic-enumeration) | Logical ID of each research topic. |

## Victory Conditions Script Context

The script context executes a Lua chunk at the end of each game turn to evaluate the victory and loss conditions status applicable to each team. Teams are evaluated from smallest logical value to highest so from `MAX_TEAM.RED` to `MAX_TEAM.DERELICT`.

Example script from standard campaign mission 1:

<details>
<summary>Click to Open / Close Lua script file</summary>
{% highlight lua %}
{% include_relative assets/files/campaign0001.mission.json %}
{% endhighlight %}
</details>
<br>

Definition of mission victory and loss conditions is optional. If a mission does not define its own rules the default rule is applied which is very simple:

```lua
-- use only basic rules that do not consider victory by elimination
return MAX_VICTORY_STATE.GENERIC
```

This means that by default victory or defeat by elimination is not considered. The generic conditions of `MAX_VICTORY_STATE.GENERIC` can be decuded from the following pseudo code:

```c++
if (
    /* are scripted victory conditions met? */
    team_status[player_team] == VICTORY_STATE_WON or

    /* are scripted defeat conditions met? */
    team_status[player_team] == VICTORY_STATE_LOST or

    /* is last team standing in general single player mission? */
    (team_status[player_team] != VICTORY_STATE_PENDING and teams_in_play == 1 and
     teams_total > 1) or

    /* is no team standing? */
    (teams_in_play == 0) or

    /* are all teams that were humans defeated? */
    (teams_humans_in_play == 0 and teams_non_computers > 0) or

    /* is turns limit reached in a turns limit based mission? */
    ((victory_type == VICTORY_TYPE_DURATION) and (victory_limit <= turn_counter)) or

    /*is team Eco-Sphere rating, or team score points limit, reached? */
    ((victory_type == VICTORY_TYPE_SCORE) and
     (TeamInfo[evaluated_team].team_type != TEAM_TYPE_NONE) and
     (TeamInfo[evaluated_team].team_points >= victory_limit))) {
    game_over();
}
```

An eliminated team's remaining military units are normally inactive. Computer players cannot get eliminated status. Even if a computer team's M.A.X. Commander leaves the plant, its military units remain on the planet and will fight back.

In case of both turns based and Eco-Sphere rating based victory type the team with the highest Eco-Sphere rating, or team score points, wins the game. In case of a tie the team with the highest total worth of team assets amassed during the mission determines the winner. In case of a tie the team with the lower team index wins i.e. red team has a natural advantage over green team.

The following read-only tables are defined additionally to common ones in the given script context:

| Table Name | Description |
|-------|--------|
| [`MAX_VICTORY_STATE`](#max_victory_state-enumeration) | Logical ID of each victory and loss operating state. |

Functions defined additionally to common ones in the given script context:

| Function Name | Description |
|-------|--------|
| [`max_has_materials`](#max_has_materials) | Total amount of a mined resource type a team has. |
| [`max_count_ready_units`](#max_count_ready_units) | Total amount of a unit type a team has. |
| [`max_has_attack_power`](#max_has_attack_power) | Whether a team has any basic attack power to continue to fight. |
| [`max_can_rebuild_complex`](#max_can_rebuild_complex) | Whether a team has any basic construction power to rebuild. |
| [`max_can_rebuild_builders`](#max_can_rebuild_builders) | Whether a team has any basic manufacturing power to produce builders. |
| [`max_count_total_mining`](#max_count_total_mining) | Total amount of resources a team mines per turn. |
| [`max_get_total_units_being_constructed`](#max_get_total_units_being_constructed) | Total amount of units of a unit type are being constructed by a team. |
| [`max_get_total_power_consumption`](#max_get_total_power_consumption) | Total amount of power consumed by a team's units of a particular type. |
| [`max_get_total_mining`](#max_get_total_mining) | Total amount of resources mined by a team per turn. |
| [`max_get_total_consumption`](#max_get_total_consumption) | Total amount of resource consumption by a team per turn. |
| [`max_has_unit_above_mark_level`](#max_has_unit_above_mark_level) | Whether a team has any unit type with mark level above limit. |
| [`max_has_unit_experience`](#max_has_unit_experience) | Whether a team has any unit of unit type with experience level above 0. |
| [`max_count_casualties`](#max_count_casualties) | Total amount of casualties a team has from a unit type. |
| [`max_count_units_above_attrib_level`](#max_count_units_above_attrib_level) | Total amount of units a team has from a unit type with an attribute level above limit. |
| [`max_count_units_with_reduced_attrib`](#max_count_units_with_reduced_attrib) | Total amount of units a team has from a unit type with any attribute level below the base level. |
| [`max_get_research_level`](#max_get_research_level) | A team's level of a research topic. |

Callback functions defined additionally to common ones in the given script context:

| Callback Function Name | Description |
|-------|--------|
| N/A |  |

## Game Rules Script Context

The script context does not have a conventional periodically called application schedule point. The application calls the `max_get_builder_type`, `max_get_buildable_units` or `max_is_buildable` lua functions on demand. These functions operate on the `max_builder_capability_list` table.

The default game rules script is loaded and executed at context creation time on mission load. Missions can optionally define their own Lua chunk that gets loaded and executed in the given context. This allows missions to override the `max_builder_capability_list` table or to override the callback functions them selves or both.

The default builder capability list and related functions:

<details>
<summary>Click to Open/Close Lua script file</summary>
{% highlight lua %}
{% include_relative assets/files/default.game_rules.lua %}
{% endhighlight %}
</details>
<br>

The following read-only tables are defined additionally to common ones in the given script context:

| Table Name | Description |
|-------|--------|
| N/A |  |

Functions defined additionally to common ones in the given script context:

| Function Name | Description |
|-------|--------|
| N/A |  |

Callback functions defined additionally to common ones in the given script context:

| Callback Function Name | Description |
|-------|--------|
| [`max_get_builder_type`](#max_get_builder_type) | Unit type that can build the queried unit type. |
| [`max_get_buildable_units`](#max_get_buildable_units) | Table of unit types that a queried unit type can build. |
| [`max_get_builder_type`](#max_get_builder_type) | Whether a unit type is buildable. |

## Game Event System Script Context

Not implemented yet.

## Music Player Script Context

Not implemented yet.

## JSON Descriptor Files

All JSON files SHALL be UTF-8 encoded. This is required so that text maps correctly to TTF fonts used by the application.

JSON descriptor files have complex structures that are validated against JSON schemas. The schema files follow the JSON Schema draft-07 feature set [\[3\]](#ref3). An online schema validator can be found at [\[4\]](#ref4) to validate data in JSON files against their associated schemas outside the application. In case a JSON descriptor file is invalid, it is rejected by the application and related error messages may be saved into the `stdout.txt` log file located in the `FilePathGamePref` file system path.

JSON script files are read by the application from the `FilePathGamePref` file system path. These files hold mandatory and optional fields, also called name:value or key:value pairs. Fields in JSON scripts are structured hierarchically. The following example shows the description field which is a JSON object that holds a single property, or subfield called text, that holds a collection of properties or subfields. In short a mission `description` is a `text` block that supports multiple locale aware languages to be specified.

```json
"description": {
	"text": {
		"en-US": "Long multi line text block. To start a new line use the \n escape sequence.",
		"fr-FR": "..."
	}
}
```

Most fields that represent human language related media like text, audio or video have subfields for language tags. Language tags follow the IETF BCP 47 [\[5\]](#ref5) format. The first part (e.g., `en`) is the ISO 639-1 language code (English). The second part (e.g., `US`) is the ISO 3166-1 country code (United States). I.e. `en-US` means English as used in the United States. Other examples: `de-DE` (German-Germany), `fr-FR` (French-France). This allows media contents to provide localized regional variants.

JSON script files could reference game assets via resource identifiers and file system paths. All file system paths in JSON descriptor files must be relative to the `FilePathGamePref` file system path. All file system paths outside `FilePathGamePref` are rejected by the application from JSON scripts.

JSON and Lua scripts embedded into application resources are minified [\[6\]](#ref6)[\[7\]](#ref7) and obfuscated by the application to prevent hostile code from detecting and twisting them. Minification of scripts also helps to reduce file sizes and is another form of format compatibility verification.

### Supported Media Types

**Warning:** File system media types are not supported yet. Use resource media types referenced by resource identifiers.

#### Video Playback

Interplay MVE video and audio container and media stream decoding. Subtitles, multiple audio streams and various chunk encodings are not supported.

#### Audio Playback

RIFF container with WAVE and smpl chunks. The latter is used for audio loop points support. PCM audio only. Music tracks may have loop points, but the application does not utilize them.

#### Image Rendering

The application supports only the proprietary MaxBigImage image format.

### JSON File Naming Rules

General rules:
- Structure: `[<subtopic>.]*<maintopic>.json`. Subtopics are optional and are separated by a dot (.). The leftmost identifier represents the highest-granularity subtopic. The rightmost identifier before `.json` SHALL denote the broadest/general topic.
- Identifiers SHALL be stable and descriptive, reflecting the semantic content of the file.
- Identifiers SHALL consist of lowercase alphanumeric characters (a–z, 0–9) and MAY include underscores (_) if necessary.
- Identifiers SHALL NOT contain whitespace or special characters other than the dot (.) used as a separator.
- The filename SHALL always terminate with the `.json` extension.

Examples: `campaign.schema.json`, `training0001.mission.json`.

### JSON Mission Descriptor File Format

Example script from standard campaign mission 1:

<details>
<summary>Click to Open/Close JSON file</summary>
{% highlight json %}
{% include_relative assets/files/campaign0001.mission.json %}
{% endhighlight %}
</details>
<br>

Curated content and related scripts are embedded into `PATCHES.RES`. These assets are loaded by the application first from resources, JSON descriptor files found in the `FilePathGamePref` folder are loaded second. Missions loaded from resources are listed in mission selection screens first in the order they are defined in the application's resource table. Missions loaded from the file system are listed after missions loaded from resources and are sorted according to locale aware lexicographical ordering of the mission titles.

#### Primary Fields

| Field Name | Is Mandatory | Description |
|-------|--------|--------|
| `$schema` | Yes | JSON schema format specification to be used for validation. |
| `author`<br>`copyright`<br>`license` | Yes | Metadata about the mission and its usage rights. |
| `category` | Yes | Mission category. Allowed values: `Custom`, `Training`, `Campaign`, `Hot-seat`, `Multiplayer`, `Demo`, `Single Player Scenario`, `Multiplayer Scenario`. |
| `mission` | Category Dependent | Mission file name. |
| `hash` | Category Dependent | Array of SHA256 hashes (64 lowercase hex digits) used for mission file integrity verification. |
| `title` | Yes | Title of the mission shown in lists and captions in-game. |
| `description` | Yes | Mission overview and goals. |
| `intro` | No | Mission briefing that plays before starting the mission. |
| `victory` | No | Mission debrifing in case the player was victorious. |
| `defeat` | No | Mission debrifing in case the player was defeated. |
| `winloss_conditions` | No | Victory conditions script. |
| `game_rules` | No | Game rules script. |
| `game_events` | No | Game event system script. |
| `game_music` | No | Music player system script. |

#### Mission Categories

- `Custom` : Pseudo category used for player saves. Do not use. Game save file extension is `.dta`.

- `Training` : Training missions listed under the Training Missions menu. Mission file extension is `.tra`. Missions in this category explain game mechanics and teach strategies.

- `Campaign` : Campaign missions listed under the Campaign Game menu. Mission file extension is `.cam`. Missions in this category belong to a campaign specified by a `<name>.campaign.json` campaign descriptor file.

- `Hot-seat` : Pseudo category used for player saves. Do not use. Game save file extension is `.hot`.

- `Multiplayer` : Pseudo category used for player saves. Do not use. Game save file extension is `.mlt`.

- `Demo` : Attract demo missions loaded automatically on the application title screen after 1 minute of idling. These missions run unattended for 5 turns and then exit back to the title screen. These missions only feature computer players. Mission file extension is `.dmo`.

- `Single Player Scenario` : Single player scenario missions listed under the Stand Alone Missions menu. Mission file extension is `.sce`.

- `Multiplayer Scenario` : Custom scenarios that can be played in single player, hot-seat, and in networked multiplayer modes. These missions are listed in the Custom Scenarios, Hot Seat Scenarios, and (Multiplayer) Scenarios menus. Mission file extension is `.mps`.

#### Mission Field

The `mission` field MUST be set for all non pseudo categories. If the field is defined the field value MUST be a string that represents a relative file system path. The following file system paths are used as base path prefixes in the given order to search for the specified file: `FilePathGameData`, `FilePathGameBase`, `FilePathGamePref`.

Examples:

```json
"mission": "SAVE1.TRA",
"mission": "mission1.tra",
"mission": "scenario_pack/scenario1.sce",
"mission": "campaign2/mission1/mission1.cam",
"mission": "./demo_100.dmo",
```

The JSON schema does not validate whether the referenced file is a supported save file format, but it does verify that the path exists and the file is a normal file system file instead of a symbolic link or similar. The application may or may not support multiple save file format versions. M.A.X. v1.04 uses binary save file format version V70. M.A.X. Port introduces save file format version V71 or newer versions to overcome design limitations and to fix defects.

Save file format version V71 embeds the JSON Mission Descriptor file contents into save files themselves, but overrides the `category` field. This means that when the user saves a game that produces a `SAVEnnn.xxx` save file it embeds the mission goals, victory conditions, game rules and so forth that were loaded from a related JSON mission descriptor at the time of game creation. If the JSON mission descriptor changes later, those changes are not applicable to existing game save files, a new game needs to be started for that to happen.

Mapping of file extensions:

| Mission Category | Mission File Extension | Save File Extension |
|-------|--------|--------|
| `Custom` | N/A | `.dta` |
| `Training` | `.tra` | `.dta` |
| `Campaign` | `.cam` | `.dta` |
| `Hot-seat` | N/A | `.hot` |
| `Multiplayer` | N/A | `.mlt` |
| `Demo` | N/A | N/A |
| `Single Player Scenario` | `.sce` | `.dta` |
| `Multiplayer Scenario` | `.mps` | `.dta`<br>`.hot`<br>`.mlt` |

#### Hash Field

The hash value is calculated over the complete file. Various retail M.A.X. CD-ROMs have different mission revisions. To support all of them the hash field is an array. The `hash` field is an array of SHA256 digests, each a 64-character lowercase hexadecimal string. These are used to verify the integrity of mission files. SHA256 is a cryptographic hash function [\[8\]](#ref8).

Example:

```json
"hash": [
	"a5104619925d10fa1c6a361ffee06a3a8d1c3f93a42d4540d4c5edf7b23673c3",
	"1d9da84128776d1bc264f26f1381d19c16dead2c0af9bba9dfe9433853a8f00e"
],
```

#### Title Field

The title field describes the mission title that is shown in mission selection lists and in-game within the mission goals panel. If the mission title is too long for the GUI to show it, the string gets truncated by the application. The `title` field has a mandatory subfield called `text` which holds one or more language tag fields. The `en-US` language tag field key is mandatory. Other language tag fields are optional. If the application sets the language tag to a value that is not available in a JSON mission descriptor, then the mandatory `en-US` field value is used.

#### Description Field

The description field describes the mission objectives and gives a short briefing. The description is shown next to mission selection lists and in-game within the mission goals panel. The mission description text can be scrolled. The `description` field has a mandatory subfield called `text` which holds one or more language tag fields. The `en-US` language tag field key is mandatory. Other language tag fields are optional. If the application sets the language tag to a value that is not available in a JSON mission descriptor, then the mandatory `en-US` field value is used.

The typical structure of mission descriptions is as follows:

```
Objective: <short summary of victory conditions>\n\n
Planet: <name or title of planet>\n
Clan: <name of clan or faction>\n\n
Notes: <Short mission briefing. Explain what is going on.>\n\n
```

#### Intro/Victory/Defeat Fields

These fields are optional narrative elements that are most useful in case of story driven campaigns. These scenes use render a full screen background image and print the `text` field value using a typewriter effect in front. Music is played in the background.

The `intro` field can be used for detailed mission briefing or story telling, the scene is shown before the mission starts.

The `victory` field can be used for mission debriefing in case of victory. The scene is not shown if the player lost the mission.

The `defeat` field can be used for mission debriefing in case of defeat. The scene is not shown if the player won the mission.

Each narrative element must define a `text` subfield which holds one or more language tag fields. The `en-US` language tag field key is mandatory. Narrative elements can hold optional media subfields called `background`, `music`, and `video`. If defined, they must contain a `resource` subfield.

```json
"<media type>": {
	"copyright": "...",
	"license": "...",
	"resource": {
		"id": [
			...
		],
		"file": [
			...
		]
	}
```

The `resource` subfields define assets to be used by the media fields of narrative elements. Either the `id` or `file` array subfields can be defined, not both.

```json
"resource": {
	"id": [
		"RESOURCEn",
		"...",
		"RESOURCEm"
	]
}
```

```json
"resource": {
	"file": [
		"./MUSICn.WAV",
		"...",
		"./MUSICm.WAV"
	]
}
```

If multiple `background` media type assets are defined, one is selected randomly. If no `background` is defined a random one is used automatically from the default set. If multiple `music` media type assets are defined, one is selected randomly. If no `music` is defined, then music that was playing at the time of loading the scene is left uninterrupted. After the scene the optional animations configured by the `video` field are played in the order they are defined. Video animations interrupt, reset music playback.

#### Victory Conditions Field

The `winloss_conditions` field is optional. The field value is a string that represents a Lua script. It is highly recommended to minify the Lua script.

#### Game Rules Field

The `game_rules` field is optional. The field value is a string that represents a Lua script. It is highly recommended to minify the Lua script.

#### Game event system Field

The `game_events` field is optional. The field value is a string that represents a Lua script. It is highly recommended to minify the Lua script.

#### Music player system Field

The `game_music` field is optional. The field value is a string that represents a Lua script. It is highly recommended to minify the Lua script.

### JSON Mission Schema File

<details>
<summary>Click to Open/Close JSON Schema file</summary>
{% highlight json %}
{% include_relative assets/files/mission.schema.json %}
{% endhighlight %}
</details>
<br>

### JSON Campaign Schema File

Not implemented yet.

### JSON World Schema File

Not implemented yet.

## API Reference

### Tables

#### MAX_UNIT (enumeration)

| Key | Value | Class | Description |
|-------|--------|--------|--------|
| COMMTWR  | 0x00 | STATIONARY | Gold Refinery |
| POWERSTN | 0x01 | STATIONARY | Power Station |
| POWGEN   | 0x02 | STATIONARY | Power Generator |
| BARRACKS | 0x03 | STATIONARY | Barracks |
| SHIELDGN | 0x04 | STATIONARY | Alien Derelict Building |
| RADAR    | 0x05 | STATIONARY | Radar |
| ADUMP    | 0x06 | STATIONARY | Storage Unit |
| FDUMP    | 0x07 | STATIONARY | Fuel Tank |
| GOLDSM   | 0x08 | STATIONARY | Gold Vault |
| DEPOT    | 0x09 | STATIONARY | Depot |
| HANGAR   | 0x0A | STATIONARY | Hangar |
| DOCK     | 0x0B | STATIONARY | Dock |
| CNCT_4W  | 0x0C | STATIONARY | Connector |
| LRGRUBLE | 0x0D | STATIONARY | Large Rubble |
| SMLRUBLE | 0x0E | STATIONARY | Small Rubble |
| LRGTAPE  | 0x0F | STATIONARY | Large Construction Tape |
| SMLTAPE  | 0x10 | STATIONARY | Small Construction Tape |
| LRGSLAB  | 0x11 | STATIONARY | Large Slab |
| SMLSLAB  | 0x12 | STATIONARY | Small Slab |
| LRGCONES | 0x13 | STATIONARY | Large Cones |
| SMLCONES | 0x14 | STATIONARY | Small Cones |
| ROAD     | 0x15 | STATIONARY | Road |
| LANDPAD  | 0x16 | STATIONARY | Landing Pad |
| SHIPYARD | 0x17 | STATIONARY | Shipyard |
| LIGHTPLT | 0x18 | STATIONARY | Light Vehicle Plant |
| LANDPLT  | 0x19 | STATIONARY | Heavy Vehicle Plant |
| SUPRTPLT | 0x1A | STATIONARY | Alien Derelict Building |
| AIRPLT   | 0x1B | STATIONARY | Air Units Plant |
| HABITAT  | 0x1C | STATIONARY | Habitat |
| RESEARCH | 0x1D | STATIONARY | Research Center |
| GREENHSE | 0x1E | STATIONARY | Eco-sphere |
| RECCENTR | 0x1F | STATIONARY | Alien Derelict Building |
| TRAINHAL | 0x20 | STATIONARY | Training Hall |
| WTRPLTFM | 0x21 | STATIONARY | Water Platform |
| GUNTURRT | 0x22 | STATIONARY | Gun Turret |
| ANTIAIR  | 0x23 | STATIONARY | Anti Aircraft |
| ARTYTRRT | 0x24 | STATIONARY | Artillery |
| ANTIMSSL | 0x25 | STATIONARY | Missile Launcher |
| BLOCK    | 0x26 | STATIONARY | Concrete Block |
| BRIDGE   | 0x27 | STATIONARY | Bridge |
| MININGST | 0x28 | STATIONARY | Mining Station |
| LANDMINE | 0x29 | STATIONARY | Land Mine |
| SEAMINE  | 0x2A | STATIONARY | Sea Mine |
| LNDEXPLD | 0x2B | STATIONARY | Land Explosion |
| AIREXPLD | 0x2C | STATIONARY | Air Explosion |
| SEAEXPLD | 0x2D | STATIONARY | Sea Explosion |
| BLDEXPLD | 0x2E | STATIONARY | Building Explosion |
| HITEXPLD | 0x2F | STATIONARY | Hit Explosion |
| MASTER   | 0x30 | MOBILE_LAND_SEA | Master Builder |
| CONSTRCT | 0x31 | MOBILE_LAND_SEA | Constructor |
| SCOUT    | 0x32 | MOBILE_LAND_SEA | Scout |
| TANK     | 0x33 | MOBILE_LAND_SEA | Tank |
| ARTILLRY | 0x34 | MOBILE_LAND_SEA | Assault Gun |
| ROCKTLCH | 0x35 | MOBILE_LAND_SEA | Rocket Launcher |
| MISSLLCH | 0x36 | MOBILE_LAND_SEA | Missile Crawler |
| SP_FLAK  | 0x37 | MOBILE_LAND_SEA | Mobile Anti Aircraft |
| MINELAYR | 0x38 | MOBILE_LAND_SEA | Mine Layer |
| SURVEYOR | 0x39 | MOBILE_LAND_SEA | Surveyor |
| SCANNER  | 0x3A | MOBILE_LAND_SEA | Scanner |
| SPLYTRCK | 0x3B | MOBILE_LAND_SEA | Supply Truck |
| GOLDTRCK | 0x3C | MOBILE_LAND_SEA | Gold Truck |
| ENGINEER | 0x3D | MOBILE_LAND_SEA | Engineer |
| BULLDOZR | 0x3E | MOBILE_LAND_SEA | Bulldozer |
| REPAIR   | 0x3F | MOBILE_LAND_SEA | Repair Unit |
| FUELTRCK | 0x40 | MOBILE_LAND_SEA | Fuel Truck |
| CLNTRANS | 0x41 | MOBILE_LAND_SEA | Personnel Carrier |
| COMMANDO | 0x42 | MOBILE_LAND_SEA | Infiltrator |
| INFANTRY | 0x43 | MOBILE_LAND_SEA | Infantry |
| FASTBOAT | 0x44 | MOBILE_LAND_SEA | Escort |
| CORVETTE | 0x45 | MOBILE_LAND_SEA | Corvette |
| BATTLSHP | 0x46 | MOBILE_LAND_SEA | Gunboat |
| SUBMARNE | 0x47 | MOBILE_LAND_SEA | Submarine |
| SEATRANS | 0x48 | MOBILE_LAND_SEA | Sea Transport |
| MSSLBOAT | 0x49 | MOBILE_LAND_SEA | Missile Cruiser |
| SEAMNLYR | 0x4A | MOBILE_LAND_SEA | Sea Mine Layer |
| CARGOSHP | 0x4B | MOBILE_LAND_SEA | Cargo Ship |
| FIGHTER  | 0x4C | MOBILE_AIR | Fighter |
| BOMBER   | 0x4D | MOBILE_AIR | Ground Attack Plane |
| AIRTRANS | 0x4E | MOBILE_AIR | Air Transport |
| AWAC     | 0x4F | MOBILE_AIR | AWAC |
| JUGGRNT  | 0x50 | MOBILE_LAND_SEA | Alien Gunboat |
| ALNTANK  | 0x51 | MOBILE_LAND_SEA | Alien Tank |
| ALNASGUN | 0x52 | MOBILE_LAND_SEA | Alien Assault Gun |
| ALNPLANE | 0x53 | MOBILE_AIR | Alien Attack Plane |
| ROCKET   | 0x54 | MISSILE | Rocket |
| TORPEDO  | 0x55 | MISSILE | Torpedo |
| ALNMISSL | 0x56 | MISSILE | Alien Missile |
| ALNTBALL | 0x57 | MISSILE | Alien Tank Ball |
| ALNABALL | 0x58 | MISSILE | Alien Assault Gun Ball |
| RKTSMOKE | 0x59 | MISSILE | Rocket Smoke |
| TRPBUBLE | 0x5A | MISSILE | Torpedo Bubble |
| HARVSTER | 0x5B | STATIONARY | Harvester |
| WALDO    | 0x5C | STATIONARY | Dead Waldo |

#### MAX_VOICE (enumeration)

| Key | Value | Category | Priority | Sample | Length |
|-------|--------|--------|--------|--------|--------|
| V_START  | 0x38F | N/A | 0 |  |  |
| V_M001   | 0x390 | 1 | 10 |  |  |
| V_F001   | 0x391 | 1 | 10 | Ready | 0s 539ms |
| V_M005   | 0x392 | 1 | 10 |  |  |
| V_F005   | 0x393 | 1 | 10 | Standing by | 0s 998ms |
| V_M006   | 0x394 | 1 | 10 |  |  |
| V_F006   | 0x395 | 1 | 10 | Awaiting orders | 1s 277ms |
| V_M004   | 0x396 | 1 | 10 |  |  |
| V_F004   | 0x397 | 1 | 10 | All systems go | 1s 219ms |
| V_M284   | 0x398 | 1 | 20 |  |  |
| V_F284   | 0x399 | 1 | 20 | Building | 0s 528ms |
| V_M138   | 0x39A | 1 | 20 |  |  |
| V_F138   | 0x39B | 1 | 20 | Ammunition is low | 1s 491ms |
| V_M142   | 0x39C | 1 | 35 |  |  |
| V_F142   | 0x39D | 1 | 35 | Ammo depleted | 1s 288ms |
| V_M145   | 0x39E | 1 | 30 |  |  |
| V_F145   | 0x39F | 1 | 30 | Movement exhausted | 1s 503ms |
| V_M150   | 0x3A0 | 1 | 40 |  |  |
| V_F150   | 0x3A1 | 1 | 40 | Status yellow | 1s 224ms |
| V_M151   | 0x3A2 | 1 | 40 |  |  |
| V_F151   | 0x3A3 | 1 | 40 | Status caution | 1s 416ms |
| V_M265   | 0x3A4 | 1 | 0 |  |  |
| V_F265   | 0x3A5 | 1 | 0 | Requesting repairs | 1s 282ms |
| V_M154   | 0x3A6 | 1 | 45 |  |  |
| V_F154   | 0x3A7 | 1 | 45 | Status red | 1s 230ms |
| V_M155   | 0x3A8 | 1 | 45 |  |  |
| V_F155   | 0x3A9 | 1 | 45 | Status critical | 1s 358ms |
| V_M158   | 0x3AA | 1 | 10 |  |  |
| V_F158   | 0x3AB | 1 | 10 | On sentry | 0s 957ms |
| V_M162   | 0x3AC | 1 | 50 |  |  |
| V_F162   | 0x3AD | 1 | 50 | Construction complete | 1s 503ms |
| V_M164   | 0x3AE | 1 | 50 |  |  |
| V_F164   | 0x3AF | 1 | 50 | Complete and ready | 1s 236ms |
| V_M163   | 0x3B0 | 1 | 50 |  |  |
| V_F163   | 0x3B1 | 1 | 50 | Ready for operation | 1s 433ms |
| V_M165   | 0x3B2 | 1 | 50 |  |  |
| V_F165   | 0x3B3 | 1 | 50 | Building finished | 1s 178ms |
| V_M166   | 0x3B4 | 1 | 50 |  |  |
| V_F166   | 0x3B5 | 1 | 50 | Unit completed | 1s 317ms |
| V_M169   | 0x3B6 | 1 | 50 |  |  |
| V_F169   | 0x3B7 | 1 | 50 | Unit finished | 1s 073ms |
| V_M171   | 0x3B8 | 1 | 10 |  |  |
| V_F171   | 0x3B9 | 1 | 10 | Clearing area | 1s 108ms |
| V_M181   | 0x3BA | 1 | 10 |  |  |
| V_F181   | 0x3BB | 1 | 10 | Select Site | 1s 190ms |
| V_M182   | 0x3BC | 1 | 10 |  |  |
| V_F182   | 0x3BD | 1 | 10 | Dropping mines | 1s 160ms |
| V_M186   | 0x3BE | 1 | 10 |  |  |
| V_F186   | 0x3BF | 1 | 10 | Removing mines | 1s 230ms |
| V_M187   | 0x3C0 | 1 | 10 |  |  |
| V_F187   | 0x3C1 | 1 | 10 | Picking up mines | 1s 253ms |
| V_M191   | 0x3C2 | 1 | 10 |  |  |
| V_F191   | 0x3C3 | 1 | 10 | Surveying | 0s 859ms |
| V_M192   | 0x3C4 | 1 | 10 |  |  |
| V_F192   | 0x3C5 | 1 | 10 | Searching | 0s 754ms |
| V_M196   | 0x3C6 | 1 | 50 |  |  |
| V_F196   | 0x3C7 | 1 | 50 | Attacking | 0s 754ms |
| V_M198   | 0x3C8 | 1 | 50 |  |  |
| V_F198   | 0x3C9 | 1 | 50 | Engaging enemy | 1s 404ms |
| V_START2 | 0x3CA | N/A | 0 |  |  |
| V_M007   | 0x3CB | 2 | 50 |  |  |
| V_F007   | 0x3CC | 2 | 50 | Attempt failed | 1s 271ms |
| V_M010   | 0x3CD | 2 | 50 |  |  |
| V_F010   | 0x3CE | 2 | 50 | Mission failed | 1s 172ms |
| V_M012   | 0x3CF | 2 | 50 |  |  |
| V_F012   | 0x3D0 | 2 | 50 | Infiltration unsuccessful | 2s 118ms |
| V_M239   | 0x3D1 | 2 | 40 |  |  |
| V_F239   | 0x3D2 | 2 | 40 | Unit captured | 1s 033ms |
| V_M242   | 0x3D3 | 2 | 40 |  |  |
| V_F242   | 0x3D4 | 2 | 40 | Unit taken | 0s 986ms |
| V_M243   | 0x3D5 | 2 | 40 |  |  |
| V_F243   | 0x3D6 | 2 | 40 | Units have been seized by the enemy | 2s 095ms |
| V_M244   | 0x3D7 | 2 | 40 |  |  |
| V_F244   | 0x3D8 | 2 | 40 | Unit disabled | 1s 155ms |
| V_M247   | 0x3D9 | 2 | 40 |  |  |
| V_F247   | 0x3DA | 2 | 40 | Unit must be repaired | 1s 497ms |
| V_M249   | 0x3DB | 2 | 40 |  |  |
| V_F249   | 0x3DC | 2 | 40 | Unit not functioning | 1s 451ms |
| V_START3 | 0x3DD | N/A | 0 |  |  |
| V_M049   | 0x3DE | 3 | 0 |  |  |
| V_F049   | 0x3DF | 3 | 0 | Position tape and click inside it to begin building | 3s 361ms |
| V_M050   | 0x3E0 | 3 | 0 |  |  |
| V_F050   | 0x3E1 | 3 | 0 | Move the tape and click | 1s 654ms |
| V_M085   | 0x3E2 | 3 | 0 |  |  |
| V_F085   | 0x3E3 | 3 | 0 | Reloaded | 0s 783ms |
| V_M089   | 0x3E4 | 3 | 0 |  |  |
| V_F089   | 0x3E5 | 3 | 0 | General reload successful | 1s 898ms |
| V_M094   | 0x3E6 | 3 | 0 |  |  |
| V_F094   | 0x3E7 | 3 | 0 | No path to destination | 1s 811ms |
| V_M095   | 0x3E8 | 3 | 0 |  |  |
| V_F095   | 0x3E9 | 3 | 0 | Path blocked | 1s 004ms |
| V_M201   | 0x3EA | 3 | 50 |  |  |
| V_F201   | 0x3EB | 3 | 50 | Submarine detected | 1s 532ms |
| V_M210   | 0x3EC | 3 | 0 |  |  |
| V_F210   | 0x3ED | 3 | 0 | Units repaired | 1s 190ms |
| V_M211   | 0x3EE | 3 | 0 |  |  |
| V_F211   | 0x3EF | 3 | 0 | Repairs complete | 1s 416ms |
| V_M219   | 0x3F0 | 3 | 0 |  |  |
| V_F219   | 0x3F1 | 3 | 0 | Unit repaired | 1s 282ms |
| V_M220   | 0x3F2 | 3 | 0 |  |  |
| V_F220   | 0x3F3 | 3 | 0 | Repaired | 0s 737ms |
| V_M224   | 0x3F4 | 3 | 0 |  |  |
| V_F224   | 0x3F5 | 3 | 0 | Transfer complete | 1s 306ms |
| V_M229   | 0x3F6 | 3 | 0 |  |  |
| V_F229   | 0x3F7 | 3 | 0 | Unit under attack | 1s 219ms |
| V_M230   | 0x3F8 | 3 | 0 |  |  |
| V_F230   | 0x3F9 | 3 | 0 | Enemy firing | 1s 102ms |
| V_M231   | 0x3FA | 3 | 0 |  |  |
| V_F231   | 0x3FB | 3 | 0 | Incoming | 0s 748ms |
| V_M232   | 0x3FC | 3 | 0 |  |  |
| V_F232   | 0x3FD | 3 | 0 | We are under attack | 1s 160ms |
| V_M255   | 0x3FE | 3 | 0 |  |  |
| V_F255   | 0x3FF | 3 | 0 | Unit under attack | 1s 184ms |
| V_M256   | 0x400 | 3 | 0 |  |  |
| V_F256   | 0x401 | 3 | 0 | Enemy firing on unit | 1s 567ms |
| V_M234   | 0x402 | 3 | 40 |  |  |
| V_F234   | 0x403 | 3 | 40 | Unit destroyed | 1s 131ms |
| V_M236   | 0x404 | 3 | 40 |  |  |
| V_F236   | 0x405 | 3 | 40 | Unit is gone | 1s 102ms |
| V_M250   | 0x406 | 3 | 0 |  |  |
| V_F250   | 0x407 | 3 | 0 | Unit firing | 1s 155ms |
| V_M251   | 0x408 | 3 | 0 |  |  |
| V_F251   | 0x409 | 3 | 0 | Unit engaging target | 1s 561ms |
| V_M070   | 0x40A | 3 | 30 |  |  |
| V_F070   | 0x40B | 3 | 30 | Enemy detected | 1s 282ms |
| V_M071   | 0x40C | 3 | 30 |  |  |
| V_F071   | 0x40D | 3 | 30 | Enemy spotted | 1s 085ms |
| V_START4 | 0x40E | N/A | 0 |  |  |
| V_M270   | 0x40F | 4 | 5 |  |  |
| V_F270   | 0x410 | 4 | 5 | Ammunitions low | 1s 358ms |
| V_M271   | 0x411 | 4 | 5 |  |  |
| V_F271   | 0x412 | 4 | 5 | Ammo is low | 1s 155ms |
| V_M279   | 0x413 | 4 | 60 |  |  |
| V_F279   | 0x414 | 4 | 60 | Red team has ended turn | 2s 031ms |
| V_M280   | 0x415 | 4 | 60 |  |  |
| V_F280   | 0x416 | 4 | 60 | Green team has ended turn | 2s 107ms |
| V_M281   | 0x417 | 4 | 60 |  |  |
| V_F281   | 0x418 | 4 | 60 | Blue team has ended turn | 2s 043ms |
| V_M282   | 0x419 | 4 | 60 |  |  |
| V_F282   | 0x41A | 4 | 60 | Gray team has ended turn | 2s 159ms |
| V_M025   | 0x41B | 4 | 60 |  |  |
| V_F025   | 0x41C | 4 | 60 | Red team has left the planet | 2s 124ms |
| V_M026   | 0x41D | 4 | 60 |  |  |
| V_F026   | 0x41E | 4 | 60 | Red team has abandoned the planet | 2s 409ms |
| V_M031   | 0x41F | 4 | 60 |  |  |
| V_F031   | 0x420 | 4 | 60 | Green team has left the planet | 2s 014ms |
| V_M032   | 0x421 | 4 | 60 |  |  |
| V_F032   | 0x422 | 4 | 60 | Green team has abandoned the planet | 2s 281ms |
| V_M037   | 0x423 | 4 | 60 |  |  |
| V_F037   | 0x424 | 4 | 60 | Blue team has left the planet | 2s 031ms |
| V_M038   | 0x425 | 4 | 60 |  |  |
| V_F038   | 0x426 | 4 | 60 | Blue team has abandoned the planet | 2s 200ms |
| V_M043   | 0x427 | 4 | 60 |  |  |
| V_F043   | 0x428 | 4 | 60 | Gray team has left the planet | 1s 973ms |
| V_M044   | 0x429 | 4 | 60 |  |  |
| V_F044   | 0x42A | 4 | 60 | Grey team has abandoned the planet | 2s 275ms |
| V_M053   | 0x42B | 4 | 0 |  |  |
| V_F053   | 0x42C | 4 | 0 | Begin | 0s 667ms |
| V_M057   | 0x42D | 4 | 5 |  |  |
| V_F057   | 0x42E | 4 | 5 | Fuel storage is full | 1s 451ms |
| V_M061   | 0x42F | 4 | 5 |  |  |
| V_F061   | 0x430 | 4 | 5 | Raw material storage is full | 2s 176ms |
| V_M066   | 0x431 | 4 | 5 |  |  |
| V_F066   | 0x432 | 4 | 5 | Gold storage is full | 1s 642ms |
| V_M075   | 0x433 | 4 | 0 |  |  |
| V_F075   | 0x434 | 4 | 0 | Waiting for remote end turn | 1s 805ms |
| V_M080   | 0x435 | 4 | 0 |  |  |
| V_F080   | 0x436 | 4 | 0 | Waiting for opponent to finish turn | 2s 380ms |
| V_M081   | 0x437 | 4 | 0 |  |  |
| V_F081   | 0x438 | 4 | 0 | Waiting for AI to finish turn | 2s 258ms |
| V_M093   | 0x439 | 4 | 10 |  |  |
| V_F093   | 0x43A | 4 | 10 | Research complete | 1s 433ms |
| V_M098   | 0x43B | 4 | 15 |  |  |
| V_F098   | 0x43C | 4 | 15 | Red team colony started | 1s 816ms |
| V_M103   | 0x43D | 4 | 15 |  |  |
| V_F103   | 0x43E | 4 | 15 | Green team colony started | 2s 008ms |
| V_M108   | 0x43F | 4 | 15 |  |  |
| V_F108   | 0x440 | 4 | 15 | Blue team colony started | 1s 753ms |
| V_M113   | 0x441 | 4 | 15 |  |  |
| V_F113   | 0x442 | 4 | 15 | Gray team colony started | 1s 950ms |
| V_M118   | 0x443 | 4 | 15 |  |  |
| V_F118   | 0x444 | 4 | 15 | Red team has the most eco-spheres. | 2s 554ms |
| V_M122   | 0x445 | 4 | 15 |  |  |
| V_F122   | 0x446 | 4 | 15 | Green team has the most eco-spheres. | 2s 530ms |
| V_M126   | 0x447 | 4 | 15 |  |  |
| V_F126   | 0x448 | 4 | 15 | Blue team has the most eco-spheres. | 2s 467ms |
| V_M130   | 0x449 | 4 | 15 |  |  |
| V_F130   | 0x44A | 4 | 15 | Gray team has the most eco-spheres. | 2s 368ms |
| V_M206   | 0x44B | 4 | 5 |  |  |
| V_F206   | 0x44C | 4 | 5 | New units available this turn | 2s 159ms |
| V_M207   | 0x44D | 4 | 5 |  |  |
| V_F207   | 0x44E | 4 | 5 | New units | 1s 079ms |
| V_M215   | 0x44F | 4 | 5 |  |  |
| V_F215   | 0x450 | 4 | 5 | New unit available this turn | 2s 124ms |
| V_M216   | 0x451 | 4 | 5 |  |  |
| V_F216   | 0x452 | 4 | 5 | New unit | 0s 859ms |
| V_M217   | 0x453 | 4 | 5 |  |  |
| V_F217   | 0x454 | 4 | 5 | Unit ready | 1s 021ms |
| V_START5 | 0x455 | N/A | 0 |  |  |
| V_M272   | 0x456 | 5 | 0 |  |  |
| V_F272   | 0x457 | 5 | 0 | 20 seconds left | 1s 282ms |
| V_M273   | 0x458 | 5 | 0 |  |  |
| V_F273   | 0x459 | 5 | 0 | 20 seconds to end turn | 1s 816ms |
| V_M275   | 0x45A | 5 | 0 |  |  |
| V_F275   | 0x45B | 5 | 0 | We are about to run out of time | 1s 753ms |
| V_M276   | 0x45C | 5 | 0 |  |  |
| V_F276   | 0x45D | 5 | 0 | Credit available | 1s 131ms |
| V_M278   | 0x45E | 5 | 0 |  |  |
| V_F278   | 0x45F | 5 | 0 | Select landing site | 1s 381ms |
| V_M176   | 0x460 | 5 | 0 |  |  |
| V_F176   | 0x461 | 5 | 0 | Select site | 1s 248ms |
| V_M177   | 0x462 | 5 | 0 |  |  |
| V_F177   | 0x463 | 5 | 0 | Choose site | 0s 975ms |
| V_M283   | 0x464 | 5 | 0 |  |  |
| V_F283   | 0x465 | 5 | 0 | Mission successful | 1s 340ms |
| V_M013   | 0x466 | 5 | 0 |  |  |
| V_F013   | 0x467 | 5 | 0 | Games saved | 1s 039ms |
| V_END    | 0x468 | N/A | 0 |  |  |

#### MAX_MUSIC (enumeration)

| Key | Value | Description |
|-------|--------|--------|
SNOW_MSC | 0x6A1 | Snow world theme |
CRTR_MSC | 0x6A2 | Crater world theme |
GREN_MSC | 0x6A3 | Green world theme |
DSRT_MSC | 0x6A4 | Desert world theme |
MAIN_MSC | 0x6A5 | Main menu theme |
BKG1_MSC | 0x6A6 | Background track 1 |
BKG2_MSC | 0x6A7 | Background track 2 |
BKG3_MSC | 0x6A8 | Background track 3 |
BKG4_MSC | 0x6A9 | Background track 4 |
BKG5_MSC | 0x6AA | Background track 5 |
BKG6_MSC | 0x6AB | Background track 6 |
BKG7_MSC | 0x6AC | Background track 7 |
BKG8_MSC | 0x6AD | Background track 8 |
BKG9_MSC | 0x6AE | Background track 9 |
CRGO_MSC | 0x6AF | N/A |
CRDT_MSC | 0x6B0 | Credits theme |
WINR_MSC | 0x6B1 | Victory theme |
LOSE_MSC | 0x6B2 | Defeat theme |

#### MAX_IMAGE (enumeration)

| Key | Value | Description |
|-------|--------|--------|
ENDGAME1 | 0x6D6 | Sea fleet |
ENDGAME2 | 0x6D7 | Power station |
ENDGAME3 | 0x6D8 | Scout horde |
ENDGAME4 | 0x6D9 | Human platoon |
ENDGAME5 | 0x6DA | Air fleet with transporter |
ENDGAME6 | 0x6DB | Base dive bombing |
ENDGAME7 | 0x6DC | Air fleet with fighter |
ENDGAME8 | 0x6DD | Master builder |
ENDGAME9 | 0x6DE | Human squad |
ENDGAM10 | 0x6DF | N/A |
ENDGAM11 | 0x6E0 | N/A |
ENDGAM12 | 0x6E1 | N/A |
ENDGAM13 | 0x6E2 | N/A |
ENDGAM14 | 0x6E3 | N/A |

#### MAX_ICON (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| LIPS | 0x295 | Lips icon in messages view of reports menu |
| I_CMPLX | 0x296 | Complex icon in messages view of reports menu |

#### MAX_TEAM (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| RED | 0 | Red team index |
| GREEN | 1 | Green team index |
| BLUE | 2 | Blue team index |
| GRAY | 3 | Gray team index |
| DERELICT | 4 | Alien derelicts team index |

#### MAX_CARGO_TYPE (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| RAW | 1 | Raw materials resource index |
| FUEL | 2 | Fuel resource index |
| GOLD | 3 | Gold resource index |

#### MAX_UNIT_CLASS (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| MISSILE | 0x0020 | Projectile unit mask |
| MOBILE_LAND_SEA | 0x0180| Mobile land and sea unit mask |
| MOBILE_AIR | 0x0040| Mobile air unit mask |
| STATIONARY | 0x0200| Stationary unit mask |

#### MAX_UNIT_ATTRIB (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| ATTACK | 0 | Attack power unit attribute |
| SHOTS | 1 | Attack count per turn unit attribute |
| RANGE | 2 | Attack range unit attribute |
| ARMOR | 3 | Armor unit attribute |
| HITS | 4 | Hit points unit attribute |
| SPEED | 5 | Movement points unit attribute |
| SCAN | 6 | Scan range unit attribute |
| COST | 7 | Construction cost unit attribute |
| AMMO | 8 | Weapon ammunition unit attribute |
| MOVE_AND_FIRE | 9 | Can move without spending attack count per turn points unit attribute |
| STORAGE | 11 | Storage capacity unit attribute |
| ATTACK_RADIUS | 12 | Area attack radius unit attribute |
| AGENT_ADJUST | 13 | Infiltrator experience level |

#### MAX_RESEARCH_TOPIC (enumeration)

| Key | Value | Description |
|-------|--------|--------|
| ATTACK | 0 | Attack power research topic |
| SHOTS | 1 | Attack count per turn research topic |
| RANGE | 2 | Attack range research topic |
| ARMOR | 3 | Armor research topic |
| HITS | 4 | Hit points research topic |
| SPEED | 5 | Movement points research topic |
| SCAN | 6 | Scan range research topic |
| COST | 7 | Construction cost research topic |

### Functions

#### Function: max_has_materials(MAX_TEAM team, MAX_CARGO_TYPE cargo_type) {#max_has_materials}
Calculate the total cargo amount the team has in all of its complexes and units.

- **Arguments:**
  - `MAX_TEAM` (integer): Team enum (e.g. `MAX_TEAM.RED`)
  - `MAX_CARGO_TYPE` (integer): Cargo type enum (e.g. `MAX_CARGO_TYPE.FUEL`)

- **Returns:**
  - `integer`: Total amount of specified cargo type for the team.

#### Function: max_count_ready_units(MAX_TEAM team, MAX_UNIT unit_type) {#max_count_ready_units}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Number of ready units of the given type for the team.

#### Function: max_has_attack_power(MAX_TEAM team, MAX_UNIT_CLASS unit_class) {#max_has_attack_power}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT_CLASS` (integer): Unit class enum (`MOBILE_LAND_SEA`, `MOBILE_AIR`, `STATIONARY`)

- **Returns:**
  - `boolean`: True if the team has attack power in the given class.

#### Function: max_can_rebuild_complex(MAX_TEAM team) {#max_can_rebuild_complex}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum

- **Returns:**
  - `boolean`: True if the team can rebuild a complex.

#### Function: max_can_rebuild_builders(MAX_TEAM team) {#max_can_rebuild_builders}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum

- **Returns:**
  - `boolean`: True if the team can rebuild builder units.

#### Function: max_count_total_mining(MAX_TEAM team) {#max_count_total_mining}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum

- **Returns:**
  - `integer, integer, integer`: Raw, fuel, and gold mining capacity.

#### Function: max_get_total_units_being_constructed(MAX_TEAM team, MAX_UNIT unit_type) {#max_get_total_units_being_constructed}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Number of units of the given type being constructed.

#### Function: max_get_total_power_consumption(MAX_TEAM team, MAX_UNIT unit_type) {#max_get_total_power_consumption}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Power consumption for the unit type.

#### Function: max_get_total_mining(MAX_TEAM team, MAX_CARGO_TYPE cargo_type) {#max_get_total_mining}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_CARGO_TYPE` (integer): Cargo type enum

- **Returns:**
  - `integer`: Total mining for the cargo type.

#### Function: max_get_total_consumption(MAX_TEAM team, MAX_CARGO_TYPE cargo_type) {#max_get_total_consumption}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_CARGO_TYPE` (integer): Cargo type enum

- **Returns:**
  - `integer`: Total consumption for the cargo type.

#### Function: max_has_unit_above_mark_level(MAX_TEAM team, MAX_UNIT unit_type, MARK_LEVEL mark) {#max_has_unit_above_mark_level}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum
  - `MARK_LEVEL` (integer): Mark level

- **Returns:**
  - `boolean`: True if the team has a unit above the specified mark level.

#### Function: max_has_unit_experience(MAX_TEAM team, MAX_UNIT unit_type) {#max_has_unit_experience}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Experience value for the unit type.

#### Function: max_count_casualties(MAX_TEAM team, MAX_UNIT unit_type) {#max_count_casualties}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Number of casualties for the unit type.

#### Function: max_count_units_above_attrib_level(MAX_TEAM team, MAX_UNIT unit_type, MAX_UNIT_ATTRIB attrib, ATTRIB_LEVEL level) {#max_count_units_above_attrib_level}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum
  - `MAX_UNIT_ATTRIB` (integer): Unit attrib enum
  - `ATTRIB_LEVEL` (integer): Unit attribute level

- **Returns:**
  - `integer`: Number of casualties for the unit type.

#### Function: max_count_units_with_reduced_attrib(MAX_TEAM team, MAX_UNIT unit_type, MAX_UNIT_ATTRIB attrib) {#max_count_units_with_reduced_attrib}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum
  - `MAX_UNIT_ATTRIB` (integer): Unit attrib enum

- **Returns:**
  - `integer`: Number of casualties for the unit type.

#### Function: max_get_research_level(MAX_TEAM team, MAX_RESEARCH_TOPIC topic) {#max_get_research_level}
- **Arguments:**
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**
  - `integer`: Number of casualties for the unit type.

#### Function: max_play_voice(MAX_VOICE start_id, MAX_VOICE end_id, NUMBER priority) {#max_play_voice}

There are five voice effect categories. Each voice effect may have alternative samples to add variety, to break monotony. These alternatives constitute a voice effect group. To play a voice effect the programming interface expects two resource IDs and a priority level.

```lua
-- schedule to play 'Red Team has left the planet.' with default or automatic priority level.
max_play_voice(MAX_VOICE.V_M025, MAX_VOICE.V_F025, 0)
```

A priority level of 0 means use the default priority level associated to the voice effect sample. Setting a positive priority level above 0 overrides the default priority level. The application itself does not override the default priority levels. A negative priority level means that the voice effect samples in the given range will be The categories do not represent priorities, priority levels. They make it possible to cancel a complete category of actively played and scheduled voice samples. possible to Voice effect samples do not play immediately, they are scheduled. It is not allowed to schedule the same voice effect group.are not played immediately, nor concurrently, they are scheduled sequentially in decreasing priority order.  The user is able to override the default priority levels.

- **Arguments:**  
  - `MAX_TEAM` (integer): Team enum
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**  
  - `integer`: Number of casualties for the unit type.

### Callback Functions

#### Callback: max_get_builder_type(MAX_UNIT unit_type) {#max_get_builder_type}
- **Arguments:**  
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**  
  - `MAX_UNIT`: Unit type enum.

#### Callback: max_get_buildable_units(MAX_UNIT unit_type) {#max_get_buildable_units}
- **Arguments:**  
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**  
  - `list`: List of Unit type enums or an empty list.

#### Callback: max_get_builder_type(MAX_UNIT unit_type) {#max_get_builder_type}
- **Arguments:**  
  - `MAX_UNIT` (integer): Unit type enum

- **Returns:**  
  - `boolean`: True if the unit type is buildable.

## References
<a name="ref1"></a>[1] [JSON Format Specification](https://www.json.org/)<br>
<a name="ref2"></a>[2] [Lua 5.4 Reference Manual](https://www.lua.org/manual/5.4/)<br>
<a name="ref3"></a>[3] [JSON Schema](https://json-schema.org/draft-07)<br>
<a name="ref4"></a>[4] [JSON Schema Validator](https://www.jsonschemavalidator.net/)<br>
<a name="ref5"></a>[5] [IETF BCP 47](https://tools.ietf.org/html/bcp47)<br>
<a name="ref6"></a>[6] [JSON Minifier](https://docs.python.org/3/library/json.html)<br>
<a name="ref7"></a>[7] [Lua Minifier](https://github.com/mathiasbynens/luamin)<br>
<a name="ref8"></a>[8] [SHA256](https://en.wikipedia.org/wiki/SHA-2)<br>
