---
layout: page
title: Networking
permalink: /network/
---

## Preface

The article tries to document the network communication related aspects of M.A.X. that could either
be interesting for an enthusiast or could be relevant for an engineer.<br>
The information found herein is not guaranteed to be complete or technically accurate.

## Network Protocols

M.A.X. supports three networking options.

- IPX protocol based local area network play for maximum four human players.
- Dial-up Modem play for two human players.
- Null Modem play for two human players.

<img src="{{ site.baseurl }}/assets/images/network.svg" alt="Network Protocols">

The implementations rely on MS-DOS DPMI services and direct I/O port manipulation. None of these are available in modern OS environments anymore.

In the case of IPX protocol the game distinguishes 3 types of message transmission mode:
- ***Unicast***: The message is sent to a single destination.
- ***Multicast***: The message is sent to all known destinations one by one as IPX protocol does not support real multicast messaging.
- ***Broadcast***: The message is sent to all nodes on the local network using the IPX broadcast address.

DOSBox emulates the IPX protocol and related DPMI services and tunnels IPX packets over UDP/IP protocol [\[1\]](#ref1). Dial-up modem emulation supports making and answering calls and routes transmission data over TCP/IP protocol [\[1\]](#ref1).

There are two potential goals for M.A.X. Port:

- Keep M.A.X. Port compatible with DOSBox and original M.A.X. v1.04. This would allow easier testing, incremental reimplementation of networking related aspects of the game and cross-play.
- Ditch the entire Link layer (or Data Link and Physical layers) together with Null Modem, Dial-Up Modem and IPX LAN play and replace all of these with a single TCP/IP or UDP/IP based alternative design. Which transport layer protocol to use depends on the netcode of the original game.

Research to date indicates that the game implements an input-synchronous, lock-step peer-to-peer networking model. For the used terminology see [\[2\]](#ref2). Turn-based and simultaneous game modes behave differently. Matchmaking uses its own netcode.

## Packet Structures

The transport protocol layer wraps M.A.X. packets with IPX or Modem / Serial protocol specific meta data.

<img src="{{ site.baseurl }}/assets/images/packets.svg" alt="Network Protocols">

All M.A.X. specific formats use little endian byte order.

### IPX Packet - IPX Protocol Header

- ***Packet Number (4 bytes)***: Only 6 LSBs are used in the value range 0 - 63. Default value is 0. The field is only incremented after an actual game is started by the Host. Each player's counter is managed individually.
- ***RNG Seed (2 bytes)***: The default value is 0x913F. The value is set to a random number when the Host starts a game. See *[Packet 32](#packet_ref32)*.
- ***CRC16 (2 bytes)***: The checksum is calculated over the M.A.X. Packet structure and the first byte of the packet number field.

### Modem Packet

- ***Packet Number (1 byte)***: Only 6 LSBs are used in the value range 0-63. Default value is 0. The field is only incremented after an actual game is started by the Host. Each player's counter is managed individually.
- ***Packet Length (2 bytes)***: Size of the M.A.X. Packet in bytes.
- ***CRC16 (2 bytes)***: The checksum is calculated over the M.A.X. Packet structure and finally two additional bytes are fed to the CRC16 algorithm, 0x00 and 0xFF.

### M.A.X. Packet

- ***Packet Type (1 byte)***: Type of data structure found in the Packet Data field. See M.A.X. Packet Types section.
- ***Entity ID (2 bytes)***: Packet Type specific meaning. E.g. Unit hash key or Node address of game client.
- ***Packet Length (2 bytes)***: Size of Packet Data field in bytes.
- ***Packet Data (0 - 550 bytes)***: Packet Type specific content.

The node address is an application layer address of the game client.

Node addresses 0 - 9 have special meaning.

Node address 0: Red Team (Player 0).<br>
Node address 1: Green Team (Player 1).<br>
Node address 2: Blue Team (Player 2).<br>
Node address 3: Gray Team (Player 3).<br>
Node address 4: Alien Derelicts (Player 4).

Normal addresses are calculated as `rand(0 to 32767) * 31991 >> 15 + 10`. The game implements its own pseudo random number generator. The host allocates all node addresses. The previous formula is rerolled as long as the resulting node address is not yet defined for any registered players.

### CRC16 Algorithm

Initial CRC value is 0x0000.

~~~ c
void crc16(unsigned short *crc, unsigned char c) {
    int i;
    int carry;

    for (i = 0; i < 8; ++i) {
        carry = *crc & 0x8000;
        *crc <<= 1;

        if (c & 0x80) {
            (*crc)++;
        }
        c <<= 1;

        if (carry) {
            *crc ^= 0x1021;
        }
    }
}
~~~

## M.A.X. Packet Types

M.A.X. v1.04 defines 53 packet types, although some of them are not implemented.

Wireshark Generic Dissector ([wsgd]({{ site.url }}/assets/files/max_v104.7z)) profile for DOSBox M.A.X. v1.04 IPX LAN network play.

### Packet Header

All fields are encoded in little endian byte order.

~~~ c
struct MAX_PacketHeader
{
  byte_order          little_endian;

  MAX_PacketTypeId    packet_type;
  uint16              entity_id;
  uint16              data_size;
}
~~~

***packet_type*** Packet type ID. See below list of packets.

***entity_id*** Packet Type specific meaning. E.g. Unit hash key or Node address of game client.

***data_size*** Size of the packet type specific data that follows the packet header.

<a name="packet_ref00"></a>
### Packet 00

~~~ c
struct MAX_Packet_00
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

<a name="packet_ref01"></a>
### Packet 01

~~~ c
struct MAX_Packet_01
{
  MAX_PacketHeader      Header;
  uint8                 frame_id;
}
~~~

<a name="packet_ref02"></a>
### Packet 02

Only implemented in early game versions.

<a name="packet_ref03"></a>
### Packet 03

Only implemented in early game versions.

<a name="packet_ref04"></a>
### Packet 04

Only implemented in early game versions.

<a name="packet_ref05"></a>
### Packet 05 - Announce IPX Local Address

IPX unicast message sent by the client to the host to acknowledge *[Packet 32](#packet_ref32)* and to announce the client's IPX local address to the host. When the host received all client's IPX local address via this message it responds with *[Packet 34](#packet_ref34)* to each client.

~~~ c
struct MAX_Packet_05
{
  MAX_PacketHeader      Header;
  MAX_TeamType          team;
  uint16                field_2;
  MAX_IpxAddress        address;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***team*** Team slot of the sender (client).

***field_2*** Always set to 0x0000. Note that the packet format of *[Packet 32](#packet_ref32)* and *[Packet 05](#packet_ref05)* are the same so the field is basically reserved.

***address*** Local IPX address of the client. See *[Packet 29](#packet_ref29)*.

<a name="packet_ref06"></a>
### Packet 06 - End Turn

IPX multicast message sent by the player that ended their turn to all other players as an event notification.

~~~ c
struct MAX_Packet_06
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***field_0*** Always set to 0x0000. Note that the field is not used for anything, it is basically reserved.

<a name="packet_ref07"></a>
### Packet 07 - Exit Game

IPX multicast message sent by the player that leaves the game to all other players. The player that sends the request to exit the game first increments their own ***request_uid*** counter. Other players respond with *[Packet 48](#packet_ref48)*.

~~~ c
struct MAX_Packet_07
{
  MAX_PacketHeader      Header;
  uint8                 request_mode;
  uint8                 request_uid;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***request_mode*** Either 0 or 1. TODO: figure out meaning.

***request_uid*** Unique request identifier. Initial value is 0x00.

<a name="packet_ref08"></a>
### Packet 08 - Unit Order

IPX multicast message sent by player that issued an order to all other players.

There are 32 different orders in M.A.X. v1.04.

~~~ c
struct MAX_OrderCategory_01
{
  uint16                parent_unit_id;

  uint16                target_grid_x;
  uint16                target_grid_y;
  uint16                enemy_unit_id;

  uint8                 repeat_build;
  uint8                 build_time;
  uint8                 build_rate;
  uint16                list_size;
  MAX_UnitType[list_size] build_orders;
}

struct MAX_OrderCategory_02
{
      uint16    parent_unit_id;

      uint16    target_grid_x;
      uint16    target_grid_y;
      uint16    enemy_unit_id;
}

struct MAX_OrderCategory_03
{
      uint16    parent_unit_id;

      uint16    target_grid_x;
      uint16    target_grid_y;
      uint16    enemy_unit_id;

      uint8     total_mining;
      uint8     raw_mining;
      uint8     fuel_mining;
      uint8     gold_mining;
}

struct MAX_OrderCategory_04
{
  uint16                parent_unit_id;
}

struct MAX_Packet_08
{
  MAX_PacketHeader      Header;

  MAX_Orders            order;
  uint8                 state;
  MAX_Orders            prior_order;
  uint8                 prior_state;
  uint8                 disabled_reaction_fire;

  switch(order)
  {
    case MAX_Orders::AWAITING :
    case MAX_Orders::TRANSFORMING : MAX_OrderCategory_01 order_data;
    case MAX_Orders::MOVING : MAX_OrderCategory_02 order_data;
    case MAX_Orders::FIRING : MAX_OrderCategory_02 order_data;
    case MAX_Orders::ORDER_BUILDING : MAX_OrderCategory_01 order_data;
    case MAX_Orders::ACTIVATE_ORDER : MAX_OrderCategory_02 order_data;
    case MAX_Orders::NEW_ALLOCATE_ORDER : MAX_OrderCategory_03 order_data;
    case MAX_Orders::POWER_ON :
    case MAX_Orders::POWER_OFF :
    case MAX_Orders::EXPLODING :
    case MAX_Orders::UNLOADING : MAX_OrderCategory_04 order_data;
    case MAX_Orders::CLEARING : MAX_OrderCategory_01 order_data;
    case MAX_Orders::SENTRY :
    case MAX_Orders::LANDING :
    case MAX_Orders::TAKING_OFF :
    case MAX_Orders::LOADING :
    case MAX_Orders::IDLE :
    case MAX_Orders::REPAIRING : MAX_OrderCategory_04 order_data;
    case MAX_Orders::REFUELING : MAX_OrderCategory_04 order_data;
    case MAX_Orders::RELOADING : MAX_OrderCategory_04 order_data;
    case MAX_Orders::TRANSFERRING : MAX_OrderCategory_02 order_data;
    case MAX_Orders::AWAITING_21 :
    case MAX_Orders::AWAITING_22 :
    case MAX_Orders::AWAITING_23 :
    case MAX_Orders::AWAITING_24 : MAX_OrderCategory_02 order_data;
    case MAX_Orders::AWAITING_25 : MAX_OrderCategory_02 order_data;
    case MAX_Orders::DISABLED : MAX_OrderCategory_01 order_data;
    case MAX_Orders::MOVING_27 : MAX_OrderCategory_02 order_data;
    case MAX_Orders::REPAIRING_28 : MAX_OrderCategory_04 order_data;
    case MAX_Orders::TRANSFERRING_29 :
    case MAX_Orders::ATTACKING : MAX_OrderCategory_02 order_data;
    case MAX_Orders::BUILDING_HALTED :
  }
}
~~~

***entity_id*** The unit's UnitHash type hash key for which the order applies to.

TODO: Describe all fields.

<a name="packet_ref09"></a>
### Packet 09 - TODO

IPX multicast message.

~~~ c
struct MAX_ResearchTopic
{
  uint32                research_level;
  uint32                turns_to_complete;
  uint32                allocation;
}

struct MAX_Packet_09
{
  MAX_PacketHeader      Header;
  uint16                team_credits;
  uint16                credits_spent;
  uint8[40]             field_4;
  string(30)            team_name;
  MAX_ResearchTopic[8]  research_topics;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

TODO: Describe all fields.

<a name="packet_ref10"></a>
### Packet 10 - Unit Type Upgrade

IPX multicast message sent by a player to all other players to inform them about a unit type specific upgrade. The UnitValues class members are sent to the other players. This packet is sent also when an upgrade is made within the Purchase Menu before starting a game.

If credits are spent to make an upgrade, then *[Packet 09](#packet_ref09)* is also sent to update the *gold_spent* metric.

~~~ c
enum16 MAX_UnitType
{
  COMMTWR 0x0
  POWERSTN 0x1
  POWGEN 0x2
  BARRACKS 0x3
  SHIELDGN 0x4
  RADAR 0x5
  ADUMP 0x6
  FDUMP 0x7
  GOLDSM 0x8
  DEPOT 0x9
  HANGAR 0xA
  DOCK 0xB
  CNCT_4W 0xC
  LRGRUBLE 0xD
  SMLRUBLE 0xE
  LRGTAPE 0xF
  SMLTAPE 0x10
  LRGSLAB 0x11
  SMLSLAB 0x12
  LRGCONES 0x13
  SMLCONES 0x14
  ROAD 0x15
  LANDPAD 0x16
  SHIPYARD 0x17
  LIGHTPLT 0x18
  LANDPLT 0x19
  SUPRTPLT 0x1A
  AIRPLT 0x1B
  HABITAT 0x1C
  RESEARCH 0x1D
  GREENHSE 0x1E
  RECCENTR 0x1F
  TRAINHAL 0x20
  WTRPLTFM 0x21
  GUNTURRT 0x22
  ANTIAIR 0x23
  ARTYTRRT 0x24
  ANTIMSSL 0x25
  BLOCK 0x26
  BRIDGE 0x27
  MININGST 0x28
  LANDMINE 0x29
  SEAMINE 0x2A
  LNDEXPLD 0x2B
  AIREXPLD 0x2C
  SEAEXPLD 0x2D
  BLDEXPLD 0x2E
  HITEXPLD 0x2F
  CONSTRCT 0x31
  SCOUT 0x32
  TANK 0x33
  ARTILLRY 0x34
  ROCKTLCH 0x35
  MISSLLCH 0x36
  SP_FLAK 0x37
  MINELAYR 0x38
  SURVEYOR 0x39
  SCANNER 0x3A
  SPLYTRCK 0x3B
  GOLDTRCK 0x3C
  ENGINEER 0x3D
  BULLDOZR 0x3E
  REPAIR 0x3F
  FUELTRCK 0x40
  CLNTRANS 0x41
  COMMANDO 0x42
  INFANTRY 0x43
  FASTBOAT 0x44
  CORVETTE 0x45
  BATTLSHP 0x46
  SUBMARNE 0x47
  SEATRANS 0x48
  MSSLBOAT 0x49
  SEAMNLYR 0x4A
  CARGOSHP 0x4B
  FIGHTER 0x4C
  BOMBER 0x4D
  AIRTRANS 0x4E
  AWAC 0x4F
  JUGGRNT 0x50
  ALNTANK 0x51
  ALNASGUN 0x52
  ALNPLANE 0x53
  ROCKET 0x54
  TORPEDO 0x55
  ALNMISSL 0x56
  ALNTBALL 0x57
  ALNABALL 0x58
  RKTSMOKE 0x59
  TRPBUBLE 0x5A
  HARVSTER 0x5B
  WALDO 0x5C
}

struct MAX_Packet_10
{
  MAX_PacketHeader      Header;
  MAX_UnitType          unit_type;
  uint16                turns;
  uint16                hits;
  uint16                armor;
  uint16                attack;
  uint16                speed;
  uint16                range;
  uint16                rounds;
  uint16                move_and_fire;
  uint16                scan;
  uint16                storage;
  uint16                ammo;
  uint16                attack_radius;
  uint16                agent_adjust;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***unit_type*** See MAX_UnitType.

***turns*** Turns required to build the unit type.

***hits*** Base health points of the unit type.

***armor*** Armour rating of the unit type.

***attack*** Attack rating of the unit type.

***speed*** Base movement points of the unit type.

***range*** Attack range of the unit type. Only meaningful if the unit has rounds > 0.

***rounds*** Shots available for the unit type per game turn.

***move_and_fire*** True or False. Available shots are not decreased if movement points are spent.

***scan*** Scan range of the unit type.

***storage*** Base storage capacity of the unit type. TODO: Document infiltrator use case.

***ammo*** Base ammunition of the unit type.

***attack_radius*** Area attack radius. 0 means a single square at the attack cursor. 1 means 9 squares in a rectange. 2 means 25 squares in a rectange. The rocket launcher is the only unit where this is non zero.

***agent_adjust*** TODO: Document infiltrator use case.

<a name="packet_ref11"></a>
### Packet 11 - Update Complex

IPX multicast message sent by the player to every other player.

For each complex of the player a dedicated message is sent. At the beginning of each turn resource consumption optimizations are made which could lead to change of resources available in a complex.

~~~ c
struct MAX_Packet_11
{
  MAX_PacketHeader      Header;
  uint16                id;
  int16                 material;
  int16                 fuel;
  int16                 gold;
  int16                 power;
  int16                 workers;
  int16                 buildings;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***id*** Complex identifier starting with index 1.

***material*** Raw materials resource available in the complex.

***fuel*** Fuel resource available in the complex.

***gold*** Gold resource available in the complex.

***power*** Power resource available in the complex. This is the available surplus only.

***workers*** Workers resource available in the complex. This is the available surplus only.

***buildings*** Number of buildings in the complex.

<a name="packet_ref12"></a>
### Packet 12 - Select Landing Zone

IPX multicast message sent by player to every other player during landing zone selection at the beginning of a game.

Overlapping landing zones are evaluated by each player by an algorithm that places supplied and purchased units around the selected starting location.

~~~ c
struct MAX_Point
{
  byte_order          little_endian;

  uint16              x;
  uint16              y;
}

struct MAX_MissionSupply
{
  byte_order          little_endian;

  MAX_UnitType        unit_type;
  uint16              unit_storage;
}

struct MAX_Packet_12
{
  MAX_PacketHeader      Header;
  uint16                field_0;
  uint16                credits;
  uint16                unit_count;
  uint16                credits_spent;
  MAX_Point             start_position;
  uint8                 field_12;
  MAX_MissionSupply[unit_count] units;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***field_0*** TODO: Unknown field.

***credits*** The sum of the starting credits and the clan credits. Note that none of the clans have credits in M.A.X. v1.04.

***unit_count*** Number of units supplied and purchased for the mission.

***credits_spent*** Credits spent in the purchase menu. The value is tracked by the game for mission statistics that are shown at the end of the game.

***start_position*** Grid coordinates selected as landing zone for the team. The given position is the top left point of a 3 by 3 rectange. The initial mining station and its power generator are not found in the ***units*** array. Units in the array are placed from top left corner in a clock wise order in the 3 by 3 rectangle. If there are more than four units a 5 by 5 rectangle is filled up again starting from top left corner and so on to 7 by 7 rectangle. The maximum ***data_size*** is 550 bytes so in theory 134 units can be allocated to a team at game startup. Of course M.A.X. v1.04 does not provide enough starting credits to do so.

***field_12*** TODO: Unknown field.

***units*** Array of units and their stored materials corresponding to the supplied and purchased units list.

<a name="packet_ref13"></a>
### Packet 13 - Update RNG Seed

IPX multicast message sent by the host to each client right after *[Packet 34](#packet_ref34)*.

~~~ c
struct MAX_Packet_13
{
  MAX_PacketHeader      Header;
  uint32                rng_seed;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***rng_seed*** The field value is used by game clients to set the RNG seed value via srand(). The value is derived from the time() C-API function from \<time.h\>. The time function determines the current calendar time which represents the time since January 1, 1970 (UTC) also known as Unix epoch time.

<a name="packet_ref14"></a>
### Packet 14 - TODO

IPX multicast message. TODO

~~~ c
struct MAX_Packet_14
{
  MAX_PacketHeader      Header;
  uint16                unit_type;
  uint16                grid_X;
  uint16                grid_y;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

TODO

<a name="packet_ref15"></a>
### Packet 15

Not implemented in M.A.X. v1.04.

<a name="packet_ref16"></a>
### Packet 16 - Save Game

IPX multicast message sent by the player that issued a save operation to all other players.

~~~ c
struct MAX_Packet_16
{
  MAX_PacketHeader      Header;
  string(30)            file_name;
  string(30)            title;
}
~~~

***entity_id*** Hardcoded to 0 by the sender and the field value is not used by the receivers.

***file_name*** Name of the save file to be created on the file system. The field is always 30 characters long.

***title*** Title of the save game set via the game GUI. The field is always 30 characters long.

<a name="packet_ref17"></a>
### Packet 17 - Update Game Settings

IPX multicast message sent by the host to each client right after *[Packet 13](#packet_ref13)*.

~~~ c
struct MAX_Packet_17
{
  MAX_PacketHeader      Header;
  uint8                 game_mode;
  uint8                 world;
  uint8                 game_file_number;
  uint8                 game_file_type;
  uint8                 play_mode;
  uint8                 all_visible;
  uint8                 quick_build;
  uint8                 real_time;
  uint8                 log_file_debug;
  uint8                 disable_fire;
  uint8                 fast_movement;
  uint16                timer;
  uint16                endturn;
  uint16                start_gold;
  uint16                autosave;
  uint8                 victory_type;
  uint16                victory_limit;
  uint16                raw_normal_low;
  uint16                raw_normal_high;
  uint16                raw_concentrate_low;
  uint16                raw_concentrate_high;
  uint16                raw_concentrate_seperation;
  uint16                raw_concentrate_diffusion;
  uint16                fuel_normal_low;
  uint16                fuel_normal_high;
  uint16                fuel_concentrate_low;
  uint16                fuel_concentrate_high;
  uint16                fuel_concentrate_seperation;
  uint16                fuel_concentrate_diffusion;
  uint16                gold_normal_low;
  uint16                gold_normal_high;
  uint16                gold_concentrate_low;
  uint16                gold_concentrate_high;
  uint16                gold_concentrate_seperation;
  uint16                gold_concentrate_diffusion;
  uint16                mixed_resource_seperation;
  uint16                min_resources;
  uint16                max_resources;
  uint16                alien_seperation;
  uint16                alien_unit_value;
}
~~~
***entity_id*** Hardcoded to 0 by the sender and the field value is not used by the receivers.

TODO: Describe all fields.

<a name="packet_ref18"></a>
### Packet 18 - In-game Chat Message

IPX unicast message sent by a player to another player. The message content is an in-game chat message. If an in-game chat message needs to be sent to multiple players than simply multiple packets are sent to the applicable IPX local address.

~~~ c
struct MAX_Packet_18
{
  MAX_PacketHeader      Header;
  string(data_size)     chat_message;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***chat_message*** Null terminated string. The packet size depends on the length of the chat message string. The sender's name is prefixed by the receiving client before the message is shown to the given player.

On the sender side the chat message is limited to 550 characters including the null character at the end.
On the receiver side the client statically reserves an 550 bytes long buffer for the message but it additionally prefixes the received chat message with a maximum 30 characters long player name and a colon plus a space character. So if a player would really be able to send such a long chat message it would corrupt all receiving player's IPX network manager state data even certain callback functions could be overwritten so it would be possible to execute hostile code on the remote computer. The in-game chat GUI only allows 60 characters to be sent though.

<a name="packet_ref19"></a>
### Packet 19

Not implemented in M.A.X. v1.04.

<a name="packet_ref20"></a>
### Packet 20

~~~ c
struct MAX_Packet_20
{
  MAX_PacketHeader      Header;
  uint16                unit_type;
  uint16                turns;
  uint16                hits;
  uint16                armor;
  uint16                attack;
  uint16                speed;
  uint16                range;
  uint16                rounds;
  uint16                move_and_fire;
  uint16                scan;
  uint16                storage;
  uint16                ammo;
  uint16                attack_radius;
  uint16                agent_adjust;
}
~~~

<a name="packet_ref21"></a>
### Packet 21

~~~ c
struct MAX_Packet_21
{
  MAX_PacketHeader      Header;
  uint16                complex_id;
  uint16                material;
  uint16                fuel;
  uint16                gold;
}
~~~

<a name="packet_ref22"></a>
### Packet 22

~~~ c
struct MAX_Packet_22
{
  MAX_PacketHeader      Header;
  uint16                team;
  uint8                 state;
  uint8                 repeat_build;
  uint16                build_time;
  uint16                build_rate;
  uint16                target_grid_x;
  uint16                target_grid_y;
  uint16                count;
  MAX_UnitType[*]       buildings;
}
~~~

<a name="packet_ref23"></a>
### Packet 23

~~~ c
struct MAX_Packet_23
{
  MAX_PacketHeader      Header;
  MAX_Orders            orders;
  uint8                 state;
  MAX_Orders            prior_orders;
  uint8                 prior_state;
  uint8                 disabled_reaction_fire;
  uint16                parent_unit_id;
  uint16                target_grid_x;
  uint16                target_grid_y;
  uint16                enemy_unit_id;
  uint8                 total_mining;
  uint8                 raw_mining;
  uint8                 fuel_mining;
  uint8                 gold_mining;
  uint16                build_time;
  uint16                build_rate;
  uint16                unit_type;
  uint16                unit_id;
  uint16                grid_x;
  uint16                grid_y;
  uint16                team;
  uint16                hits;
  uint16                speed;
  uint16                shots;
  uint16                storage;
  uint16                ammo;
}
~~~

<a name="packet_ref24"></a>
### Packet 24

~~~ c
struct MAX_Packet_24
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

<a name="packet_ref25"></a>
### Packet 25 - Remote Debug Log

Not implemented in M.A.X. v1.04.

<a name="packet_ref26"></a>
### Packet 26 - Set Team Clan

IPX multicast message sent by player to each other player after the host sent *[Packet 17](#packet_ref17)*.

~~~ c
struct MAX_Packet_26
{
  MAX_PacketHeader      Header;
  uint8                 team_clan;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***team_clan*** Clan index. Valid value range is 0 - 7.

<a name="packet_ref27"></a>
### Packet 27

Not implemented in M.A.X. v1.04.

<a name="packet_ref28"></a>
### Packet 28 - Join Request

IPX broadcast message sent by client that wants to join a game. The packet is only sent by a client if the host and its node address is not yet known. E.g. if the client did not receive any *[Packet 44](#packet_ref44)* messages yet.

~~~ c
struct MAX_Packet_28
{
  MAX_PacketHeader      Header;
  uint16                source_node;
  raw(4)                data;
}
~~~

***entity_id*** Hardcoded to 0 by the sender and the field value is not used by the receivers.

***source_node*** Node address of the client. As the host is not known yet the client's node address is also not known yet so the field is set to 0.

***data*** See [Packet 31](#packet_ref31).

<a name="packet_ref29"></a>
### Packet 29

IPX broadcast message sent by client to host indicating that client wants to join the game that host is setting up. The host is identified via its *[Packet 44](#packet_ref44)* message contents.

~~~ c
alias MAX_IpxAddress uint8[6];

struct MAX_Packet_29
{
  MAX_PacketHeader      Header;
  MAX_IpxAddress        address;
}
~~~

***entity_id*** The host's node address. 

***address*** On MS-DOS systems this is the real IPX address of the network interface. In DOSBox the first 4 bytes is the IPv4 address, the last two bytes are the UDP source port.

<a name="packet_ref30"></a>
### Packet 30

IPX unicast message sent to client from host as response to *[Packet 29](#packet_ref29)*. The packet synchronizes all game setup settings with the client.

~~~ c
struct MAX_Packet_30
{
  MAX_PacketHeader      Header;
  uint16                host_node;
  string(30)[4]         team_name;
  uint16[4]             node;
  string(30)            world_name;
  uint8                 ini_world_index;
  uint8                 ini_play_mode;
  uint16                ini_timer;
  uint16                ini_endturn;
  uint8                 ini_opponent;
  uint16                ini_start_gold;
  uint8                 ini_raw_resource;
  uint8                 ini_fuel_resource;
  uint8                 ini_gold_resource;
  uint8                 ini_alien_derelicts;
  uint8                 ini_victory_type;
  uint16                ini_victory_limit;
  uint8                 field_1310;
  uint8                 field_1311;
  string(30)[4]         field_1312;
  uint8                 field_1432;
  uint8[4]              field_1433;
}
~~~

***entity_id*** The client's new node address assigned by the host.

***host_node*** Node address of the host. The client saves this node address to be able to address the host directly.

***team_name [4]*** Name of each known player. The field is always 4x 30 characters long.

***node [4]*** Node address of each known player. Note that the client does not get its new node address from the host this way. The packet's ***entity_id*** field is used for that purpose. Normally the host does not yet know which team slot will be requested by the newly registered client via *[Packet 35](#packet_ref35)*.

TODO: Describe all fields.

<a name="packet_ref31"></a>
### Packet 31 - Leave Game

IPX broadcast message sent by a player during game setup phase to denounce itself.

~~~ c
struct MAX_Packet_31
{
  MAX_PacketHeader      Header;
  uint16                field_0;
  raw(4)                data;
}
~~~

***entity_id*** The host's node address.

***field_0***  Hardcoded to 0 by the sender and the field value is not used by the receivers.

***data*** The packet size is always 6 bytes, but the last 4 bytes (this field) are not set. Unset random buffer content is sent by the player.

<a name="packet_ref32"></a>
### Packet 32 - Start Game

IPX broadcast message sent by the host to start the configured network game. Each client sends *[Packet 05](#packet_ref05)* as response to the host.

~~~ c
struct MAX_Packet_32
{
  MAX_PacketHeader      Header;
  MAX_TeamType          team;
  uint16                random_seed;
  MAX_IpxAddress        address;
}
~~~

***entity_id*** The host's node address.

***team*** Team slot of the sender (host).

***random_seed*** A newly generated random number in the range 0 to 32767. Starting from the next packet the ***RNG Seed*** field inside the IPX Protocol Header will be set to this field's value for every node participating in the communication.

***address*** Local IPX address of the host. See *[Packet 29](#packet_ref29)*.

<a name="packet_ref33"></a>
### Packet 33 - Chat Message

IPX broadcast message sent by the player that wants to send a chat message.

~~~ c
struct MAX_Packet_33
{
  MAX_PacketHeader      Header;
  string(data_size)     message;
}
~~~

***entity_id*** The host's node address. All peers registered for the given host's game receives the message.

***message*** Null terminated string. The field is always 120 bytes long. The message is prefixed with the sender's team name ("Red Team: Hello M.A.X. Commander Green Team!"). In theory this means that the message contents could be 87 (120 - 1 - 30 -2) bytes long maximum. It is not possible to send a message to a single player only.

<a name="packet_ref34"></a>
### Packet 34 - Announce IPX Addresses

IPX multicast message sent by the host to all clients to acknowledge *[Packet 05](#packet_ref05)* and to announce all players' IPX local address.

Starting from this host message each player needs to increment their ***Packet Number*** field inside the IPX Protocol Header. The host keeps track of its packet counter for each player individually. Clients just increment their counter whenever they send a packet. This packet starts with ***Packet Number*** value 0.

~~~ c
struct MAX_Packet_34
{
  MAX_PacketHeader      Header;
  MAX_IpxAddress[4]     address;
}
~~~

***entity_id*** The host's node address.

***address [4]*** Local IPX addresses of each player. See *[Packet 29](#packet_ref29)*.

<a name="packet_ref35"></a>
### Packet 35 - Change Team Slot

IPX broadcast message sent by a player that selected a new team slot (red, green, blue, gray).

The host always sends this message at least once after it sent the first *[Packet 44](#packet_ref44)* announcement message.

Joining clients send this message as response to *[Packet 30](#packet_ref30)*.

~~~ c
struct MAX_Packet_35
{
  MAX_PacketHeader      Header;
  uint16                source_node;
  uint16                team_slot;
  uint8                 ready_state;
  string(30)            team_name;
}
~~~

***entity_id*** The host's node address.

***source_node*** Node address of player that changed team slot.

***team_slot*** The team slot of the player. Valid value range is 0 - 3. The value is used as offset into various network game menu manager object fields so a malformed field could corrupt quite a lot of things considering that the team_slot field is 16 bits.

***ready_state*** 0 (false) if the client is not ready to start the game. 1 (true) if the client is ready. If a client changed to ready state the only way to unready itself is to push the Cancel button which also denounces the player (see *[Packet 31](#packet_ref31)*). The host can only start the actual game when each joined client sets this field to 1.

***team_name*** The player (team) name as a null terminated string. The field is always 30 characters long. The game uses strcpy() to store the field for which a 30 bytes buffer is reserved so the name should not be longer than 29 characters. If the packet is malformed the network game menu manager object could be corrupted.
The game does not allow an empty string as the ***team_name***. If the player attempts to set such a name via the GUI the resulting name will be "**No Name**". So a value of 0x00 for the ***team_name*** field is also malformed.

M.A.X. v1.04 implements an alternative Packet 35 format too, but it is unused. Probably dead code.

<a name="packet_ref36"></a>
### Packet 36 - Change Player Name

IPX broadcast message sent by a player that changed their name.

~~~ c
struct MAX_Packet_36
{
  MAX_PacketHeader      Header;
  string(30)            team_name;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***team_name*** See *[Packet 35](#packet_ref35)* and *[Packet 44](#packet_ref44)*. Note that players could have identical names which could be confusing as chat messages are prefixed with the team names only.

<a name="packet_ref37"></a>
### Packet 37 - Change Game Options

IPX broadcast message sent by host when map is changed, a previous network game is loaded, a multiplayer scenario is selected or game options like starting credit is changed.

~~~ c
enum8 MAX_WorldIndex
{
  SNOWCRAB 0
  FRIGIA 1
  ICE_BERG 2
  THE_COOLER 3
  ULTIMA_THULE 4
  LONG_FLOES 5
  IRON_CROSS 6
  SPLATTERSCAPE 7
  PEAK-A-BOO 8
  VALENTINES_PLANET 9
  THREE_RINGS 10
  GREAT_DIVIDE 11
  NEW_LUZON 12
  MIDDLE_SEA 13
  HIGH_IMPACT 14
  SANCTUARY 15
  ISLANDIA 16
  HAMMERHEAD 17
  FRECKLES 18
  SANDSPIT 19
  GREAT_CIRCLE 20
  LONG_PASSAGE 21
  FLASH_POINT 22
  BOTTLENECK 23
}

struct MAX_Packet_37
{
  MAX_PacketHeader      Header;
  string(30)            world_name;
  MAX_WorldIndex        ini_world_index;
  uint8                 ini_play_mode;
  uint16                ini_timer;
  uint16                ini_endturn;
  uint8                 ini_opponent;
  uint16                ini_start_gold;
  uint8                 ini_raw_resource;
  uint8                 ini_fuel_resource;
  uint8                 ini_gold_resource;
  uint8                 ini_alien_derelicts;
  uint8                 ini_victory_type;
  uint16                ini_victory_limit;
  uint8                 field_1310;
  uint8                 is_multi_scenario;
  string(30)[4]         default_team_name;
  uint8                 multi_scenario_id;
  uint8[4]              field_1433;
}
~~~

***entity_id*** The host's node address.

***world_name*** Name of the selected map or multiplayer scenario or save slot. The field is always 30 bytes long. The field is a Null terminated string.

***ini_world_index*** The index of the map. The game supports 24 worlds, 6 worlds in 4 galaxies.

TODO: Describe all fields.

<a name="packet_ref38"></a>
### Packet 38 - Unit Path

IPX multicast message sent by player to all other players when unit path is changed. The UnitPath (or derived) path object of the given unit is replaced with a new object constructed from the data found in this packet. The packet is sent usually right after *[Packet 08](#packet_ref08)*.

~~~ c
struct MAX_PathStep
{
  byte_order            little_endian;

  int8                  x;
  int8                  y;
}

struct MAX_Packet_38
{
  MAX_PacketHeader      Header;
  MAX_Orders            order;
  uint8                 state;
  uint16                target_grid_x;
  uint16                target_grid_y;
  uint16                end_x;
  uint16                end_y;
  uint32                distance_x;
  uint32                distance_y;
  uint16                euclidean_distance;
  uint8                 field_79;
  uint8                 speed;
  uint8                 move_fraction;
  uint8                 max_velocity;
  uint16                list_size;
  MAX_PathStep[list_size] steps;
}
~~~

***entity_id*** The unit's UnitHash type hash key for which the order applies to.

***order*** Current unit order. See *[Packet 08](#packet_ref08)*.

***state*** State variable of the current order. TODO: Figure out states.

***target_grid_x*** X coordinate of the calculated path's end point on the grid. Note that the game GUI shows grid position +1.

***target_grid_x*** Y coordinate of the calculated path's end point on the grid. Note that the game GUI shows grid position +1.

***end_x*** 

***end_y*** 

***distance_x***

***distance_y***

***euclidean_distance***

***field_79***

***speed***

***move_fraction***

***max_velocity***

***list_size*** Number of path steps that follows.

***steps*** Each element represents an 8-direction path step. E.g. {-1,0} means take one step to the left. Units not looking in the given direction need to turn.

TODO: Describe all fields.

<a name="packet_ref39"></a>
### Packet 39 - Pause Game

IPX multicast message sent by player to all other players to pause the game. See *[Packet 40](#packet_ref40)* to continue the game.

~~~ c
struct MAX_Packet_39
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***field_0*** Always set to 0. Note that the field is not used for anything, it is basically reserved.

<a name="packet_ref40"></a>
### Packet 40 - Unpause Game

IPX multicast message sent by each player to all other players to unpause the game.

~~~ c
struct MAX_Packet_40
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***field_0*** Always set to 0. Note that the field is not used for anything, it is basically reserved.

<a name="packet_ref41"></a>
### Packet 41 - Path Blocked

IPX multicast message sent by player to all other players that owns the blocked unit.

In M.A.X. v1.04 the function which builds packet 41 forgets to set the packet header's ***data_size*** field so random garbage in random size is sent over the network. Typically a single byte of data is sent as the most common packet on the network has one data byte.

~~~ c
struct MAX_Packet_41
{
  MAX_PacketHeader      Header;
  raw(*)                data;
}
~~~

***entity_id*** The unit's UnitHash type hash key for which the order applies to.

<a name="packet_ref42"></a>
### Packet 42 - 

IPX multicast message. TODO

~~~ c
struct MAX_Packet_42
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***field_0*** Always set to 0. Note that the field is not used for anything, it is basically reserved.

<a name="packet_ref43"></a>
### Packet 43 - Rename Unit

IPX multicast message sent by player to all other players when unit name has changed.

~~~ c
struct MAX_Packet_43
{
  MAX_PacketHeader      Header;
  string(data_size)     unit_name;
}
~~~

***entity_id*** The unit's UnitHash type hash key for which the order applies to.

***unit_name*** New name of the unit. Null terminated string. The maximum length of the name is 30 bytes including the null character. The game GUI does not allow longer names to be set. A longer (malformed) packet does not corrupt memory as the name is stored in heap memory. A malformed packet where the null character is missing could potentially lead to segmentation fault or other issues still.

<a name="packet_ref44"></a>
### Packet 44 - Announce Host

IPX broadcast message sent by host every 3 seconds or so during game setup phase to announce itself.

~~~ c
struct MAX_Packet_44
{
  MAX_PacketHeader      Header;
  string(data_size-30)  version_string;
  string(30)            team_name;
}
~~~

***entity_id*** The host's node address.

***version_string*** The value in M.A.X. v1.04 is "v1.04" (5 bytes) without the quotes. The version string is not null terminated.

***team_name*** The host player's name like "**Red Team**" or "**Player 1**" or "**Mech Commander** - **Carlton L.**". The field is always 30 characters long and it is processed as a null terminated string. Therefore at least the last byte shall be 0x00.

<a name="packet_ref45"></a>
### Packet 45 - 

IPX multicast message sent by player that started their turn to the others. The other players respond with their own calculation results for the sender specific local data. If any player's calculation of the current player's data does not match a network desynchronization error is reported. The game cannot be continued in such cases, but the last backup save could be reloaded or a restart sequence could be attempted.

~~~ c
struct MAX_Packet_45
{
  MAX_PacketHeader      Header;
  uint8                 next_turn;
  uint16{d=hex}         checksum;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***next_turn*** Current turn index plus one. See *[Packet 52](#packet_ref52)*.

***checksum*** A 16 bit special CRC checksum calculated over specific unit lists. The algorithm is simple, but the data fed to the algorithm is complex and numerous. As `xor` operation is involved in the algorithm the order of data fed to the algorithm matters.

Initial CRC value is 0xFFFF.

~~~ c
unsigned short crc16(unsigned short crc, unsigned short data) {
    for (int i = 0; i < 16; ++i) {
        if (data & 0x8000) {
            data *= 2;
            data ^= 0x1021;
        } else {
            data *= 2;
        }
    }

    return crc ^ data;
}
~~~

<a name="packet_ref46"></a>
### Packet 46 - 

IPX multicast message. TODO
~~~ c
struct MAX_Packet_46
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
  uint32                field_1;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

TODO

<a name="packet_ref47"></a>
### Packet 47

Not implemented in M.A.X. v1.04.

<a name="packet_ref48"></a>
### Packet 48 - Acknowledge Exit Game

IPX multicast message sent by player to each other player after receiving *[Packet 07](#packet_ref07)*.

~~~ c
struct MAX_Packet_48
{
  MAX_PacketHeader      Header;
  uint8                 request_uid;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***request_uid*** Unique request identifier from previously received *[Packet 07](#packet_ref07)*.

<a name="packet_ref49"></a>
### Packet 49

IPX multicast message.

TODO

~~~ c
struct MAX_Packet_49
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Hardcoded to 0 by the sender and the field value is not used by the receivers.

TODO

<a name="packet_ref50"></a>
### Packet 50 - Enemy Spotted

IPX multicast message sent by player to all other players.

~~~ c
struct MAX_Packet_50
{
  MAX_PacketHeader      Header;
}
~~~

***entity_id*** The unit's UnitHash type hash key for which the order applies to.

<a name="packet_ref51"></a>
### Packet 51 - 

IPX multicast message sent by player to all other players.

TODO

~~~ c
struct MAX_Packet_51
{
  MAX_PacketHeader      Header;
  uint8                 field_0;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

TODO

<a name="packet_ref52"></a>
### Packet 52 - Acknowledge End Turn

IPX multicast message sent by player to each other player after receiving *[Packet 06](#packet_ref06)*.

~~~ c
struct MAX_Packet_52
{
  MAX_PacketHeader      Header;
  uint8                 turn_index;
}
~~~

***entity_id*** Sender's team slot (node address 0 - 3).

***turn_index*** Current turn counter value. Each player's turn increments this field. The turn counter displayed in-game is not equal to this counter value in turn based games.

## M.A.X. Protocol Behaviors

### Game Setup Phase

Typical message sequence when a Client joins a Host during game setup:

<img src="{{ site.baseurl }}/assets/images/protocol_basic_join.svg" alt="Network Protocol - Basic Join">

Typical message sequence when the Host starts a configured game with two Clients:

## References
<a name="ref1"></a>\[1\] [DOSBox Connectivity](https://www.dosbox.com/wiki/Connectivity)<br>
<a name="ref1"></a>\[2\] [Age of Empires Multiplayer Design](https://www.gamedeveloper.com/programming/1500-archers-on-a-28-8-network-programming-in-age-of-empires-and-beyond)<br>