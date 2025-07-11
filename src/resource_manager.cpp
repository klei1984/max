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

#include "resource_manager.hpp"

#include <fstream>
#include <memory>
#include <string>

#include "access.hpp"
#include "assertmenu.hpp"
#include "cursor.hpp"
#include "drawloadbar.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "missionregistry.hpp"
#include "screendump.h"
#include "scripter.hpp"
#include "sha2.h"
#include "sound_manager.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define INIFILE_BUFFER_SIZE 2048

struct res_index {
    char tag[8];
    int32_t data_offset;
    int32_t data_size;
};

static_assert(sizeof(struct res_index) == 16, "The structure needs to be packed.");

struct res_header {
    char id[4];
    int32_t offset;
    int32_t size;
};

static_assert(sizeof(struct res_header) == 12, "The structure needs to be packed.");

struct GameResourceMeta {
    ResourceID res_file_item_index;
    uint8_t *resource_buffer;
    uint8_t res_file_id;
};

static constexpr int32_t ResourceManager_MinimumMemory = 6;
static constexpr int32_t ResourceManager_MinimumMemoryEnhancedGfx = 13;
static constexpr int32_t ResourceManager_MinimumDiskSpace = 1024 * 1024;

static std::unique_ptr<std::ofstream> ResourceManager_LogFile;
static std::unique_ptr<MissionRegistry> ResourceManager_MissionRegistry;

FILE *res_file_handle_array[2];
struct res_index *ResourceManager_ResItemTable;
struct GameResourceMeta *ResourceManager_ResMetaTable;
uint8_t ResourceManager_ResFileCount;
int32_t ResourceManager_ResItemCount;
int32_t resource_buffer_size;
ColorIndex *color_animation_buffer;

const uint8_t ResourceManager_TeamRedColorIndices[8] = {56, 57, 58, 59, 60, 61, 62, 63};
const uint8_t ResourceManager_TeamGreenColorIndices[8] = {32, 33, 34, 35, 36, 37, 38, 39};
const uint8_t ResourceManager_TeamBlueColorIndices[8] = {48, 49, 50, 51, 52, 53, 54, 55};
const uint8_t ResourceManager_TeamGrayColorIndices[8] = {255, 161, 172, 169, 216, 213, 212, 207};
const uint8_t ResourceManager_TeamDerelictColorIndices[8] = {216, 215, 214, 213, 212, 211, 210, 209};

ColorIndex *ResourceManager_TeamRedColorIndexTable;
ColorIndex *ResourceManager_TeamGreenColorIndexTable;
ColorIndex *ResourceManager_TeamBlueColorIndexTable;
ColorIndex *ResourceManager_TeamGrayColorIndexTable;
ColorIndex *ResourceManager_TeamDerelictColorIndexTable;
ColorIndex *ResourceManager_ColorIndexTable06;
ColorIndex *ResourceManager_ColorIndexTable07;
ColorIndex *ResourceManager_ColorIndexTable08;
ColorIndex *ResourceManager_ColorIndexTable09;
ColorIndex *ResourceManager_ColorIndexTable10;
ColorIndex *ResourceManager_ColorIndexTable11;
ColorIndex *ResourceManager_ColorIndexTable12;
ColorIndex *ResourceManager_ColorIndexTable13x8;

uint8_t *ResourceManager_Minimap;
uint8_t *ResourceManager_MinimapUnits;
uint8_t *ResourceManager_MinimapFov;
uint8_t *ResourceManager_MinimapBgImage;

uint16_t *ResourceManager_MapTileIds;
uint8_t *ResourceManager_MapTileBuffer;
uint8_t *ResourceManager_MapSurfaceMap;
uint16_t *ResourceManager_CargoMap;
uint16_t ResourceManager_MapTileCount;

Point ResourceManager_MapSize;
Point ResourceManager_MinimapWindowSize;
Point ResourceManager_MinimapWindowOffset;
double ResourceManager_MinimapWindowScale;
uint8_t *ResourceManager_MainmapBgImage;
int32_t ResourceManager_MainmapZoomLimit;

const char *const ResourceManager_ResourceIdList[RESOURCE_E] = {
    "COMMTWR",  "POWERSTN", "POWGEN",   "BARRACKS", "SHIELDGN", "RADAR",    "ADUMP",    "FDUMP",    "GOLDSM",
    "DEPOT",    "HANGAR",   "DOCK",     "CNCT_4W",  "LRGRUBLE", "SMLRUBLE", "LRGTAPE",  "SMLTAPE",  "LRGSLAB",
    "SMLSLAB",  "LRGCONES", "SMLCONES", "ROAD",     "LANDPAD",  "SHIPYARD", "LIGHTPLT", "LANDPLT",  "SUPRTPLT",
    "AIRPLT",   "HABITAT",  "RESEARCH", "GREENHSE", "RECCENTR", "TRAINHAL", "WTRPLTFM", "GUNTURRT", "ANTIAIR",
    "ARTYTRRT", "ANTIMSSL", "BLOCK",    "BRIDGE",   "MININGST", "LANDMINE", "SEAMINE",  "LNDEXPLD", "AIREXPLD",
    "SEAEXPLD", "BLDEXPLD", "HITEXPLD", "MASTER",   "CONSTRCT", "SCOUT",    "TANK",     "ARTILLRY", "ROCKTLCH",
    "MISSLLCH", "SP_FLAK",  "MINELAYR", "SURVEYOR", "SCANNER",  "SPLYTRCK", "GOLDTRCK", "ENGINEER", "BULLDOZR",
    "REPAIR",   "FUELTRCK", "CLNTRANS", "COMMANDO", "INFANTRY", "FASTBOAT", "CORVETTE", "BATTLSHP", "SUBMARNE",
    "SEATRANS", "MSSLBOAT", "SEAMNLYR", "CARGOSHP", "FIGHTER",  "BOMBER",   "AIRTRANS", "AWAC",     "JUGGRNT",
    "ALNTANK",  "ALNASGUN", "ALNPLANE", "ROCKET",   "TORPEDO",  "ALNMISSL", "ALNTBALL", "ALNABALL", "RKTSMOKE",
    "TRPBUBLE", "HARVSTER", "WALDO",    "UNIT_END", "S_COMMTW", "S_POWERS", "S_POWGEN", "S_BARRAC", "S_SHIELD",
    "S_RADAR",  "S_ADUMP",  "S_FDUMP",  "S_GOLDSM", "S_DEPOT",  "S_HANGAR", "S_DOCK",   "S_CNCT4W", "S_LRGRBL",
    "S_SMLRBL", "S_LRGSLA", "S_LRGCON", "S_SMLSLA", "S_SMLCON", "S_ROAD",   "S_LANDPA", "S_SHIPYA", "S_LIGHTP",
    "S_LANDPL", "S_SUPRTP", "S_AIRPLT", "S_HABITA", "S_RESEAR", "S_GREENH", "S_RECCEN", "S_TRAINH", "S_WTRPLT",
    "S_GUNTUR", "S_ANTIAI", "S_ARTYTR", "S_ANTIMS", "S_BLOCK",  "S_BRIDGE", "S_MINING", "S_LANDMI", "S_SEAMIN",
    "S_MASTER", "S_CONSTR", "S_SCOUT",  "S_TANK",   "S_ARTILL", "S_ROCKTL", "S_MISSLL", "S_FLAK",   "S_MINELA",
    "S_SURVEY", "S_SCANNE", "S_SPLYTR", "S_GOLDTR", "S_ENGINE", "S_BULLDO", "S_REPAIR", "S_FUELTR", "S_CLNTRA",
    "S_COMMAN", "S_INFANT", "S_FASTBO", "S_CORVET", "S_BATTLS", "S_SUBMAR", "S_SEATRA", "S_MSSLBO", "S_SEAMNL",
    "S_CARGOS", "S_FIGHTE", "S_BOMBER", "S_AIRTRA", "S_AWAC",   "S_JUGGRN", "S_ALNTAN", "S_ALNASG", "S_ALNPLA",
    "S_HARVST", "MOVEPT",   "NOMOVEPT", "FIREPT",   "NOFIREPT", "NOCARGO",  "UNTBTN_U", "UNTBTN_D", "DONE_OF",
    "DONE_ON",  "NEXT_OF",  "NEXT_ON",  "PREV_OF",  "PREV_ON",  "XFORM_OF", "XFORM_ON", "BUILD_OF", "BUILD_ON",
    "ALLOC_OF", "ALLOC_ON", "ATTK_OF",  "ATTK_ON",  "SNTRY_OF", "SNTRY_ON", "WAIT_OF",  "WAIT_ON",  "XFER_OF",
    "XFER_ON",  "LOAD_OF",  "LOAD_ON",  "ACTVT_OF", "ACTVT_ON", "STOP_OF",  "STOP_ON",  "REPAR_OF", "REPAR_ON",
    "FLLOW_OF", "FLLOW_ON", "CLEAR_OF", "CLEAR_ON", "RECHG_OF", "RECHG_ON", "START_OF", "START_ON", "UPGRD_OF",
    "UPGRD_ON", "BYUPG_OF", "BYUPG_ON", "RLOAD_OF", "RLOAD_ON", "RSRCH_OF", "RSRCH_ON", "PLACE_OF", "PLACE_ON",
    "REMVE_OF", "REMVE_ON", "ENTER_OF", "ENTER_ON", "TOPIC_OF", "TOPIC_ON", "TIMER",    "XYPOS",    "UNITNAME",
    "TURNS",    "SELFD_OF", "SELFD_ON", "STEAL_OF", "STEAL_ON", "DISBL_OF", "DISBL_ON", "STATS_OF", "STATS_ON",
    "R_RSRC_U", "R_RSRC_D", "R_FACL_U", "R_FACL_D", "R_PERF_U", "R_PERF_D", "R_PROD_U", "R_PROD_D", "R_MINE_U",
    "R_MINE_D", "R_UNIT_U", "R_UNIT_D", "R_UPGR_U", "R_UPGR_D", "R_CSLT_U", "R_CSLT_D", "R_ETA__U", "R_ETA__D",
    "R_STAT_U", "R_STAT_D", "R_CPLX_U", "R_CPLX_D", "R_CLSE_U", "R_CLSE_D", "R_UPAR_U", "R_UPAR_D", "R_DNAR_U",
    "R_DNAR_D", "ENDTRN_U", "R_ENDT_D", "G_ENDT_D", "B_ENDT_D", "W_ENDT_D", "QWKUP_OF", "QWKUP_ON", "QWKDN_OF",
    "QWKDN_ON", "QWKCN_OF", "QWKCN_ON", "BLDUP__U", "BLDUP__D", "BLDUP__X", "BLDDWN_U", "BLDDWN_D", "BLDDWN_X",
    "BLDBLD_U", "BLDBLD_D", "BLDONE_U", "BLDONE_D", "BLDREP_U", "BLDREP_D", "BLDDEL_U", "BLDDEL_D", "BLDDES_U",
    "BLDDES_D", "BLDCAN_U", "BLDCAN_D", "BLDHLP_U", "BLDHLP_D", "BLDPTH_U", "BLDPTH_D", "BLD2X_U",  "BLD2X_D",
    "BLD4X_U",  "BLD4X_D",  "AL_DNE_U", "AL_DNE_D", "UPGDNE_U", "UPGDNE_D", "UPGCAN_U", "UPGCAN_D", "UPGRGT_U",
    "UPGRGT_D", "UPGRGT_X", "UPGLFT_U", "UPGLFT_D", "UPGLFT_X", "SELGRD_U", "SELGRD_D", "SELAIR_U", "SELAIR_D",
    "SELSEA_U", "SELSEA_D", "SELBLD_U", "SELBLD_D", "SELCBT_U", "SELCBT_D", "XFRDNE_U", "XFRDNE_D", "XFRHLP_U",
    "XFRHLP_D", "XFRCAN_U", "XFRCAN_D", "XFRRGT_U", "XFRRGT_D", "XFRLFT_U", "XFRLFT_D", "XFRLFARO", "XFRRTARO",
    "DLOGOK_U", "DLOGOK_D", "HELPOK_U", "HELPOK_D", "ENDOK_U",  "ENDOK_D",  "SLFDAR_U", "SLFDAR_D", "SLFDCN_U",
    "SLFDCN_D", "SLFDOK_U", "SLFDOK_D", "SLFDOPN1", "SLFDOPN2", "SLFDOPN3", "SLFDOPN4", "SLFDOPN5", "SLFDOPN6",
    "PNLHLP_U", "PNLHLP_D", "PNLCAN_U", "PNLCAN_D", "CHTCAN_U", "CHTCAN_D", "CHTHLP_U", "CHTHLP_D", "CHTSND_U",
    "CHTSND_D", "CHTRED_U", "CHTRED_D", "CHTGRN_U", "CHTGRN_D", "CHTBLU_U", "CHTBLU_D", "CHTGRY_U", "CHTGRY_D",
    "PRFCAN_U", "PRFCAN_D", "PRFHLP_U", "PRFHLP_D", "PRFDNE_U", "PRFDNE_D", "PREFEDIT", "PREFNAME", "PRFSLIDE",
    "PRFSLIT",  "CRGTOG_U", "CRGTOG_D", "CRGDEL_U", "CRGDEL_D", "CRGDEL_X", "CRGBUY_U", "CRGBUY_D", "CRGBUY_X",
    "CRGSELUX", "CRGSELDX", "DPTBAYUP", "DPTBAYDN", "DPTMNUUP", "DPTMNUDN", "DPTUP_UP", "DPTUP_DN", "DPTUP_X",
    "DPTDN_UP", "DPTDN_DN", "DPTDN_X",  "DPTHP_UP", "DPTHP_DN", "RAWMSK0",  "RAWMSK1",  "RAWMSK2",  "RAWMSK3",
    "RAWMSK4",  "RAWMSK5",  "RAWMSK6",  "RAWMSK7",  "RAWMSK8",  "RAWMSK9",  "RAWMSK10", "RAWMSK11", "RAWMSK12",
    "RAWMSK13", "RAWMSK14", "RAWMSK15", "RAWMSK16", "FUELMK0",  "FUELMK1",  "FUELMK2",  "FUELMK3",  "FUELMK4",
    "FUELMK5",  "FUELMK6",  "FUELMK7",  "FUELMK8",  "FUELMK9",  "FUELMK10", "FUELMK11", "FUELMK12", "FUELMK13",
    "FUELMK14", "FUELMK15", "FUELMK16", "GOLDMK0",  "GOLDMK1",  "GOLDMK2",  "GOLDMK3",  "GOLDMK4",  "GOLDMK5",
    "GOLDMK6",  "GOLDMK7",  "GOLDMK8",  "GOLDMK9",  "GOLDMK10", "GOLDMK11", "GOLDMK12", "GOLDMK13", "GOLDMK14",
    "GOLDMK15", "GOLDMK16", "SMRWMK1",  "SMRWMK2",  "SMRWMK3",  "SMRWMK4",  "SMRWMK5",  "SMRWMK6",  "SMRWMK7",
    "SMRWMK8",  "SMRWMK9",  "SMRWMK10", "SMRWMK11", "SMRWMK12", "SMRWMK13", "SMRWMK14", "SMRWMK15", "SMRWMK16",
    "SMFLMK1",  "SMFLMK2",  "SMFLMK3",  "SMFLMK4",  "SMFLMK5",  "SMFLMK6",  "SMFLMK7",  "SMFLMK8",  "SMFLMK9",
    "SMFLMK10", "SMFLMK11", "SMFLMK12", "SMFLMK13", "SMFLMK14", "SMFLMK15", "SMFLMK16", "SMGDMK1",  "SMGDMK2",
    "SMGDMK3",  "SMGDMK4",  "SMGDMK5",  "SMGDMK6",  "SMGDMK7",  "SMGDMK8",  "SMGDMK9",  "SMGDMK10", "SMGDMK11",
    "SMGDMK12", "SMGDMK13", "SMGDMK14", "SMGDMK15", "SMGDMK16", "BULB",     "MARKER0",  "MARKER1",  "MARKER2",
    "MARKER3",  "MARKER4",  "STATNUM0", "STATNUM1", "STATNUM2", "STATNUM3", "STATNUM4", "STATNUM5", "STATNUM6",
    "STATNUM7", "STATNUM8", "STATNUM9", "FUELDOT",  "GOLDDOT",  "LFARO_OF", "LFARO_ON", "PRODDOT",  "PRODSLOT",
    "RAWDOT",   "RTARO_OF", "RTARO_ON", "BARRAW",   "BARFUEL",  "BARGOLD",  "BARUNUS",  "BARTAPE",  "BARTAPE2",
    "BARTAPE3", "BARTAPEX", "SMBRRAW",  "SMBRFUEL", "SMBRGOLD", "VERTRAW",  "VERTFUEL", "VERTGOLD", "LOADBAR",
    "CLOSE_OF", "CLOSE_ON", "PURCHOFF", "PURCHON",  "RETURNOF", "RETURNON", "EDITWNDO", "BIGSCRNL", "BIGSCRNR",
    "SUBTITLE", "BGRSCRNL", "BGRSCRNR", "BLDMRK1",  "BLDMRK2",  "BLDMRK3",  "BLDMRK4",  "BLDMRK5",  "HIDDNPTR",
    "HANDPTR",  "CONDLITE", "FUELLITE", "AMMOLITE", "CARGOLIT", "CLOSED",   "ARROW_N",  "ARROW_NE", "ARROW_E",
    "ARROW_SE", "ARROW_S",  "ARROW_SW", "ARROW_W",  "ARROW_NW", "MAP_PTR",  "MINI_PTR", "MINI_X",   "MINI_X2",
    "FRND_FIX", "FRND_XFR", "FRND_LOD", "FRND_FUE", "FRND_PTR", "ENMY_PTR", "PTR_FTRG", "WAY_PTR",  "GROUPPTR",
    "ACTVTPTR", "PTR_RLD",  "STEALPTR", "DISBLPTR", "PTR_PATH", "PTR_HELP", "ZOOMPNL1", "ZOOMPNL2", "ZOOMPNL3",
    "ZOOMPNL4", "ZOOMPNL5", "ZOOMPNL6", "ZOOMPNL7", "ZOOMPTR",  "UNIT_GO",  "UNIT_NGO", "LIGHTOFF", "LIGHTON",
    "I_HRDATK", "I_ATTACK", "I_SHOTS",  "I_RANGE",  "I_AMMO",   "I_ARMOR",  "I_SFTARM", "I_CHARGE", "I_HITS",
    "I_SCAN",   "I_SPEED",  "I_FUEL",   "I_RAW",    "I_RAWE",   "I_GOLD",   "I_GOLDE",  "I_POWER",  "I_LIFE",
    "SI_HITSB", "EI_HITSB", "SI_HITSY", "EI_HITSY", "SI_HITSR", "EI_HITSR", "SI_AMMO",  "EI_AMMO",  "SI_RAW",
    "EI_RAW",   "SI_FUEL",  "EI_FUEL",  "SI_GOLD",  "EI_GOLD",  "SI_LAND",  "EI_LAND",  "SI_SEA",   "EI_SEA",
    "SI_AIR",   "EI_AIR",   "SI_WORK",  "EI_WORK",  "SI_SPEED", "EI_SPEED", "SI_SHOTS", "EI_SHOTS", "SI_POWER",
    "EI_POWER", "IL_SPEED", "IL_SHOTS", "IL_DSBLD", "LIPS",     "I_CMPLX",  "BUY_OFF",  "BUY_ON",   "MENU_OFF",
    "MENU_ON",  "REPT_OFF", "REPT_ON",  "STAT_OFF", "STAT_ON",  "RANG_OFF", "RANG_ON",  "VISN_OFF", "VISN_ON",
    "REDY_OFF", "REDY_ON",  "TRGT_OFF", "TRGT_ON",  "FIND_OFF", "FIND_ON",  "CHAT_OFF", "CHAT_ON",  "GOAL_OFF",
    "GOAL_ON",  "PREF_OFF", "PREF_ON",  "SYSM_OFF", "SYSM_ON",  "FILES_OF", "FILES_ON", "SURV_OFF", "SURV_ON",
    "GRID_OFF", "GRID_ON",  "NAMES_UP", "NAMES_DN", "CHECKED",  "UNCHKED",  "BLANK_UP", "BLANK_DN", "SBLNK_UP",
    "SBLNK_DN", "OBAR_UP",  "OBAR_DN",  "HELP_OF",  "HELP_ON",  "UDONE_OF", "UDONE_ON", "MIN2X_OF", "MIN2X_ON",
    "MINFL_OF", "MINFL_ON", "AMMO_OF",  "AMMO_ON",  "HITS_OF",  "HITS_ON",  "COLOR_OF", "COLOR_ON", "PAUSE_OF",
    "PAUSE_ON", "PLAY_OF",  "PLAY_ON",  "LOCK_OF",  "LOCK_ON",  "CLN0LOGO", "CLN1LOGO", "CLN2LOGO", "CLN3LOGO",
    "CLN4LOGO", "CLN5LOGO", "CLN6LOGO", "CLN7LOGO", "CLN8LOGO", "D_DEFALT", "D_LRGBLD", "D_SMLBLD", "D_MOBILE",
    "D_FIRING", "D_RADAR",  "D_ANTIAI", "D_FIXED",  "D_TANK",   "D_ALTANK", "D_SP_FLK", "D_UEXPLD", "D_SEXPLD",
    "D_BEXPLD", "D_SMOKE",  "D_BUBBLE", "D_LRGRBL", "D_SMLRBL", "D_LRGSLA", "D_AWAC",   "D_AMPHIB", "D_BATTLS",
    "D_BRIDGE", "D_COMMAN", "D_INFANT", "D_TORPDO", "D_ALNMSL", "D_ALNPBL", "D_ESCORT", "D_MINING", "D_SCANNR",
    "D_SUB",    "PNLSEQ_1", "PNLSEQ_2", "PNLSEQ_3", "PNLSEQ_4", "PNLSEQ_5", "BPNLSQ_1", "BPNLSQ_2", "BPNLSQ_3",
    "BPNLSQ_4", "PANELTOP", "PANELBTM", "CH_HUM_U", "CH_HUM_D", "CH_CMP_U", "CH_CMP_D", "CH_NON_U", "CH_NON_D",
    "CH_UP_UP", "CH_UP_DN", "CH_DWN_U", "CH_DWN_D", "CH_QST_U", "CH_QST_D", "CH_TM1_U", "CH_TM1_D", "CH_TM2_U",
    "CH_TM2_D", "CH_TM3_U", "CH_TM3_D", "CH_TM4_U", "CH_TM4_D", "M_CLAN_U", "M_CLAN_D", "CH_CN1_U", "CH_CN1_D",
    "CH_CN2_U", "CH_CN2_D", "CH_CN3_U", "CH_CN3_D", "CH_CN4_U", "CH_CN4_D", "CH_CN5_U", "CH_CN5_D", "CH_CN6_U",
    "CH_CN6_D", "CH_CN7_U", "CH_CN7_D", "CH_CN8_U", "CH_CN8_D", "PL_NXT_U", "PL_NXT_D", "PL_LST_U", "PL_LST_D",
    "MNUBTN1U", "MNUBTN1D", "MNUBTN2U", "MNUBTN2D", "MNUBTN3U", "MNUBTN3D", "MNUBTN4U", "MNUBTN4D", "MNUBTN5U",
    "MNUBTN5D", "MNUBTN6U", "MNUBTN6D", "MNULAROU", "MNULAROD", "MNURAROU", "MNURAROD", "MNUUAROU", "MNUUAROD",
    "MNUDAROU", "MNUDAROD", "I_TRANSP", "I_POWSTN", "I_POWGEN", "I_BARRCK", "I_SHIELD", "I_RADAR",  "I_SMSTOR",
    "I_SMFUEL", "I_SMVLT",  "I_DEPOT",  "I_HANGAR", "I_DOCK",   "I_CONNEC", "I_ROAD",   "I_LANDPD", "I_SHIPYD",
    "I_LGHTPL", "I_HVYPLT", "I_LIFESP", "I_AIRPLT", "I_HABITA", "I_RESEAR", "I_GREENH", "I_RECCTR", "I_TRNHLL",
    "I_WATER",  "I_GUNTUR", "I_FXAA",   "I_ARTYTR", "I_FXROCK", "I_BLOCK",  "I_BRIDGE", "I_MINING", "I_LANDMN",
    "I_SEAMIN", "I_MASTER", "I_CONTRC", "I_SCOUT",  "I_TANK",   "I_ARTY",   "I_ROCKET", "I_MISSIL", "I_AA",
    "I_MNELAY", "I_SURVEY", "I_SCANNR", "I_SPLYTR", "I_GOLDTR", "I_ENGINR", "I_BULLDZ", "I_REPAIR", "I_FUELTR",
    "I_COLNST", "I_COMMAN", "I_INFANT", "I_ESCORT", "I_CORVET", "I_GUNBT",  "I_SUB",    "I_SEATRN", "I_MSLCR",
    "I_SEAMNL", "I_CARGOS", "I_FIGHTR", "I_BOMBER", "I_AIRTRN", "I_AWAC",   "I_JUGGER", "I_ALNTAN", "I_ALNASG",
    "I_ALNPLA", "MEM_END",  "V_START",  "V_M001",   "V_F001",   "V_M005",   "V_F005",   "V_M006",   "V_F006",
    "V_M004",   "V_F004",   "V_M284",   "V_F284",   "V_M138",   "V_F138",   "V_M142",   "V_F142",   "V_M145",
    "V_F145",   "V_M150",   "V_F150",   "V_M151",   "V_F151",   "V_M265",   "V_F265",   "V_M154",   "V_F154",
    "V_M155",   "V_F155",   "V_M158",   "V_F158",   "V_M162",   "V_F162",   "V_M164",   "V_F164",   "V_M163",
    "V_F163",   "V_M165",   "V_F165",   "V_M166",   "V_F166",   "V_M169",   "V_F169",   "V_M171",   "V_F171",
    "V_M181",   "V_F181",   "V_M182",   "V_F182",   "V_M186",   "V_F186",   "V_M187",   "V_F187",   "V_M191",
    "V_F191",   "V_M192",   "V_F192",   "V_M196",   "V_F196",   "V_M198",   "V_F198",   "V_START2", "V_M007",
    "V_F007",   "V_M010",   "V_F010",   "V_M012",   "V_F012",   "V_M239",   "V_F239",   "V_M242",   "V_F242",
    "V_M243",   "V_F243",   "V_M244",   "V_F244",   "V_M247",   "V_F247",   "V_M249",   "V_F249",   "V_START3",
    "V_M049",   "V_F049",   "V_M050",   "V_F050",   "V_M085",   "V_F085",   "V_M089",   "V_F089",   "V_M094",
    "V_F094",   "V_M095",   "V_F095",   "V_M201",   "V_F201",   "V_M210",   "V_F210",   "V_M211",   "V_F211",
    "V_M219",   "V_F219",   "V_M220",   "V_F220",   "V_M224",   "V_F224",   "V_M229",   "V_F229",   "V_M230",
    "V_F230",   "V_M231",   "V_F231",   "V_M232",   "V_F232",   "V_M255",   "V_F255",   "V_M256",   "V_F256",
    "V_M234",   "V_F234",   "V_M236",   "V_F236",   "V_M250",   "V_F250",   "V_M251",   "V_F251",   "V_M070",
    "V_F070",   "V_M071",   "V_F071",   "V_START4", "V_M270",   "V_F270",   "V_M271",   "V_F271",   "V_M279",
    "V_F279",   "V_M280",   "V_F280",   "V_M281",   "V_F281",   "V_M282",   "V_F282",   "V_M025",   "V_F025",
    "V_M026",   "V_F026",   "V_M031",   "V_F031",   "V_M032",   "V_F032",   "V_M037",   "V_F037",   "V_M038",
    "V_F038",   "V_M043",   "V_F043",   "V_M044",   "V_F044",   "V_M053",   "V_F053",   "V_M057",   "V_F057",
    "V_M061",   "V_F061",   "V_M066",   "V_F066",   "V_M075",   "V_F075",   "V_M080",   "V_F080",   "V_M081",
    "V_F081",   "V_M093",   "V_F093",   "V_M098",   "V_F098",   "V_M103",   "V_F103",   "V_M108",   "V_F108",
    "V_M113",   "V_F113",   "V_M118",   "V_F118",   "V_M122",   "V_F122",   "V_M126",   "V_F126",   "V_M130",
    "V_F130",   "V_M206",   "V_F206",   "V_M207",   "V_F207",   "V_M215",   "V_F215",   "V_M216",   "V_F216",
    "V_M217",   "V_F217",   "V_START5", "V_M272",   "V_F272",   "V_M273",   "V_F273",   "V_M275",   "V_F275",
    "V_M276",   "V_F276",   "V_M278",   "V_F278",   "V_M176",   "V_F176",   "V_M177",   "V_F177",   "V_M283",
    "V_F283",   "V_M013",   "V_F013",   "V_END",    "F_COMMT",  "F_POWERS", "F_BARRAC", "F_SHIELD", "F_RADAR",
    "F_DEPOT",  "F_HANGAR", "F_DOCK",   "F_ADUMP",  "F_FDUMP",  "F_GOLDSM", "F_POWGEN", "F_CNCT4W", "F_ROAD",
    "F_LANDPA", "F_SHIPYA", "F_LIGHTP", "F_LANDPL", "F_SUPRTP", "F_AIRPLT", "F_HABITA", "F_RESEAR", "F_GREENH",
    "F_RECCEN", "F_TRAINH", "F_WTRPLT", "F_GUNTUR", "F_ANTIAI", "F_ARTYTR", "F_ANTIMS", "F_BLOCK",  "F_BRIDGE",
    "F_MINING", "F_LANDMI", "F_SEAMIN", "F_MASTER", "F_CONSTR", "F_SCOUT",  "F_TANK",   "F_ARTILL", "F_ROCKTL",
    "F_MISSLL", "F_FLAK",   "F_MINELA", "F_SURVEY", "F_SCANNE", "F_SPLYTR", "F_GOLDTR", "F_ENGINE", "F_BULLDO",
    "F_REPAIR", "F_FUELTR", "F_CLNTRA", "F_COMMAN", "F_INFANT", "F_FASTBO", "F_CORVET", "F_BATTLS", "F_SUBMAR",
    "F_SEATRA", "F_MSSLBO", "F_SEAMNL", "F_CARGOS", "F_FIGHTE", "F_BOMBER", "F_AIRTRA", "F_AWAC",   "F_JUGGRN",
    "F_ALNTAN", "F_ALNASG", "F_ALNPLA", "TRANSFLC", "FILESFLC", "F_ICE",    "F_SAND",   "F_STARS",  "FILE_BUP",
    "FILE_BDN", "FNAME_UP", "FNAME_DN", "FTYPE_UP", "FTYPE_DN", "FDESC_UP", "FDESC_DN", "LOAD_BUP", "LOAD_BDN",
    "SAVE_BUP", "SAVE_BDN", "CNCL_BUP", "CNCL_BDN", "QUIT_BUP", "QUIT_BDN", "FILE1_UP", "FILE2_UP", "FILE3_UP",
    "FILE4_UP", "FILE5_UP", "FILE6_UP", "FILE7_UP", "FILE8_UP", "FILE9_UP", "FILE10UP", "FILE1_DN", "FILE2_DN",
    "FILE3_DN", "FILE4_DN", "FILE5_DN", "FILE6_DN", "FILE7_DN", "FILE8_DN", "FILE9_DN", "FILE10DN", "INTROSND",
    "ENEMBEEP", "RADRPING", "ERRDING",  "MBUTT0",   "MENUOP",   "NHUMN0",   "NCOMP0",   "NNONE0",   "NCLAN0",
    "NOPTI0",   "NCANC0",   "NHELP0",   "NDONE0",   "CCHOS0",   "CCRIM0",   "CVONG0",   "CAYER0",   "CMUSA0",
    "CSACR0",   "CKNIG0",   "CAXIS0",   "CRAND0",   "CCANC0",   "CHELP0",   "CDONE0",   "PWINO0",   "PWINC0",
    "PSELW0",   "PSELM0",   "PRAND0",   "PCANC0",   "PHELP0",   "PDONE0",   "KWINO0",   "KWINC0",   "KSELE0",
    "KBUY0",    "KCARG0",   "KHELP0",   "CBOPE0",   "CBCLO0",   "CBSEL0",   "CBBUI0",   "CBCAN0",   "CBHLP0",
    "CBDON0",   "IOPEN0",   "ICLOS0",   "IZOOM0",   "ISTAT0",   "ICOLO0",   "IHITS0",   "IAMMO0",   "IRANG0",
    "IVISI0",   "IGRID0",   "ISURV0",   "INAME0",   "FOPEN",    "FCLOS",    "FBUBB",    "FSAVE",    "FQUIT",
    "FCANC",    "FHELP",    "FLOAD",    "FXS_STRT", "GEN_IDLE", "WGN_IDLE", "GEN_DRVE", "WGN_DRVE", "GEN_STOP",
    "WGN_STOP", "GEN_XFRM", "GEN_BLDG", "GEN_SHNK", "GEN_XPND", "GEN_TRRT", "GEN_FIRE", "GEN_HIT",  "GEN_XPLD",
    "GEN_PRCS", "GEN_PRCE", "GEN_LAND", "GEN_TAKE", "MONOP10",  "MONOP15",  "MONOP16",  "MONOP17",  "MONOP18",
    "POWST10",  "POWST15",  "POWST16",  "POWST17",  "POWST18",  "POWGN10",  "POWGN15",  "POWGN16",  "POWGN17",
    "POWGN18",  "BARRA10",  "BARRA15",  "BARRA16",  "BARRA17",  "BARRA18",  "GOLDR10",  "GOLDR15",  "GOLDR16",
    "GOLDR17",  "GOLDR18",  "RADAR13",  "RADAR15",  "RADAR16",  "SSTOR15",  "SSTOR16",  "SSTOR17",  "SFUEL15",
    "SFUEL16",  "SFUEL17",  "SGOLD15",  "SGOLD16",  "SGOLD17",  "DEPOT10",  "DEPOT15",  "DEPOT16",  "DEPOT17",
    "DEPOT18",  "HANGR10",  "HANGR15",  "HANGR16",  "HANGR17",  "HANGR18",  "DOCK10",   "DOCK15",   "DOCK16",
    "DOCK17",   "DOCK18",   "ROAD15",   "ROAD16",   "LPAD10",   "LPAD15",   "LPAD16",   "SUNIT10",  "SUNIT15",
    "SUNIT16",  "SUNIT17",  "SUNIT18",  "LVP10",    "LVP15",    "LVP16",    "LVP17",    "LVP18",    "HVP10",
    "HVP15",    "HVP16",    "HVP17",    "HVP18",    "AUNIT10",  "AUNIT15",  "AUNIT16",  "AUNIT17",  "AUNIT18",
    "DORMI10",  "DORMI15",  "DORMI16",  "DORMI17",  "DORMI18",  "RESEAR10", "RESEAR15", "RESEAR16", "RESEAR17",
    "RESEAR18", "ECOSP10",  "ECOSP15",  "ECOSP16",  "ECOSP17",  "ECOSP18",  "TRAIN10",  "TRAIN15",  "TRAIN16",
    "TRAIN17",  "TRAIN18",  "WPLAT15",  "WPLAT16",  "FGUN13",   "FGUN14",   "FGUN15",   "FGUN16",   "FANTI13",
    "FANTI14",  "FANTI15",  "FANTI16",  "FARTY13",  "FARTY14",  "FARTY15",  "FARTY16",  "FROCK13",  "FROCK14",
    "FROCK15",  "FROCK16",  "BLOCK15",  "BLOCK16",  "BRIDG15",  "BRIDG16",  "MSTAT10",  "MSTAT15",  "MSTAT16",
    "MSTAT17",  "MSTAT18",  "LMINE16",  "CMINE16",  "EMPTYLND", "EMPTYWTR", "MASTR1",   "MASTR5",   "MASTR7",
    "MASTR9",   "MASTR15",  "MASTR16",  "CONST1",   "CONST2",   "CONST5",   "CONST6",   "CONST7",   "CONST8",
    "CONST10",  "CONST15",  "CONST16",  "SCOUT1",   "SCOUT2",   "SCOUT5",   "SCOUT6",   "SCOUT7",   "SCOUT8",
    "SCOUT14",  "SCOUT15",  "SCOUT16",  "TANK1",    "TANK5",    "TANK7",    "TANK14",   "TANK15",   "TANK16",
    "ASGUN1",   "ASGUN5",   "ASGUN7",   "ASGUN14",  "ASGUN15",  "ASGUN16",  "RLNCH1",   "RLNCH5",   "RLNCH7",
    "RLNCH14",  "RLNCH15",  "RLNCH16",  "MSLNC1",   "MSLNC5",   "MSLNC7",   "MSLNC14",  "MSLNC15",  "MSLNC16",
    "MANTI1",   "MANTI5",   "MANTI7",   "MANTI14",  "MANTI15",  "MANTI16",  "MLAYER1",  "MLAYER5",  "MLAYER7",
    "MLAYER15", "MLAYER16", "MLAYER17", "MLAYER18", "SURVY1",   "SURVY2",   "SURVY5",   "SURVY6",   "SURVY7",
    "SURVY8",   "SURVY15",  "SURVY16",  "SCAN1",    "SCAN5",    "SCAN7",    "SCAN15",   "SCAN16",   "MTRUK1",
    "MTRUK5",   "MTRUK7",   "MTRUK15",  "MTRUK16",  "MTRUK17",  "GTRUK1",   "GTRUK5",   "GTRUK7",   "GTRUK15",
    "GTRUK16",  "GTRUK17",  "ENGIN1",   "ENGIN2",   "ENGIN5",   "ENGIN6",   "ENGIN7",   "ENGIN8",   "ENGIN10",
    "ENGIN15",  "ENGIN16",  "BULL1",    "BULL5",    "BULL7",    "BULL10",   "BULL15",   "BULL16",   "REPAIR1",
    "REPAIR5",  "REPAIR7",  "REPAIR15", "REPAIR16", "REPAIR17", "FTRUK1",   "FTRUK5",   "FTRUK7",   "FTRUK15",
    "FTRUK16",  "FTRUK17",  "APC1",     "APC2",     "APC5",     "APC6",     "APC7",     "APC8",     "APC15",
    "APC16",    "APC17",    "APC18",    "INFIL5",   "INFIL14",  "INFIL15",  "INFIL16",  "INFIL17",  "INFAN5",
    "INFAN14",  "INFAN15",  "INFAN16",  "ESCRT2",   "ESCRT6",   "ESCRT8",   "ESCRT14",  "ESCRT15",  "ESCRT16",
    "CORVT2",   "CORVT6",   "CORVT8",   "CORVT14",  "CORVT15",  "CORVT16",  "GUNBT2",   "GUNBT6",   "GUNBT8",
    "GUNBT14",  "GUNBT15",  "GUNBT16",  "SUB2",     "SUB6",     "SUB8",     "SUB14",    "SUB15",    "SUB16",
    "STRANS2",  "STRANS6",  "STRANS8",  "STRANS15", "STRANS16", "STRANS17", "STRANS18", "MSLCR2",   "MSLCR6",
    "MSLCR8",   "MSLCR14",  "MSLCR15",  "MSLCR16",  "SMINE2",   "SMINE6",   "SMINE8",   "SMINE15",  "SMINE16",
    "SMINE17",  "SMINE18",  "CSHIP2",   "CSHIP6",   "CSHIP8",   "CSHIP15",  "CSHIP16",  "CSHIP17",  "FIGHT1",
    "FIGHT5",   "FIGHT7",   "FIGHT14",  "FIGHT15",  "FIGHT16",  "FIGHT19",  "FIGHT20",  "ATACK1",   "ATACK5",
    "ATACK7",   "ATACK14",  "ATACK15",  "ATACK16",  "ATACK19",  "ATACK20",  "ATRANS1",  "ATRANS5",  "ATRANS7",
    "ATRANS15", "ATRANS16", "ATRANS17", "ATRANS18", "ATRANS19", "ATRANS20", "AWAC1",    "AWAC5",    "AWAC7",
    "AWAC14",   "AWAC15",   "AWAC16",   "AWAC19",   "AWAC20",   "JUGGR1",   "JUGGR5",   "JUGGR7",   "JUGGR14",
    "JUGGR15",  "JUGGR16",  "ALNTK1",   "ALNTK5",   "ALNTK7",   "ALNTK14",  "ALNTK15",  "ALNTK16",  "ALNAG1",
    "ALNAG5",   "ALNAG7",   "ALNAG14",  "ALNAG15",  "ALNAG16",  "ALNPL1",   "ALNPL5",   "ALNPL7",   "ALNPL14",
    "ALNPL15",  "ALNPL16",  "ALNPL19",  "ALNPL20",  "FXS_END",  "SNOW_MSC", "CRTR_MSC", "GREN_MSC", "DSRT_MSC",
    "MAIN_MSC", "BKG1_MSC", "BKG2_MSC", "BKG3_MSC", "BKG4_MSC", "BKG5_MSC", "BKG6_MSC", "BKG7_MSC", "BKG8_MSC",
    "BKG9_MSC", "CRGO_MSC", "CRDT_MSC", "WINR_MSC", "LOSE_MSC", "LOGOFLIC", "INTROFLC", "DEMO1FLC", "DEMO2FLC",
    "DEMO3FLC", "DEMO4FLC", "DEMO5FLC", "REPORTS",  "REP_FRME", "ALLOCFRM", "STATS",    "FRAMEPIC", "INTROPIC",
    "MAINPIC",  "SETUPPIC", "LOADPIC",  "NETPIC",   "OPTPIC",   "LSTATPIC", "EXITPIC",  "FACBUILD", "CONBUILD",
    "QWKBUILD", "UPGRADE",  "CARGOPIC", "XFERPIC",  "PREFSPIC", "CLANSEL",  "PLANETSE", "MULTGAME", "CHATWNDO",
    "DEPOTFRM", "HANGRFRM", "DOCKFRM",  "RSRCHPIC", "ENDGAME1", "ENDGAME2", "ENDGAME3", "ENDGAME4", "ENDGAME5",
    "ENDGAME6", "ENDGAME7", "ENDGAME8", "ENDGAME9", "ENDGAM10", "ENDGAM11", "ENDGAM12", "ENDGAM13", "ENDGAM14",
    "ENDARM",   "DIALGPIC", "HELPFRAM", "ILOGO",    "MLOGO",    "SELFDSTR", "CREDITS",  "STARTNET", "JOINNET",
    "STARTMOD", "CHOSPLYR", "BLACKOUT", "OEMONE",   "OEMTWO",   "OPTNFRM",  "P_TRANSP", "P_POWSTN", "P_POWGEN",
    "P_BARRCK", "P_SHIELD", "P_RADAR",  "P_SMSTOR", "P_SMFUEL", "P_SMVLT",  "P_DEPOT",  "P_HANGAR", "P_DOCK",
    "P_CONNEC", "P_ROAD",   "P_LANDPD", "P_SHIPYD", "P_LGHTPL", "P_HVYPLT", "P_LIFESP", "P_AIRPLT", "P_HABITA",
    "P_RESEAR", "P_GREENH", "P_RECCTR", "P_TRNHLL", "P_WATER",  "P_GUNTUR", "P_FXAA",   "P_ARTYTR", "P_FXROCK",
    "P_BLOCK",  "P_BRIDGE", "P_MINING", "P_LANDMN", "P_SEAMIN", "P_MASTER", "P_CONTRC", "P_SCOUT",  "P_TANK",
    "P_ARTY",   "P_ROCKET", "P_MISSIL", "P_AA",     "P_MNELAY", "P_SURVEY", "P_SCANNR", "P_SPLYTR", "P_GOLDTR",
    "P_ENGINR", "P_BULLDZ", "P_REPAIR", "P_FUELTR", "P_COLNST", "P_COMMAN", "P_INFANT", "P_ESCORT", "P_CORVET",
    "P_GUNBT",  "P_SUB",    "P_SEATRN", "P_MSLCR",  "P_SEAMNL", "P_CARGOS", "P_FIGHTR", "P_BOMBER", "P_AIRTRN",
    "P_AWAC",   "P_JUGGER", "P_ALNTAN", "P_ALNASG", "P_ALNPLA", "A_MASTER", "A_CONTRC", "A_SCOUT",  "A_TANK",
    "A_ARTY",   "A_ROCKET", "A_MISSIL", "A_AA",     "A_MNELAY", "A_SURVEY", "A_SCANNR", "A_SPLYTR", "A_GOLDTR",
    "A_ENGINR", "A_BULLDZ", "A_REPAIR", "A_FUELTR", "A_COLNST", "A_COMMAN", "A_INFANT", "A_ESCORT", "A_CORVET",
    "A_GUNBT",  "A_SUB",    "A_SEATRN", "A_MSLCR",  "A_SEAMNL", "A_CARGOS", "A_FIGHTR", "A_BOMBER", "A_AIRTRN",
    "A_AWAC",   "A_JUGGER", "A_ALNTAN", "A_ALNASG", "A_ALNPLA", "E_DEPOT",  "E_HANGAR", "E_DOCK",   "ATTRIBS",
    "CLANATRB", "SOUNDVOL", "TIPS",     "HELP_ENG", "HELP_FRE", "HELP_GRM", "SNOW_PIC", "CRTR_PIC", "GREN_PIC",
    "DSRT_PIC", "STAR_PIC", "WORLD_S",  "SNOW_1",   "SNOW_2",   "SNOW_3",   "SNOW_4",   "SNOW_5",   "SNOW_6",
    "CRATER_1", "CRATER_2", "CRATER_3", "CRATER_4", "CRATER_5", "CRATER_6", "GREEN_1",  "GREEN_2",  "GREEN_3",
    "GREEN_4",  "GREEN_5",  "GREEN_6",  "DESERT_1", "DESERT_2", "DESERT_3", "DESERT_4", "DESERT_5", "DESERT_6",
    "WORLD_E",  "FONT_S",   "FONT_1",   "FONT_2",   "FONT_5",   "FONT_E",   "SCRIPT_S", "SC_SCHEM", "SC_T0001",
    "SC_T0002", "SC_T0003", "SC_T0004", "SC_T0005", "SC_T0006", "SC_T0007", "SC_T0008", "SC_T0009", "SC_T0010",
    "SC_T0011", "SC_T0012", "SC_T0013", "SC_T0014", "SC_T0015", "SCRIPT_E",
};

const uint8_t ResourceManager_GenericTable[32] = {0x6c, 0x57, 0x36, 0xe6, 0x81, 0xe0, 0x72, 0x8a, 0xd3, 0xd9, 0xff,
                                                  0x54, 0x48, 0x3a, 0xcd, 0x75, 0xd5, 0x0d, 0xe9, 0xe7, 0x7a, 0x57,
                                                  0xae, 0xeb, 0x61, 0x16, 0xb0, 0x35, 0x55, 0x62, 0xed, 0xd1};

std::filesystem::path ResourceManager_FilePathGameData;
std::filesystem::path ResourceManager_FilePathGameBase;
std::filesystem::path ResourceManager_FilePathGamePref;
std::filesystem::path ResourceManager_FilePathMovie;
std::filesystem::path ResourceManager_FilePathText;
std::filesystem::path ResourceManager_FilePathFlic;
std::filesystem::path ResourceManager_FilePathVoice;
std::filesystem::path ResourceManager_FilePathSfx;
std::filesystem::path ResourceManager_FilePathMusic;

bool ResourceManager_DisableEnhancedGraphics;

const char *const ResourceManager_ErrorCodes[] = {"",      _(1347), _(c164), _(c116), _(9edb), _(6bc6),
                                                  _(5ced), _(3b69), _(c499), _(4afd), _(bc1c), _(908e),
                                                  _(b2b3), _(00a7), _(eb11), _(3004), _(07f8)};

TeamUnits ResourceManager_TeamUnitsRed;
TeamUnits ResourceManager_TeamUnitsGreen;
TeamUnits ResourceManager_TeamUnitsBlue;
TeamUnits ResourceManager_TeamUnitsGray;
TeamUnits ResourceManager_TeamUnitsDerelict;

static void ResourceManager_InitSDL();
static void ResourceManager_TestMemory();
static void ResourceManager_TestDiskSpace();
static void ResourceManager_InitInternals();
static void ResourceManager_InitMissionRegistry();
static int32_t ResourceManager_InitResManager();
static void ResourceManager_TestMouse();
static bool ResourceManager_GetGameDataPath(std::filesystem::path &path);
static int32_t ResourceManager_BuildResourceTable(const char *const cstr, const ResourceType type);
static int32_t ResourceManager_BuildColorTables();
static void ResourceManager_InitMinimapResources();
static void ResourceManager_InitMainmapResources();
static bool ResourceManager_LoadMapTiles(FILE *fp, DrawLoadBar *loadbar);
static void ResourceManager_SetClanUpgrades(int32_t clan, ResourceID unit_type, UnitValues *unit_values);
static SDL_AssertState SDLCALL ResourceManager_AssertionHandler(const SDL_AssertData *data, void *userdata);
static void ResourceManager_LogOutputHandler(void *userdata, int category, SDL_LogPriority priority,
                                             const char *message);
static inline void ResourceManager_FixWorldFiles(const ResourceID world);

static inline std::filesystem::path ResourceManager_GetResourcePath(ResourceType type) {
    auto path{ResourceManager_FilePathGameData};

    switch (type) {
        case ResourceType_GameBase: {
            path = ResourceManager_FilePathGameBase;
        } break;

        case ResourceType_GamePref: {
            path = ResourceManager_FilePathGamePref;
        } break;

        case ResourceType_GameData: {
            path = ResourceManager_FilePathGameData;
        } break;

        case ResourceType_Voice: {
            path = ResourceManager_FilePathVoice;
        } break;

        case ResourceType_Movie: {
            path = ResourceManager_FilePathMovie;
        } break;

        case ResourceType_Text: {
            path = ResourceManager_FilePathText;
        } break;

        case ResourceType_Flic: {
            path = ResourceManager_FilePathFlic;
        } break;

        case ResourceType_Sfx: {
            path = ResourceManager_FilePathSfx;
        } break;

        case ResourceType_Music: {
            path = ResourceManager_FilePathMusic;
        } break;
    }

    return path;
}

bool ResourceManager_GetBasePath(std::filesystem::path &path) {
    bool result;
    std::filesystem::path local_path;
    std::error_code ec;

    auto platform = SDL_GetPlatform();

    if (!std::strcmp(platform, "Windows")) {
        auto base_path = SDL_GetBasePath();

        // SDL_GetBasePath may fail if SDL is not initialized yet
        if (base_path) {
            local_path = std::filesystem::path(base_path).lexically_normal();
            SDL_free(base_path);
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::current_path(ec).lexically_normal();
        }

    } else if (!std::strcmp(platform, "Linux")) {
        auto xdg_data_dirs = SDL_getenv("XDG_DATA_DIRS");

        if (xdg_data_dirs) {
            std::istringstream data(xdg_data_dirs);
            for (std::string xdg_data_dir; std::getline(data, xdg_data_dir, ':');) {
                local_path = (std::filesystem::path(xdg_data_dir) / "max-port/").lexically_normal();

                if (std::filesystem::exists(local_path, ec) && !ec) {
                    break;
                }
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::path("/usr/share/max-port/").lexically_normal();

            if (!std::filesystem::exists(local_path, ec) || ec) {
                local_path = std::filesystem::path("/usr/local/share/max-port/").lexically_normal();

                if (!std::filesystem::exists(local_path, ec) || ec) {
                    auto xdg_data_home = SDL_getenv("XDG_DATA_HOME");

                    if (xdg_data_home) {
                        local_path = (std::filesystem::path(xdg_data_home) / "max-port/").lexically_normal();
                    }

#if SDL_VERSION_ATLEAST(2, 0, 1)
                    if (!std::filesystem::exists(local_path, ec) || ec) {
                        // SDL_GetPrefPath may fail if SDL is not initialized yet
                        auto pref_path = SDL_GetPrefPath("", "max-port");

                        if (pref_path) {
                            local_path = std::filesystem::path(pref_path).lexically_normal();
                            SDL_free(pref_path);
                        }
                    }
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
                }
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
            local_path = std::filesystem::current_path(ec).lexically_normal();
        }
    }

    if (!std::filesystem::exists(local_path / "PATCHES.RES", ec) || ec) {
        result = false;

    } else {
        path = local_path;

        result = true;
    }

    return result;
}

bool ResourceManager_GetPrefPath(std::filesystem::path &path) {
    bool result;
    std::filesystem::path local_path;
    std::error_code ec;

    auto platform = SDL_GetPlatform();

    if (!std::strcmp(platform, "Windows")) {
        std::filesystem::path base_path;

        if (ResourceManager_GetBasePath(base_path)) {
            if (std::filesystem::exists(base_path / ".portable", ec) && !ec) {
                local_path = base_path;
            }
        }

        if (!std::filesystem::exists(local_path, ec) || ec) {
#if SDL_VERSION_ATLEAST(2, 0, 1)
            // SDL_GetPrefPath may fail if SDL is not initialized yet
            auto pref_path = SDL_GetPrefPath("", "M.A.X. Port");

            if (pref_path) {
                local_path = std::filesystem::path(pref_path).lexically_normal();
                SDL_free(pref_path);
            } else
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
            {
                auto app_data = SDL_getenv("APPDATA");

                if (app_data) {
                    local_path = (std::filesystem::path(app_data) / "M.A.X. Port/").lexically_normal();
                }
            }
        }

    } else if (!std::strcmp(platform, "Linux")) {
        auto xdg_data_home = SDL_getenv("XDG_DATA_HOME");

        if (xdg_data_home) {
            local_path = (std::filesystem::path(xdg_data_home) / "max-port/").lexically_normal();
        }

#if SDL_VERSION_ATLEAST(2, 0, 1)
        if (!std::filesystem::exists(local_path, ec) || ec) {
            // SDL_GetPrefPath may fail if SDL is not initialized yet
            auto pref_path = SDL_GetPrefPath("", "max-port");

            if (pref_path) {
                local_path = std::filesystem::path(pref_path).lexically_normal();
                SDL_free(pref_path);
            }
        }
#endif /* SDL_VERSION_ATLEAST(2, 0, 1) */
    }

    if (!std::filesystem::exists(local_path / "settings.ini", ec) || ec) {
        result = false;

    } else {
        path = local_path;

        result = true;
    }

    return result;
}

bool ResourceManager_GetGameDataPath(std::filesystem::path &path) {
    auto filepath{ResourceManager_FilePathGamePref / "settings.ini"};
    Ini_descriptor ini;
    bool result{false};

    if (inifile_init_ini_object_from_ini_file(&ini, filepath.string().c_str())) {
        constexpr int32_t buffer_size{INIFILE_BUFFER_SIZE};
        auto buffer = std::make_unique<char[]>(buffer_size);

        if (inifile_ini_seek_section(&ini, "SETUP") && inifile_ini_seek_param(&ini, "game_data")) {
            if (inifile_ini_get_string(&ini, buffer.get(), buffer_size, 1, false)) {
                auto local_path = std::filesystem::path(buffer.get()).lexically_normal();
                std::error_code ec;

                if (std::filesystem::exists(local_path, ec) && !ec) {
                    path = local_path;

                    result = true;
                }
            }
        }
    }

    inifile_save_to_file_and_free_buffer(&ini, true);

    return result;
}

void ResourceManager_InitPaths() {
    if (!ResourceManager_GetBasePath(ResourceManager_FilePathGameBase)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(2690), nullptr);
        SDL_Log("%s", _(2690));
        exit(1);
    }

    if (!ResourceManager_GetPrefPath(ResourceManager_FilePathGamePref)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(416b), nullptr);
        SDL_Log("%s", _(416b));
        exit(1);
    }

    if (!ResourceManager_GetGameDataPath(ResourceManager_FilePathGameData)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), _(0f72), nullptr);
        SDL_Log("%s", _(0f72));
        exit(1);
    }

    ResourceManager_FilePathVoice = ResourceManager_FilePathGameData;
    ResourceManager_FilePathMovie = ResourceManager_FilePathGameData;
    ResourceManager_FilePathText = ResourceManager_FilePathGameData;
    ResourceManager_FilePathFlic = ResourceManager_FilePathGameData;
    ResourceManager_FilePathSfx = ResourceManager_FilePathGameData;
    ResourceManager_FilePathMusic = ResourceManager_FilePathGameData;
}

void ResourceManager_InitResources() {
    ResourceManager_InitSDL();
    Scripter::Init();
    ResourceManager_InitPaths();
    ResourceManager_TestMemory();
    ResourceManager_TestDiskSpace();
    ResourceManager_InitInternals();
    ResourceManager_TestMouse();
    ResourceManager_InitMissionRegistry();
}

void ResourceManager_InitSDL() {
    SDL_LogSetOutputFunction(&ResourceManager_LogOutputHandler, nullptr);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("%s", _(438a));
        SDL_Log("%s", SDL_GetError());
        exit(1);
    }

    atexit(SDL_Quit);
}

void ResourceManager_TestMemory() {
    int32_t memory;

    memory = SDL_GetSystemRAM();
    if (memory < ResourceManager_MinimumMemory) {
        SmartString message;

        message.Sprintf(200, _(d0a2), ResourceManager_MinimumMemory, memory);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _(cf05), message.GetCStr(), nullptr);
        SDL_Log("%s", message.GetCStr());
        exit(1);
    }
}

void ResourceManager_TestDiskSpace() {
    std::error_code ec;
    const auto info = std::filesystem::space(ResourceManager_FilePathGamePref, ec);

    if (!ec && info.available < ResourceManager_MinimumDiskSpace) {
        SmartString message;

        message.Sprintf(200, _(efb0), static_cast<float>(ResourceManager_MinimumDiskSpace) / (1024 * 1024));

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, _(b7f4), message.GetCStr(), nullptr);
        SDL_Log("%s", message.GetCStr());
    }
}

void ResourceManager_InitInternals() {
    int32_t error_code;

    error_code = ResourceManager_InitResManager();

    if (error_code) {
        ResourceManager_ExitGame(error_code);
    }

    ini_config.Init();
    ini_clans.Init();

    if (WindowManager_Init()) {
        ResourceManager_ExitGame(EXIT_CODE_SCREEN_INIT_FAILED);
    }

    SDL_SetAssertionHandler(&ResourceManager_AssertionHandler, nullptr);

    if (SDL_GetSystemRAM() < ResourceManager_MinimumMemoryEnhancedGfx) {
        ini_set_setting(INI_ENHANCED_GRAPHICS, false);
    }

    ResourceManager_DisableEnhancedGraphics = !ini_get_setting(INI_ENHANCED_GRAPHICS);

    SoundManager_Init();
    register_pause(-1, nullptr);
    register_screendump(GNW_KB_KEY_LALT_C, screendump_pcx);
}

void ResourceManager_TestMouse() {
    if (!mouse_query_exist()) {
        ResourceManager_ExitGame(EXIT_CODE_NO_MOUSE);
    }
}

void ResourceManager_ExitGame(int32_t error_code) {
    if ((error_code == EXIT_CODE_NO_ERROR) || (error_code == EXIT_CODE_THANKS)) {
        menu_draw_exit_logos();
    }

    SoundManager_Deinit();
    win_exit();
    Svga_Deinit();
    SDL_Log("%s", ResourceManager_ErrorCodes[error_code]);
    exit(0);
}

int32_t ResourceManager_InitResManager() {
    int32_t result;

    ResourceManager_ResMetaTable = new (std::nothrow) GameResourceMeta[RESOURCE_E];

    if (ResourceManager_ResMetaTable) {
        for (int32_t i = 0; i < RESOURCE_E; i++) {
            ResourceManager_ResMetaTable[i].resource_buffer = nullptr;
            ResourceManager_ResMetaTable[i].res_file_item_index = INVALID_ID;
        }

        ResourceManager_ResItemCount = 0;

        result = ResourceManager_BuildResourceTable("PATCHES.RES", ResourceType_GameBase);

        if (result == EXIT_CODE_RES_FILE_NOT_FOUND) {
            result = ResourceManager_BuildResourceTable("PATCHES.RES", ResourceType_GameData);
        }

        if (result == EXIT_CODE_NO_ERROR || result == EXIT_CODE_RES_FILE_NOT_FOUND) {
            result = ResourceManager_BuildResourceTable("MAX.RES", ResourceType_GameData);

            if (result == EXIT_CODE_NO_ERROR) {
                if (ResourceManager_BuildColorTables()) {
                    Cursor_Init();

                    for (int16_t j = 0; j < UNIT_END; ++j) {
                        UnitsManager_BaseUnits[j].Init(&UnitsManager_AbstractUnits[j]);
                    }

                    result = EXIT_CODE_NO_ERROR;

                } else {
                    result = EXIT_CODE_INSUFFICIENT_MEMORY;
                }
            }
        }
    } else {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;
    }

    return result;
}

uint8_t *ResourceManager_ReadResource(ResourceID id) {
    uint8_t *resource_buffer;

    if (id == INVALID_ID) {
        resource_buffer = nullptr;
    } else {
        SDL_assert(id > MEM_END && id < RESOURCE_E);

        if (ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
            resource_buffer = nullptr;
        } else {
            FILE *fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
            int32_t data_size =
                ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
            int32_t data_offset =
                ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

            fseek(fp, data_offset, SEEK_SET);

            uint8_t *buffer = new (std::nothrow) uint8_t[data_size + sizeof('\0')];
            if (!buffer) {
                ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
            }

            if (!fread(buffer, data_size, 1, fp)) {
                ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
            }

            buffer[data_size] = '\0';
            resource_buffer = buffer;
        }
    }

    return resource_buffer;
}

uint8_t *ResourceManager_LoadResource(ResourceID id) {
    uint8_t *resource_buffer;

    if (id == INVALID_ID) {
        resource_buffer = nullptr;
    } else {
        SDL_assert(id < MEM_END);

        if (ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
            resource_buffer = nullptr;
        } else {
            if ((resource_buffer = ResourceManager_ResMetaTable[id].resource_buffer) == nullptr) {
                FILE *fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
                int32_t data_size =
                    ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
                int32_t data_offset =
                    ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

                fseek(fp, data_offset, SEEK_SET);

                resource_buffer = new (std::nothrow) uint8_t[data_size];
                if (!resource_buffer) {
                    ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
                }

                if (!fread(resource_buffer, data_size, 1, fp)) {
                    ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
                }

                ResourceManager_ResMetaTable[id].resource_buffer = resource_buffer;
                resource_buffer_size += data_size;
            }
        }
    }

    return resource_buffer;
}

uint32_t ResourceManager_GetResourceSize(ResourceID id) {
    uint32_t data_size;

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        data_size = 0;
    } else {
        data_size = ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
    }

    return data_size;
}

int32_t ResourceManager_ReadImageHeader(ResourceID id, struct ImageBigHeader *buffer) {
    int32_t result;

    SDL_assert(ResourceManager_ResMetaTable);

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        result = false;
    } else {
        FILE *fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
        int32_t data_offset =
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);

        if (!fread(buffer, sizeof(struct ImageBigHeader), 1, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        result = true;
    }

    return result;
}

int32_t ResourceManager_GetResourceFileID(ResourceID id) {
    return ResourceManager_ResMetaTable[id].res_file_item_index;
}

const char *ResourceManager_GetResourceID(ResourceID id) { return ResourceManager_ResourceIdList[id]; }

FILE *ResourceManager_GetFileHandle(ResourceID id) {
    FILE *fp;

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        fp = nullptr;
    } else {
        fp = res_file_handle_array[ResourceManager_ResMetaTable[id].res_file_id];
        int32_t data_offset =
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;

        fseek(fp, data_offset, SEEK_SET);
    }

    return fp;
}

FILE *ResourceManager_OpenFileResource(const char *const cstr, const ResourceType type, const char *const mode,
                                       std::filesystem::path *path) {
    auto buffer = std::make_unique<char[]>(strlen(cstr) + 1);
    FILE *handle{nullptr};

    strcpy(buffer.get(), cstr);

    ResourceManager_ToLowerCase(buffer.get());

    auto pathprefix = ResourceManager_GetResourcePath(type);
    auto filepath = pathprefix / buffer.get();

    handle = fopen(filepath.lexically_normal().string().c_str(), mode);

    if (!handle) {
        ResourceManager_ToUpperCase(buffer.get());

        filepath = pathprefix / buffer.get();

        handle = fopen(filepath.lexically_normal().string().c_str(), mode);
    }

    if (handle && path) {
        *path = filepath.lexically_normal();
    }

    return handle;
}

FILE *ResourceManager_OpenFileResource(const ResourceID id, const ResourceType type, const char *const mode,
                                       std::filesystem::path *path) {
    char *resource{reinterpret_cast<char *>(ResourceManager_ReadResource(id))};
    FILE *handle{nullptr};

    if (resource) {
        handle = ResourceManager_OpenFileResource(resource, type, mode, path);

        delete[] resource;
    }

    return handle;
}

ResourceID ResourceManager_GetResourceID(int32_t index) {
    char buffer[9];

    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, ResourceManager_ResItemTable[index].tag, sizeof(((struct res_index){0}).tag));
    for (int32_t i = 0; i < RESOURCE_E; ++i) {
        if (!strncmp(buffer, ResourceManager_ResourceIdList[i], sizeof(buffer))) {
            return static_cast<ResourceID>(i);
        }
    }

    return INVALID_ID;
}

void ResourceManager_Realloc(ResourceID id, uint8_t *buffer, int32_t data_size) {
    if (ResourceManager_ResMetaTable[id].resource_buffer) {
        delete[] ResourceManager_ResMetaTable[id].resource_buffer;
        resource_buffer_size -=
            ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_size;
    }

    ResourceManager_ResMetaTable[id].resource_buffer = buffer;
    resource_buffer_size += data_size;
}

int32_t ResourceManager_GetFileOffset(ResourceID id) {
    int32_t data_offset;

    if (id == INVALID_ID || ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
        data_offset = 0;
    } else {
        data_offset = ResourceManager_ResItemTable[ResourceManager_ResMetaTable[id].res_file_item_index].data_offset;
    }

    return data_offset;
}

void ResourceManager_FreeResources() {
    mouse_hide();

    for (int16_t i = 0; i < MEM_END; ++i) {
        if (ResourceManager_ResMetaTable[i].resource_buffer) {
            delete[] ResourceManager_ResMetaTable[i].resource_buffer;
            ResourceManager_ResMetaTable[i].resource_buffer = nullptr;
        }
    }

    for (int16_t j = 0; j < UNIT_END; ++j) {
        UnitsManager_BaseUnits[j].sprite = nullptr;
        UnitsManager_BaseUnits[j].shadows = nullptr;
        UnitsManager_BaseUnits[j].field_47 = nullptr;
    }

    resource_buffer_size = 0;

    Cursor_Init();
    Cursor_SetCursor(CURSOR_HAND);
    mouse_show();
}

int32_t ResourceManager_BuildColorTables() {
    int32_t result;
    ColorIndex *aligned_buffer;

    color_animation_buffer = new (std::nothrow) ColorIndex[20 * PALETTE_SIZE + PALETTE_SIZE];
    aligned_buffer =
        reinterpret_cast<ColorIndex *>(((reinterpret_cast<intptr_t>(color_animation_buffer) + PALETTE_SIZE) >> 8) << 8);

    if (color_animation_buffer) {
        ResourceManager_TeamRedColorIndexTable = &aligned_buffer[0 * PALETTE_SIZE];
        ResourceManager_TeamGreenColorIndexTable = &aligned_buffer[1 * PALETTE_SIZE];
        ResourceManager_TeamBlueColorIndexTable = &aligned_buffer[2 * PALETTE_SIZE];
        ResourceManager_TeamGrayColorIndexTable = &aligned_buffer[3 * PALETTE_SIZE];
        ResourceManager_TeamDerelictColorIndexTable = &aligned_buffer[4 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable06 = &aligned_buffer[5 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable07 = &aligned_buffer[6 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable08 = &aligned_buffer[7 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable09 = &aligned_buffer[8 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable10 = &aligned_buffer[9 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable11 = &aligned_buffer[10 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable12 = &aligned_buffer[11 * PALETTE_SIZE];
        ResourceManager_ColorIndexTable13x8 = &aligned_buffer[12 * PALETTE_SIZE];

        {
            ColorIndex *buffer = &aligned_buffer[19 * PALETTE_SIZE];

            for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
                ResourceManager_TeamDerelictColorIndexTable[i] = i;
                ResourceManager_TeamGrayColorIndexTable[i] = i;
                ResourceManager_TeamBlueColorIndexTable[i] = i;
                ResourceManager_TeamGreenColorIndexTable[i] = i;
                ResourceManager_TeamRedColorIndexTable[i] = i;
                buffer[i] = i;
            }
        }

        {
            int32_t j = 0;
            int32_t k = 32;

            while (j < 24) {
                if (j == 8) {
                    k += 8;
                }
                ResourceManager_TeamRedColorIndexTable[k] = ResourceManager_TeamRedColorIndices[j & 7];
                ResourceManager_TeamGreenColorIndexTable[k] = ResourceManager_TeamGreenColorIndices[j & 7];
                ResourceManager_TeamBlueColorIndexTable[k] = ResourceManager_TeamBlueColorIndices[j & 7];
                ResourceManager_TeamGrayColorIndexTable[k] = ResourceManager_TeamGrayColorIndices[j & 7];
                ResourceManager_TeamDerelictColorIndexTable[k] = ResourceManager_TeamDerelictColorIndices[j & 7];
                k++;
                j++;
            }
        }

        ResourceManager_TeamUnitsRed.hash_team_id = HASH_TEAM_RED;
        ResourceManager_TeamUnitsRed.color_index_table = ResourceManager_TeamRedColorIndexTable;

        ResourceManager_TeamUnitsGreen.hash_team_id = HASH_TEAM_GREEN;
        ResourceManager_TeamUnitsGreen.color_index_table = ResourceManager_TeamGreenColorIndexTable;

        ResourceManager_TeamUnitsBlue.hash_team_id = HASH_TEAM_BLUE;
        ResourceManager_TeamUnitsBlue.color_index_table = ResourceManager_TeamBlueColorIndexTable;

        ResourceManager_TeamUnitsGray.hash_team_id = HASH_TEAM_GRAY;
        ResourceManager_TeamUnitsGray.color_index_table = ResourceManager_TeamGrayColorIndexTable;

        ResourceManager_TeamUnitsDerelict.hash_team_id = HASH_TEAM_DERELICT;
        ResourceManager_TeamUnitsDerelict.color_index_table = ResourceManager_TeamDerelictColorIndexTable;

        result = 1;
    } else {
        result = 0;
    }

    return result;
}

int32_t ResourceManager_BuildResourceTable(const char *const cstr, const ResourceType type) {
    int32_t result;
    FILE *fp;
    struct res_header header;

    fp = ResourceManager_OpenFileResource(cstr, type);

    res_file_handle_array[ResourceManager_ResFileCount] = fp;

    if (fp) {
        if (fread(&header, sizeof(header), 1, fp)) {
            if (!strncmp("RES0", header.id, sizeof(res_header::id))) {
                if (ResourceManager_ResItemTable) {
                    ResourceManager_ResItemTable = static_cast<struct res_index *>(
                        realloc(static_cast<void *>(ResourceManager_ResItemTable),
                                header.size + ResourceManager_ResItemCount * sizeof(struct res_index)));
                } else {
                    ResourceManager_ResItemTable = static_cast<struct res_index *>(malloc(header.size));
                }

                if (ResourceManager_ResItemTable) {
                    fseek(fp, header.offset, SEEK_SET);
                    if (fread(&ResourceManager_ResItemTable[ResourceManager_ResItemCount], header.size, 1, fp)) {
                        int16_t new_item_count = ResourceManager_ResItemCount + header.size / sizeof(struct res_index);

                        for (int32_t i = ResourceManager_ResItemCount; i < new_item_count; ++i) {
                            ResourceID id = ResourceManager_GetResourceID(i);
                            if (id != INVALID_ID &&
                                ResourceManager_ResMetaTable[id].res_file_item_index == INVALID_ID) {
                                ResourceManager_ResMetaTable[id].res_file_item_index = static_cast<ResourceID>(i);
                                ResourceManager_ResMetaTable[id].res_file_id = ResourceManager_ResFileCount;
                            }
                        }

                        ResourceManager_ResItemCount = new_item_count;
                        ++ResourceManager_ResFileCount;
                        result = EXIT_CODE_NO_ERROR;
                    } else {
                        result = EXIT_CODE_CANNOT_READ_RES_FILE;
                    }
                } else {
                    result = EXIT_CODE_INSUFFICIENT_MEMORY;
                }
            } else {
                result = EXIT_CODE_RES_FILE_INVALID_ID;
            }
        } else {
            result = EXIT_CODE_CANNOT_READ_RES_FILE;
        }
    } else {
        result = EXIT_CODE_RES_FILE_NOT_FOUND;
    }

    return result;
}

const char *ResourceManager_ToUpperCase(char *cstr) {
    for (char *cstring = cstr; *cstring; ++cstring) {
        *cstring = toupper(*cstring);
    }

    return cstr;
}

const char *ResourceManager_ToUpperCase(std::string &string) {
    for (auto &c : string) c = toupper(c);

    return string.c_str();
}

const char *ResourceManager_ToLowerCase(char *cstr) {
    for (char *cstring = cstr; *cstring; ++cstring) {
        *cstring = tolower(*cstring);
    }

    return cstr;
}

const char *ResourceManager_ToLowerCase(std::string &string) {
    for (auto &c : string) c = tolower(c);

    return string.c_str();
}

void ResourceManager_InitMinimapResources() {
    const WindowInfo *mmw = WindowManager_GetWindow(WINDOW_MINIMAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    delete[] ResourceManager_MinimapFov;
    ResourceManager_MinimapFov = nullptr;

    delete[] ResourceManager_Minimap;
    ResourceManager_Minimap = nullptr;

    delete[] ResourceManager_MinimapUnits;
    ResourceManager_MinimapUnits = nullptr;

    ResourceManager_MinimapFov = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];
    ResourceManager_Minimap = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];
    ResourceManager_MinimapUnits = new (std::nothrow) uint8_t[ResourceManager_MapSize.x * ResourceManager_MapSize.y];

    if (ResourceManager_MinimapFov == nullptr || ResourceManager_Minimap == nullptr ||
        ResourceManager_MinimapUnits == nullptr) {
        ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
    }

    SDL_assert(mmw_width == mmw_height);

    Point offset;
    Point map_size{ResourceManager_MapSize};
    double scale{1.0};

    if (ResourceManager_MapSize.x != mmw_width || ResourceManager_MapSize.y != mmw_height) {
        if (ResourceManager_MapSize.x > ResourceManager_MapSize.y) {
            if (ResourceManager_MapSize.x == mmw_width) {
                map_size.x = mmw_width;
                map_size.y = ResourceManager_MapSize.y;
                offset.y = (mmw_height - map_size.y) / 2;

            } else {
                scale = static_cast<double>(mmw_width) / ResourceManager_MapSize.x;

                map_size.x = mmw_width;
                map_size.y = ResourceManager_MapSize.y * scale;
                offset.y = (mmw_height - map_size.y) / 2;
            }

        } else if (ResourceManager_MapSize.x == ResourceManager_MapSize.y) {
            map_size.x = mmw_width;
            map_size.y = mmw_height;
            scale = static_cast<double>(mmw_width) / ResourceManager_MapSize.x;

        } else {
            if (ResourceManager_MapSize.y == mmw_height) {
                map_size.x = ResourceManager_MapSize.x;
                map_size.y = mmw_height;
                offset.x = (mmw_width - map_size.x) / 2;

            } else {
                scale = static_cast<double>(mmw_height) / ResourceManager_MapSize.y;

                map_size.x = ResourceManager_MapSize.x * scale;
                map_size.y = mmw_height;
                offset.x = (mmw_width - map_size.x) / 2;
            }
        }
    }

    ResourceManager_MinimapWindowSize = map_size;
    ResourceManager_MinimapWindowOffset = offset;
    ResourceManager_MinimapWindowScale = scale;
}

void ResourceManager_InitMainmapResources() {
    const WindowInfo *mmw = WindowManager_GetWindow(WINDOW_MAIN_MAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    delete[] ResourceManager_MainmapBgImage;
    ResourceManager_MainmapBgImage = new (std::nothrow) uint8_t[mmw_width * mmw_height];

    buf_to_buf(mmw->buffer, mmw_width, mmw_height, mmw->width, ResourceManager_MainmapBgImage, mmw_width);

    ResourceManager_MainmapZoomLimit = std::max(std::min(WindowManager_MapWidth / ResourceManager_MapSize.x,
                                                         WindowManager_MapHeight / ResourceManager_MapSize.y),
                                                4);
}

void ResourceManager_InitInGameAssets(int32_t world) {
    WindowInfo *window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    int32_t progress_bar_value;
    uint16_t map_layer_count = 0;
    Point map_layer_dimensions[12];
    int32_t map_minimap_count;
    int32_t file_position;
    int32_t file_offset;
    uint16_t map_tile_count = 0;

    ini_set_setting(INI_WORLD, world);

    UnitsManager_GroundCoverUnits.Clear();
    UnitsManager_MobileLandSeaUnits.Clear();
    UnitsManager_ParticleUnits.Clear();
    UnitsManager_StationaryUnits.Clear();
    UnitsManager_MobileAirUnits.Clear();

    Hash_UnitHash.Clear();
    Hash_MapHash.Clear();

    GameManager_SelectedUnit = nullptr;

    delete[] ResourceManager_MapTileIds;
    ResourceManager_MapTileIds = nullptr;

    delete[] ResourceManager_MapTileBuffer;
    ResourceManager_MapTileBuffer = nullptr;

    delete[] ResourceManager_MapSurfaceMap;
    ResourceManager_MapSurfaceMap = nullptr;

    delete[] ResourceManager_CargoMap;
    ResourceManager_CargoMap = nullptr;

    delete[] ResourceManager_MinimapBgImage;
    ResourceManager_MinimapBgImage = nullptr;

    SoundManager_FreeMusic();

    WindowManager_LoadBigImage(FRAMEPIC, window, window->width, true, false, -1, -1, false, true);

    const WindowInfo *mmw = WindowManager_GetWindow(WINDOW_MINIMAP);
    const int32_t mmw_width{mmw->window.lrx - mmw->window.ulx + 1};
    const int32_t mmw_height{mmw->window.lry - mmw->window.uly + 1};

    ResourceManager_MinimapBgImage = new (std::nothrow) uint8_t[mmw_width * mmw_height];

    buf_to_buf(mmw->buffer, mmw_width, mmw_height, mmw->width, ResourceManager_MinimapBgImage, mmw_width);

    GameManager_InitLandingSequenceMenu(GameManager_GameState == GAME_STATE_7_SITE_SELECT);

    win_draw(window->id);

    world = SNOW_1 + world;

    auto fp{ResourceManager_OpenFileResource(static_cast<ResourceID>(world), ResourceType_GameData)};

    if (!fp) {
        ResourceManager_ExitGame(EXIT_CODE_WRL_FILE_OPEN_ERROR);
    }

    progress_bar_value = 0;
    DrawLoadBar load_bar(_(df4f));

    if (fseek(fp, 3, SEEK_SET) || 1 != fread(&map_layer_count, sizeof(uint16_t), 1, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (map_layer_count != fread(&map_layer_dimensions, sizeof(Point), map_layer_count, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    map_minimap_count = 1;

    ResourceManager_MapSize = map_layer_dimensions[map_minimap_count - 1];

    ResourceManager_InitMainmapResources();
    ResourceManager_InitMinimapResources();

    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    if (fseek(fp, (map_minimap_count - 1) * map_cell_count, SEEK_CUR) ||
        map_cell_count != fread(ResourceManager_MinimapFov, sizeof(uint8_t), map_cell_count, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    memcpy(ResourceManager_Minimap, ResourceManager_MinimapFov, map_cell_count);

    file_position = ftell(fp);

    if (file_position == -1) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (fseek(fp, sizeof(uint16_t) * map_cell_count, SEEK_CUR) ||
        1 != fread(&map_tile_count, sizeof(uint16_t), 1, fp)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    {
        uint8_t *palette;

        palette = new (std::nothrow) uint8_t[PALETTE_STRIDE * PALETTE_SIZE];

        if (fseek(fp, GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE * map_tile_count, SEEK_CUR) ||
            PALETTE_STRIDE * PALETTE_SIZE != fread(palette, sizeof(uint8_t), PALETTE_STRIDE * PALETTE_SIZE, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 64 * PALETTE_STRIDE; i < 160 * PALETTE_STRIDE; ++i) {
            WindowManager_ColorPalette[i] = palette[i] / 4;
        }

        Color_SetSystemPalette(WindowManager_ColorPalette);
        Color_SetColorPalette(WindowManager_ColorPalette);

        delete[] palette;
    }

    if (fseek(fp, file_position, SEEK_SET)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    file_offset = (map_layer_count - map_minimap_count) * map_cell_count;

    for (int32_t i = map_minimap_count; --i;) {
        file_offset += sizeof(uint16_t) * map_layer_dimensions[i].x * map_layer_dimensions[i].y;
    }

    if (fseek(fp, file_offset, SEEK_CUR)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    ResourceManager_MapTileIds = new (std::nothrow) uint16_t[map_cell_count];

    if (ResourceManager_MapTileIds == nullptr) {
        ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
    }

    {
        uint32_t data_chunk_size = map_cell_count / 4;

        for (uint32_t data_offset = 0; data_offset < map_cell_count;) {
            data_chunk_size = std::min(data_chunk_size, map_cell_count - data_offset);
            if (fread(&ResourceManager_MapTileIds[data_offset], sizeof(uint16_t), data_chunk_size, fp) !=
                data_chunk_size) {
                ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
            }

            data_offset += data_chunk_size;
            load_bar.SetValue(20 * data_offset / map_cell_count);
        }
    }

    for (int32_t i = map_layer_count - map_minimap_count; --i > -1;) {
        file_offset += sizeof(uint16_t) * map_layer_dimensions[map_minimap_count + i].x *
                       map_layer_dimensions[map_minimap_count + i].y;
    }

    if (fseek(fp, file_offset, SEEK_CUR)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    if (1 != fread(&ResourceManager_MapTileCount, sizeof(ResourceManager_MapTileCount), 1, fp) ||
        !ResourceManager_LoadMapTiles(fp, &load_bar)) {
        ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
    }

    {
        uint8_t *palette;

        progress_bar_value = 70;

        palette = new (std::nothrow) uint8_t[PALETTE_STRIDE * PALETTE_SIZE];

        if (PALETTE_STRIDE * PALETTE_SIZE != fread(palette, sizeof(uint8_t), PALETTE_STRIDE * PALETTE_SIZE, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 64 * PALETTE_STRIDE; i < 160 * PALETTE_STRIDE; ++i) {
            WindowManager_ColorPalette[i] = palette[i] / 4;
        }

        Color_SetSystemPalette(WindowManager_ColorPalette);
        Color_SetColorPalette(WindowManager_ColorPalette);

        progress_bar_value += 3;

        load_bar.SetValue(progress_bar_value);

        delete[] palette;
    }

    {
        const uint8_t ResourceManager_PassData[] = {SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, SURFACE_TYPE_COAST,
                                                    SURFACE_TYPE_AIR};
        uint8_t *pass_table{new (std::nothrow) uint8_t[ResourceManager_MapTileCount]};

        ResourceManager_MapSurfaceMap = new (std::nothrow) uint8_t[map_cell_count];

        if (ResourceManager_MapTileCount != fread(pass_table, sizeof(uint8_t), ResourceManager_MapTileCount, fp)) {
            ResourceManager_ExitGame(EXIT_CODE_CANNOT_READ_RES_FILE);
        }

        for (int32_t i = 0; i < ResourceManager_MapTileCount; ++i) {
            pass_table[i] = ResourceManager_PassData[pass_table[i]];
        }

        for (uint32_t i = 0; i < map_cell_count; ++i) {
            ResourceManager_MapSurfaceMap[i] = pass_table[ResourceManager_MapTileIds[i]];
        }

        delete[] pass_table;
    }

    fclose(fp);

    ResourceManager_CargoMap = new (std::nothrow) uint16_t[map_cell_count];

    for (uint32_t i = 0; i < map_cell_count; ++i) {
        ResourceManager_CargoMap[i] = 0;
    }

    progress_bar_value += 3;

    load_bar.SetValue(progress_bar_value);

    if (world >= SNOW_1 && world <= SNOW_6) {
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 63, 0, 0, 63, ResourceManager_ColorIndexTable06);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 63, 0, 63, ResourceManager_ColorIndexTable07);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 0, 63, 63, ResourceManager_ColorIndexTable08);

    } else {
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 63, 0, 0, 31, ResourceManager_ColorIndexTable06);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 63, 0, 31, ResourceManager_ColorIndexTable07);
        Color_GenerateIntensityTable3(WindowManager_ColorPalette, 0, 0, 63, 31, ResourceManager_ColorIndexTable08);
    }

    if (world >= CRATER_1 && world <= CRATER_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 63, 0, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= GREEN_1 && world <= GREEN_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 31, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= DESERT_1 && world <= DESERT_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 63, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 31, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);

    } else if (world >= SNOW_1 && world <= SNOW_6) {
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 0, 0, ResourceManager_ColorIndexTable10);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 0, 0, 63, ResourceManager_ColorIndexTable11);
        Color_GenerateIntensityTable2(WindowManager_ColorPalette, 63, 63, 0, ResourceManager_ColorIndexTable09);
    }

    for (int32_t i = 0, j = 0; i < PALETTE_STRIDE * PALETTE_SIZE; i += PALETTE_STRIDE, ++j) {
        int32_t r;
        int32_t g;
        int32_t b;

        int32_t factor;

        r = WindowManager_ColorPalette[i];
        g = WindowManager_ColorPalette[i + 1];
        b = WindowManager_ColorPalette[i + 2];

        factor = 7;

        r = (std::max(factor, r) * 31) / 63;
        g = (std::max(factor, g) * 31) / 63;
        b = (std::max(factor, b) * 31) / 63;

        ResourceManager_ColorIndexTable12[j] = Color_MapColor(WindowManager_ColorPalette, r, g, b, false);
    }

    progress_bar_value += 3;

    load_bar.SetValue(progress_bar_value);

    for (int32_t i = 0, l = 0; i < 7 * 32; i += 32, ++l) {
        for (int32_t j = 0, k = 0; j < PALETTE_STRIDE * PALETTE_SIZE; j += PALETTE_STRIDE, ++k) {
            if (j == PALETTE_STRIDE * 31) {
                ResourceManager_ColorIndexTable13x8[l * PALETTE_SIZE + k] = 31;

            } else {
                int32_t r = (WindowManager_ColorPalette[j] * i) / (7 * 32);
                int32_t g = (WindowManager_ColorPalette[j + 1] * i) / (7 * 32);
                int32_t b = (WindowManager_ColorPalette[j + 2] * i) / (7 * 32);

                ResourceManager_ColorIndexTable13x8[l * PALETTE_SIZE + k] =
                    Color_MapColor(WindowManager_ColorPalette, r, g, b, false);
            }
        }

        progress_bar_value += 3;

        load_bar.SetValue(progress_bar_value);
    }

    ResourceManager_FixWorldFiles(static_cast<ResourceID>(world));
}

bool ResourceManager_LoadMapTiles(FILE *fp, DrawLoadBar *loadbar) {
    int32_t tile_size{GFX_MAP_TILE_SIZE};
    int32_t tile_count_stride{(ResourceManager_MapTileCount + 7) / 8};
    uint8_t *tile_data_chunk{nullptr};

    if (ResourceManager_DisableEnhancedGraphics) {
        tile_size /= 2;
        tile_data_chunk = new (std::nothrow) uint8_t[tile_count_stride * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE];
    }

    ResourceManager_MapTileBuffer = new (std::nothrow) uint8_t[ResourceManager_MapTileCount * tile_size * tile_size];

    for (int32_t i = 0; i < ResourceManager_MapTileCount; i += tile_count_stride) {
        loadbar->SetValue(i * 50 / ResourceManager_MapTileCount + 20);

        if (!ResourceManager_DisableEnhancedGraphics) {
            tile_data_chunk = &ResourceManager_MapTileBuffer[i * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE];
        }

        const int32_t tile_count = std::min(tile_count_stride, ResourceManager_MapTileCount - i);
        const uint32_t data_size = tile_count * GFX_MAP_TILE_SIZE * GFX_MAP_TILE_SIZE;

        if (data_size != fread(tile_data_chunk, sizeof(uint8_t), data_size, fp)) {
            delete[] tile_data_chunk;
            return false;
        }

        if (ResourceManager_DisableEnhancedGraphics) {
            uint8_t *source_address{tile_data_chunk};
            uint8_t *destination_address{&ResourceManager_MapTileBuffer[tile_size * tile_size * i]};

            for (int32_t j = 0; j < tile_count; ++j) {
                for (int32_t k = 0; k < tile_size; ++k) {
                    for (int32_t l = 0; l < tile_size; ++l) {
                        *destination_address = *source_address;
                        destination_address += 1;
                        source_address += 2;
                    }

                    source_address += 64;
                }
            }
        }
    }

    loadbar->SetValue(70);

    if (ResourceManager_DisableEnhancedGraphics) {
        delete[] tile_data_chunk;
    }

    return true;
}

void ResourceManager_SetClanUpgrades(int32_t clan, ResourceID unit_type, UnitValues *unit_values) {
    if (ini_clans.SeekUnit(clan, unit_type)) {
        for (int16_t attribute, value; ini_clans.GetNextUnitUpgrade(&attribute, &value);) {
            if (attribute == ATTRIB_TURNS) {
                unit_values->SetAttribute(attribute, value);

            } else {
                unit_values->AddAttribute(attribute, value);
            }
        }
    }
}

void ResourceManager_InitClanUnitValues(uint16_t team) {
    SmartPointer<UnitValues> unit_values;
    TeamUnits *team_units{nullptr};
    int32_t team_clan{TEAM_CLAN_RANDOM};

    switch (team) {
        case PLAYER_TEAM_RED: {
            team_units = &ResourceManager_TeamUnitsRed;
            team_clan = ini_get_setting(INI_RED_TEAM_CLAN);
        } break;

        case PLAYER_TEAM_GREEN: {
            team_units = &ResourceManager_TeamUnitsGreen;
            team_clan = ini_get_setting(INI_GREEN_TEAM_CLAN);
        } break;

        case PLAYER_TEAM_BLUE: {
            team_units = &ResourceManager_TeamUnitsBlue;
            team_clan = ini_get_setting(INI_BLUE_TEAM_CLAN);
        } break;

        case PLAYER_TEAM_GRAY: {
            team_units = &ResourceManager_TeamUnitsGray;
            team_clan = ini_get_setting(INI_GRAY_TEAM_CLAN);
        } break;

        case PLAYER_TEAM_ALIEN: {
            team_units = &ResourceManager_TeamUnitsDerelict;
            team_clan = TEAM_CLAN_THE_CHOSEN;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    if (team_clan == TEAM_CLAN_RANDOM) {
        team_clan = ((dos_rand() << 3) >> 15) + 1;
        ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + team), team_clan);
    }

    UnitsManager_TeamInfo[team].team_clan = team_clan;
    team_units->ClearComplexes();
    team_units->Init();

    for (int32_t i = 0; i < UNIT_END; ++i) {
        unit_values = new (std::nothrow) UnitValues(*team_units->GetBaseUnitValues(i));

        ResourceManager_SetClanUpgrades(team_clan, static_cast<ResourceID>(i), &*unit_values);
        team_units->SetBaseUnitValues(i, *unit_values);
        team_units->SetCurrentUnitValues(i, *unit_values);
    }
}

void ResourceManager_InitHeatMaps(uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type) {
        const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

        UnitsManager_TeamInfo[team].heat_map_complete = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_complete, 0, map_cell_count);

        UnitsManager_TeamInfo[team].heat_map_stealth_sea = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_stealth_sea, 0, map_cell_count);

        UnitsManager_TeamInfo[team].heat_map_stealth_land = new (std::nothrow) int8_t[map_cell_count];
        memset(UnitsManager_TeamInfo[team].heat_map_stealth_land, 0, map_cell_count);

    } else {
        UnitsManager_TeamInfo[team].heat_map_complete = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_sea = nullptr;
        UnitsManager_TeamInfo[team].heat_map_stealth_land = nullptr;
    }
}

void ResourceManager_InitTeamInfo() {
    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        UnitsManager_TeamMissionSupplies[i].units.Clear();
        UnitsManager_TeamInfo[i].Reset();

        if (i < PLAYER_TEAM_MAX - 1) {
            UnitsManager_TeamInfo[i].team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i));
            UnitsManager_TeamInfo[i].team_clan = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + i));

        } else {
            UnitsManager_TeamInfo[i].team_type = TEAM_TYPE_NONE;
            UnitsManager_TeamInfo[i].team_clan = TEAM_CLAN_RANDOM;
        }
    }

    ResourceManager_TeamUnitsRed.ClearComplexes();
    ResourceManager_TeamUnitsGreen.ClearComplexes();
    ResourceManager_TeamUnitsBlue.ClearComplexes();
    ResourceManager_TeamUnitsGray.ClearComplexes();
    ResourceManager_TeamUnitsDerelict.ClearComplexes();

    UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_units = &ResourceManager_TeamUnitsRed;
    UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_units = &ResourceManager_TeamUnitsGreen;
    UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_units = &ResourceManager_TeamUnitsBlue;
    UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_units = &ResourceManager_TeamUnitsGray;
    UnitsManager_TeamInfo[PLAYER_TEAM_ALIEN].team_units = &ResourceManager_TeamUnitsDerelict;

    MessageManager_ClearMessageLogs();
}

uint8_t *ResourceManager_GetBuffer(ResourceID id) {
    return id == INVALID_ID ? nullptr : ResourceManager_ResMetaTable[id].resource_buffer;
}

SDL_AssertState SDLCALL ResourceManager_AssertionHandler(const SDL_AssertData *data, void *userdata) {
    char caption[500];

    snprintf(caption, sizeof(caption), "Assertion failure at %s (%s:%d), triggered %u %s: '%s'.", data->function,
             data->filename, data->linenum, data->trigger_count, (data->trigger_count == 1) ? "time" : "times",
             data->condition);

    return static_cast<SDL_AssertState>(AssertMenu(caption).Run());
}

void ResourceManager_LogOutputHandler(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    if (!ResourceManager_LogFile) {
        auto filepath = (ResourceManager_FilePathGamePref / "stdout.txt").lexically_normal();

        ResourceManager_LogFile = std::make_unique<std::ofstream>(filepath.string().c_str(), std::ofstream::trunc);
    }

    *ResourceManager_LogFile << message;
}

std::string ResourceManager_Sha256(const ResourceID world) {
    constexpr size_t BLOCK_SIZE{4096};

    auto fp{ResourceManager_OpenFileResource(static_cast<ResourceID>(world), ResourceType_GameData)};
    char hex_digest[SHA256_DIGEST_SIZE * 2 + sizeof('\0')];
    uint8_t data[BLOCK_SIZE];
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256_ctx ctx;
    size_t data_read;

    if (!fp) {
        ResourceManager_ExitGame(EXIT_CODE_WRL_FILE_OPEN_ERROR);
    }

    sha256_init(&ctx);

    while ((data_read = fread(data, sizeof(uint8_t), sizeof(data), fp)) != 0) {
        sha256_update(&ctx, data, data_read);
    }

    sha256_final(&ctx, digest);

    for (size_t i = 0; i < sizeof(digest); ++i) {
        sprintf(hex_digest + (i * 2), "%02x", digest[i]);
    }

    hex_digest[sizeof(hex_digest) - 1] = '\0';

    fclose(fp);

    return hex_digest;
}

void ResourceManager_FixWorldFiles(const ResourceID world) {
    switch (world) {
        case GREEN_4: {
            /* fix tile 190 pass table info */
            const std::string reference{"dbcfc4495334640776e6a0b1776651f904583aa87f193a43436e6b1f04635241"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_190{190u};

                const uint32_t map_cell_count{
                    static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

                for (uint32_t i = 0; i < map_cell_count; ++i) {
                    if (ResourceManager_MapTileIds[i] == tile_190) {
                        ResourceManager_MapSurfaceMap[i] = SURFACE_TYPE_LAND;
                    }
                }
            }
        } break;

        case SNOW_3: {
            /* fix tile at grid cell position 057,057 in tile id map */
            const std::string reference{"a625d409ca1d5cd24cdeed165813e5b79c538b123491791016372492f5106801"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_55{55u};
                constexpr uint16_t grid_x_57{56u};
                constexpr uint16_t grid_y_57{56u};

                ResourceManager_MapTileIds[ResourceManager_MapSize.x * grid_y_57 + grid_x_57] = tile_55;
            }
        } break;

        case SNOW_5: {
            /* fix missing tile art */
            const std::string reference{"11a9255e65af53b6438803d0aef4ca96ed9d57575da3cf13779838ccda25a6e9"};
            auto hash{ResourceManager_Sha256(world)};

            if (reference == hash) {
                constexpr uint16_t tile_411{411u};
                constexpr uint16_t grid_x_55{54u};
                constexpr uint16_t grid_y_66{65u};

                const uint8_t tile_data[4096] = {
                    0x49, 0x4F, 0x59, 0x4E, 0x49, 0x52, 0x49, 0x49, 0x4E, 0x49, 0x4E, 0x40, 0x53, 0x4F, 0x49, 0x53,
                    0x53, 0x49, 0x53, 0x4F, 0x53, 0x53, 0x53, 0x49, 0x52, 0x49, 0x59, 0x42, 0x4E, 0x4F, 0x54, 0x52,
                    0x4F, 0x42, 0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x4F, 0x53, 0x49, 0x42, 0x53, 0x4E,
                    0x42, 0x44, 0x4F, 0x4A, 0x4E, 0x4F, 0x40, 0x42, 0x53, 0x4E, 0x54, 0x4E, 0x4F, 0x53, 0x52, 0x59,
                    0x49, 0x42, 0x49, 0x49, 0x42, 0x40, 0x49, 0x53, 0x54, 0x4F, 0x49, 0x53, 0x4F, 0x59, 0x53, 0x4F,
                    0x59, 0x49, 0x4F, 0x4F, 0x4F, 0x54, 0x4F, 0x5A, 0x52, 0x4E, 0x42, 0x49, 0x4E, 0x52, 0x52, 0x42,
                    0x40, 0x53, 0x42, 0x49, 0x53, 0x53, 0x42, 0x40, 0x53, 0x42, 0x4F, 0x4E, 0x42, 0x4F, 0x53, 0x42,
                    0x49, 0x42, 0x49, 0x42, 0x42, 0x40, 0x49, 0x49, 0x4F, 0x42, 0x42, 0x52, 0x42, 0x4E, 0x53, 0x44,
                    0x5A, 0x52, 0x4E, 0x49, 0x44, 0x49, 0x40, 0x54, 0x49, 0x4F, 0x4E, 0x42, 0x4F, 0x59, 0x4F, 0x42,
                    0x53, 0x42, 0x49, 0x53, 0x4F, 0x49, 0x4E, 0x5A, 0x53, 0x42, 0x59, 0x53, 0x42, 0x59, 0x52, 0x53,
                    0x54, 0x4F, 0x53, 0x49, 0x42, 0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4F, 0x42, 0x54, 0x53, 0x57, 0x54,
                    0x53, 0x4F, 0x4F, 0x42, 0x54, 0x54, 0x53, 0x4E, 0x54, 0x4F, 0x52, 0x4F, 0x42, 0x49, 0x52, 0x49,
                    0x59, 0x42, 0x53, 0x42, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x54, 0x53, 0x4E, 0x57, 0x59,
                    0x52, 0x49, 0x53, 0x40, 0x5A, 0x59, 0x53, 0x5F, 0x53, 0x59, 0x49, 0x4E, 0x42, 0x4E, 0x42, 0x42,
                    0x54, 0x4F, 0x4E, 0x42, 0x4F, 0x53, 0x42, 0x49, 0x42, 0x49, 0x40, 0x53, 0x42, 0x49, 0x53, 0x53,
                    0x42, 0x40, 0x53, 0x42, 0x59, 0x53, 0x59, 0x49, 0x42, 0x49, 0x4E, 0x52, 0x4F, 0x4E, 0x42, 0x4A,
                    0x4F, 0x53, 0x4E, 0x42, 0x42, 0x59, 0x42, 0x54, 0x49, 0x4F, 0x59, 0x4E, 0x49, 0x59, 0x49, 0x5F,
                    0x49, 0x52, 0x57, 0x52, 0x53, 0x59, 0x59, 0x57, 0x52, 0x42, 0x59, 0x54, 0x52, 0x42, 0x42, 0x53,
                    0x53, 0x49, 0x54, 0x4E, 0x53, 0x52, 0x4F, 0x54, 0x53, 0x4F, 0x54, 0x4F, 0x53, 0x49, 0x42, 0x53,
                    0x4E, 0x42, 0x44, 0x4F, 0x53, 0x57, 0x4E, 0x59, 0x4F, 0x4E, 0x49, 0x5A, 0x57, 0x59, 0x57, 0x52,
                    0x4F, 0x40, 0x42, 0x49, 0x40, 0x4E, 0x4F, 0x44, 0x49, 0x42, 0x49, 0x49, 0x42, 0x49, 0x53, 0x49,
                    0x4F, 0x4E, 0x53, 0x59, 0x4F, 0x57, 0x49, 0x49, 0x4A, 0x4F, 0x4F, 0x52, 0x42, 0x42, 0x52, 0x4E,
                    0x42, 0x54, 0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x54, 0x4F, 0x4E, 0x42, 0x4F, 0x53,
                    0x42, 0x49, 0x42, 0x49, 0x4F, 0x4F, 0x42, 0x54, 0x4F, 0x53, 0x4F, 0x4E, 0x52, 0x57, 0x57, 0x42,
                    0x53, 0x49, 0x54, 0x4F, 0x49, 0x4F, 0x42, 0x44, 0x4F, 0x49, 0x44, 0x54, 0x44, 0x4E, 0x49, 0x49,
                    0x53, 0x52, 0x53, 0x59, 0x57, 0x49, 0x44, 0x52, 0x40, 0x52, 0x4E, 0x44, 0x40, 0x42, 0x4F, 0x52,
                    0x42, 0x54, 0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x53, 0x49, 0x54, 0x4E, 0x53, 0x52,
                    0x4F, 0x54, 0x53, 0x4F, 0x4F, 0x4F, 0x42, 0x4F, 0x53, 0x49, 0x4E, 0x53, 0x54, 0x57, 0x4E, 0x52,
                    0x4E, 0x49, 0x53, 0x49, 0x4E, 0x4F, 0x49, 0x57, 0x53, 0x49, 0x49, 0x42, 0x4F, 0x52, 0x54, 0x42,
                    0x4E, 0x54, 0x59, 0x54, 0x42, 0x4F, 0x44, 0x4E, 0x59, 0x49, 0x49, 0x4F, 0x42, 0x4E, 0x59, 0x49,
                    0x53, 0x54, 0x44, 0x4E, 0x4E, 0x4E, 0x44, 0x4F, 0x59, 0x52, 0x42, 0x54, 0x4E, 0x49, 0x53, 0x54,
                    0x49, 0x4F, 0x59, 0x49, 0x40, 0x53, 0x42, 0x49, 0x4E, 0x4F, 0x4F, 0x4E, 0x4E, 0x40, 0x53, 0x40,
                    0x42, 0x4E, 0x52, 0x44, 0x4E, 0x53, 0x52, 0x57, 0x4E, 0x53, 0x4E, 0x49, 0x53, 0x4E, 0x52, 0x59,
                    0x54, 0x53, 0x52, 0x52, 0x40, 0x4E, 0x54, 0x4A, 0x4A, 0x44, 0x42, 0x4F, 0x42, 0x53, 0x44, 0x49,
                    0x4E, 0x52, 0x54, 0x4A, 0x40, 0x40, 0x42, 0x52, 0x52, 0x53, 0x4F, 0x52, 0x40, 0x54, 0x4F, 0x42,
                    0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x54, 0x52, 0x40, 0x4F, 0x44, 0x4F, 0x4E, 0x4F,
                    0x40, 0x44, 0x42, 0x4E, 0x59, 0x57, 0x57, 0x4E, 0x40, 0x49, 0x40, 0x40, 0x49, 0x49, 0x54, 0x40,
                    0x4E, 0x4E, 0x59, 0x40, 0x4F, 0x42, 0x4F, 0x52, 0x4E, 0x49, 0x53, 0x4E, 0x4F, 0x40, 0x49, 0x4F,
                    0x44, 0x40, 0x4E, 0x49, 0x4A, 0x40, 0x4F, 0x44, 0x4F, 0x52, 0x53, 0x4F, 0x49, 0x54, 0x4F, 0x42,
                    0x54, 0x53, 0x57, 0x54, 0x53, 0x4F, 0x4F, 0x42, 0x4E, 0x49, 0x42, 0x40, 0x49, 0x59, 0x4F, 0x49,
                    0x40, 0x42, 0x4E, 0x49, 0x4A, 0x54, 0x59, 0x40, 0x40, 0x40, 0x42, 0x52, 0x4F, 0x4E, 0x53, 0x49,
                    0x49, 0x42, 0x49, 0x52, 0x44, 0x4A, 0x4A, 0x4F, 0x4E, 0x49, 0x54, 0x42, 0x40, 0x40, 0x40, 0x52,
                    0x49, 0x44, 0x40, 0x49, 0x52, 0x42, 0x49, 0x42, 0x40, 0x49, 0x49, 0x53, 0x49, 0x53, 0x40, 0x53,
                    0x42, 0x49, 0x53, 0x53, 0x42, 0x40, 0x53, 0x42, 0x4A, 0x42, 0x53, 0x49, 0x53, 0x4F, 0x4F, 0x4F,
                    0x49, 0x4F, 0x4A, 0x4A, 0x53, 0x49, 0x40, 0x4A, 0x49, 0x40, 0x4E, 0x40, 0x44, 0x40, 0x44, 0x4F,
                    0x4E, 0x42, 0x42, 0x54, 0x54, 0x4F, 0x59, 0x52, 0x40, 0x40, 0x49, 0x49, 0x44, 0x4F, 0x4F, 0x53,
                    0x49, 0x40, 0x44, 0x42, 0x53, 0x49, 0x49, 0x49, 0x44, 0x49, 0x49, 0x44, 0x53, 0x42, 0x54, 0x4F,
                    0x53, 0x49, 0x42, 0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4E, 0x59, 0x54, 0x59, 0x52, 0x49, 0x54, 0x57,
                    0x4A, 0x54, 0x53, 0x49, 0x42, 0x40, 0x40, 0x54, 0x4A, 0x53, 0x42, 0x40, 0x4A, 0x4F, 0x42, 0x4F,
                    0x4F, 0x52, 0x52, 0x44, 0x49, 0x4A, 0x4E, 0x53, 0x40, 0x4F, 0x40, 0x4A, 0x42, 0x59, 0x4F, 0x4A,
                    0x4F, 0x53, 0x49, 0x44, 0x52, 0x49, 0x4A, 0x4F, 0x4F, 0x49, 0x4F, 0x42, 0x4F, 0x4E, 0x54, 0x4F,
                    0x4E, 0x42, 0x4F, 0x53, 0x42, 0x49, 0x42, 0x49, 0x59, 0x54, 0x4E, 0x59, 0x4F, 0x4F, 0x59, 0x53,
                    0x53, 0x4E, 0x40, 0x49, 0x4A, 0x42, 0x49, 0x4A, 0x4E, 0x53, 0x4E, 0x42, 0x42, 0x4A, 0x40, 0x40,
                    0x49, 0x40, 0x52, 0x40, 0x42, 0x4A, 0x49, 0x4F, 0x4F, 0x54, 0x42, 0x53, 0x4E, 0x44, 0x52, 0x40,
                    0x49, 0x49, 0x4E, 0x42, 0x49, 0x42, 0x49, 0x4A, 0x4E, 0x40, 0x49, 0x40, 0x40, 0x4E, 0x53, 0x49,
                    0x54, 0x4E, 0x53, 0x52, 0x4F, 0x54, 0x53, 0x4F, 0x52, 0x49, 0x52, 0x53, 0x4F, 0x4F, 0x53, 0x5A,
                    0x4A, 0x49, 0x4A, 0x42, 0x40, 0x42, 0x49, 0x4F, 0x49, 0x4F, 0x40, 0x40, 0x49, 0x54, 0x59, 0x40,
                    0x44, 0x40, 0x42, 0x4F, 0x49, 0x4F, 0x49, 0x57, 0x42, 0x44, 0x52, 0x52, 0x52, 0x49, 0x53, 0x49,
                    0x57, 0x4F, 0x52, 0x5A, 0x4F, 0x57, 0x4F, 0x4E, 0x4F, 0x49, 0x57, 0x49, 0x53, 0x52, 0x42, 0x54,
                    0x4E, 0x49, 0x53, 0x54, 0x49, 0x4F, 0x59, 0x49, 0x4A, 0x42, 0x59, 0x59, 0x59, 0x53, 0x42, 0x4E,
                    0x49, 0x42, 0x53, 0x40, 0x4A, 0x4A, 0x40, 0x4E, 0x44, 0x42, 0x40, 0x40, 0x4F, 0x59, 0x4E, 0x49,
                    0x44, 0x40, 0x40, 0x40, 0x57, 0x4F, 0x49, 0x57, 0x49, 0x57, 0x54, 0x52, 0x4E, 0x42, 0x42, 0x4F,
                    0x59, 0x49, 0x59, 0x57, 0x40, 0x4F, 0x40, 0x40, 0x49, 0x4E, 0x49, 0x4E, 0x4A, 0x49, 0x4E, 0x49,
                    0x4F, 0x53, 0x57, 0x53, 0x4E, 0x44, 0x42, 0x4E, 0x53, 0x42, 0x59, 0x4E, 0x59, 0x4A, 0x4E, 0x52,
                    0x40, 0x44, 0x54, 0x40, 0x44, 0x42, 0x57, 0x53, 0x42, 0x44, 0x4A, 0x49, 0x52, 0x57, 0x4F, 0x42,
                    0x4E, 0x53, 0x4F, 0x4F, 0x53, 0x49, 0x4E, 0x59, 0x57, 0x53, 0x4A, 0x52, 0x59, 0x52, 0x49, 0x4E,
                    0x42, 0x59, 0x5A, 0x54, 0x4F, 0x49, 0x42, 0x53, 0x49, 0x4A, 0x42, 0x49, 0x42, 0x42, 0x4F, 0x4F,
                    0x49, 0x44, 0x49, 0x4F, 0x4F, 0x54, 0x4E, 0x57, 0x49, 0x49, 0x53, 0x4A, 0x52, 0x59, 0x4F, 0x4F,
                    0x40, 0x49, 0x53, 0x49, 0x49, 0x53, 0x53, 0x49, 0x4F, 0x42, 0x4E, 0x4F, 0x40, 0x4F, 0x40, 0x49,
                    0x57, 0x52, 0x49, 0x4E, 0x53, 0x40, 0x44, 0x4E, 0x49, 0x4F, 0x44, 0x59, 0x54, 0x44, 0x44, 0x42,
                    0x4F, 0x57, 0x4F, 0x57, 0x4F, 0x49, 0x52, 0x49, 0x53, 0x49, 0x4E, 0x42, 0x52, 0x4F, 0x4F, 0x49,
                    0x42, 0x42, 0x52, 0x52, 0x59, 0x49, 0x40, 0x57, 0x4F, 0x49, 0x4F, 0x49, 0x59, 0x59, 0x52, 0x49,
                    0x4F, 0x53, 0x40, 0x52, 0x40, 0x52, 0x57, 0x52, 0x54, 0x4F, 0x49, 0x52, 0x4E, 0x49, 0x49, 0x42,
                    0x4F, 0x4F, 0x4F, 0x59, 0x54, 0x49, 0x4F, 0x42, 0x4F, 0x4E, 0x54, 0x4F, 0x59, 0x4F, 0x40, 0x42,
                    0x49, 0x4A, 0x49, 0x57, 0x52, 0x49, 0x57, 0x4F, 0x4A, 0x52, 0x4F, 0x4A, 0x54, 0x44, 0x44, 0x49,
                    0x52, 0x49, 0x52, 0x49, 0x4F, 0x44, 0x4E, 0x4E, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x4F, 0x49, 0x85,
                    0x44, 0x40, 0x40, 0x40, 0x53, 0x53, 0x4E, 0x54, 0x54, 0x49, 0x52, 0x4E, 0x4F, 0x4F, 0x40, 0x42,
                    0x44, 0x44, 0x52, 0x59, 0x59, 0x44, 0x52, 0x44, 0x49, 0x44, 0x42, 0x42, 0x53, 0x4E, 0x57, 0x59,
                    0x4A, 0x53, 0x49, 0x52, 0x53, 0x53, 0x4F, 0x44, 0x42, 0x42, 0x53, 0x52, 0x54, 0x40, 0x40, 0x44,
                    0x4E, 0x4A, 0x53, 0x59, 0x53, 0x4F, 0x59, 0x59, 0x4F, 0x4F, 0x4E, 0x52, 0x4E, 0x8C, 0x9B, 0x9B,
                    0x44, 0x57, 0x40, 0x42, 0x4E, 0x49, 0x4E, 0x4F, 0x59, 0x52, 0x53, 0x4E, 0x53, 0x52, 0x4A, 0x40,
                    0x49, 0x54, 0x49, 0x4F, 0x4E, 0x4F, 0x53, 0x4F, 0x4F, 0x49, 0x40, 0x44, 0x44, 0x52, 0x53, 0x4F,
                    0x52, 0x44, 0x49, 0x4F, 0x42, 0x4F, 0x42, 0x49, 0x4A, 0x4E, 0x57, 0x52, 0x49, 0x40, 0x42, 0x49,
                    0x4F, 0x53, 0x42, 0x4F, 0x85, 0xDB, 0xD6, 0x8E, 0x87, 0x49, 0x87, 0xDB, 0xC7, 0x9B, 0x9B, 0x9B,
                    0x49, 0x40, 0x40, 0x4A, 0x42, 0x53, 0x4E, 0x4F, 0x42, 0x57, 0x4A, 0x53, 0x42, 0x4E, 0x44, 0x4F,
                    0x44, 0x49, 0x4E, 0x54, 0x54, 0x52, 0x53, 0x49, 0x4F, 0x57, 0x4F, 0x4F, 0x4F, 0x4E, 0x53, 0x57,
                    0x57, 0x52, 0x57, 0x49, 0x57, 0x54, 0x4F, 0x49, 0x57, 0x53, 0x52, 0x40, 0x85, 0x49, 0x4F, 0x44,
                    0x82, 0x8C, 0x89, 0xC7, 0x9B, 0x9B, 0x9B, 0x9B, 0x9E, 0x9E, 0x9B, 0x9B, 0x9B, 0x9B, 0x9D, 0x9B,
                    0x40, 0x40, 0x52, 0x4F, 0x4E, 0x49, 0x59, 0x49, 0x59, 0x53, 0x54, 0x4F, 0x53, 0x4F, 0x4E, 0x53,
                    0x44, 0x42, 0x59, 0x52, 0x53, 0x40, 0x52, 0x59, 0x49, 0x40, 0x42, 0x4A, 0x49, 0x4F, 0x40, 0x53,
                    0x4E, 0x42, 0x4E, 0x44, 0x4E, 0x4F, 0x40, 0x40, 0x52, 0x53, 0xD9, 0xFB, 0x9B, 0x9B, 0x9B, 0x9D,
                    0x9D, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9D, 0x9D, 0x9D, 0x9D, 0x9B, 0x9B, 0x9B, 0x9D,
                    0x40, 0x40, 0x4F, 0x4E, 0x54, 0x4E, 0x52, 0x53, 0x4E, 0x57, 0x59, 0x49, 0x4E, 0x54, 0x44, 0x52,
                    0x4E, 0x49, 0x52, 0x4F, 0x42, 0x40, 0x40, 0x49, 0x40, 0x53, 0x57, 0x52, 0x40, 0x4F, 0x42, 0x4E,
                    0x54, 0x42, 0x53, 0x52, 0x59, 0x49, 0x42, 0x40, 0x42, 0xE4, 0xFB, 0xDC, 0x94, 0xC3, 0x94, 0x9E,
                    0xE8, 0x9B, 0x9B, 0x9B, 0xDC, 0x97, 0xC3, 0xB5, 0xB6, 0xD9, 0x9B, 0x9D, 0x9B, 0x94, 0xD9, 0x87,
                    0x40, 0x42, 0x52, 0x59, 0x4F, 0x4E, 0x42, 0x40, 0x4F, 0x4F, 0x52, 0x4E, 0x49, 0x49, 0x49, 0x42,
                    0x49, 0x4A, 0x49, 0x49, 0x49, 0x40, 0x40, 0x40, 0x42, 0x4F, 0x52, 0x42, 0x49, 0x42, 0x4F, 0x52,
                    0x53, 0x4F, 0x49, 0x49, 0x59, 0x53, 0x49, 0x42, 0xE4, 0xFB, 0x97, 0x97, 0x49, 0x42, 0x42, 0xB2,
                    0x9D, 0x9D, 0x9D, 0x9E, 0x9A, 0xFB, 0x54, 0x4F, 0x49, 0x49, 0x87, 0x87, 0x53, 0x4F, 0x4E, 0x4A,
                    0x52, 0x49, 0x54, 0x53, 0x52, 0x49, 0x53, 0x5A, 0x5A, 0x40, 0x53, 0x4E, 0x4F, 0x49, 0x4E, 0x53,
                    0x52, 0x42, 0x57, 0x59, 0x49, 0x53, 0x4E, 0x49, 0x49, 0x52, 0x49, 0x49, 0x4F, 0x42, 0x4F, 0x53,
                    0x59, 0x4F, 0x49, 0x49, 0x49, 0x40, 0x40, 0x87, 0xFB, 0x9B, 0xDC, 0x97, 0x53, 0x53, 0x4F, 0x84,
                    0x9D, 0xE8, 0x9E, 0x9E, 0x9F, 0x9E, 0x9D, 0x84, 0x4F, 0x4E, 0x54, 0x57, 0x4F, 0x4F, 0x5A, 0x52,
                    0x42, 0x40, 0x4E, 0x4F, 0x59, 0x4E, 0x49, 0x52, 0x49, 0x42, 0x4E, 0x49, 0x4A, 0x49, 0x42, 0x53,
                    0x44, 0x42, 0x52, 0x59, 0x4E, 0x59, 0x57, 0x4E, 0x4A, 0x4A, 0x42, 0x42, 0x49, 0x40, 0x4E, 0x54,
                    0x59, 0x49, 0x59, 0x4F, 0x40, 0x40, 0x4E, 0x4F, 0x95, 0x9B, 0xE7, 0x9B, 0xC1, 0x59, 0x4E, 0x4F,
                    0xC5, 0x9E, 0x9D, 0x9E, 0x9D, 0x9D, 0xDC, 0x5F, 0x4F, 0x4F, 0x52, 0x52, 0x4F, 0x52, 0x59, 0x59,
                    0x49, 0x53, 0x53, 0x59, 0x5A, 0x4F, 0x40, 0x52, 0x59, 0x57, 0x52, 0x59, 0x42, 0x49, 0x40, 0x54,
                    0x53, 0x42, 0x40, 0x52, 0x52, 0x52, 0x4F, 0x4E, 0x49, 0x49, 0x57, 0x49, 0x4E, 0x59, 0x54, 0x4E,
                    0x52, 0x4E, 0x4E, 0x42, 0x42, 0x4F, 0x49, 0x52, 0x8E, 0x9B, 0x9B, 0x97, 0x8A, 0x40, 0x49, 0x4F,
                    0x4F, 0xB3, 0xE8, 0x9D, 0x9D, 0x9B, 0x94, 0x80, 0x49, 0x52, 0x57, 0x5F, 0x57, 0x57, 0x52, 0x52,
                    0x44, 0x53, 0x4E, 0x49, 0x52, 0x59, 0x52, 0x54, 0x4E, 0x5A, 0x49, 0x53, 0x53, 0x54, 0x40, 0x42,
                    0x4A, 0x53, 0x44, 0x59, 0x57, 0x5A, 0x54, 0x4F, 0x4F, 0x4E, 0x59, 0x49, 0x53, 0x4F, 0x4E, 0x49,
                    0x49, 0x5A, 0x59, 0x5A, 0x40, 0x53, 0x44, 0x52, 0x4F, 0xC7, 0x9B, 0x9B, 0xB5, 0x42, 0x4F, 0x4F,
                    0x49, 0x59, 0x9B, 0x9D, 0x9D, 0x9D, 0x9A, 0x84, 0x49, 0x52, 0x59, 0x57, 0x54, 0x57, 0x5F, 0x57,
                    0x4A, 0x4E, 0x42, 0x42, 0x59, 0x54, 0x49, 0x49, 0x4F, 0x59, 0x4E, 0x42, 0x49, 0x52, 0x42, 0x4E,
                    0x53, 0x4E, 0x52, 0x4F, 0x42, 0x49, 0x40, 0x57, 0x59, 0x53, 0x53, 0x49, 0x42, 0x52, 0x53, 0x49,
                    0x4E, 0x4E, 0x54, 0x5F, 0x4E, 0x44, 0x4F, 0x42, 0x53, 0x87, 0x9B, 0x97, 0xB1, 0x40, 0x4A, 0x4F,
                    0x4E, 0x54, 0x9D, 0x9E, 0x9E, 0x9F, 0x9F, 0x9E, 0xD8, 0x4F, 0x4F, 0x4F, 0x4F, 0x4E, 0x52, 0x57,
                    0x4E, 0x4E, 0x4F, 0x54, 0x54, 0x59, 0x5A, 0x49, 0x44, 0x4A, 0x59, 0x4E, 0x4E, 0x53, 0x53, 0x54,
                    0x4F, 0x4E, 0x42, 0x5A, 0x54, 0x52, 0x44, 0x54, 0x53, 0x42, 0x40, 0x4F, 0x52, 0x53, 0x54, 0x5A,
                    0x4F, 0x42, 0x52, 0x52, 0x54, 0x57, 0x59, 0x54, 0x49, 0xDA, 0x9B, 0x9B, 0x90, 0x42, 0x4A, 0x4F,
                    0x4F, 0x57, 0xCA, 0xE8, 0xE8, 0x9E, 0x9D, 0xE7, 0xC7, 0xB3, 0x52, 0x59, 0x52, 0x4F, 0x4F, 0x4F,
                    0x53, 0x42, 0x4A, 0x59, 0x4E, 0x5A, 0x53, 0x40, 0x49, 0x4E, 0x59, 0x4E, 0x40, 0x4F, 0x54, 0x59,
                    0x42, 0x53, 0x42, 0x49, 0x57, 0x53, 0x54, 0x53, 0x53, 0x4A, 0x49, 0x4F, 0x54, 0x49, 0x4E, 0x57,
                    0x53, 0x4E, 0x4F, 0x4F, 0x42, 0x4E, 0x4E, 0x4E, 0x54, 0x8B, 0x9D, 0x9D, 0x9D, 0x9A, 0x5F, 0x4F,
                    0x49, 0x4F, 0x52, 0xC5, 0x9D, 0x9D, 0x9D, 0x9B, 0x90, 0x80, 0x4F, 0x57, 0x53, 0x52, 0x4F, 0x4F,
                    0x52, 0x52, 0x59, 0x4F, 0x4F, 0x49, 0x59, 0x53, 0x42, 0x4F, 0x4F, 0x42, 0x49, 0x4E, 0x49, 0x54,
                    0x4E, 0x4A, 0x42, 0x49, 0x42, 0x54, 0x54, 0x52, 0x53, 0x54, 0x4F, 0x49, 0x54, 0x42, 0x49, 0x59,
                    0x53, 0x52, 0x49, 0x4F, 0x52, 0x53, 0x59, 0x4F, 0x52, 0x4F, 0x92, 0x9E, 0x9D, 0x9D, 0xDA, 0x4F,
                    0x49, 0x49, 0x4F, 0x80, 0x9B, 0x9D, 0x9D, 0x9B, 0xC4, 0x59, 0x42, 0x4F, 0x52, 0x52, 0x52, 0x4F,
                    0x59, 0x59, 0x59, 0x52, 0x42, 0x42, 0x57, 0x42, 0x4F, 0x44, 0x49, 0x40, 0x42, 0x4F, 0x4A, 0x4E,
                    0x49, 0x4E, 0x54, 0x4E, 0x42, 0x4E, 0x4F, 0x53, 0x4F, 0x52, 0x44, 0x53, 0x57, 0x53, 0x49, 0x4A,
                    0x59, 0x57, 0x4F, 0x54, 0x54, 0x52, 0x52, 0x42, 0x4E, 0x59, 0x4A, 0x8C, 0x9B, 0x9B, 0xB7, 0x42,
                    0x4F, 0x49, 0x4F, 0x4E, 0x9A, 0x9D, 0x9E, 0x9E, 0x9A, 0xC4, 0x52, 0x57, 0x57, 0x52, 0x54, 0x52,
                    0x5A, 0x54, 0x40, 0x42, 0x49, 0x54, 0x53, 0x53, 0x59, 0x4F, 0x54, 0x40, 0x42, 0x54, 0x40, 0x49,
                    0x53, 0x4E, 0x42, 0x44, 0x4F, 0x4F, 0x4E, 0x49, 0x4A, 0x4E, 0x4F, 0x40, 0x49, 0x4F, 0x49, 0x4F,
                    0x59, 0x52, 0x52, 0x57, 0x49, 0x44, 0x42, 0x40, 0x49, 0x4E, 0x42, 0xD7, 0x9B, 0x97, 0x59, 0x40,
                    0x42, 0x49, 0x4F, 0x85, 0xCA, 0xE8, 0x9D, 0x9E, 0x9E, 0x9D, 0x9B, 0x87, 0x52, 0x52, 0x59, 0x52,
                    0x5A, 0x4A, 0x40, 0x4F, 0x57, 0x4E, 0x40, 0x4E, 0x4E, 0x4E, 0x53, 0x59, 0x54, 0x53, 0x44, 0x4E,
                    0x4F, 0x4E, 0x54, 0x40, 0x40, 0x4A, 0x53, 0x4E, 0x54, 0x42, 0x4F, 0x4F, 0x4A, 0x4F, 0x4E, 0x44,
                    0x5A, 0x4E, 0x4F, 0x57, 0x49, 0x49, 0x53, 0x54, 0x52, 0x40, 0x40, 0xD7, 0x9D, 0x9D, 0x9E, 0xD3,
                    0x8C, 0x4F, 0x4E, 0x4F, 0x87, 0x9E, 0x9E, 0x9D, 0x9E, 0x9E, 0xC7, 0x85, 0x52, 0x4F, 0x52, 0x52,
                    0x4A, 0x42, 0x44, 0x53, 0x42, 0x53, 0x54, 0x4F, 0x57, 0x57, 0x40, 0x49, 0x52, 0x49, 0x40, 0x42,
                    0x4A, 0x52, 0x4E, 0x53, 0x53, 0x49, 0x42, 0x44, 0x40, 0x40, 0x4A, 0x52, 0x53, 0x4A, 0x42, 0x49,
                    0x44, 0x40, 0x49, 0x49, 0x42, 0x49, 0x42, 0x4F, 0x49, 0x4E, 0x42, 0x42, 0xD3, 0x9F, 0x9F, 0x9E,
                    0xDA, 0x42, 0x4F, 0x4F, 0x42, 0x84, 0x9D, 0x9B, 0x9B, 0x9B, 0xC1, 0x40, 0x4A, 0x4F, 0x4F, 0x52,
                    0x42, 0x59, 0x57, 0x42, 0x59, 0x53, 0x5A, 0x44, 0x53, 0x42, 0x4E, 0x4F, 0x4E, 0x53, 0x40, 0x4F,
                    0x4E, 0x4F, 0x52, 0x4F, 0x4F, 0x53, 0x4E, 0x53, 0x4E, 0x5A, 0x52, 0x42, 0x4F, 0x5A, 0x4E, 0x53,
                    0x53, 0x44, 0x49, 0x42, 0x44, 0x53, 0x49, 0x44, 0x57, 0x49, 0x49, 0x59, 0x52, 0x96, 0x9D, 0x9B,
                    0xB5, 0x42, 0x4E, 0x4F, 0x4E, 0xC4, 0xE7, 0x9B, 0x9B, 0xC7, 0xBF, 0x42, 0x4A, 0x52, 0x4F, 0x57,
                    0x4E, 0x4A, 0x4E, 0x40, 0x4E, 0x42, 0x44, 0x4F, 0x40, 0x4A, 0x44, 0x42, 0x42, 0x40, 0x52, 0x59,
                    0x42, 0x53, 0x53, 0x49, 0x40, 0x4F, 0x42, 0x4F, 0x5A, 0x59, 0x4F, 0x4F, 0x4E, 0x4F, 0x49, 0x52,
                    0x59, 0x44, 0x4E, 0x57, 0x54, 0x59, 0x42, 0x4E, 0x44, 0x40, 0x42, 0x57, 0x4F, 0x87, 0x97, 0xFB,
                    0xB2, 0x40, 0x49, 0x59, 0xC5, 0x9B, 0x9B, 0x9B, 0xC7, 0x90, 0x81, 0x4A, 0x52, 0x52, 0x52, 0x57,
                    0x59, 0x54, 0x54, 0x53, 0x59, 0x42, 0x49, 0x59, 0x4A, 0x53, 0x4E, 0x40, 0x40, 0x52, 0x4F, 0x49,
                    0x44, 0x42, 0x40, 0x49, 0x4E, 0x49, 0x49, 0x59, 0x5A, 0x57, 0x52, 0x57, 0x54, 0x53, 0x52, 0x53,
                    0x4F, 0x49, 0x44, 0x59, 0x57, 0x4E, 0x4E, 0x42, 0x40, 0x53, 0x59, 0x52, 0x59, 0xC5, 0x9B, 0x9B,
                    0xC1, 0x44, 0x83, 0x9B, 0x9B, 0x9B, 0x9B, 0x94, 0x90, 0x81, 0x49, 0x4F, 0x4F, 0x4F, 0x52, 0x4F,
                    0x49, 0x49, 0x4E, 0x42, 0x49, 0x49, 0x40, 0x54, 0x53, 0x52, 0x59, 0x44, 0x59, 0x49, 0x49, 0x42,
                    0x40, 0x54, 0x42, 0x42, 0x42, 0x49, 0x4F, 0x4A, 0x52, 0x49, 0x42, 0x4F, 0x53, 0x59, 0x42, 0x4E,
                    0x57, 0x49, 0x53, 0x4F, 0x59, 0x59, 0x4F, 0x4E, 0x40, 0x44, 0x42, 0x49, 0x42, 0x96, 0x9E, 0x9D,
                    0x9B, 0xDC, 0x9B, 0x9B, 0x9B, 0x9B, 0xC4, 0x90, 0x4E, 0x42, 0x49, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                    0x49, 0x40, 0x40, 0x40, 0x4E, 0x57, 0x4F, 0x44, 0x4F, 0x42, 0x42, 0x53, 0x42, 0x40, 0x52, 0x4F,
                    0x4E, 0x52, 0x4A, 0x54, 0x44, 0x52, 0x57, 0x42, 0x49, 0x52, 0x4E, 0x57, 0x57, 0x52, 0x49, 0x49,
                    0x44, 0x42, 0x4E, 0x4F, 0x4F, 0x42, 0x59, 0x52, 0x42, 0x49, 0x40, 0x57, 0x59, 0x49, 0x8E, 0x9D,
                    0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0xC7, 0x9B, 0x53, 0x42, 0x42, 0x49, 0x4F, 0x4F, 0x4E, 0x4F, 0x4F,
                    0x42, 0x40, 0x4F, 0x42, 0x42, 0x53, 0x4A, 0x59, 0x53, 0x40, 0x49, 0x52, 0x4F, 0x4E, 0x4E, 0x4E,
                    0x4F, 0x54, 0x59, 0x4E, 0x4F, 0x53, 0x4E, 0x4E, 0x42, 0x4E, 0x44, 0x4F, 0x59, 0x52, 0x4E, 0x40,
                    0x49, 0x52, 0x49, 0x49, 0x42, 0x49, 0x52, 0x4F, 0x42, 0x4F, 0x52, 0x44, 0x4E, 0x53, 0x4A, 0xD3,
                    0x9D, 0x9E, 0x9E, 0x9E, 0x9E, 0x9F, 0xD7, 0x87, 0x84, 0xB3, 0x8C, 0xD3, 0x96, 0xB4, 0x52, 0x4F,
                    0x52, 0x4A, 0x49, 0x4E, 0x44, 0x53, 0x42, 0x4E, 0x57, 0x49, 0x49, 0x40, 0x53, 0x49, 0x42, 0x4E,
                    0x59, 0x57, 0x4F, 0x57, 0x49, 0x53, 0x53, 0x4E, 0x4F, 0x44, 0x49, 0x42, 0x49, 0x4F, 0x53, 0x54,
                    0x4A, 0x49, 0x53, 0x4E, 0x52, 0x53, 0x4F, 0x53, 0x49, 0x52, 0x53, 0x4A, 0x4F, 0x54, 0x4F, 0x88,
                    0x9D, 0x9E, 0x9D, 0x9D, 0x9D, 0x9A, 0xCB, 0xCE, 0x9E, 0x9B, 0xE8, 0xCE, 0xE8, 0xE8, 0x95, 0x57,
                    0x42, 0x49, 0x52, 0x4E, 0x42, 0x4E, 0x40, 0x42, 0x52, 0x52, 0x52, 0x4E, 0x4E, 0x40, 0x42, 0x53,
                    0x42, 0x4F, 0x4E, 0x49, 0x40, 0x42, 0x52, 0x49, 0x54, 0x57, 0x53, 0x42, 0x42, 0x4E, 0x54, 0x57,
                    0x59, 0x57, 0x4A, 0x53, 0x5A, 0x4E, 0x53, 0x53, 0x4F, 0x57, 0x4A, 0x40, 0x42, 0x49, 0x57, 0x59,
                    0xC7, 0x9B, 0x9B, 0x9B, 0x9D, 0xCE, 0xCE, 0xE8, 0xE8, 0x9D, 0xCE, 0x98, 0x87, 0xD1, 0x9E, 0xCA,
                    0x54, 0x53, 0x59, 0x53, 0x49, 0x4A, 0x59, 0x59, 0x59, 0x54, 0x54, 0x53, 0x4E, 0x4F, 0x52, 0x53,
                    0x53, 0x54, 0x49, 0x49, 0x49, 0x42, 0x54, 0x4F, 0x53, 0x4F, 0x59, 0x4F, 0x53, 0x59, 0x52, 0x53,
                    0x4E, 0x4A, 0x49, 0x52, 0x57, 0x53, 0x57, 0x57, 0x4A, 0x4F, 0x40, 0x42, 0x52, 0x4F, 0x44, 0x53,
                    0x9D, 0x9D, 0x9E, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0xCE, 0xDB, 0x9B, 0xD2, 0xD3,
                    0x53, 0x49, 0x54, 0x54, 0x4F, 0x4F, 0x57, 0x59, 0x59, 0x4E, 0x49, 0x49, 0x40, 0x4E, 0x4F, 0x42,
                    0x4F, 0x53, 0x4E, 0x49, 0x53, 0x53, 0x4F, 0x4F, 0x4E, 0x4F, 0x52, 0x49, 0x53, 0x54, 0x59, 0x52,
                    0x42, 0x42, 0x49, 0x49, 0x4F, 0x4E, 0x54, 0x49, 0x4E, 0x49, 0x4F, 0x4E, 0x54, 0x44, 0x40, 0x4E,
                    0x52, 0x9F, 0x9F, 0xCF, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0xE8, 0x9E,
                    0x4F, 0x44, 0x42, 0x4F, 0x4E, 0x53, 0x4F, 0x4F, 0x4F, 0x4F, 0x52, 0x59, 0x53, 0x4F, 0x53, 0x42,
                    0x49, 0x4E, 0x49, 0x54, 0x40, 0x42, 0x4E, 0x49, 0x57, 0x5F, 0x4E, 0x57, 0x40, 0x40, 0x4E, 0x4F,
                    0x4F, 0x59, 0x49, 0x4E, 0x59, 0x53, 0x53, 0x4F, 0x4E, 0x52, 0x49, 0x4E, 0x49, 0x44, 0x42, 0x49,
                    0x54, 0x85, 0xDE, 0x9F, 0x9F, 0x9F, 0xD2, 0x96, 0xD2, 0xE8, 0xCE, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F,
                    0x4F, 0x49, 0x4E, 0x57, 0x53, 0x4A, 0x59, 0x52, 0x54, 0x4F, 0x4F, 0x40, 0x52, 0x53, 0x44, 0x42,
                    0x4A, 0x53, 0x49, 0x49, 0x49, 0x42, 0x54, 0x42, 0x4E, 0x53, 0x4F, 0x49, 0x49, 0x52, 0x54, 0x42,
                    0x42, 0x4F, 0x42, 0x4F, 0x4E, 0x42, 0x4E, 0x59, 0x52, 0x53, 0x53, 0x52, 0x42, 0x59, 0x54, 0x42,
                    0x42, 0x4F, 0x40, 0xD4, 0x9E, 0x9E, 0xD3, 0x92, 0x92, 0x96, 0x92, 0xD2, 0x9F, 0x9F, 0x9F, 0x9F,
                    0x49, 0x54, 0x52, 0x4F, 0x54, 0x5A, 0x49, 0x57, 0x54, 0x5A, 0x49, 0x40, 0x49, 0x57, 0x42, 0x4F,
                    0x53, 0x53, 0x53, 0x42, 0x53, 0x42, 0x57, 0x4F, 0x49, 0x4A, 0x4F, 0x4E, 0x49, 0x4E, 0x4F, 0x40,
                    0x54, 0x4A, 0x40, 0x4E, 0x40, 0x4E, 0x59, 0x4A, 0x54, 0x4E, 0x53, 0x53, 0x49, 0x59, 0x49, 0x49,
                    0x49, 0x4E, 0x40, 0x4E, 0x9B, 0x9D, 0xD4, 0x92, 0x92, 0x92, 0x92, 0xCA, 0x9E, 0x9F, 0x9F, 0x9F,
                    0x5A, 0x52, 0x57, 0x52, 0x49, 0x53, 0x4E, 0x53, 0x57, 0x54, 0x57, 0x49, 0x52, 0x4E, 0x49, 0x44,
                    0x4E, 0x52, 0x49, 0x59, 0x4F, 0x53, 0x4F, 0x4F, 0x54, 0x40, 0x4A, 0x40, 0x4A, 0x49, 0x4F, 0x4F,
                    0x4F, 0x4F, 0x40, 0x42, 0x40, 0x40, 0x54, 0x53, 0x4A, 0x42, 0x4E, 0x49, 0x53, 0x5A, 0x53, 0x42,
                    0x4E, 0x42, 0x40, 0x4E, 0x8E, 0x9D, 0x9D, 0xD4, 0x96, 0x92, 0x92, 0x92, 0x98, 0x9F, 0x9F, 0x9F,
                    0x40, 0x4E, 0x54, 0x54, 0x53, 0x42, 0x4F, 0x4A, 0x54, 0x40, 0x4F, 0x4E, 0x40, 0x4A, 0x4E, 0x42,
                    0x57, 0x49, 0x52, 0x5A, 0x49, 0x4F, 0x42, 0x49, 0x4E, 0x4E, 0x4E, 0x49, 0x49, 0x52, 0x49, 0x44,
                    0x42, 0x53, 0x4E, 0x49, 0x42, 0x4A, 0x53, 0x49, 0x4F, 0x4E, 0x53, 0x49, 0x49, 0x49, 0x49, 0x40,
                    0x59, 0x59, 0x52, 0x53, 0x82, 0xCA, 0x9D, 0x9D, 0xC6, 0x96, 0x92, 0x92, 0x92, 0xD3, 0xE8, 0x9E,
                    0x4F, 0x53, 0x54, 0x53, 0x49, 0x4F, 0x4E, 0x57, 0x5A, 0x53, 0x4A, 0x57, 0x42, 0x57, 0x49, 0x42,
                    0x49, 0x49, 0x4E, 0x49, 0x49, 0x4E, 0x42, 0x4F, 0x4E, 0x49, 0x40, 0x4E, 0x59, 0x53, 0x4F, 0x42,
                    0x52, 0x53, 0x40, 0x4E, 0x57, 0x54, 0x4A, 0x4F, 0x4A, 0x4A, 0x4E, 0x4F, 0x49, 0x40, 0x42, 0x4F,
                    0x54, 0x4F, 0x40, 0x4E, 0x42, 0x4E, 0x9B, 0x9B, 0xB4, 0x4F, 0x8E, 0x92, 0x92, 0xD3, 0x9E, 0x9B,
                    0x4E, 0x49, 0x4E, 0x4F, 0x4F, 0x57, 0x53, 0x52, 0x42, 0x49, 0x53, 0x54, 0x4F, 0x59, 0x54, 0x53,
                    0x54, 0x59, 0x4F, 0x42, 0x49, 0x42, 0x4E, 0x49, 0x4F, 0x53, 0x52, 0x53, 0x4E, 0x44, 0x59, 0x4F,
                    0x4E, 0x54, 0x49, 0x4E, 0x4E, 0x4F, 0x53, 0x4F, 0x4E, 0x52, 0x54, 0x59, 0x53, 0x4F, 0x59, 0x53,
                    0x49, 0x54, 0x42, 0x53, 0x49, 0x53, 0xC7, 0x9B, 0x9B, 0x84, 0x4A, 0x85, 0x92, 0xD2, 0x9E, 0x9E,
                    0x57, 0x59, 0x42, 0x49, 0x49, 0x49, 0x53, 0x5A, 0x4E, 0x59, 0x59, 0x49, 0x4E, 0x49, 0x57, 0x53,
                    0x49, 0x53, 0x4F, 0x42, 0x49, 0x4F, 0x4E, 0x44, 0x53, 0x42, 0x4F, 0x4A, 0x49, 0x49, 0x52, 0x57,
                    0x42, 0x52, 0x53, 0x53, 0x4F, 0x49, 0x4F, 0x40, 0x4E, 0x49, 0x49, 0x49, 0x53, 0x4F, 0x52, 0x49,
                    0x42, 0x4A, 0x42, 0x42, 0x44, 0x49, 0xD4, 0x9D, 0x9D, 0x9B, 0xDB, 0x4F, 0x57, 0x8F, 0xCE, 0xE8,
                    0x53, 0x4F, 0x4E, 0x4E, 0x4F, 0x49, 0x5A, 0x59, 0x59, 0x59, 0x49, 0x4E, 0x42, 0x49, 0x49, 0x44,
                    0x4F, 0x49, 0x49, 0x40, 0x53, 0x42, 0x49, 0x42, 0x54, 0x57, 0x42, 0x49, 0x44, 0x4F, 0x4F, 0x4E,
                    0x42, 0x53, 0x57, 0x42, 0x42, 0x4E, 0x49, 0x49, 0x4F, 0x49, 0x42, 0x4F, 0x57, 0x57, 0x54, 0x42,
                    0x4E, 0x40, 0x44, 0x44, 0x42, 0x44, 0x40, 0xD4, 0x9D, 0x9D, 0x9B, 0x84, 0x4A, 0x42, 0x4F, 0xD6,
                    0x54, 0x5A, 0x5A, 0x4F, 0x49, 0x5A, 0x4E, 0x52, 0x57, 0x4F, 0x4E, 0x59, 0x49, 0x49, 0x52, 0x42,
                    0x4E, 0x4E, 0x4A, 0x42, 0x49, 0x42, 0x53, 0x4E, 0x49, 0x59, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x53,
                    0x4E, 0x49, 0x4F, 0x4F, 0x49, 0x49, 0x42, 0x4F, 0x4A, 0x42, 0x40, 0x40, 0x52, 0x4E, 0x40, 0x44,
                    0x57, 0x49, 0x49, 0x42, 0x4E, 0x4E, 0x42, 0x49, 0xCA, 0x9B, 0xFB, 0x59, 0x49, 0x49, 0x42, 0x40,
                    0x49, 0x59, 0x42, 0x49, 0x53, 0x4F, 0x42, 0x49, 0x57, 0x52, 0x52, 0x4E, 0x42, 0x40, 0x44, 0x40,
                    0x52, 0x4E, 0x59, 0x57, 0x4F, 0x57, 0x49, 0x53, 0x49, 0x59, 0x49, 0x4E, 0x53, 0x59, 0x4E, 0x49,
                    0x49, 0x40, 0x5A, 0x4F, 0x42, 0x54, 0x4F, 0x57, 0x53, 0x49, 0x4E, 0x49, 0x42, 0x42, 0x40, 0x53,
                    0x4E, 0x49, 0x4E, 0x42, 0x52, 0x4E, 0x42, 0x49, 0xC5, 0x9B, 0x9B, 0x9B, 0xD7, 0x57, 0x4A, 0x40,
                    0x4F, 0x4F, 0x49, 0x4F, 0x49, 0x4A, 0x53, 0x42, 0x49, 0x4E, 0x4F, 0x42, 0x52, 0x57, 0x40, 0x4F,
                    0x42, 0x53, 0x42, 0x4F, 0x4E, 0x49, 0x40, 0x42, 0x4F, 0x49, 0x49, 0x42, 0x42, 0x49, 0x49, 0x4E,
                    0x4F, 0x53, 0x54, 0x42, 0x42, 0x49, 0x40, 0x40, 0x42, 0x42, 0x4F, 0x42, 0x40, 0x4F, 0x4A, 0x42,
                    0x49, 0x52, 0x53, 0x42, 0x4F, 0x4F, 0x4E, 0x4A, 0x9D, 0x9E, 0x9F, 0x9F, 0x9D, 0x84, 0x40, 0x40,
                    0x4E, 0x4F, 0x49, 0x49, 0x4E, 0x53, 0x53, 0x57, 0x53, 0x4E, 0x49, 0x49, 0x49, 0x53, 0x44, 0x4F,
                    0x59, 0x53, 0x53, 0x54, 0x49, 0x49, 0x49, 0x42, 0x40, 0x4E, 0x54, 0x4E, 0x53, 0x4F, 0x4F, 0x49,
                    0x4F, 0x59, 0x52, 0x4F, 0x4E, 0x4F, 0x42, 0x40, 0x4A, 0x4F, 0x57, 0x49, 0x49, 0x4E, 0x42, 0x44,
                    0x54, 0x53, 0x40, 0x59, 0x44, 0x4F, 0x49, 0x4F, 0x4E, 0x98, 0x9F, 0x9D, 0xFB, 0x59, 0x40, 0xD8,
                    0x42, 0x4E, 0x4F, 0x4E, 0x4F, 0x49, 0x59, 0x4F, 0x52, 0x49, 0x53, 0x4E, 0x42, 0x4F, 0x42, 0x53,
                    0x49, 0x42, 0x4F, 0x53, 0x4E, 0x49, 0x53, 0x53, 0x49, 0x53, 0x40, 0x49, 0x53, 0x4E, 0x49, 0x52,
                    0x53, 0x44, 0x49, 0x4F, 0x59, 0x53, 0x53, 0x59, 0x49, 0x42, 0x49, 0x49, 0x4E, 0x4F, 0x40, 0x49,
                    0x4E, 0x40, 0x42, 0x4E, 0x4F, 0x53, 0x49, 0x4E, 0x4F, 0x4F, 0x8C, 0x9B, 0x9B, 0xB1, 0x4E, 0x97,
                    0x52, 0x53, 0x52, 0x4F, 0x54, 0x54, 0x4E, 0x44, 0x53, 0x42, 0x49, 0x53, 0x4E, 0x42, 0x53, 0x42,
                    0x53, 0x53, 0x53, 0x4F, 0x49, 0x54, 0x40, 0x42, 0x4F, 0x59, 0x42, 0x42, 0x42, 0x49, 0x4A, 0x49,
                    0x59, 0x4E, 0x49, 0x44, 0x53, 0x4E, 0x49, 0x54, 0x59, 0x49, 0x4E, 0x4E, 0x4A, 0x4F, 0x52, 0x44,
                    0x40, 0x40, 0x49, 0x40, 0x49, 0x44, 0x49, 0x49, 0x5A, 0x42, 0xDB, 0x97, 0x97, 0x90, 0x59, 0x97,
                    0x59, 0x53, 0x42, 0x42, 0x49, 0x59, 0x59, 0x4F, 0x4F, 0x52, 0x49, 0x49, 0x4E, 0x42, 0x49, 0x49,
                    0x49, 0x53, 0x4A, 0x42, 0x49, 0x49, 0x49, 0x42, 0x44, 0x53, 0x42, 0x40, 0x4E, 0x4E, 0x40, 0x49,
                    0x4E, 0x53, 0x4A, 0x49, 0x4E, 0x40, 0x4F, 0x40, 0x4F, 0x54, 0x54, 0x49, 0x42, 0x4E, 0x54, 0x4F,
                    0x4F, 0x59, 0x4F, 0x4F, 0x42, 0x40, 0x4E, 0x53, 0x40, 0x4F, 0xC7, 0x9B, 0x9B, 0x97, 0xC6, 0x9B,
                    0x44, 0x4E, 0x4E, 0x4E, 0x42, 0x4E, 0x49, 0x42, 0x5A, 0x53, 0x49, 0x42, 0x40, 0x42, 0x40, 0x54,
                    0x49, 0x4E, 0x49, 0x42, 0x53, 0x42, 0x53, 0x42, 0x49, 0x42, 0x40, 0x44, 0x49, 0x4F, 0x4A, 0x4E,
                    0x40, 0x52, 0x4F, 0x42, 0x49, 0x42, 0x42, 0x40, 0x42, 0x40, 0x4A, 0x40, 0x42, 0x53, 0x53, 0x54,
                    0x59, 0x59, 0x52, 0x49, 0x42, 0x4E, 0x59, 0x59, 0x59, 0x49, 0x85, 0x9D, 0x9D, 0x9B, 0x9B, 0x9B,
                };

                /* update pass table */
                ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * grid_y_66 + grid_x_55] = SURFACE_TYPE_AIR;

                /* extend tile buffer */
                {
                    const int32_t tile_size{ResourceManager_DisableEnhancedGraphics ? (GFX_MAP_TILE_SIZE / 2)
                                                                                    : GFX_MAP_TILE_SIZE};
                    const uint32_t map_cell_count{
                        static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};
                    auto map_tile_buffer =
                        new (std::nothrow) uint8_t[(ResourceManager_MapTileCount + 1u) * tile_size * tile_size];

                    memcpy(map_tile_buffer, ResourceManager_MapTileBuffer,
                           ResourceManager_MapTileCount * tile_size * tile_size);

                    if (ResourceManager_DisableEnhancedGraphics) {
                        uint8_t *destination_address{
                            &map_tile_buffer[ResourceManager_MapTileCount * tile_size * tile_size]};

                        for (int32_t i = 0; i < tile_size; ++i) {
                            for (int32_t j = 0; j < tile_size; ++j) {
                                destination_address[i * tile_size + j] = tile_data[(i * GFX_MAP_TILE_SIZE + j) * 2];
                            }
                        }

                    } else {
                        memcpy(&map_tile_buffer[ResourceManager_MapTileCount * tile_size * tile_size], tile_data,
                               tile_size * tile_size);
                    }

                    delete[] ResourceManager_MapTileBuffer;

                    ResourceManager_MapTileBuffer = map_tile_buffer;
                }

                /* update tile count */
                ++ResourceManager_MapTileCount;

                /* update tile index vector */
                ResourceManager_MapTileIds[ResourceManager_MapSize.x * grid_y_66 + grid_x_55] = tile_411;
            }
        } break;
    }
}

void ResourceManager_InitMissionRegistry() {
    auto registry = std::make_unique<MissionRegistry>(ResourceManager_FilePathGamePref);

    ResourceManager_MissionRegistry = std::move(registry);
}
