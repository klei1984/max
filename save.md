---
layout: page
title: Save File Format
permalink: /save/
---

<h3 class="no-toc">Table of Contents</h3>

* TOC
{:toc}

## Preface

The article documents the binary game save file format of M.A.X. v1.04 (Save File Format Version V70).

## Overview

This technical information enables postmortem analysis of certain network game desynchronization issues.

During its development M.A.X. supported two save file formats. There is a human readable format, which is disabled by default, and there is the normal binary format. Unfortunately it seems that the human readable format was not fully maintained at later stages of development and the feature to pretty print enumerated types got stripped from release builds of the game.

With this information it also technically becomes possible to develop a scenario editor for the original game, but certain limitations apply.

Training missions, scenarios and even the campaign games are normal game save files. In theory this means that one can develop additional missions for the game. But unfortunately M.A.X. hardcodes mission goals, or victory and loss conditions, and thus the number of supported scenarios, campaign and training missions.

For example campaign game #2 is lost as soon as the player loses a research center or won as soon as the victory turns limit configured within the save file is reached. One can change the victory turns limit which is 30 turns currently, but there is no way to change the loss condition.

Without reimplementing and externalizing hardcoded missions specific aspects of the game many constraints apply to what kind of missions, scenarios an enthusiast could create in a hypothetical scenario editor.

Nevertheless the first step to do is the specification of the original binary game save file format.

## Save File Location

M.A.X. does not use a conventional folder structure. Basically every installed file is copied into a single installation folder. Assumably this is done to conserve RAM space.

By default the game is installed to `C:\INTRPLAY\MAX`, but under DOSBox or similar virtual machines this is not that interesting.

In short, game save files are located at the installation folder where the M.A.X. game executable resides.

A few trivial examples:<br>
`c:\Program Files (x86)\GOG Galaxy\Games\MAX\`<br>
`c:\Program Files (x86)\Steam\steamapps\common\M.A.X. Mechanized Assault & Exploration\max\`<br>
`c:\Program Files\Epic Games\MAX\max`

## Save File Format

The format specification is described as a set of strongly typed basic fields, member fields of structures or arrays that are laid out in a hierarchical manner. This format specification uses ISO C99 basic types wherever possible, but it is important to note that standard alignment rules do not apply, there are no implicit padding bytes or the like. All M.A.X. specific formats use little endian [\[1\]](#ref1) byte order.

The format contains conditional sections, subsections. The signedness of fields and properties are not 100% accurate yet.

ImHex pattern language ([profile]({{ site.baseurl }}/assets/files/max_v104_saveformat.7z)) for M.A.X. save file format v70. Tested with ImHex v1.26.2.

### Serialized Class Objects

The game implements its own class serializer. Each class gets a unique type index called `class_type`. Each emitted object gets an item index called `object_index`. The item index starts from 1 and regardless of the class being serialized the value is monotonically incremented by one. Pointers to serializable objects found within other serializable objects are not emitted in a special way. Instead of the pointers, the actual objects to where the pointer points gets serialized. This could potentially lead to circular references, but the game handles these corner cases elegantly. The item index that each emitted object gets is used to tell whether the object was already emitted once. If the object item index is bigger than the last emitted object's item index it means it is not yet saved into the save file so the full object gets dumped to the save file while if the object was already emitted based on its item index only its allocated item index gets written into the file.

The above concepts imply that the class serializer must register supported classes in a specific order so that their `class_type` field could be matched. This is achieved by putting classes into a map with sorted or ordered keys where the key matches the class name. But of course this also means that the algorithm saving the objects to a save file must be matched with the algorithm that is loading the objects from the save file. It is not a big deal, but the reimplementation of these algorithms should be fully accurate to be able to parse and reproduce the original save file format.

### File Header

~~~ c
enum FileType : unsigned char
{
  Custom,
  Tutorial,
  Campaign,
  Hot_seat,
  Multiplayer,
  Demo,
  Debug,
  Text,
  Scenario,
  Multi_scenario
};

enum PlanetType : unsigned char
{
  Snowcrab,
  Frigia,
  Ice_Berg,
  The_Cooler,
  Ultima_Thule,
  Long_Floes,
  Iron_Cross,
  Splatterscape,
  Peak_a_boo,
  Valentines_Planet,
  Three_Rings,
  Great_divide,
  New_Luzon,
  Middle_Sea,
  High_Impact,
  Sanctuary,
  Islandia,
  Hammerhead,
  Freckles,
  Sandspit,
  Great_Circle,
  Long_Passage,
  Flash_Point,
  Bottleneck
};

struct __attribute__((packed)) SaveFormatHeader
{
  unsigned short version;
  FileType save_game_type;
  char save_game_name[30];
  PlanetType planet;
  unsigned short mission_index;
  char team_name[4][30];
  TeamType team_type[5];
  TeamClan team_clan[5];
  unsigned int rng_seed;
  OpponentType opponent;
  unsigned short turn_timer_time;
  unsigned short endturn_time;
  PlayMode play_mode;
};
~~~

***version***: Version of the save file format. M.A.X. 1 does not support conversion between various formats.

| Publication Date* | Type of Publication | Game Version** | Save File Format Version |
|:-----:|:-----:|:-----:|:-----:|
| 1996-08-15 | M.A.X. 1 Interactive Demo    | V1.00 Demo  | V54 |
| 1996-08-21 | M.A.X. 1 Internal Test Build | V1.01 Demo  | V54 |
| 1996-10-09 | M.A.X. 1 Interactive Demo    | V1.01 Demo  | V64 |
| 1996-11-20 | M.A.X. 1 Internal Test Build | V1.56       | V69 |
| 1996-12-08 | M.A.X. 1 Retail (CD)         | V1.00       | V70 |
| 1996-12-10 | M.A.X. 1 Retail (CD)         | V1.01       | V70 |
| 1996-12-12 | M.A.X. 1 Retail (CD)         | V1.02       | V70 |
| 1997-01-08 | M.A.X. 1 Retail (patch)      | V1.03       | V70 |
| 1997-01-16 | M.A.X. 1 Interactive Demo    | V1.03a Demo | V70 |
| 1997-03-26 | M.A.X. 1 Retail (patch)      | V1.04       | **V70** |
| 1998-01-16 | M.A.X. 2 Alpha Demo          | V.091 Alpha | V101 |
| 1998-05-26 | M.A.X. 2 BETA Demo           | V.161 Beta  | V130 |
| 1998-06-17 | M.A.X. 2 Retail (CD)         | V1.0        | V135 |
| 1998-06-19 | M.A.X. 2 Retail (CD)         | V1.1        | V137 |
| 1998-06-29 | M.A.X. 2 Retail (CD)         | V1.2        | V137 |
| 1998-06-30 | M.A.X. 2 Demo                | V1.1c Demo  | V137 |
| 1998-07-10 | M.A.X. 2 Retail (patch)      | V1.3        | V137 |
| 1998-07-22 | M.A.X. 2 Retail (patch #2)   | V1.3        | V137 |
| 1998-09-17 | M.A.X. 2 Retail (patch)      | V1.40       | V137 |

\* The publication date is based on file system date and time stamps. Date format used is YYYY-MM-DD.
<br>
\*\* The version number is taken from the game executable.

***save_game_type***: Type of game save file.

| Type Index | Save File Type | File Extensions** |
|:-----:|:-----:|:-----:|
| 0 | Single player **Custom** game | dta |
| 1 | Tutorial or **Learning** game | tra |
| 2 | **Campaign** game | cam |
| 3 | **Hot seat** game | hot |
| 4 | **Multi** player game | mlt |
| 5 | **Demo** or attract mode game | dmo |
| 6 | **Debug** mode game save* | dbg |
| 7 | **Text** or human readable format save file | txt |
| 8 | Single player **Scenario** game | sce |
| 9 | Multi player scenario (**MPS**) game | mps |

\*  Only created during simultaneous multiplayer games if the *log_file_debug* feature is enabled.

\** The file extensions listed in this column do not necessarily match the file extensions given when players save their games. For example campaign games created by the developers have the *.cam file extension, while if players save their campaign games the actual game save files get the *.dta extension.

***save_game_name***: Null terminated string. Name given to the saved game by the player or the name of the scenario or mission visible in menus.

<a name="cross_ref01"></a>
***planet***: Planet index. There are four world types offered by M.A.X. [\[2\]](#ref2). Each world type has six planets to choose from.

| Planet Index | Planet Name | World Type |
|:-----:|:-----:|:-----:|
| 0 | Snowcrab | Frozen **Snow** world |
| 1 | Frigia | Frozen **Snow** world |
| 2 | Ice Berg | Frozen **Snow** world |
| 3 | The Cooler | Frozen **Snow** world |
| 4 | Ultima Thule | Frozen **Snow** world |
| 5 | Long Floes | Frozen **Snow** world |
| 6 | Iron Cross | Rocky **Crater** world |
| 7 | Splatterscape | Rocky **Crater** world |
| 8 | Peak-a-boo | Rocky **Crater** world |
| 9 | Valentine's Planet | Rocky **Crater** world |
| 10 | Three Rings | Rocky **Crater** world |
| 11 | Great divide | Rocky **Crater** world |
| 12 | New Luzon |  Lush tropical **Green** world |
| 13 | Middle Sea |  Lush tropical **Green** world |
| 14 | High Impact |  Lush tropical **Green** world |
| 15 | Sanctuary |  Lush tropical **Green** world |
| 16 | Islandia |  Lush tropical **Green** world |
| 17 | Hammerhead |  Lush tropical **Green** world |
| 18 | Freckles | Barren **Desert** world |
| 19 | Sandspit | Barren **Desert** world |
| 20 | Great Circle | Barren **Desert** world |
| 21 | Long Passage | Barren **Desert** world |
| 22 | Flash Point | Barren **Desert** world |
| 23 | Bottleneck | Barren **Desert** world |

***mission_index***: Index of the mission. This field determines the victory and loss conditions of training missions, campaign games and single player scenarios or stand alone missions. The rules are hardcoded by the game executable. Valid indices start from 1 as 0 means generic custom game rules apply. See *[victory conditions](#cross_ref02)* section.

***team_name***: Array of null terminated strings. Each player's name. Order of teams: red, green, blue, gray. Note that the size of the strings are limited to 30 bytes including the delimiter so special care must be taken for UTF-8 encoded grapheme clusters.

***team_type***: Array defining the type of players. In single player games only one *HUMAN_TEAM* can be defined, the player. In a self running demo game, attract mode of main menu, there are only *COMPUTER_TEAM*s. In a multi player game every opponent is a human *REMOTE_TEAM*. Terminated opponents are *ELIMINATED_TEAM*s.

<a name="cross_ref03"></a>
~~~ c
enum TeamType : unsigned char
{
  TEAM_TYPE_NONE = 0,
  TEAM_TYPE_HUMAN = 1,
  TEAM_TYPE_COMPUTER = 2,
  TEAM_TYPE_REMOTE = 3,
  TEAM_TYPE_ELIMINATED = 4
};
~~~

***team_clan***: Array defining clan of teams.

<a name="cross_ref04"></a>
~~~ c
enum TeamClan : unsigned char
{
  TEAM_CLAN_THE_CHOSEN = 1,
  TEAM_CLAN_CRIMSON_PATH = 2,
  TEAM_CLAN_VON_GRIFFIN = 3,
  TEAM_CLAN_AYERS_HAND = 4,
  TEAM_CLAN_MUSASHI = 5,
  TEAM_CLAN_SACRED_EIGHTS = 6,
  TEAM_CLAN_7_KNIGHTS = 7,
  TEAM_CLAN_AXIS_INC = 8
};
~~~

***rng_seed***: The game uses this as seed value for it's pseudo random number generator. Network games share the same *rng_seed* to be fully deterministic. The value is derived from the time() C-API function from \<time.h\>. The time function determines the current calendar time which represents the time since January 1, 1970 (UTC) also known as Unix epoch time.

<a name="cross_ref08"></a>
***opponent***: Difficulty level of *TEAM_TYPE_COMPUTER* teams.

~~~ c
enum OpponentType : unsigned char
{
  CLUELESS = 0,
  APPRENTICE = 1,
  AVERAGE = 2,
  EXPERT = 3,
  MASTER = 4,
  GOD = 5
};
~~~

<a name="cross_ref05"></a>
***turn_timer_time*** and ***endturn_time***: Timer values are in seconds. 0 means infinite.

The Game Clock works somewhat like a chess timer. Two timer values are of significance: the Turn Timer, and the End Turn Timer. As each new turn is started, the Turn Timer resets and starts to count-down to zero. The default Turn time is 180 seconds, or three minutes. This is the time available for each turn. When the timer gets to zero, the turn ends automatically, and the next turn starts. You may click the End Turn button when you are finished with each turn to reset the timer and start a new turn. After clicking End Turn, the End Turn Timer starts counting down to allow your opponent(s) the opportunity to finish their turns. The default End Turn time is 45 seconds [\[2\]](#ref2).

<a name="cross_ref06"></a>
***play_mode***: M.A.X. can be played in standard Turn-Based mode or in Concurrent mode, in which players take their turns simultaneously as in a realtime game [\[2\]](#ref2).

~~~ c
enum PlayMode : unsigned char
{
  PLAY_MODE_TURN_BASED = 0,
  PLAY_MODE_SIMULTANEOUS_MOVES = 1
};
~~~

<a name="cross_ref02"></a>
### Victory and Loss Conditions

#### Generic Rules

If ***mission_index*** is 0, then generic *custom game* victory and loss conditions are applicable.

Two *victory conditions* are available during *custom games*, and are based either on reaching a certain goal of points or upon the number of turns played [\[2\]](#ref2).

<a name="cross_ref07"></a>
***Victory Conditions***:

Option 1: When playing for a set number of victory points, the first player who reaches the goal is designated the winner.

Option 2: When playing based on the number of turns, the player with the highest number of victory points when the turn limit is reached wins the game.

Regardless of selecting option 1 or option 2 during game setup, if all opponents are eliminated, when all of them fulfill the loss conditions, the remaining player wins.

In case there is a tie based on victory points derived from eco-spheres the game calculates the sum of turns that were required to build all built and still standing buildings at build rate 1x speed and the player with the more “valuable” base wins.

The turns required to build certain buildings depend on clan (dis)advantages. But even worse the game uses the given team's latest upgraded unit attributes tables. This means that a highly researched turns attribute puts players at a disadvantage as the calculated “value” of their bases will be lower. In case of a tie this could greatly affect which team would win at the end.

If there is still a tie even after comparing the “value” of bases, the player with the smaller team index wins. Red team’s index is the smallest, and the gray team’s is the highest.

***Loss Conditions***:

The following diagram sums up the generic loss conditions.

<img src="{{ site.baseurl }}/assets/images/loss_conditions.svg" alt="Loss Conditions">

There are corner cases that are not handled well. For example if the *construction power* loss condition needs to be evaluated and the team has mining stations but they cannot mine materials as there are no raw material deposits beneath them the game will not signal defeat. The first mining station of a team always has at least 10 raw materials and 7 fuel, but that mining station could get destroyed.

We could rationalize that in case we have working eco-spheres our victory points may still be increasing passively even if we have no military power and we cannot build anything at all. The game play would be rather boring, but maybe just a few turns are remaining till victory conditions are met.

It is also strange why stealth units do not count towards military power while fighters (airplane) do. It is true that most buildings have higher armor rating than the attack power of infiltrators, but they could steal mobile enemy or neutral (alien) military units. On a map like Sandspit there is a high chance that a few submarines could destroy some enemy eco-spheres or could protect their own ones that could considerably affect victory points. We could rationalize that the losing team would be able to play hide and seek to troll the winning team which would not add to desired game play experience.

#### Training Missions

All tutorial missions that are not explicitly listed below use generic rules. Generic loss conditions always apply unless stated otherwise.

**Training Mission 1**

*Victory Conditions:* Red Team has count `Raw Materials > 0` and `Fuel > 0` and `UNIT_TYPE_FUEL_TANK > 0`.

*Loss Conditions:* Red Team has count `UNIT_TYPE_MINING_STATION == 0` or `UNIT_TYPE_ENGINEER == 0`. The generic loss conditions do not apply for this mission for any of the teams.

**Training Mission 2**

*Victory Conditions:* Red Team has count `UNIT_TYPE_LIGHT_VEHICLE_PLANT > 0` and `UNIT_TYPE_GUN_TURRET > 0`.

*Loss Conditions:* Red Team has count (`UNIT_TYPE_ENGINEER == 0` and `UNIT_TYPE_GUN_TURRET == 0`) or (`UNIT_TYPE_CONSTRUCTOR == 0` and `UNIT_TYPE_LIGHT_VEHICLE_PLANT = 0`).

**Training Mission 3**

*Victory Conditions:* Red Team has count `UNIT_TYPE_HEAVY_VEHICLE_PLANT > 0`, `UNIT_TYPE_SCOUT > 2`.

*Loss Conditions:* Red Team has count (`UNIT_TYPE_LIGHT_VEHICLE_PLANT == 0` and `UNIT_TYPE_SCOUT < 3`) or `UNIT_TYPE_MINING_STATION == 0` or (`UNIT_TYPE_CONSTRUCTOR == 0` and `UNIT_TYPE_HEAVY_VEHICLE_PLANT = 0`).

**Training Mission 4**

*Victory Conditions:* Red Team has `Total Raw Materials Mining > 21`.

*Loss Conditions:* Red Team has count `Total Raw Materials Mining < 22` and ((`UNIT_TYPE_CONSTRUCTOR == 0` or `UNIT_TYPE_MINING_STATION == 0`).

**Training Mission 5**

*Victory Conditions:* Green Team has count `UNIT_TYPE_MINING_STATION == 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 6**

*Victory Conditions:* Red Team has count `UNIT_TYPE_GUN_TURRET > 0` under construction at end of turn.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 7**

*Victory Conditions:* Red Team `total power consumption of all UNIT_TYPE_LIGHT_VEHICLE_PLANT and UNIT_TYPE_HEAVY_VEHICLE_PLANT units > 1` and `total Fuel mining - total Fuel consumption > 0`.

*Loss Conditions:* Red Team has count `UNIT_TYPE_LIGHT_VEHICLE_PLANT + UNIT_TYPE_HEAVY_VEHICLE_PLANT < 2` or `UNIT_TYPE_MINING_STATION == 0`.

**Training Mission 8**

*Victory Conditions:* Red Team has count `Gold > 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 9**

*Victory Conditions:* Red Team has count `Upgraded UNIT_TYPE_TANK > 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 10**

*Victory Conditions:* Red Team has `Team Points > 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 11**

*Victory Conditions:* Read Team has at least one `UNIT_TYPE_INFILTRATOR` with `Experience > 0`.

*Loss Conditions:* Red Team has count `UNIT_TYPE_MINING_STATION == 0` or `UNIT_TYPE_TRAINING_HALL == 0` or `UNIT_TYPE_HABITAT == 0`.

**Training Mission 12**

*Victory Conditions:* Red Team has count `UNIT_TYPE_LAND_MINE > 0` and Green Team has casualties count `UNIT_TYPE_LAND_MINE > 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Training Mission 13**

*Victory Conditions:* Red Team has count `UNIT_TYPE_TANK > 2` and count of `Damaged UNIT_TYPE_TANK == 0`.

*Loss Conditions:* Red Team has count (`UNIT_TYPE_TANK < 3` or `Damaged UNIT_TYPE_TANK > 0`) and casualties count `UNIT_TYPE_TANK > 0`.

**Training Mission 14**

*Victory Conditions:* Red Team has count `UNIT_TYPE_TANK > 2` and count of `Missing Ammo UNIT_TYPE_TANK == 0`.

*Loss Conditions:* Red Team has count (`UNIT_TYPE_TANK < 3` or `Missing Ammo UNIT_TYPE_TANK > 0`) and casualties count `UNIT_TYPE_TANK > 0`.

**Training Mission 15**

*Victory Conditions:* Red Team has `Armor Research Level > 0`.

*Loss Conditions:* Red Team has count `UNIT_TYPE_RESEARCH_CENTER == 0`.

#### Stand Alone Missions

All scenario missions that are not explicitly listed below use generic rules. Generic loss conditions always apply unless stated otherwise.

**Scenario Mission 9**

*Victory Conditions:* Red Team `Team Points` > Green Team `Team Points`.

*Loss Conditions:* This mission has no unique loss conditions.

**Scenario Mission 18**

*Victory Conditions:* Green Team has count `UNIT_TYPE_RESEARCH_CENTER == 0`.

*Loss Conditions:* This mission has no unique loss conditions.

**Scenario Mission 20**

*Victory Conditions:* As long as Red Team has forecasted `Team Points` >= Green Team `Team Points` generic rules apply. Forecasted Team Points is the sum of earned Team Points plus Team owned `UNIT_TYPE_ECOSPHERE` count.

*Loss Conditions:* Red Team has forecasted `Team Points` < Green Team `Team Points`.

**Scenario Mission 24**

*Victory Conditions:* Green Team has count `UNIT_TYPE_ECOSPHERE == 0`.

*Loss Conditions:* This mission has no unique loss conditions.

#### Campaign Game

Generic loss conditions always apply unless stated otherwise.

**Campaign Mission 1**

*Victory Conditions:* Green Team has count `UNIT_TYPE_ECOSPHERE == 0`.

*Loss Conditions:* Green Team has count `UNIT_TYPE_RESEARCH_CENTER == 0`.

**Campaign Mission 2**

*Victory Conditions:* `Turn Counter == 30.`

*Loss Conditions:* Red Team has casualties count `UNIT_TYPE_RESEARCH_CENTER > 0`.

**Campaign Mission 3**

*Victory Conditions:* Red Team has count `UNIT_TYPE_SUPPLY_TRUCK > 1`.

*Loss Conditions:* Red Team plus Green Team has count total `UNIT_TYPE_SUPPLY_TRUCK < 2` or Red Team has count `UNIT_TYPE_INFILTRATOR == 0`.

**Campaign Missions 4, 5 and 9**

*Victory Conditions:* Green Team has count `UNIT_TYPE_ECOSPHERE == 0`.

*Loss Conditions:* These missions have no unique loss conditions.

**Campaign Mission 8**

*Victory Conditions:* Red Team has count total `UNIT_TYPE_ALIEN_* > 3` when `Turn Counter == Turn Limit (100)`.

*Loss Conditions:* Red Team has count total `UNIT_TYPE_ALIEN_* < 4` or casualties count `UNIT_TYPE_RESEARCH_CENTER > 0`.

All campaign missions that are not explicitly listed above use the following rules:

**Team Points Based Missions**

*Victory Conditions:* Red Team `Team Points` reaches `Team Points Limit`.

*Loss Conditions:* Green Team `Team Points` reaches `Team Points Limit`.

**Turn Limit Based Missions**

*Victory Conditions:* Red Team `Turn Counter == Turn Limit`.

*Loss Conditions:* These missions have no unique loss conditions.

### Options Section of INI settings

M.A.X. v1.04 defines the following INI configuration settings within the OPTIONS section. The numeric values are just default values. These settings are all numeric so they are stored as 32 bit integers. An alphanumeric setting would be stored as a null terminated string always on 30 bytes.

~~~ c
[OPTIONS]
world=0
timer=180
endturn=45
start_gold=150
play_mode=1
victory_type=0
victory_limit=50
opponent=1
raw_resource=1
fuel_resource=1
gold_resource=1
alien_derelicts=0
~~~

The above settings are stored as follows:

~~~ c
struct __attribute__((packed)) IniOptions
{
  signed int world;
  signed int turn_timer;
  signed int endturn;
  signed int start_gold;
  signed int play_mode;
  signed int victory_type;
  signed int victory_limit;
  signed int opponent;
  signed int raw_resource;
  signed int fuel_resource;
  signed int gold_resource;
  signed int alien_derelicts;
};
~~~

In case of campaign, single and multi player scenario games the ***opponent***, ***timer***, ***endturn*** and ***play_mode*** settings are overwritten by the Header fields of the same name.

***world***: See *[planet field](#cross_ref01)*.

***turn_timer*** and ***endturn***: See *[reference](#cross_ref05)*.

***start_gold***: This is the starting credits ranging from 0 to 250 credits.

***play_mode***: See *[reference](#cross_ref06)*.

***victory_type*** and ***victory_limit***: See *[reference](#cross_ref07)*.

~~~ c
enum VictoryType : char
{
  VICTORY_TYPE_DURATION = 0,
  VICTORY_TYPE_SCORE = 1
};
~~~

***opponent***: See *[reference](#cross_ref08)*.

***raw_resource***, ***fuel_resource*** and ***gold_resource***: The Resource Levels options allow to set the amount and distribution of the various resources (Raw Material, Fuel and Gold) that will be available during the game [\[2\]](#ref2).

The valid value range is 0 to 2 representing Poor, Medium and Rich settings.

***alien_derelicts***: The option allows to set the amount and distribution of alien derelicts.

The valid value range is 0 to 2 representing None, Rare and Common settings.

### Planet Surface Map

M.A.X. uses a top-down square grid or tile map system. The coordinate system is based on a standard x,y grid, with coordinate 0,0 at the top left of the map. X-coordinate measures from left to right and Y-coordinates from top to bottom [\[2\]](#ref2).

~~~ c
enum SurfaceType : unsigned char
{
  SURFACE_TYPE_LAND = 1,
  SURFACE_TYPE_WATER = 2,
  SURFACE_TYPE_COAST = 4,
  SURFACE_TYPE_AIR = 8
};

enum SurfaceType SurfaceMap[x][y];
~~~

In M.A.X. the grid map size is always 112 x 112 tiles so `x = y = 112;` in `SurfaceMap[x][y]`. Note that map dimensions are specified by the world (WRL) file format.

Each element in the above array defines the accessibility property of the associated grid map cell. For example if a cell can only be crossed by air units, then the surface type is *SURFACE_TYPE_AIR*.

### Planet Resource Map

~~~ c
CARGO_FUEL 0x20
CARGO_GOLD 0x40
CARGO_MATERIALS 0x80

TEAM_VISIBILITY_RED 0x2000
TEAM_VISIBILITY_GREEN 0x1000
TEAM_VISIBILITY_BLUE 0x800
TEAM_VISIBILITY_GRAY 0x400

RESOURCE_AMOUNT_MASK 0x1F
RESOURCE_TYPE_MASK 0xE0
TEAM_VISIBILITY_MASK 0x1C00

short GridResourceMapEntry[x][y];
~~~

In M.A.X. the grid map size is always 112 x 112 tiles so `x = y = 112;` in `GridResourceMapEntry[x][y]`. Note that map dimensions are specified by the world (WRL) file format.

<img src="{{ site.baseurl }}/assets/images/bitfield_save_rmap_entry.svg" alt="Resource Map Entry Bitfield">

***cargo amount***: The 5 LSBs represent the amount of  resource available at the cell. The game limits the usable amount to 16 though.

***cargo type***: The type of resource present at the cell. Bit 5 is *CARGO_FUEL*, bit 6 is *CARGO_GOLD* and bit 7 is *CARGO_MATERIALS*. Only one bit should be set.

***team visibility***: If a specific team surveyed the cell, then the team specific bit is set. Bit 10 is *GRAY_TEAM*, bit 11 is *BLUE_TEAM*, bit 12 is *GREEN_TEAM* and bit 13 is *RED_TEAM*.

Example: the 16 bit value of 0x208C means 12 raw materials visible by red team only.

### Serialized Team Info Objects

~~~ c
struct __attribute__((packed)) Point
{
  signed short x;
  signed short y;
};

struct __attribute__((packed)) ResearchTopicInfo
{
  unsigned int research_level;
  unsigned int turns_to_complete;
  unsigned int allocation;
};

struct __attribute__((packed)) ScreenLocation
{
  signed char x;
  signed char y;
};

struct __attribute__((packed)) TeamInfo
{
  Point markers[10];
  TeamType team_type;
  bool finished_turn;
  TeamClan team_clan;
  ResearchTopicInfo research_topics[8];
  unsigned int team_points;
  unsigned short number_of_objects_created;
  unsigned char unit_counters[93];
  ScreenLocation camera_positions[6];
  signed short score_graph[50];
  unsigned short selected_unit;
  unsigned short zoom_level;
  Point camera_position;
  signed char display_button_range;
  signed char display_button_scan;
  signed char display_button_status;
  signed char display_button_colors;
  signed char display_button_hits;
  signed char display_button_ammo;
  signed char display_button_minimap_2x;
  signed char display_button_minimap_tnt;
  signed char display_button_grid;
  signed char display_button_names;
  signed char display_button_survey;
  signed short stats_factories_built;
  signed short stats_mines_built;
  signed short stats_buildings_built;
  signed short stats_units_built;
  unsigned short casualties[93];
  signed short stats_gold_spent_on_upgrades;
};
~~~

Four instances of the `TeamInfo` structure are written to the save file representing the four teams. Red is the first, green is the second, blue is the third, gray is the fourth.

***markers***: Array elements are initialized to {-1, -1} or simply 0xFFFF, 0xFFFF. The feature was removed from the game so this field is useless.

***team_type***: Type of the given team. See *[TeamType](#cross_ref03)* type.

***finished_turn***: Indicates whether a team is ready to progress to the next game turn.

***team_clan***: Clan of the given team. See *[TeamClan](#cross_ref04)* type.

***research_topics***: There are 8 research topics. The *research_level* field defines the number of research cycles already finished.  The *turns_to_complete* field defines the number of turns required to complete a given research cycle if a single research center would be assigned to work on the topic. If the value is 0 it means the next level's research is not yet started. The formula for the actual turns required is as follows: `turns = ceil(turns_to_complete / allocation);`. The *allocation* field defines the number of research centers allocated to a given topic.

~~~ c
enum ResearchTopic : unsigned char
{
  RESEARCH_TOPIC_ATTACK = 0,
  RESEARCH_TOPIC_SHOTS = 1,
  RESEARCH_TOPIC_RANGE = 2,
  RESEARCH_TOPIC_ARMOR = 3,
  RESEARCH_TOPIC_HITS = 4,
  RESEARCH_TOPIC_SPEED = 5,
  RESEARCH_TOPIC_SCAN = 6,
  RESEARCH_TOPIC_COST = 7
};
~~~

***team_points***: Number of victory points acquired for operating eco-spheres.

***number_of_objects_created***: The valid value range is 1 to 8191. Unit hash identifiers are derived from this counter's value.

***unit_counters***: There are 93 different types of units in the game. Each array element represents a given type of unit. The value is used to derive basic unit names to newly created units. For example if a team has 10 tanks then `unit_counters[UNIT_TYPE_TANK] = 11` as the next unit index will be 11. The array elements are initialized to 1. The 10th tank would get the basic unit name *MK 1 TANK 10*. The counter could easily overflow during a long game which means that multiple units could have the exact same name. In case of an overflow index 0 is skipped.

***camera_positions***: {x, y} coordinates of the previously saved screen locations. The array has 6 elements which is odd as only four function keys can be used to set or recall locations using F5 to F8. Array elements are initialized to {-1, -1} or simply 0xFF, 0xFF. See chapter *Setting Locations* in [\[2\]](#ref2) for further details on the feature.

***score_graph***: The score report presents a graph of each teams point totals over a period of the last 50 turns [\[2\]](#ref2).

***selected_unit***: Default value is 0xFFFF which means no unit is selected. Otherwise the value is set to the hash ID of the selected unit. Not every *UnitInfo* object gets a hash ID, in cases the default 0xFFFF could also be used.

The hash ID is derived from the *number_of_objects_created* field and the unit's *team index*. Mind that there are 5 teams actually. All neutral alien derelicts related units belong to the fifth team.

<img src="{{ site.baseurl }}/assets/images/bitfield_unit_hash_id.svg" alt="Unit Hash ID Bitfield">

***zoom_level***: Resolution of the main map display window. The minimum value is 4, maximum is 64. Zoom level 64 is pixel perfect 1:1 resolution.

***camera_position***: {x, y} grid coordinates to be used as the center of the screen if possible. The valid value range for the coordinates is 0 to 111.

***display_button_xyz***: Each of the display buttons toggle the activation state of a visual display of some kind on the tactical map display window [\[2\]](#ref2). 0 means the toggle is off, any other value means on.

***stats_factories_built***: Number of factory buildings built by the team during the game. The statistic is displayed on the game over screen.

***stats_mines_built***: Number of mining stations built by the team during the game. The statistic is displayed on the game over screen.

***stats_buildings_built***: Number of buildings built by the team during the game. The statistic is displayed on the game over screen.

***stats_units_built***: Number of mobile units built by the team during the game. The statistic is displayed on the game over screen.

***casualties***: There are 93 different types of units in the game. Each array element represents the number of units of a particular type lost by the team during the game. The statistic is displayed in the reports menu. The array is also used from time to time to determine victory and loss conditions.

***stats_gold_spent_on_upgrades***: Amount of gold spent by the team on unit upgrades during the game. The statistic is displayed on the game over screen.

### Game Manager State

~~~ c
enum TeamIndex : unsigned char
{
  TEAM_INDEX_RED = 0,
  TEAM_INDEX_GREEN = 1,
  TEAM_INDEX_BLUE = 2,
  TEAM_INDEX_GRAY = 3
};

struct __attribute__((packed)) GameManagerState
{
  TeamIndex active_turn_team;
  TeamIndex player_team;
  signed int turn_counter;
  unsigned short game_state;
  unsigned short turn_timer;
};
~~~

***active_turn_team***: Team that is in control of the game UI. The value has relevance in concurrent play mode too.

***player_team***: Player team. In single player mode this is normally a human player, but if only computers are playing against each other, then this is set to the lowest team index in play. In hot seat mode this is the active human player.

***turn_counter***: Turn Counter value displayed on screen representing the current game turn.

***game_state***: There are 16 different game states. Many of them are transient states. They are not fully deciphered yet.

***turn_timer***: Turn Timer value displayed on screen representing the current turn's remaining time.

### Preferences Section of INI settings

M.A.X. v1.04 defines the following INI configuration settings within the PREFERENCES section. The numeric values are just default values. These settings are all numeric so they are stored as 32 bit integers. An alphanumeric setting would be stored as a null terminated string always on 30 bytes including the delimiter so special care must be taken for UTF-8 encoded grapheme clusters.

~~~ c
[PREFERENCES]
effects=1
click_scroll=1
quick_scroll=16
fast_movement=1
follow_unit=0
auto_select=0
enemy_halt=1
~~~

The above settings are stored as follows:

~~~ c
struct __attribute__((packed)) IniPreferences
{
  signed int effects;
  signed int click_scroll;
  signed int quick_scroll;
  signed int fast_movement;
  signed int follow_unit;
  signed int auto_select;
  signed int enemy_halt;
};
~~~

See the game user manual for detailed descriptions of the settings [\[2\]](#ref2). Most settings are toggles so their valid value range is 0 to 1.

***effects***: Animate effects.

***click_scroll***: Click to scroll.

***quick_scroll***: Quick scroll. The valid value range is 4 to 128.

***fast_movement***: Fast unit moves.

***follow_unit***: Follow unit.

***auto_select***: Automatically switch to the next unit when the selected unit has run out of movement points.

***enemy_halt***: Halt movement when enemy is detected.

### Serialized Team Units Objects

~~~ c
struct __attribute__((packed)) UnitValues
{
  unsigned short object_index;

  if (object_index < last_object_index)
  {
    last_object_index++;

    unsigned short class_type;
    unsigned short turns;
    unsigned short hits;
    unsigned short armor;
    unsigned short attack;
    unsigned short speed;
    unsigned short range;
    unsigned short rounds;
    unsigned char move_and_fire;
    unsigned short scan;
    unsigned short storage;
    unsigned short ammo;
    unsigned short attack_radius;
    unsigned short agent_adjust;
    unsigned short version;
    unsigned char units_built;
  }
};

struct __attribute__((packed)) Complex
{
  unsigned short object_index;

  if (object_index < last_object_index)
  {
    last_object_index++;

    unsigned short class_type;
    short material;
    short fuel;
    short gold;
    short power;
    short workers;
    short buildings;
    short id;
  }
};

struct __attribute__((packed)) TeamUnits
{
  signed short gold;
  UnitValues base_unit_values[93];
  UnitValues current_unit_values[93];
  unsigned short complex_count;
  Complex complexes[complex_count];
};
~~~

Four instances of the `TeamUnits` structure are written to the save file representing the four teams. Red is the first, green is the second, blue is the third, gray is the fourth.

***gold***: Team credits that could be used to purchase upgrades in gold refineries. The field is stored as a 16 bit signed value.

***base_unit_values***: There are 93 different types of units in the game. Each array element represents a unit type's base unit values data. Base unit values incorporate clan upgrades. This might not seem to be a big deal at first, but it is as the higher the initial unit attribute value the cheaper it is to upgrade later. So an already excellent clan special unit type could be upgraded into a superb unit type faster. The *version* field defines the mark level of a unit. The initial value of the *version* field is 1. If the *units_built* field is zero when a new version is created from the unit type the *version* field is not incremented. The *storage* field has special meaning for units that gather experience.

***current_unit_values***: Each array element represents a unit type's current unit values data. This array represents the latest technology. New units will be created using these unit values.

***complex_count***: Complexes are buildings that are connected together. Complexes share power and cargo supplies. The number of team complexes dynamically change during the course of game play. This field defines the number of serialized *Complex* class objects that follows. In case the given team does not have any complexes the value is 0 and no *Complex* structure follows.

***complexes***: The *material*, *fuel* and *gold* fields represent the stored and the produced resources. The *power* and *workers* fields represent the surplus of generated power and unallocated workforce within the complex. The *buildings* field defines the number of buildings currently associated with the given complex. The *id* field is the unique index of the given complex. The game stores complexes in an ordered linked list so the *id* should monotonically increase in the save file for each team.

### Serialized Unit Info Object Lists

Even though there are 6 *UnitInfo* object lists, only 5 of them are saved.

~~~ c
enum UnitType : unsigned short
{
  UNIT_TYPE_GOLD_REFINERY = 0,
  UNIT_TYPE_POWER_STATION = 1,
  UNIT_TYPE_POWER_GENERATOR = 2,
  UNIT_TYPE_BARRACKS = 3,
  UNIT_TYPE_ALIEN_BUILDING_1 = 4,
  UNIT_TYPE_RADAR = 5,
  UNIT_TYPE_STORAGE_UNIT = 6,
  UNIT_TYPE_FUEL_TANK = 7,
  UNIT_TYPE_GOLD_VAULT = 8,
  UNIT_TYPE_DEPOT = 9,
  UNIT_TYPE_HANGAR = 10,
  UNIT_TYPE_DOCK = 11,
  UNIT_TYPE_CONNECTOR = 12,
  UNIT_TYPE_LARGE_RUBBLE_1 = 13,
  UNIT_TYPE_SMALL_RUBBLE_1 = 14,
  UNIT_TYPE_LARGE_TAPE = 15,
  UNIT_TYPE_SMALL_TAPE = 16,
  UNIT_TYPE_LARGE_SLAB = 17,
  UNIT_TYPE_SMALL_SLAB = 18,
  UNIT_TYPE_LARGE_CONES = 19,
  UNIT_TYPE_SMALL_CONES = 20,
  UNIT_TYPE_ROAD = 21,
  UNIT_TYPE_LANDING_PAD = 22,
  UNIT_TYPE_SHIPYARD = 23,
  UNIT_TYPE_LIGHT_VEHICLE_PLANT = 24,
  UNIT_TYPE_HEAVY_VEHICLE_PLANT = 25,
  UNIT_TYPE_ALIEN_BUILDING_2 = 26,
  UNIT_TYPE_AIR_UNITS_PLANT = 27,
  UNIT_TYPE_HABITAT = 28,
  UNIT_TYPE_RESEARCH_CENTER = 29,
  UNIT_TYPE_ECOSPHERE = 30,
  UNIT_TYPE_ALIEN_BUILDING_3 = 31,
  UNIT_TYPE_TRAINING_HALL = 32,
  UNIT_TYPE_WATER_PLATFORM = 33,
  UNIT_TYPE_GUN_TURRET = 34,
  UNIT_TYPE_ANTI_AIRCRAFT = 35,
  UNIT_TYPE_ARTILLERY = 36,
  UNIT_TYPE_MISSILE_LAUNCHER = 37,
  UNIT_TYPE_CONCRETE_BLOCK = 38,
  UNIT_TYPE_BRIDGE = 39,
  UNIT_TYPE_MINING_STATION = 40,
  UNIT_TYPE_LAND_MINE = 41,
  UNIT_TYPE_SEA_MINE = 42,
  UNIT_TYPE_LAND_EXPLOSION = 43,
  UNIT_TYPE_AIR_EXPLOSION = 44,
  UNIT_TYPE_SEA_EXPLOSION = 45,
  UNIT_TYPE_BUILDING_EXPLOSION = 46,
  UNIT_TYPE_HIT_EXPLOSION = 47,
  UNIT_TYPE_MASTER_BUILDER = 48,
  UNIT_TYPE_CONSTRUCTOR = 49,
  UNIT_TYPE_SCOUT = 50,
  UNIT_TYPE_TANK = 51,
  UNIT_TYPE_ASSAULT_GUN  = 52,
  UNIT_TYPE_ROCKET_LAUNCHER = 53,
  UNIT_TYPE_MISSILE_CRAWLER = 54,
  UNIT_TYPE_MOBILE_ANTI_AIRCRAFT = 55,
  UNIT_TYPE_MINE_LAYER = 56,
  UNIT_TYPE_SURVEYOR = 57,
  UNIT_TYPE_SCANNER = 58,
  UNIT_TYPE_SUPPLY_TRUCK = 59,
  UNIT_TYPE_GOLD_TRUCK = 60,
  UNIT_TYPE_ENGINEER = 61,
  UNIT_TYPE_BULLDOZER = 62,
  UNIT_TYPE_REPAIR_UNIT = 63,
  UNIT_TYPE_FUEL_TRUCK = 64,
  UNIT_TYPE_PERSONNEL_CARRIER = 65,
  UNIT_TYPE_INFILTRATOR = 66,
  UNIT_TYPE_INFANTRY = 67,
  UNIT_TYPE_ESCORT = 68,
  UNIT_TYPE_CORVETTE = 69,
  UNIT_TYPE_GUNBOAT = 70,
  UNIT_TYPE_SUBMARINE = 71,
  UNIT_TYPE_SEA_TRANSPORT = 72,
  UNIT_TYPE_MISSILE_CRUISER = 73,
  UNIT_TYPE_SEA_MINE_LAYER = 74,
  UNIT_TYPE_CARGO_SHIP = 75,
  UNIT_TYPE_FIGHTER = 76,
  UNIT_TYPE_GROUND_ATTACK_PLANE = 77,
  UNIT_TYPE_AIR_TRANSPORT = 78,
  UNIT_TYPE_AWAC = 79,
  UNIT_TYPE_ALIEN_GUNBOAT = 80,
  UNIT_TYPE_ALIEN_TANK = 81,
  UNIT_TYPE_ALIEN_ASSAULT_GUN = 82,
  UNIT_TYPE_ALIEN_ATTACK_PLANE = 83,
  UNIT_TYPE_MISSILE = 84,
  UNIT_TYPE_TORPEDO = 85,
  UNIT_TYPE_ALIEN_MISSILE = 86,
  UNIT_TYPE_TANK_PLASMA_BALL = 87,
  UNIT_TYPE_ARTILLERY_PLASMA_BALL = 88,
  UNIT_TYPE_SMOKE_TRAIL = 89,
  UNIT_TYPE_BUBBLE_TRAIL = 90,
  UNIT_TYPE_HARVESTER = 91,
  UNIT_TYPE_DEAD_WALDO = 92
};

enum OrderType : unsigned char
{
  ORDER_TYPE_AWAIT = 0x0,
  ORDER_TYPE_TRANSFORM = 0x1,
  ORDER_TYPE_MOVE = 0x2,
  ORDER_TYPE_FIRE = 0x3,
  ORDER_TYPE_BUILD = 0x4,
  ORDER_TYPE_ACTIVATE = 0x5,
  ORDER_TYPE_NEW_ALLOCATE = 0x6,
  ORDER_TYPE_POWER_ON = 0x7,
  ORDER_TYPE_POWER_OFF = 0x8,
  ORDER_TYPE_EXPLODE = 0x9,
  ORDER_TYPE_UNLOAD = 0xA,
  ORDER_TYPE_CLEAR = 0xB,
  ORDER_TYPE_SENTRY = 0xC,
  ORDER_TYPE_LAND = 0xD,
  ORDER_TYPE_TAKE_OFF = 0xE,
  ORDER_TYPE_LOAD = 0xF,
  ORDER_TYPE_IDLE = 0x10,
  ORDER_TYPE_REPAIR = 0x11,
  ORDER_TYPE_REFUEL = 0x12,
  ORDER_TYPE_RELOAD = 0x13,
  ORDER_TYPE_TRANSFER = 0x14,
  ORDER_TYPE_HALT_BUILDING = 0x15,
  ORDER_TYPE_AWAIT_SCALING = 0x16,
  ORDER_TYPE_AWAIT_TAPE_POSITIONING = 0x17,
  ORDER_TYPE_AWAIT_STEAL_UNIT = 0x18,
  ORDER_TYPE_AWAIT_DISABLE_UNIT = 0x19,
  ORDER_TYPE_DISABLE = 0x1A,
  ORDER_TYPE_MOVE_TO_UNIT = 0x1B,
  ORDER_TYPE_UPGRADE = 0x1C,
  ORDER_TYPE_LAY_MINE = 0x1D,
  ORDER_TYPE_MOVE_TO_ATTACK = 0x1E,
  ORDER_TYPE_HALT_BUILDING_2 = 0x1F
};

enum OrderStateType : unsigned char
{
  ORDER_STATE_INIT = 0x0,
  ORDER_STATE_EXECUTING_ORDER = 0x1,
  ORDER_STATE_READY_TO_EXECUTE_ORDER = 0x2,
  ORDER_STATE_STORE = 0x3,
  ORDER_STATE_PREPARE_STORE = 0x4,
  ORDER_STATE_IN_PROGRESS = 0x5,
  ORDER_STATE_IN_TRANSITION = 0x6,
  ORDER_STATE_ISSUING_PATH = 0x7,
  ORDER_STATE_READY_TO_FIRE = 0x8,
  ORDER_STATE_FIRE_IN_PROGRESS = 0x9,
  ORDER_STATE_10 = 0xA,
  ORDER_STATE_BUILD_IN_PROGRESS = 0xB,
  ORDER_STATE_PATH_REQUEST_CANCEL = 0xC,
  ORDER_STATE_BUILD_CANCEL = 0xD,
  ORDER_STATE_DESTROY = 0xE,
  ORDER_STATE_LOADING_IN_PROGRESS = 0xF,
  ORDER_STATE_FINISH_LOADING = 0x10,
  ORDER_STATE_UNLOADING_IN_PROGRESS = 0x11,
  ORDER_STATE_FINISH_UNLOADING = 0x12,
  ORDER_STATE_FINISH_LANDING = 0x13,
  ORDER_STATE_FINISH_TAKE_OFF = 0x14,
  ORDER_STATE_21 = 0x15,
  ORDER_STATE_PROGRESS_TRANSFORMING = 0x16,
  ORDER_STATE_FINISH_TRANSFORMING = 0x17,
  ORDER_STATE_CLEAR_PATH = 0x18,
  ORDER_STATE_SELECT_SITE = 0x19,
  ORDER_STATE_BUILD_CLEARING = 0x1A,
  ORDER_STATE_EXPLODE = 0x1B,
  ORDER_STATE_MOVE_INIT = 0x1C,
  ORDER_STATE_MOVE_GETTING_PATH = 0x1D,
  ORDER_STATE_BUILDING_READY = 0x1E,
  ORDER_STATE_UNIT_READY = 0x1F,
  ORDER_STATE_EXPAND = 0x20,
  ORDER_STATE_SHRINK = 0x21,
  ORDER_STATE_TAPE_POSITIONING_INIT = 0x22,
  ORDER_STATE_TAPE_POSITIONING_ENTER = 0x23,
  ORDER_STATE_TAPE_POSITIONING_DEINIT = 0x24,
  ORDER_STATE_TAPE_POSITIONING_LEAVE = 0x25,
  ORDER_STATE_ELEVATE = 0x26,
  ORDER_STATE_LOWER = 0x27,
  ORDER_STATE_ATTACK_PENDING = 0x28,
  ORDER_STATE_ATTACK_BEGINNING = 0x29,
  ORDER_STATE_NEW_ORDER = 0x2A,
  ORDER_STATE_INACTIVE = 0x2B,
  ORDER_STATE_PLACING_MINES = 0x2C,
  ORDER_STATE_REMOVING_MINES = 0x2D,
  ORDER_STATE_BUILD_ABORT = 0x2E,
};

struct __attribute__((packed)) Rect
{
  signed int ulx;
  signed int uly;
  signed int lrx;
  signed int lry;
};

enum ClassType : unsigned char
{
  CLASS_TYPE_AIR_PATH = 1,
  CLASS_TYPE_BUILDER_PATH = 2,
  CLASS_TYPE_GROUND_PATH = 4
};

struct __attribute__((packed)) PathStep
{
  signed char x;
  signed char y;
};

struct __attribute__((packed)) Path
{
  unsigned short object_index;

  if (object_index < last_object_index)
  {
    last_object_index++;

    unsigned short class_type;

    if (class_type == CLASS_TYPE_AIR_PATH)
    {
      short length;
      unsigned char angle;
      Point pixel_start;
      Point pixel_end;
      signed int x_step;
      signed int y_step;
      signed int delta_x;
      signed int delta_y;
    }
    else if (class_type == CLASS_TYPE_GROUND_PATH)
    {
      Point pixel_end;
      signed short index;
      signed short steps_count;
      PathStep steps[steps_count];
    }
    else if (class_type == CLASS_TYPE_BUILDER_PATH)
    {
      Point coordinate;
    }
    else
    {
      assert(0, "Unknown path class");
    }
};

struct __attribute__((packed)) UnitTypeArray
{
  unsigned short object_count;

  UnitType array[object_count];
};

struct __attribute__((packed)) UnitInfo
{
  unsigned short object_index;

  if (object_index < last_object_index)
  {
    last_object_index++;

    unsigned short class_type;
    UnitType unit_type;

    if (unit_type == UNIT_TYPE_DEAD_WALDO)
    {
      print("Found Waldo!");
    }

    unsigned short hash_id;
    unsigned int flags;
    Point pixel_position;
    Point grid_position;
    unsigned short name_length;
    char name[name_length];
    Point shadow_offset;
    TeamIndex team;
    unsigned char unit_id;
    unsigned char brightness;
    unsigned char angle;
    unsigned char visible_to_team[5];
    unsigned char spotted_by_team[5];
    unsigned char max_velocity;
    unsigned char velocity;
    unsigned char sound;
    unsigned char scaler_adjust;
    Rect sprite_bounds;
    Rect shadow_bounds;
    unsigned char turret_angle;
    signed char turret_offset_x;
    signed char turret_offset_y;
    signed short total_images;
    signed short image_base;
    signed short turret_image_base;
    signed short firing_image_base;
    signed short connector_image_base;
    signed short image_index;
    signed short turret_image_index;
    signed short image_index_max;
    OrderType orders;
    OrderStateType state;
    OrderType prior_orders;
    OrderStateType prior_state;
    unsigned char laying_state;
    Point target_grid;
    unsigned char build_time;
    unsigned char total_mining;
    unsigned char raw_mining;
    unsigned char fuel_mining;
    unsigned char gold_mining;
    unsigned char raw_mining_max;
    unsigned char gold_mining_max;
    unsigned char fuel_mining_max;
    unsigned char hits;
    unsigned char speed;
    unsigned char shots;
    unsigned char move_and_fire;
    signed short storage;
    unsigned char ammo;
    unsigned char targeting_mode;
    unsigned char enter_mode;
    unsigned char cursor;
    signed char recoil_delay;
    unsigned char delayed_reaction;
    bool damaged_this_turn;
    unsigned char research_topic;
    unsigned char moved;
    bool bobbed;
    unsigned char shake_effect_state;
    unsigned char engine;
    unsigned char weapon;
    unsigned char comm;
    unsigned char fuel_distance;
    unsigned char move_fraction;
    bool energized;
    unsigned char repeat_build;
    unsigned short build_rate;
    bool disabled_reaction_fire;
    bool auto_survey;
    unsigned int field_221;
    Path path;
    unsigned short connectors;
    UnitValues base_values;
    Complex complex;
    UnitInfo parent_unit;
    UnitInfo enemy_unit;
    UnitTypeArray build_list;
  }
};

struct __attribute__((packed)) UnitInfoList
{
  unsigned short unitinfo_count;
  UnitInfo units[unitinfo_count];
};
~~~

### Serialized Hash Maps

There are two hash maps. The first is a Hash_UnitInfo type object, the second is a Hash_MapHash type object.

~~~c
struct __attribute__((packed)) Hash_UnitInfo
{
  unsigned short hash_size;
  UnitInfoList map[hash_size];
};

struct __attribute__((packed)) MapHash
{
  Point coordinates;
  UnitInfoList units;
};

struct __attribute__((packed)) MapHashList
{
  unsigned short maphash_count;
  MapHash objects[maphash_count];
};

struct __attribute__((packed)) Hash_MapHash
{
  unsigned short hash_size;
  short x_shift;
  MapHashList map[hash_size];
};
~~~

### Heat Maps

~~~c
struct __attribute__((packed)) TeamHeatMaps
{
  char heatmap_complete[x][y];
  char heatmap_stealth_sea[x][y];
  char heatmap_stealth_land[x][y];
};
~~~

In M.A.X. the grid map size is always 112 x 112 tiles so `x = y = 112;` in `heatmap_xyz[x][y]`. Note that map dimensions are specified by the world (WRL) file format.

Heat maps are only emitted for valid teams.

### Serialized Message Log Object List

~~~c
struct __attribute__((packed)) MessageLog
{
  unsigned short length;
  char text[length];
  UnitInfo unit;
  Point coordinates;
  bool is_alert_message;
  unsigned short resource_id;
};

struct __attribute__((packed)) MessageLogList
{
  unsigned short message_log_count;
  MessageLog entires[message_log_count];
};
~~~

***is_alert_message***: The *coordinates* field only contains valid data if this flag is set.

### Serialized AI Team Objects

~~~c
enum TeamIndex16 : unsigned short
{
  TEAM_INDEX_RED = 0,
  TEAM_INDEX_GREEN = 1,
  TEAM_INDEX_BLUE = 2,
  TEAM_INDEX_GRAY = 3
};

enum AiStrategyType : uint8_t
{
  AI_STRATEGY_RANDOM,
  AI_STRATEGY_DEFENSIVE,
  AI_STRATEGY_MISSILES,
  AI_STRATEGY_AIR,
  AI_STRATEGY_SEA,
  AI_STRATEGY_SCOUT_HORDE,
  AI_STRATEGY_TANK_HORDE,
  AI_STRATEGY_FAST_ATTACK,
  AI_STRATEGY_COMBINED_ARMS,
  AI_STRATEGY_ESPIONAGE
};

struct __attribute__((packed)) SpottedUnit
{
  UnitInfo unit;
  TeamIndex16 team;
  bool visible_to_team;
  Point last_position;
};

struct __attribute__((packed)) SpottedUnitList
{
  unsigned short spotted_units_count;
  SpottedUnit objects[spotted_units_count];
};

struct __attribute__((packed)) AiPlayer
{
  TeamIndex16 player_team;
  AiStrategyType strategy;
  signed short field_3;
  signed short field_5;
  signed short field_7;
  TeamIndex16 target_team;
  SpottedUnitList map_list;
  unsigned short info_map_item_count;
  
  if (info_map_item_count)
  {
    unsigned char info_map[x][y];
  }
  
  unsigned short mine_map_item_count;
  
  if (mine_map_item_count)
  {
    signed char mine_map[x][y];
  }
  
  Point target_location;
};
~~~

In M.A.X. the grid map size is always 112 x 112 tiles so `x = y = 112;` in the above xyz maps. Note that map dimensions are specified by the world (WRL) file format.

### Complete Save File

~~~c
struct __attribute__((packed)) SaveFile
{
  unsigned short version;
  FileType save_game_type;
  char save_game_name[30];
  PlanetType planet;
  unsigned short mission_index;
  char team_name_red[30];
  char team_name_green[30];
  char team_name_blue[30];
  char team_name_gray[30];
  TeamType team_type_red;
  TeamType team_type_green;
  TeamType team_type_blue;
  TeamType team_type_gray;
  TeamType team_type_alien;
  TeamClan team_clan_red;
  TeamClan team_clan_green;
  TeamClan team_clan_blue;
  TeamClan team_clan_gray;
  TeamClan team_clan_alien;
  unsigned int rng_seed;
  OpponentType opponent;
  unsigned short turn_timer_time;
  unsigned short endturn_time;
  PlayMode play_mode;
  IniOptions options;
  SurfaceType surface_map[112*112];
  GridResourceMapEntry GridResourceMap[112*112];
  TeamInfo team_info_red;
  TeamInfo team_info_green;
  TeamInfo team_info_blue;
  TeamInfo team_info_gray;
  TeamIndex active_turn_team;
  TeamIndex player_team;
  int turn_counter;
  unsigned short game_state;
  unsigned short turn_timer;
  IniPreferences preferences;
  TeamUnits team_units_red;
  TeamUnits team_units_green;
  TeamUnits team_units_blue;
  TeamUnits team_units_gray;
  UnitInfoList unit_info_list_ground_cover_units;
  UnitInfoList unit_info_list_mobile_land_sea_units;
  UnitInfoList unit_info_list_stationary_units;
  UnitInfoList unit_info_list_mobile_air_units;
  UnitInfoList unit_info_list_particles;
  Hash_UnitInfo hash_map_unit_info;
  Hash_MapHash hash_map_map_hash;

  if (team_type_red != TEAM_TYPE_NONE)
  {
    TeamHeatMaps heat_maps_red;
  }

  if (team_type_green != TEAM_TYPE_NONE)
  {
    TeamHeatMaps heat_maps_green;
  }

  if (team_type_blue != TEAM_TYPE_NONE)
  {
    TeamHeatMaps heat_maps_blue;
  }

  if (team_type_gray != TEAM_TYPE_NONE)
  {
    TeamHeatMaps heat_maps_gray;
  }

  MessageLogList message_log_red;
  MessageLogList message_log_green;
  MessageLogList message_log_blue;
  MessageLogList message_log_gray;
  
  if (team_type_red == TEAM_TYPE_COMPUTER)
  {
    AiPlayer ai_player_red;
  }

  if (team_type_green == TEAM_TYPE_COMPUTER)
  {
    AiPlayer ai_player_green;
  }

  if (team_type_blue == TEAM_TYPE_COMPUTER)
  {
    AiPlayer ai_player_blue;
  }

  if (team_type_gray == TEAM_TYPE_COMPUTER)
  {
    AiPlayer ai_player_gray;
  }
};
~~~

## References
<a name="ref1"></a>\[1\] [Endianness](https://en.wikipedia.org/wiki/Endianness)<br>
<a name="ref2"></a>\[2\] M.A.X. User Manual (MC-ICD-082-GEN)<br>
