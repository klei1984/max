/* Copyright (c) 2021 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "units_manager.hpp"

#include "access.hpp"
#include "builder.hpp"
#include "cursor.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "smartlist.hpp"
#include "sound_manager.hpp"
#include "teamunits.hpp"
#include "unitinfo.hpp"
#include "window.hpp"
#include "window_manager.hpp"

static bool UnitsManager_SelfDestructActiveMenu(WindowInfo* window);
static bool UnitsManager_SelfDestructMenu();
static void UnitsManager_UpdateUnitPosition(UnitInfo* unit, int grid_x, int grid_y);
static void UnitsManager_UpdateMapHash(UnitInfo* unit, int grid_x, int grid_y);

unsigned short UnitsManager_Team;

SmartList<UnitInfo> UnitsManager_GroundCoverUnits;
SmartList<UnitInfo> UnitsManager_MobileLandSeaUnits;
SmartList<UnitInfo> UnitsManager_ParticleUnits;
SmartList<UnitInfo> UnitsManager_StationaryUnits;
SmartList<UnitInfo> UnitsManager_MobileAirUnits;
SmartList<UnitInfo> UnitsManager_UnitList6;

BaseUnit UnitsManager_BaseUnits[UNIT_END];

bool UnitsManager_OrdersPending;

AbstractUnit UnitsManager_AbstractUnits[UNIT_END] = {
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ COMMTWR,
        /* shadow          */ S_COMMTW,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_COMMT,
        /* portrait        */ P_TRANSP,
        /* icon            */ I_TRANSP,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Gold Refinery",
        /* plural name     */ "Gold Refineries",
        /* description */
        "Refinery for converting gold into credits.  Credits are required to purchase unit improvements.  To run, a "
        "gold refinery needs a source of power and a source of gold ore.  A mining station can produce gold ore if one "
        "of its four squares covers a square with underground gold."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ POWERSTN,
        /* shadow          */ S_POWERS,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_POWERS,
        /* portrait        */ P_POWSTN,
        /* icon            */ I_POWSTN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Power Station",
        /* plural name     */ "Power Stations",
        /* description */
        "A power station consumes six fuel each turn, and provides enough power for six factories or mining stations.\n"
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ POWGEN,
        /* shadow          */ S_POWGEN,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_POWGEN,
        /* portrait        */ P_POWGEN,
        /* icon            */ I_POWGEN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Power Generator",
        /* plural name     */ "Power Generators",
        /* description */
        "A power generator consumes two fuel each turn, and provides enough power for one mining station or factory.  "
        "The power generator must be connected to the fuel source (usually a mining station) and the building that "
        "needs the power."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ BARRACKS,
        /* shadow          */ S_BARRAC,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_BARRAC,
        /* portrait        */ P_BARRCK,
        /* icon            */ I_BARRCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ "Barracks",
        /* plural name     */ "Barracks",
        /* description */
        "A barracks holds infiltrators and infantry units.  Inside the barracks is a machine shop for repairing and "
        "improving powered suits, and for manufacturing ammunition.  The barracks needs raw materials to repair, "
        "upgrade, or resupply, so it should be connected to a storage unit or a mining station."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ SHIELDGN,
        /* shadow          */ S_SHIELD,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SHIELD,
        /* portrait        */ P_SHIELD,
        /* icon            */ I_SHIELD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Monopole Mine",
        /* plural name     */ "Monopole Mines",
        /* description */
        "Specialized mining station for extracting magnetic monopoles."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ ANIMATED | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB,
        /* sprite          */ RADAR,
        /* shadow          */ S_RADAR,
        /* data            */ D_RADAR,
        /* flics animation */ F_RADAR,
        /* portrait        */ P_RADAR,
        /* icon            */ I_RADAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Radar",
        /* plural name     */ "Radars",
        /* description */
        "Stationary, long-range radar.  Longer-range units like missile launchers and artillery cannot fire on what "
        "they cannot see, so it's important to have a scanner or radar nearby."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ ADUMP,
        /* shadow          */ S_ADUMP,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_ADUMP,
        /* portrait        */ P_SMSTOR,
        /* icon            */ I_SMSTOR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Storage Unit",
        /* plural name     */ "Storage Units",
        /* description */
        "Holds raw materials.  Mining stations produce raw materials every turn.  To save extra raw materials, a "
        "mining station must be connected to a storage unit.",
        /* tutorial description (optional) */
        "Storage Units hold raw materials produced by any mining stations that are connected to them.  To fill an "
        "adjacent Engineer or Constructor, click the Xfer button and then click on the Engineer or Constructor."),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ FDUMP,
        /* shadow          */ S_FDUMP,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_FDUMP,
        /* portrait        */ P_SMFUEL,
        /* icon            */ I_SMFUEL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ FUEL,
        /* gender          */ 'N',
        /* singular name   */ "Fuel Tank",
        /* plural name     */ "Fuel Tanks",
        /* description */
        "Holds fuel reserves.  Mining stations produce fuel every turn.  To save extra fuel, a mining station must be "
        "connected to a fuel tank.",
        /* tutorial description (optional) */
        "Holds fuel reserves.  Mining stations produce fuel every turn.  To save extra fuel, a mining station must be "
        "connected to a fuel tank."),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE | REQUIRES_SLAB,
        /* sprite          */ GOLDSM,
        /* shadow          */ S_GOLDSM,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_GOLDSM,
        /* portrait        */ P_SMVLT,
        /* icon            */ I_SMVLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ GOLD,
        /* gender          */ 'N',
        /* singular name   */ "Gold Vault",
        /* plural name     */ "Gold Vaults",
        /* description */
        "Holds unrefined gold ore.  Mining stations produce gold every turn, if they have gold ore underneath them.  "
        "To store the ore, a mining station must be connected to a gold vault.",
        /* tutorial description (optional) */
        "Holds unrefined gold ore.  Mining stations produce gold every turn, if they have gold ore underneath them.  "
        "To store the ore, a mining station must be connected to a gold vault."),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ DEPOT,
        /* shadow          */ S_DEPOT,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_DEPOT,
        /* portrait        */ P_DEPOT,
        /* icon            */ I_DEPOT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ "Depot",
        /* plural name     */ "Depots",
        /* description */
        "A depot can repair damage, manufacture ammunition, and refit ground units with the newest technology.  All of "
        "these operations require raw materials, so depots should be connected to stored materials.",
        /* tutorial description (optional) */
        "Depots perform repairs and supply ammunition. To drive a unit into the depot, click 'Load' and then click on "
        "the unit.  To look inside the depot, click 'Activate'."),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ HANGAR,
        /* shadow          */ S_HANGAR,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_HANGAR,
        /* portrait        */ P_HANGAR,
        /* icon            */ I_HANGAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ 6,
        /* gender          */ 'N',
        /* singular name   */ "Hangar",
        /* plural name     */ "Hangars",
        /* description */
        "A hangar can repair damage, manufacture ammunition, and refit planes with the newest technology.  All of "
        "these operations require raw materials, so hangars should be connected to stored materials."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ DOCK,
        /* shadow          */ S_DOCK,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_DOCK,
        /* portrait        */ P_DOCK,
        /* icon            */ I_DOCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ 5,
        /* gender          */ 'N',
        /* singular name   */ "Dock",
        /* plural name     */ "Docks",
        /* description */
        "A dock can repair damage, manufacture ammunition, and refit ships with the newest technology.  All of these "
        "operations require raw materials, so docks should be connected to stored materials."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ CONNECTOR_UNIT | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ CNCT_4W,
        /* shadow          */ S_CNCT4W,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_CNCT4W,
        /* portrait        */ P_CONNEC,
        /* icon            */ I_CONNEC,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ 0,
        /* gender          */ 'N',
        /* singular name   */ "Connector",
        /* plural name     */ "Connectors",
        /* description */
        "Buildings must be connected to share power, fuel, raw materials, and gold.  You can connect buildings by "
        "placing them next to each other, or by building connectors from one building to the other.",
        /* tutorial description (optional) */
        "Buildings must be connected to share power, fuel, raw materials, and gold.  You can connect buildings by "
        "placing them next to each other, or by building connectors from one building to the other."),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGRUBLE,
        /* shadow          */ S_LRGRBL,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large Rubble 1",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLRUBLE,
        /* shadow          */ S_SMLRBL,
        /* data            */ D_SMLRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small Rubble 1",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY | SELECTABLE,
        /* sprite          */ LRGTAPE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large tape",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | SELECTABLE,
        /* sprite          */ SMLTAPE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_LRGRBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small tape",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGSLAB,
        /* shadow          */ S_LRGSLA,
        /* data            */ D_LRGSLA,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large Slab",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLSLAB,
        /* shadow          */ S_SMLSLA,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small Slab",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | BUILDING | STATIONARY,
        /* sprite          */ LRGCONES,
        /* shadow          */ S_LRGCON,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Large cones",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ SMLCONES,
        /* shadow          */ S_SMLCON,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Small cones",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | SELECTABLE,
        /* sprite          */ ROAD,
        /* shadow          */ S_ROAD,
        /* data            */ D_DEFALT,
        /* flics animation */ F_ROAD,
        /* portrait        */ P_ROAD,
        /* icon            */ I_ROAD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Road",
        /* plural name     */ "Roads",
        /* description */
        "Units move twice as fast over road squares."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ LANDPAD,
        /* shadow          */ S_LANDPA,
        /* data            */ D_DEFALT,
        /* flics animation */ F_LANDPA,
        /* portrait        */ P_LANDPD,
        /* icon            */ I_LANDPD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ 6,
        /* gender          */ 'N',
        /* singular name   */ "Landing Pad",
        /* plural name     */ "Landing Pads",
        /* description */
        "Landing facility for planes.  Does not repair, resupply, or refuel planes.  To resupply a plane on a landing "
        "pad, use a nearby truck or storage unit."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE,
        /* sprite          */ SHIPYARD,
        /* shadow          */ S_SHIPYA,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SHIPYA,
        /* portrait        */ P_SHIPYD,
        /* icon            */ I_SHIPYD,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Shipyard",
        /* plural name     */ "Shipyards",
        /* description */
        "Shipyards manufacture ships, such as submarines, sea transports, and missile cruisers.  A shipyard requires "
        "three raw materials a turn to operate, and power from a power generator or power station."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ LIGHTPLT,
        /* shadow          */ S_LIGHTP,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_LIGHTP,
        /* portrait        */ P_LGHTPL,
        /* icon            */ I_LGHTPL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Light Vehicle Plant",
        /* plural name     */ "Light Vehicle Plants",
        /* description */
        "Builds light vehicles: engineers, trucks, mobile repair units, scouts, and mobile anti-aircraft units.  A "
        "light vehicle plant requires three raw materials a turn to operate, and power from a power generator or power "
        "station.",
        /* tutorial description (optional) */
        "Click on the light vehicle plant to show the Build button.  Factories need to be connected to power and a "
        "source of raw materials (usually a mining station) to operate."),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ LANDPLT,
        /* shadow          */ S_LANDPL,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_LANDPL,
        /* portrait        */ P_HVYPLT,
        /* icon            */ I_HVYPLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Heavy Vehicle Plant",
        /* plural name     */ "Heavy Vehicle Plants",
        /* description */
        "Builds constructors, mobile scanners, and the heavy fighting units: tanks, assault guns, rocket launchers, "
        "and missile crawlers.  A heavy factory needs three raw materials a turn to operate, and power from a power "
        "generator or power station.",
        /* tutorial description (optional) */
        "Click on the heavy vehicle plant to show the Build button.  Factories need to be connected to power and a "
        "source of raw materials (usually a mining station) to operate."),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ SUPRTPLT,
        /* shadow          */ S_SUPRTP,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_SUPRTP,
        /* portrait        */ P_LIFESP,
        /* icon            */ I_LIFESP,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT | SELECTABLE |
            REQUIRES_SLAB,
        /* sprite          */ AIRPLT,
        /* shadow          */ S_AIRPLT,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_AIRPLT,
        /* portrait        */ P_AIRPLT,
        /* icon            */ I_AIRPLT,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Air Units Plant",
        /* plural name     */ "Air Units Plants",
        /* description */
        "Builds fighters, ground attack planes, and air transports.  A heavy factory needs three raw materials a turn "
        "to operate, and power from a power generator or power station."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ HABITAT,
        /* shadow          */ S_HABITA,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_HABITA,
        /* portrait        */ P_HABITA,
        /* icon            */ I_HABITA,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Habitat",
        /* plural name     */ "Habitats",
        /* description */
        "Habitats house colonists.  Each habitat houses enough colonists to operate three research centers, training "
        "halls, or eco-spheres.  The colonist buildings must be connected to the habitat before they can operate."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ RESEARCH,
        /* shadow          */ S_RESEAR,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_RESEAR,
        /* portrait        */ P_RESEAR,
        /* icon            */ I_RESEAR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Research Center",
        /* plural name     */ "Research Centers",
        /* description */
        "Research centers investigate ways to improve your units.  Unlike upgrades purchased with refined gold, "
        "research takes time, and the improvements are small.  However, research affects all of your units instead of "
        "just one type, and doesn't require a source of gold ore.  A research center requires colonists from a Habitat "
        "to operate, and power."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ GREENHSE,
        /* shadow          */ S_GREENH,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_GREENH,
        /* portrait        */ P_GREENH,
        /* icon            */ I_GREENH,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Eco-sphere",
        /* plural name     */ "Eco-spheres",
        /* description */
        "Domed building containing an earthlike environment.  Your ultimate goal is to provide as many of these for "
        "your colonists as possible.  Every turn an ecosphere operates, it increases your colony rating.  If its "
        "destroyed, you lose all of the improvements the ecosphere made to your rating.  Ecospheres require colonists "
        "from a Habitat and power to operate."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | REQUIRES_SLAB,
        /* sprite          */ RECCENTR,
        /* shadow          */ S_RECCEN,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_RECCEN,
        /* portrait        */ P_RECCTR,
        /* icon            */ I_RECCTR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Recreation Center",
        /* plural name     */ "Recreation Centers",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ TRAINHAL,
        /* shadow          */ S_TRAINH,
        /* data            */ D_LRGBLD,
        /* flics animation */ F_TRAINH,
        /* portrait        */ P_TRNHLL,
        /* icon            */ I_TRNHLL,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Training Hall",
        /* plural name     */ "Training Halls",
        /* description */
        "Trains infiltrators who can disable or capture enemy units, and infantry to guard against infiltrators.  Only "
        "infantry and other infiltrators can detect infiltrators.  A training hall requires colonists from a Habitat "
        "and power to operate."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ WTRPLTFM,
        /* shadow          */ S_WTRPLT,
        /* data            */ D_DEFALT,
        /* flics animation */ F_WTRPLT,
        /* portrait        */ P_WATER,
        /* icon            */ I_WATER,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 6,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Water Platform",
        /* plural name     */ "Water Platforms",
        /* description */
        "Provides a surface over water on which to construct other buildings.  Engineers can build water platforms "
        "over shore as well as open water.  Water platforms are particularly important on island maps, or when a "
        "valuable material deposit is under water."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ GUNTURRT,
        /* shadow          */ S_GUNTUR,
        /* data            */ D_FIXED,
        /* flics animation */ F_GUNTUR,
        /* portrait        */ P_GUNTUR,
        /* icon            */ I_GUNTUR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Gun Turret",
        /* plural name     */ "Gun Turrets",
        /* description */
        "Heavily armored stationary gun turret.  Gun turrets have good firepower, heavy armor, and moderate range.  "
        "Because they take little time to build, they are a good choice when you need defense in a hurry.",
        /* tutorial description (optional) */
        "Gun turrets fire automatically on enemy units.  Of course they can't fire on what they can't see, so it's "
        "important to have a scout, scanner, or radar nearby."),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ANTIAIR,
        /* shadow          */ S_ANTIAI,
        /* data            */ D_ANTIAI,
        /* flics animation */ F_ANTIAI,
        /* portrait        */ P_FXAA,
        /* icon            */ I_FXAA,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Anti Aircraft",
        /* plural name     */ "Anti Aircraft",
        /* description */
        "Low caliber, high speed anti aircraft cannon.   Anti aircraft make short work of planes, but cannot attack "
        "ground units."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE | STANDALONE |
            REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ARTYTRRT,
        /* shadow          */ S_ARTYTR,
        /* data            */ D_FIXED,
        /* flics animation */ F_ARTYTR,
        /* portrait        */ P_ARTYTR,
        /* icon            */ I_ARTYTR,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Artillery",
        /* plural name     */ "Artillery",
        /* description */
        "High powered stationary gun.  Artillery is a compromise between the cheap, rugged gun turret and the "
        "expensive, long-range missile turret."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | STANDALONE | REQUIRES_SLAB | TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ ANTIMSSL,
        /* shadow          */ S_ANTIMS,
        /* data            */ D_FIXED,
        /* flics animation */ F_ANTIMS,
        /* portrait        */ P_FXROCK,
        /* icon            */ I_FXROCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Missile Launcher",
        /* plural name     */ "Missile Launchers",
        /* description */
        "Long-range missile launcher on a fixed mount. Excellent range and firepower, but only slightly more armor "
        "than the mobile version.  Best surrounded by gun turrets and anti aircraft."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY | UPGRADABLE | SELECTABLE | STANDALONE,
        /* sprite          */ BLOCK,
        /* shadow          */ S_BLOCK,
        /* data            */ D_SMLBLD,
        /* flics animation */ F_BLOCK,
        /* portrait        */ P_BLOCK,
        /* icon            */ I_BLOCK,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Concrete Block",
        /* plural name     */ "Concrete Blocks",
        /* description */
        "Hardened, armored block for blocking enemy movement."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE,
        /* sprite          */ BRIDGE,
        /* shadow          */ S_BRIDGE,
        /* data            */ D_BRIDGE,
        /* flics animation */ F_BRIDGE,
        /* portrait        */ P_BRIDGE,
        /* icon            */ I_BRIDGE,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 6,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Bridge",
        /* plural name     */ "Bridges",
        /* description */
        "Pontoon bridge for crossing water.  Bridges, unlike water platforms, do not block the movement of ships."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ BUILDING | STATIONARY | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | REQUIRES_SLAB,
        /* sprite          */ MININGST,
        /* shadow          */ S_MINING,
        /* data            */ D_MINING,
        /* flics animation */ F_MINING,
        /* portrait        */ P_MINING,
        /* icon            */ I_MINING,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Mining Station",
        /* plural name     */ "Mining Stations",
        /* description */
        "Extracts raw materials, fuel, and gold from the ground every turn.  Buildings must be connected to a mine to "
        "use the resources a mine produces.  If there are no storage buildings, fuel tanks, or gold vaults to store "
        "extra material, fuel, or gold, it will be lost.",
        /* tutorial description (optional) */
        "A mining station produces raw materials, fuel, and gold every turn.  Buildings must be connected to the mine "
        "to use the resources the mine produces."),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ LANDMINE,
        /* shadow          */ S_LANDMI,
        /* data            */ D_DEFALT,
        /* flics animation */ F_LANDMI,
        /* portrait        */ P_LANDMN,
        /* icon            */ I_LANDMN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Land Mine",
        /* plural name     */ "Land Mines",
        /* description */
        "Small, hard to detect explosive device.  Only surveyors, minelayers, and infiltrators can spot mines.  Once "
        "spotted, almost any damage will destroy it."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY | UPGRADABLE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEAMINE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_DEFALT,
        /* flics animation */ F_SEAMIN,
        /* portrait        */ P_SEAMIN,
        /* icon            */ I_SEAMIN,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Sea Mine",
        /* plural name     */ "Sea Mines",
        /* description */
        "Small, hard to detect explosive device.  Only surveyors, minelayers, and infiltrators can spot mines.  Once "
        "spotted, almost any damage will destroy it."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ LNDEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ AIREXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ SEAEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_UEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ BLDEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_BEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | STATIONARY,
        /* sprite          */ HITEXPLD,
        /* shadow          */ INVALID_ID,
        /* data            */ D_SEXPLD,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "explosion",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MASTER,
        /* shadow          */ S_MASTER,
        /* data            */ D_MOBILE,
        /* flics animation */ F_MASTER,
        /* portrait        */ P_MASTER,
        /* icon            */ I_MASTER,
        /* armory portrait */ A_MASTER,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Master Builder",
        /* plural name     */ "Master Builders",
        /* description */
        "Specialized vehicle which transforms to become a new mining station."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CONSTRCT,
        /* shadow          */ S_CONSTR,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_CONSTR,
        /* portrait        */ P_CONTRC,
        /* icon            */ I_CONTRC,
        /* armory portrait */ A_CONTRC,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Constructor",
        /* plural name     */ "Constructors",
        /* description */
        "Large construction vehicle for creating large buildings, such as mines and factories.  A constructor consumes "
        "two material per turn while operating.",
        /* tutorial description (optional) */
        "Constructors build large buildings like factories.  Click once on the constructor to select it, click a "
        "second time to show the Build button."),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SCOUT,
        /* shadow          */ S_SCOUT,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_SCOUT,
        /* portrait        */ P_SCOUT,
        /* icon            */ I_SCOUT,
        /* armory portrait */ A_SCOUT,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Scout",
        /* plural name     */ "Scouts",
        /* description */
        "High speed scouting vehicle for exploration and spotting enemy units.  Can cross water without a bridge, but "
        "is faster on land.\n\nVery fast and good scan range, but much weaker than most combat units.",
        /* tutorial description (optional) */
        "A Scout can see a long way, and is very fast. It also has a small gun for harassing the enemy.  To move the "
        "Scout, click on it, and then click where you want to move.  To attack an enemy, move within 3 squares, and "
        "then click on the enemy."),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ TANK,
        /* shadow          */ S_TANK,
        /* data            */ D_TANK,
        /* flics animation */ F_TANK,
        /* portrait        */ P_TANK,
        /* icon            */ I_TANK,
        /* armory portrait */ A_TANK,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Tank",
        /* plural name     */ "Tanks",
        /* description */
        "Heavily armored fighting vehicle.  Best used in the front line to prevent enemy units from reaching "
        "lightly-armored support units such as assault guns and rocket launchers.",
        /* tutorial description (optional) */
        "Tanks are big and tough, but not as fast as scouts.  Like most units, tanks can either move or fire, but not "
        "always both.  If you don't move a tank, it can fire twice each turn.  If you move it a short way, it can fire "
        "once.  If you move it a long way, it can't fire at all."),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ ARTILLRY,
        /* shadow          */ S_ARTILL,
        /* data            */ D_FIRING,
        /* flics animation */ F_ARTILL,
        /* portrait        */ P_ARTY,
        /* icon            */ I_ARTY,
        /* armory portrait */ A_ARTY,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Assault Gun",
        /* plural name     */ "Assault Guns",
        /* description */
        "Lightly armored vehicle with a long range gun firing high-explosive shells.  Fast and effective on the "
        "attack, but fragile."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ ROCKTLCH,
        /* shadow          */ S_ROCKTL,
        /* data            */ D_FIRING,
        /* flics animation */ F_ROCKTL,
        /* portrait        */ P_ROCKET,
        /* icon            */ I_ROCKET,
        /* armory portrait */ A_ROCKET,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Rocket Launcher",
        /* plural name     */ "Rocket Launchers",
        /* description */
        "Lightly armored vehicle firing medium-range rockets which affect all units within 2 squares of the target.  "
        "Most effective against groups of enemy units."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MISSLLCH,
        /* shadow          */ S_MISSLL,
        /* data            */ D_FIRING,
        /* flics animation */ F_MISSLL,
        /* portrait        */ P_MISSIL,
        /* icon            */ I_MISSIL,
        /* armory portrait */ A_MISSIL,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Missile Crawler",
        /* plural name     */ "Missile Crawlers",
        /* description */
        "Missile launcher on a lightly armored chassis.  This mobile launcher is slightly less sturdy than the "
        "stationary version, and requires more material to build.  However, the ability to move makes it much more "
        "flexible."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ SP_FLAK,
        /* shadow          */ S_FLAK,
        /* data            */ D_SP_FLK,
        /* flics animation */ F_FLAK,
        /* portrait        */ P_AA,
        /* icon            */ I_AA,
        /* armory portrait */ A_AA,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Mobile Anti Aircraft",
        /* plural name     */ "Mobile Anti Aircraft",
        /* description */
        "Light, fast vehicle with a rapid fire cannon for shooting down enemy planes.  Mobile anti aircraft are "
        "generally defensive units, because they aren't as fast as planes, and can't move while firing.  Use them to "
        "protect an area once your fighters have chased away enemy ground attack planes."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MINELAYR,
        /* shadow          */ S_MINELA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_MINELA,
        /* portrait        */ P_MNELAY,
        /* icon            */ I_MNELAY,
        /* armory portrait */ A_MNELAY,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'M',
        /* singular name   */ "Mine Layer",
        /* plural name     */ "Mine Layers",
        /* description */
        "Minelayers are specialized construction vehicles which place, detect, and remove mines.  Minelayers convert "
        "onboard raw materials into mines.  They can also remove those mines later and convert them back into "
        "materials.  They cannot remove enemy minefields - those most be exploded with gunfire and rockets."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ SURVEYOR,
        /* shadow          */ S_SURVEY,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_SURVEY,
        /* portrait        */ P_SURVEY,
        /* icon            */ I_SURVEY,
        /* armory portrait */ A_SURVEY,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Surveyor",
        /* plural name     */ "Surveyors",
        /* description */
        "Light, amphibious vehicle with sophisticated sensors for detecting underground minerals and enemy mines.  "
        "Unlike most amphibious units, surveyors are just as fast on water as on land.",
        /* tutorial description (optional) */
        "Surveyors spot minerals under the ground. To find a good spot for a mining station, move the surveyor around "
        "until you find a square with a white 'Raw Materials' symbol.  This may take several turns."),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT |
            SPINNING_TURRET,
        /* sprite          */ SCANNER,
        /* shadow          */ S_SCANNE,
        /* data            */ D_SCANNR,
        /* flics animation */ F_SCANNE,
        /* portrait        */ P_SCANNR,
        /* icon            */ I_SCANNR,
        /* armory portrait */ A_SCANNR,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Scanner",
        /* plural name     */ "Scanners",
        /* description */
        "Mobile radar platform.  Slower and more expensive than a scout, but with a better scan range.  A scanner is "
        "an excellent way to spot targets for very long ranged units like missile crawlers."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SPLYTRCK,
        /* shadow          */ S_SPLYTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SPLYTR,
        /* portrait        */ P_SPLYTR,
        /* icon            */ I_SPLYTR,
        /* armory portrait */ A_SPLYTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Supply Truck",
        /* plural name     */ "Supply Trucks",
        /* description */
        "Truck for hauling raw material.  Useful for resupplying engineers and constructors, and for supplying combat "
        "units with new ammunition."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ GOLDTRCK,
        /* shadow          */ S_GOLDTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_GOLDTR,
        /* portrait        */ P_GOLDTR,
        /* icon            */ I_GOLDTR,
        /* armory portrait */ A_GOLDTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ GOLD,
        /* gender          */ 'N',
        /* singular name   */ "Gold Truck",
        /* plural name     */ "Gold Trucks",
        /* description */
        "Unit for moving gold ore from one location to another."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | CONSTRUCTOR_UNIT | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ ENGINEER,
        /* shadow          */ S_ENGINE,
        /* data            */ D_AMPHIB,
        /* flics animation */ F_ENGINE,
        /* portrait        */ P_ENGINR,
        /* icon            */ I_ENGINR,
        /* armory portrait */ A_ENGINR,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Engineer",
        /* plural name     */ "Engineers",
        /* description */
        "Small construction vehicle for creating small buildings and stationary weapons.",
        /* tutorial description (optional) */
        "You have an engineer selected.  To show the build button, click again on the engineer.  Click on any other "
        "unit to select that unit."),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ BULLDOZR,
        /* shadow          */ S_BULLDO,
        /* data            */ D_MOBILE,
        /* flics animation */ F_BULLDO,
        /* portrait        */ P_BULLDZ,
        /* icon            */ I_BULLDZ,
        /* armory portrait */ A_BULLDZ,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Bulldozer",
        /* plural name     */ "Bulldozers",
        /* description */
        "Vehicle for clearing rubble and demolishing buildings. Bulldozers can scavenge material from debris."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ REPAIR,
        /* shadow          */ S_REPAIR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_REPAIR,
        /* portrait        */ P_REPAIR,
        /* icon            */ I_REPAIR,
        /* armory portrait */ A_REPAIR,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Repair Unit",
        /* plural name     */ "Repair Units",
        /* description */
        "Light vehicle which can rapidly repair ground units and buildings.  Repairs require materials.  The most "
        "extensive the damage, the more materials the repair requires.  Depots, docks, hangars, and barracks can also "
        "repair units."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ FUELTRCK,
        /* shadow          */ S_FUELTR,
        /* data            */ D_MOBILE,
        /* flics animation */ F_FUELTR,
        /* portrait        */ P_FUELTR,
        /* icon            */ I_FUELTR,
        /* armory portrait */ A_FUELTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ FUEL,
        /* gender          */ 'N',
        /* singular name   */ "Fuel Truck",
        /* plural name     */ "Fuel Trucks",
        /* description */
        "Truck for moving fuel."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ CLNTRANS,
        /* shadow          */ S_CLNTRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_CLNTRA,
        /* portrait        */ P_COLNST,
        /* icon            */ I_COLNST,
        /* armory portrait */ A_COLNST,
        /* unknown         */ INVALID_ID,
        /* land type       */ 7,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ "Personnel Carrier",
        /* plural name     */ "Personnel Carriers",
        /* description */
        "Fast armored to transport infantry and infiltrators.  The APC can move slowly underwater as well as on land.  "
        "Only enemy corvettes can detect an APC moving underwater."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ COMMANDO,
        /* shadow          */ S_COMMAN,
        /* data            */ D_COMMAN,
        /* flics animation */ F_COMMAN,
        /* portrait        */ P_COMMAN,
        /* icon            */ I_COMMAN,
        /* armory portrait */ A_COMMAN,
        /* unknown         */ INVALID_ID,
        /* land type       */ 5,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Infiltrator",
        /* plural name     */ "Infiltrators",
        /* description */
        "Commando trained in the arts of stealth and electronic warfare.  Infiltrators can disable or capture enemy "
        "units. Infiltrators are normally invisible until they make a mistake trying to capture enemy units, or until "
        "spotted by infantry or infiltrators."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ INFANTRY,
        /* shadow          */ S_INFANT,
        /* data            */ D_INFANT,
        /* flics animation */ F_INFANT,
        /* portrait        */ P_INFANT,
        /* icon            */ I_INFANT,
        /* armory portrait */ A_INFANT,
        /* unknown         */ INVALID_ID,
        /* land type       */ 5,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Infantry",
        /* plural name     */ "Infantry",
        /* description */
        "Human soldiers equipped with light anti-tank shoulder weapons.  Infantry can spot enemy inflitrators."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ FASTBOAT,
        /* shadow          */ S_FASTBO,
        /* data            */ D_ESCORT,
        /* flics animation */ F_FASTBO,
        /* portrait        */ P_ESCORT,
        /* icon            */ I_ESCORT,
        /* armory portrait */ A_ESCORT,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Escort",
        /* plural name     */ "Escorts",
        /* description */
        "High speed boat with good radar and rapid fire anti aircraft cannon.  Escorts are the eyes of a fleet, and a "
        "fleet's protection from planes."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CORVETTE,
        /* shadow          */ S_CORVET,
        /* data            */ D_FIRING,
        /* flics animation */ F_CORVET,
        /* portrait        */ P_CORVET,
        /* icon            */ I_CORVET,
        /* armory portrait */ A_CORVET,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Corvette",
        /* plural name     */ "Corvettes",
        /* description */
        "High speed boat with torpedo tubes and sonar.  Corvettes can attack any sea unit, but their real strength is "
        "their ability to spot and attack submarines."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT,
        /* sprite          */ BATTLSHP,
        /* shadow          */ S_BATTLS,
        /* data            */ D_BATTLS,
        /* flics animation */ F_BATTLS,
        /* portrait        */ P_GUNBT,
        /* icon            */ I_GUNBT,
        /* armory portrait */ A_GUNBT,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Gunboat",
        /* plural name     */ "Gunboat",
        /* description */
        "Heavily armored ship with a high caliber, medium range cannon.  Excellent at destroying other ships and "
        "bombarding shore targets."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT,
        /* sprite          */ SUBMARNE,
        /* shadow          */ S_SUBMAR,
        /* data            */ D_SUB,
        /* flics animation */ F_SUBMAR,
        /* portrait        */ P_SUB,
        /* icon            */ I_SUB,
        /* armory portrait */ A_SUB,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Submarine",
        /* plural name     */ "Submarines",
        /* description */
        "Lurking beneath the surface, submarines must sneak close to the enemy to fire their powerful torpedoes.  "
        "Submarines do not carry active sonar, which might reveal their positions.  Only corvettes can spot a "
        "submarine before it fires.  Only submarines, corvettes, and ground attack planes can attack them."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEATRANS,
        /* shadow          */ S_SEATRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SEATRA,
        /* portrait        */ P_SEATRN,
        /* icon            */ I_SEATRN,
        /* armory portrait */ A_SEATRN,
        /* unknown         */ INVALID_ID,
        /* land type       */ 6,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ "Sea Transport",
        /* plural name     */ "Sea Transports",
        /* description */
        "Heavily armored ship with space to carry up to six land units.  Sea transports are much slower than air "
        "transports, but also much more likely to survive an enemy attack."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ MSSLBOAT,
        /* shadow          */ S_MSSLBO,
        /* data            */ D_FIRING,
        /* flics animation */ F_MSSLBO,
        /* portrait        */ P_MSLCR,
        /* icon            */ I_MSLCR,
        /* armory portrait */ A_MSLCR,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "Missile Cruiser",
        /* plural name     */ "Missile Cruisers",
        /* description */
        "Ship mounted missile launcher.  Excellent range, high attack strength, and medium armor makes this a powerful "
        "unit."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ SEAMNLYR,
        /* shadow          */ S_SEAMNL,
        /* data            */ D_MOBILE,
        /* flics animation */ F_SEAMNL,
        /* portrait        */ P_SEAMNL,
        /* icon            */ I_SEAMNL,
        /* armory portrait */ A_SEAMNL,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Sea Mine Layer",
        /* plural name     */ "Sea Mine Layers",
        /* description */
        "Seaborne version of the minelayer.  Like its land based counterpart, the sea minelayer can manufacture mines, "
        "detect them, and convert them back into raw material."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ CARGOSHP,
        /* shadow          */ S_CARGOS,
        /* data            */ D_MOBILE,
        /* flics animation */ F_CARGOS,
        /* portrait        */ P_CARGOS,
        /* icon            */ I_CARGOS,
        /* armory portrait */ A_CARGOS,
        /* unknown         */ INVALID_ID,
        /* land type       */ 6,
        /* cargo type      */ MATERIALS,
        /* gender          */ 'N',
        /* singular name   */ "Cargo Ship",
        /* plural name     */ "Cargo Ship",
        /* description */
        "Ships for hauling raw material.  Useful for resupplying engineers and sea combat units with new ammunition."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ FIGHTER,
        /* shadow          */ S_FIGHTE,
        /* data            */ D_FIRING,
        /* flics animation */ F_FIGHTE,
        /* portrait        */ P_FIGHTR,
        /* icon            */ I_FIGHTR,
        /* armory portrait */ A_FIGHTR,
        /* unknown         */ INVALID_ID,
        /* land type       */ 15,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Fighter",
        /* plural name     */ "Fighters",
        /* description */
        "Fast but fragile aircraft carrying a limited number of air-to-air missiles.  Fighters are the best way to "
        "drive enemy planes out of an area, and to defend a moving force.  Ground based anti aircraft is a better "
        "choice for defending a fixed area."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT |
            SELECTABLE | SENTRY_UNIT,
        /* sprite          */ BOMBER,
        /* shadow          */ S_BOMBER,
        /* data            */ D_FIRING,
        /* flics animation */ F_BOMBER,
        /* portrait        */ P_BOMBER,
        /* icon            */ I_BOMBER,
        /* armory portrait */ A_BOMBER,
        /* unknown         */ INVALID_ID,
        /* land type       */ 15,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Ground Attack Plane",
        /* plural name     */ "Ground Attack Planes",
        /* description */
        "A heavier plane carrying a full load of air to ground missiles.  Ground Attack Planes are expensive, but the "
        "advantages are worth the cost.  Their great speed lets them reach any trouble spot quickly, and once they "
        "arrive only fighters and antiaircraft can defend against them."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT,
        /* sprite          */ AIRTRANS,
        /* shadow          */ S_AIRTRA,
        /* data            */ D_MOBILE,
        /* flics animation */ F_AIRTRA,
        /* portrait        */ P_AIRTRN,
        /* icon            */ I_AIRTRN,
        /* armory portrait */ A_AIRTRN,
        /* unknown         */ INVALID_ID,
        /* land type       */ 15,
        /* cargo type      */ 4,
        /* gender          */ 'N',
        /* singular name   */ "Air Transport",
        /* plural name     */ "Air Transports",
        /* description */
        "Heavy aircraft capable of holding up to three ground units."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | UPGRADABLE | ELECTRONIC_UNIT | SELECTABLE | SENTRY_UNIT |
            SPINNING_TURRET,
        /* sprite          */ AWAC,
        /* shadow          */ S_AWAC,
        /* data            */ D_AWAC,
        /* flics animation */ F_AWAC,
        /* portrait        */ P_AWAC,
        /* icon            */ I_AWAC,
        /* armory portrait */ A_AWAC,
        /* unknown         */ INVALID_ID,
        /* land type       */ 15,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "AWAC",
        /* plural name     */ "AWACs",
        /* description */
        "Airborne Warning And Control plane.  Basically a flying radar dish, an AWAC is essential to spot enemy anti "
        "aircraft before your planes fly into firing range."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_SEA_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ JUGGRNT,
        /* shadow          */ S_JUGGRN,
        /* data            */ D_FIRING,
        /* flics animation */ F_JUGGRN,
        /* portrait        */ P_JUGGER,
        /* icon            */ I_JUGGER,
        /* armory portrait */ A_JUGGER,
        /* unknown         */ INVALID_ID,
        /* land type       */ WATER,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Alien Gunboat",
        /* plural name     */ "Alien Gunboats",
        /* description */
        "Heavily armored and armed warship of alien design.  Like the human gunboat, the alien gunboat excels at "
        "destroying other ships and land targets near the shore.  All alien units repair themselves, and improve their "
        "abilities with experience."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            TURRET_SPRITE | SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNTANK,
        /* shadow          */ S_ALNTAN,
        /* data            */ D_ALTANK,
        /* flics animation */ F_ALNTAN,
        /* portrait        */ P_ALNTAN,
        /* icon            */ I_ALNTAN,
        /* armory portrait */ A_ALNTAN,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Alien Tank",
        /* plural name     */ "Alien Tanks",
        /* description */
        "Heavily armored tank of alien design.  Alien tanks are best used at the front of an attack to soak up enemy "
        "fire.  All alien units repair themselves, and improve their abilities with experience."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_LAND_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNASGUN,
        /* shadow          */ S_ALNASG,
        /* data            */ D_FIRING,
        /* flics animation */ F_ALNASG,
        /* portrait        */ P_ALNASG,
        /* icon            */ I_ALNASG,
        /* armory portrait */ A_ALNASG,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Alien Assault Gun",
        /* plural name     */ "Alien Assault Guns",
        /* description */
        "Powerful plasma weapon mounted on a fast, medium-armored chassis.  While not as fragile as human assault "
        "guns, the alien assault gun should still avoid enemy fire whenever possible.  All alien units repair "
        "themselves, and improve their abilities with experience."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MOBILE_AIR_UNIT | HAS_FIRING_SPRITE | FIRES_MISSILES | ELECTRONIC_UNIT | SELECTABLE |
            SENTRY_UNIT | REGENERATING_UNIT,
        /* sprite          */ ALNPLANE,
        /* shadow          */ S_ALNPLA,
        /* data            */ D_FIRING,
        /* flics animation */ F_ALNPLA,
        /* portrait        */ P_ALNPLA,
        /* icon            */ I_ALNPLA,
        /* armory portrait */ A_ALNPLA,
        /* unknown         */ INVALID_ID,
        /* land type       */ 15,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'M',
        /* singular name   */ "Alien Attack Plane",
        /* plural name     */ "Alien Attack Planes",
        /* description */
        "Alien fighter-bomber.  While the alien attack plane is primarily designed to attack ground targets, it can "
        "also fire its missiles at other planes.  Though powerful, the alien attack plane is still vulnerable to anti "
        "aircraft fire.  Like other alien units, alien attack planes repair themselves, and improve their abilities "
        "with experience."
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ROCKET,
        /* shadow          */ INVALID_ID,
        /* data            */ D_TORPDO,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "missile",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | MISSILE_UNIT,
        /* sprite          */ TORPEDO,
        /* shadow          */ INVALID_ID,
        /* data            */ D_TORPDO,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "torpedo",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNMISSL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNMSL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "alien missile",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNTBALL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNPBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "tank plasma ball",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ MISSILE_UNIT,
        /* sprite          */ ALNABALL,
        /* shadow          */ INVALID_ID,
        /* data            */ D_ALNPBL,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "artillery plasma ball",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ EXPLODING | ANIMATED | MISSILE_UNIT,
        /* sprite          */ RKTSMOKE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_SMOKE,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "smoke trail",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | EXPLODING | ANIMATED | MISSILE_UNIT,
        /* sprite          */ TRPBUBLE,
        /* shadow          */ INVALID_ID,
        /* data            */ D_BUBBLE,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ 255,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "bubble trail",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ STATIONARY,
        /* sprite          */ HARVSTER,
        /* shadow          */ S_HARVST,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "harvester",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        ),
    AbstractUnit(
        /* flags           */ GROUND_COVER | STATIONARY,
        /* sprite          */ WALDO,
        /* shadow          */ INVALID_ID,
        /* data            */ D_DEFALT,
        /* flics animation */ INVALID_ID,
        /* portrait        */ INVALID_ID,
        /* icon            */ INVALID_ID,
        /* armory portrait */ INVALID_ID,
        /* unknown         */ INVALID_ID,
        /* land type       */ LAND,
        /* cargo type      */ NO_CARGO,
        /* gender          */ 'N',
        /* singular name   */ "dead waldo",
        /* plural name     */ "",
        /* description */
        ""
        /* tutorial description (optional) */
        )};

CTInfo UnitsManager_TeamInfo[PLAYER_TEAM_MAX];

TeamMissionSupplies UnitsManager_TeamMissionSupplies[PLAYER_TEAM_MAX];

const char* const UnitsManager_BuildTimeEstimates[] = {"%s %i will be available in %i turns.",
                                                       "%s %i will be available in %i turns.",
                                                       "%s %i will be available in %i turns."};

bool UnitsManager_SelfDestructActiveMenu(WindowInfo* window) {
    Button* button_destruct;
    bool event_click_destruct;
    bool event_click_cancel;
    bool event_release;

    for (unsigned short id = SLFDOPN1; id <= SLFDOPN6; ++id) {
        unsigned int time_Stamp = timer_get_stamp32();

        WindowManager_LoadImage2(static_cast<ResourceID>(id), 13, 11, 0, window);
        win_draw(window->id);
        GameManager_ProcessState(true);

        while (timer_get_stamp32() - time_Stamp < TIMER_FPS_TO_TICKS(48)) {
        }
    }

    button_destruct = new (std::nothrow) Button(SLFDOK_U, SLFDOK_D, 13, 11);
    button_destruct->SetRValue(GNW_KB_KEY_RETURN);
    button_destruct->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_destruct->SetSfx(NDONE0);
    button_destruct->RegisterButton(window->id);

    win_draw(window->id);

    event_click_destruct = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_destruct && !event_click_cancel) {
        int key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            event_click_destruct = true;
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_destruct->PlaySound();
            } else {
                SoundManager.PlaySfx(NCANC0);
            }

            event_release = true;
        }

        GameManager_ProcessState(true);
    }

    delete button_destruct;

    return event_click_destruct;
}

bool UnitsManager_SelfDestructMenu() {
    Window destruct_window(SELFDSTR, WINDOW_MAIN_MAP);
    WindowInfo window;
    Button* button_arm;
    Button* button_cancel;
    bool event_click_arm;
    bool event_click_cancel;
    bool event_release;

    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    destruct_window.SetFlags(0x10);

    destruct_window.Add();
    destruct_window.FillWindowInfo(&window);

    button_arm = new (std::nothrow) Button(SLFDAR_U, SLFDAR_D, 89, 14);
    button_arm->SetCaption("Arm");
    button_arm->SetFlags(0x05);
    button_arm->SetPValue(GNW_KB_KEY_RETURN);
    button_arm->SetSfx(MBUTT0);
    button_arm->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(SLFDCN_U, SLFDCN_D, 89, 46);
    button_cancel->SetCaption("Cancel");
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);
    button_cancel->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    win_draw(window.id);

    event_click_arm = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_arm && !event_click_cancel) {
        int key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            button_arm->PlaySound();
            button_arm->Disable();
            if (UnitsManager_SelfDestructActiveMenu(&window)) {
                event_click_arm = true;
            } else {
                event_click_cancel = true;
            }
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            button_cancel->PlaySound();
            event_release = true;
        }

        GameManager_ProcessState(true);
    }

    delete button_arm;
    delete button_cancel;

    return event_click_arm;
}

void UnitsManager_PerformAction(UnitInfo* unit) {
    /// \todo
}

int UnitsManager_CalculateAttackDamage(UnitInfo* attacker_unit, UnitInfo* target_unit, int damage_potential) {
    int target_armor = target_unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR);
    int attacker_damage = target_unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);

    if (damage_potential > 1) {
        attacker_damage *= (5 - damage_potential) / 4;
    }

    if (target_unit->unit_type == SUBMARNE &&
        (attacker_unit->unit_type == BOMBER || attacker_unit->unit_type == ALNPLANE)) {
        attacker_damage /= 2;
    }

    if (target_armor < attacker_damage) {
        attacker_damage -= target_armor;
    } else {
        attacker_damage = 1;
    }

    return attacker_damage;
}

UnitValues* UnitsManager_GetCurrentUnitValues(CTInfo* team_info, ResourceID unit_type) {
    return team_info->team_units->GetCurrentUnitValues(unit_type);
}

void UnitsManager_AddAxisMissionLoadout(unsigned short team, SmartObjectArray<ResourceID> units) {
    if (ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + team)) == TEAM_CLAN_AXIS_INC) {
        int credits;
        ResourceID unit_type;
        unsigned short engineer_turns;
        unsigned short constructor_turns;

        credits = ((ini_get_setting(INI_START_GOLD) / 3) + 18) / 3;
        engineer_turns =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], ENGINEER)->GetAttribute(ATTRIB_TURNS);
        constructor_turns =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], CONSTRCT)->GetAttribute(ATTRIB_TURNS);

        while (credits >= engineer_turns || credits >= constructor_turns) {
            if (credits >= engineer_turns) {
                unit_type = ENGINEER;
                units.PushBack(&unit_type);
                credits -= engineer_turns;
            }

            if (credits >= constructor_turns) {
                unit_type = CONSTRCT;
                units.PushBack(&unit_type);
                credits -= constructor_turns;
            }
        }
    }
}

int UnitsManager_AddDefaultMissionLoadout(unsigned short team) {
    ResourceID unit_type;
    unsigned short cargo;
    int units_count;

    UnitsManager_TeamMissionSupplies[team].team_gold = 0;
    UnitsManager_TeamInfo[team].stats_gold_spent_on_upgrades = 0;

    SmartObjectArray<ResourceID> units(UnitsManager_TeamMissionSupplies[team].units);
    SmartObjectArray<unsigned short> cargos(UnitsManager_TeamMissionSupplies[team].cargos);

    units.Clear();

    unit_type = CONSTRCT;
    units.PushBack(&unit_type);

    unit_type = ENGINEER;
    units.PushBack(&unit_type);

    unit_type = SURVEYOR;
    units.PushBack(&unit_type);

    UnitsManager_AddAxisMissionLoadout(team, units);

    units_count = units.GetCount();

    cargos.Clear();

    cargo = 40;
    cargos.PushBack(&cargo);

    cargo = 20;
    cargos.PushBack(&cargo);

    for (int i = 2; i < units_count; ++i) {
        cargo = 0;
        cargos.PushBack(&cargo);
    }

    return units_count;
}

bool UnitsManager_CanDeployMasterBuilder(UnitInfo* unit, int grid_x, int grid_y) {
    /// \todo
}

void UnitsManager_InitPopupMenus() {
    /// \todo
}

int UnitsManager_GetStealthChancePercentage(UnitInfo* unit1, UnitInfo* unit2, int order) {
    /// \todo
}

SmartPointer<UnitInfo> UnitsManager_SpawnUnit(ResourceID unit_type, unsigned short team, int grid_x, int grid_y,
                                              UnitInfo* parent) {
    /// \todo
}

void UnitsManager_MoveUnit(UnitInfo* unit, int grid_x, int grid_y) {
    if (unit->grid_x != grid_x || unit->grid_x != grid_x) {
        unit->RefreshScreen();
        UnitsManager_UpdateMapHash(unit, grid_x, grid_y);
        unit->RefreshScreen();
    }
}

unsigned int UnitsManager_MoveUnitAndParent(UnitInfo* unit, int grid_x, int grid_y) {
    ResourceID unit_type;
    SmartPointer<UnitInfo> parent;
    unsigned int result;

    unit_type = unit->unit_type;
    parent = unit->GetParent();

    Hash_MapHash.Remove(unit);

    if (parent != nullptr) {
        Hash_MapHash.Remove(&*parent);

        unit_type = parent->GetConstructedUnitType();
    }

    if (UnitsManager_BaseUnits[unit->unit_type].flags & BUILDING) {
        result = Builder_IsAccessible(unit->team, unit_type, grid_x, grid_y);

    } else {
        if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_AIR) {
            result = 0;

        } else {
            result = Access_IsAccessible(unit_type, unit->team, grid_x, grid_y, 0x2);
        }
    }

    Hash_MapHash.Add(unit);

    if (parent != nullptr) {
        Hash_MapHash.Add(&*parent);
    }

    return result;
}

void UnitsManager_StartBuild(UnitInfo* unit) {
    /// \todo
}

void UnitsManager_UpdateUnitPosition(UnitInfo* unit, int grid_x, int grid_y) {
    unit->grid_x = grid_x;
    unit->grid_y = grid_y;

    grid_x = grid_x * 64 + 32;
    grid_y = grid_y * 64 + 32;

    if (unit->flags & BUILDING) {
        grid_x += 31;
        grid_y += 31;
    }

    grid_x -= unit->x;
    grid_y -= unit->y;

    unit->OffsetDrawZones(grid_x, grid_y);
}

void UnitsManager_UpdateMapHash(UnitInfo* unit, int grid_x, int grid_y) {
    Hash_MapHash.Remove(unit);
    UnitsManager_UpdateUnitPosition(unit, grid_x, grid_y);
    Hash_MapHash.Add(unit);
}

void UnitsManager_ProcessRemoteOrders() {
    /// \todo
}

void UnitsManager_SetNewOrderInt(UnitInfo* unit, int order, int state) {
    /// \todo
}

void UnitsManager_UpdatePathsTimeLimit() {
    /// \todo
}

void UnitsManager_SetNewOrder(UnitInfo* unit, int order, int state) {
    UnitsManager_SetNewOrderInt(unit, order, state);

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_08(unit);
    }
}

bool UnitsManager_IsUnitUnderWater(UnitInfo* unit) {
    bool result;

    if (unit->unit_type == SUBMARNE) {
        result = true;

    } else if (unit->unit_type == CLNTRANS) {
        if (Access_GetModifiedSurfaceType(unit->grid_x, unit->grid_y) == SURFACE_TYPE_WATER) {
            result = true;
        } else {
            result = false;
        }
    }

    return result;
}

void UnitsManager_DestroyUnit(UnitInfo* unit) {
    /// \todo
}

int UnitsManager_GetTurnsToBuild(ResourceID unit_type, unsigned short team) {
    /// \todo
}

SmartPointer<UnitInfo> UnitsManager_DeployUnit(ResourceID unit_type, unsigned short team, Complex* complex, int grid_x,
                                               int grid_y, unsigned char unit_angle, bool is_existing_unit,
                                               bool skip_map_status_update) {
    /// \todo
}
