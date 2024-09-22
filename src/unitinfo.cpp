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

#include "unitinfo.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "paths_manager.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "task_manager.hpp"
#include "unitinfogroup.hpp"
#include "units_manager.hpp"

static const std::vector<SoundElement> UnitInfo_SfxDefaultUnit = {{{SFX_TYPE_IDLE, GEN_IDLE},
                                                                   {SFX_TYPE_WATER_IDLE, WGN_IDLE},
                                                                   {SFX_TYPE_DRIVE, GEN_DRVE},
                                                                   {SFX_TYPE_WATER_DRIVE, WGN_DRVE},
                                                                   {SFX_TYPE_STOP, GEN_STOP},
                                                                   {SFX_TYPE_WATER_STOP, WGN_STOP},
                                                                   {SFX_TYPE_TRANSFORM, GEN_XFRM},
                                                                   {SFX_TYPE_BUILDING, GEN_BLDG},
                                                                   {SFX_TYPE_SHRINK, GEN_SHNK},
                                                                   {SFX_TYPE_EXPAND, GEN_XPND},
                                                                   {SFX_TYPE_TURRET, GEN_TRRT},
                                                                   {SFX_TYPE_FIRE, GEN_FIRE},
                                                                   {SFX_TYPE_HIT, GEN_HIT},
                                                                   {SFX_TYPE_EXPLOAD, GEN_XPLD},
                                                                   {SFX_TYPE_POWER_CONSUMPTION_START, GEN_PRCS},
                                                                   {SFX_TYPE_POWER_CONSUMPTION_END, GEN_PRCE},
                                                                   {SFX_TYPE_LAND, GEN_LAND},
                                                                   {SFX_TYPE_TAKE, GEN_TAKE}}};

static const std::vector<SoundElement> UnitInfo_SfxMonopoleMine = {{{SFX_TYPE_BUILDING, MONOP10},
                                                                    {SFX_TYPE_HIT, MONOP15},
                                                                    {SFX_TYPE_EXPLOAD, MONOP16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, MONOP17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, MONOP18}}};

static const std::vector<SoundElement> UnitInfo_SfxPowerStation = {{{SFX_TYPE_BUILDING, POWST10},
                                                                    {SFX_TYPE_HIT, POWST15},
                                                                    {SFX_TYPE_EXPLOAD, POWST16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, POWST17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, POWST18}}};

static const std::vector<SoundElement> UnitInfo_SfxPowerGenerator = {{{SFX_TYPE_BUILDING, POWGN10},
                                                                      {SFX_TYPE_HIT, POWGN15},
                                                                      {SFX_TYPE_EXPLOAD, POWGN16},
                                                                      {SFX_TYPE_POWER_CONSUMPTION_START, POWGN17},
                                                                      {SFX_TYPE_POWER_CONSUMPTION_END, POWGN18}}};

static const std::vector<SoundElement> UnitInfo_SfxBarracks = {{{SFX_TYPE_HIT, BARRA15}, {SFX_TYPE_EXPLOAD, BARRA16}}};

static const std::vector<SoundElement> UnitInfo_SfxGoldRefinery = {{{SFX_TYPE_BUILDING, GOLDR10},
                                                                    {SFX_TYPE_HIT, GOLDR15},
                                                                    {SFX_TYPE_EXPLOAD, GOLDR16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, GOLDR17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, GOLDR18}}};

static const std::vector<SoundElement> UnitInfo_SfxRadar = {
    {{SFX_TYPE_IDLE, RADAR13}, {SFX_TYPE_HIT, RADAR15}, {SFX_TYPE_EXPLOAD, RADAR16}}};

static const std::vector<SoundElement> UnitInfo_SfxMaterialStorage = {
    {{SFX_TYPE_HIT, SSTOR15}, {SFX_TYPE_EXPLOAD, SSTOR16}}};

static const std::vector<SoundElement> UnitInfo_SfxFuelStorage = {
    {{SFX_TYPE_HIT, SFUEL15}, {SFX_TYPE_EXPLOAD, SFUEL16}}};

static const std::vector<SoundElement> UnitInfo_SfxGoldVault = {{{SFX_TYPE_HIT, SGOLD15}, {SFX_TYPE_EXPLOAD, SGOLD16}}};

static const std::vector<SoundElement> UnitInfo_SfxDepot = {{{SFX_TYPE_HIT, DEPOT15}, {SFX_TYPE_EXPLOAD, DEPOT16}}};

static const std::vector<SoundElement> UnitInfo_SfxHangar = {{{SFX_TYPE_HIT, HANGR15}, {SFX_TYPE_EXPLOAD, HANGR16}}};

static const std::vector<SoundElement> UnitInfo_SfxDock = {{{SFX_TYPE_HIT, DOCK15}, {SFX_TYPE_EXPLOAD, DOCK16}}};

static const std::vector<SoundElement> UnitInfo_SfxRoad = {{{SFX_TYPE_HIT, ROAD15}, {SFX_TYPE_EXPLOAD, ROAD16}}};

static const std::vector<SoundElement> UnitInfo_SfxLandingPad = {{{SFX_TYPE_HIT, LPAD15}, {SFX_TYPE_EXPLOAD, LPAD16}}};

static const std::vector<SoundElement> UnitInfo_SfxShipyard = {{{SFX_TYPE_BUILDING, SUNIT10},
                                                                {SFX_TYPE_HIT, SUNIT15},
                                                                {SFX_TYPE_EXPLOAD, SUNIT16},
                                                                {SFX_TYPE_POWER_CONSUMPTION_START, SUNIT17},
                                                                {SFX_TYPE_POWER_CONSUMPTION_END, SUNIT18}}};

static const std::vector<SoundElement> UnitInfo_SfxLightVehiclePlant = {{{SFX_TYPE_BUILDING, LVP10},
                                                                         {SFX_TYPE_HIT, LVP15},
                                                                         {SFX_TYPE_EXPLOAD, LVP16},
                                                                         {SFX_TYPE_POWER_CONSUMPTION_START, LVP17},
                                                                         {SFX_TYPE_POWER_CONSUMPTION_END, LVP18}}};

static const std::vector<SoundElement> UnitInfo_SfxHeavyVehiclePlant = {{{SFX_TYPE_BUILDING, HVP10},
                                                                         {SFX_TYPE_HIT, HVP15},
                                                                         {SFX_TYPE_EXPLOAD, HVP16},
                                                                         {SFX_TYPE_POWER_CONSUMPTION_START, HVP17},
                                                                         {SFX_TYPE_POWER_CONSUMPTION_END, HVP18}}};

static const std::vector<SoundElement> UnitInfo_SfxAirUnitsPlant = {{{SFX_TYPE_BUILDING, AUNIT10},
                                                                     {SFX_TYPE_HIT, AUNIT15},
                                                                     {SFX_TYPE_EXPLOAD, AUNIT16},
                                                                     {SFX_TYPE_POWER_CONSUMPTION_START, AUNIT17},
                                                                     {SFX_TYPE_POWER_CONSUMPTION_END, AUNIT18}}};

static const std::vector<SoundElement> UnitInfo_SfxHabitat = {
    {{SFX_TYPE_BUILDING, DORMI10}, {SFX_TYPE_HIT, DORMI15}, {SFX_TYPE_EXPLOAD, DORMI16}}};

static const std::vector<SoundElement> UnitInfo_SfxResearchCentre = {{{SFX_TYPE_BUILDING, RESEAR10},
                                                                      {SFX_TYPE_HIT, RESEAR15},
                                                                      {SFX_TYPE_EXPLOAD, RESEAR16},
                                                                      {SFX_TYPE_POWER_CONSUMPTION_START, RESEAR17},
                                                                      {SFX_TYPE_POWER_CONSUMPTION_END, RESEAR18}}};

static const std::vector<SoundElement> UnitInfo_SfxEcoSphere = {{{SFX_TYPE_BUILDING, ECOSP10},
                                                                 {SFX_TYPE_HIT, ECOSP15},
                                                                 {SFX_TYPE_EXPLOAD, ECOSP16},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_START, ECOSP17},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_END, ECOSP18}}};

static const std::vector<SoundElement> UnitInfo_SfxTrainingHall = {{{SFX_TYPE_BUILDING, TRAIN10},
                                                                    {SFX_TYPE_HIT, TRAIN15},
                                                                    {SFX_TYPE_EXPLOAD, TRAIN16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, TRAIN17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, TRAIN18}}};

static const std::vector<SoundElement> UnitInfo_SfxWaterPlatform = {
    {{SFX_TYPE_HIT, WPLAT15}, {SFX_TYPE_EXPLOAD, WPLAT16}}};

static const std::vector<SoundElement> UnitInfo_SfxGunTurret = {
    {{SFX_TYPE_FIRE, FGUN14}, {SFX_TYPE_HIT, FGUN15}, {SFX_TYPE_EXPLOAD, FGUN16}}};

static const std::vector<SoundElement> UnitInfo_SfxArtillery = {
    {{SFX_TYPE_FIRE, FARTY14}, {SFX_TYPE_HIT, FARTY15}, {SFX_TYPE_EXPLOAD, FARTY16}}};

static const std::vector<SoundElement> UnitInfo_SfxAntiAir = {
    {{SFX_TYPE_FIRE, FANTI14}, {SFX_TYPE_HIT, FANTI15}, {SFX_TYPE_EXPLOAD, FANTI16}}};

static const std::vector<SoundElement> UnitInfo_SfxMissileLauncher = {
    {{SFX_TYPE_FIRE, FROCK14}, {SFX_TYPE_HIT, FROCK15}, {SFX_TYPE_EXPLOAD, FROCK16}}};

static const std::vector<SoundElement> UnitInfo_SfxConcreteBlock = {
    {{SFX_TYPE_HIT, BLOCK15}, {SFX_TYPE_EXPLOAD, BLOCK16}}};

static const std::vector<SoundElement> UnitInfo_SfxBridge = {{{SFX_TYPE_HIT, BRIDG15}, {SFX_TYPE_EXPLOAD, BRIDG16}}};

static const std::vector<SoundElement> UnitInfo_SfxMiningStation = {{{SFX_TYPE_BUILDING, MSTAT10},
                                                                     {SFX_TYPE_HIT, MSTAT15},
                                                                     {SFX_TYPE_EXPLOAD, MSTAT16},
                                                                     {SFX_TYPE_POWER_CONSUMPTION_START, MSTAT17},
                                                                     {SFX_TYPE_POWER_CONSUMPTION_END, MSTAT18}}};

static const std::vector<SoundElement> UnitInfo_SfxLandMine = {{{SFX_TYPE_EXPLOAD, LMINE16}}};

static const std::vector<SoundElement> UnitInfo_SfxSeaMine = {{{SFX_TYPE_EXPLOAD, CMINE16}}};

static const std::vector<SoundElement> UnitInfo_SfxHitExplosion = {
    {{SFX_TYPE_HIT, EMPTYLND}, {SFX_TYPE_EXPLOAD, EMPTYWTR}}};

static const std::vector<SoundElement> UnitInfo_SfxMasterBuilder = {{{SFX_TYPE_IDLE, MASTR1},
                                                                     {SFX_TYPE_DRIVE, MASTR5},
                                                                     {SFX_TYPE_STOP, MASTR7},
                                                                     {SFX_TYPE_TRANSFORM, MASTR15},
                                                                     {SFX_TYPE_HIT, MASTR9},
                                                                     {SFX_TYPE_EXPLOAD, MASTR16}}};

static const std::vector<SoundElement> UnitInfo_SfxConstructor = {{{SFX_TYPE_IDLE, CONST1},
                                                                   {SFX_TYPE_DRIVE, CONST5},
                                                                   {SFX_TYPE_STOP, CONST7},
                                                                   {SFX_TYPE_BUILDING, CONST10},
                                                                   {SFX_TYPE_HIT, CONST15},
                                                                   {SFX_TYPE_EXPLOAD, CONST16}}};

static const std::vector<SoundElement> UnitInfo_SfxScout = {{{SFX_TYPE_IDLE, SCOUT1},
                                                             {SFX_TYPE_DRIVE, SCOUT5},
                                                             {SFX_TYPE_STOP, SCOUT7},
                                                             {SFX_TYPE_FIRE, SCOUT14},
                                                             {SFX_TYPE_HIT, SCOUT15},
                                                             {SFX_TYPE_EXPLOAD, SCOUT16}}};

static const std::vector<SoundElement> UnitInfo_SfxTank = {{{SFX_TYPE_IDLE, TANK1},
                                                            {SFX_TYPE_DRIVE, TANK5},
                                                            {SFX_TYPE_STOP, TANK7},
                                                            {SFX_TYPE_FIRE, TANK14},
                                                            {SFX_TYPE_HIT, TANK15},
                                                            {SFX_TYPE_EXPLOAD, TANK16}}};

static const std::vector<SoundElement> UnitInfo_SfxAssaultGun = {{{SFX_TYPE_IDLE, ASGUN1},
                                                                  {SFX_TYPE_DRIVE, ASGUN5},
                                                                  {SFX_TYPE_STOP, ASGUN7},
                                                                  {SFX_TYPE_FIRE, ASGUN14},
                                                                  {SFX_TYPE_HIT, ASGUN15},
                                                                  {SFX_TYPE_EXPLOAD, ASGUN16}}};

static const std::vector<SoundElement> UnitInfo_SfxRocketLauncher = {{{SFX_TYPE_IDLE, RLNCH1},
                                                                      {SFX_TYPE_DRIVE, RLNCH5},
                                                                      {SFX_TYPE_STOP, RLNCH7},
                                                                      {SFX_TYPE_FIRE, RLNCH14},
                                                                      {SFX_TYPE_HIT, RLNCH15},
                                                                      {SFX_TYPE_EXPLOAD, RLNCH16}}};

static const std::vector<SoundElement> UnitInfo_SfxMissileCrawler = {{{SFX_TYPE_IDLE, MSLNC1},
                                                                      {SFX_TYPE_DRIVE, MSLNC5},
                                                                      {SFX_TYPE_STOP, MSLNC7},
                                                                      {SFX_TYPE_FIRE, MSLNC14},
                                                                      {SFX_TYPE_HIT, MSLNC15},
                                                                      {SFX_TYPE_EXPLOAD, MSLNC16}}};

static const std::vector<SoundElement> MobileAntiAir = {{{SFX_TYPE_IDLE, MANTI1},
                                                         {SFX_TYPE_DRIVE, MANTI5},
                                                         {SFX_TYPE_STOP, MANTI7},
                                                         {SFX_TYPE_FIRE, MANTI14},
                                                         {SFX_TYPE_HIT, MANTI15},
                                                         {SFX_TYPE_EXPLOAD, MANTI16}}};

static const std::vector<SoundElement> UnitInfo_SfxMineLayer = {{{SFX_TYPE_IDLE, MLAYER1},
                                                                 {SFX_TYPE_DRIVE, MLAYER5},
                                                                 {SFX_TYPE_STOP, MLAYER7},
                                                                 {SFX_TYPE_HIT, MLAYER15},
                                                                 {SFX_TYPE_EXPLOAD, MLAYER16},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_START, MLAYER17},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_END, MLAYER18}}};

static const std::vector<SoundElement> UnitInfo_SfxSurveyor = {{{SFX_TYPE_IDLE, SURVY1},
                                                                {SFX_TYPE_DRIVE, SURVY5},
                                                                {SFX_TYPE_STOP, SURVY7},
                                                                {SFX_TYPE_HIT, SURVY15},
                                                                {SFX_TYPE_EXPLOAD, SURVY16}}};

static const std::vector<SoundElement> UnitInfo_SfxScanner = {{{SFX_TYPE_IDLE, SCAN1},
                                                               {SFX_TYPE_DRIVE, SCAN5},
                                                               {SFX_TYPE_STOP, SCAN7},
                                                               {SFX_TYPE_HIT, SCAN15},
                                                               {SFX_TYPE_EXPLOAD, SCAN16}}};

static const std::vector<SoundElement> UnitInfo_SfxSupplyTruck = {{{SFX_TYPE_IDLE, MTRUK1},
                                                                   {SFX_TYPE_DRIVE, MTRUK5},
                                                                   {SFX_TYPE_STOP, MTRUK7},
                                                                   {SFX_TYPE_HIT, MTRUK15},
                                                                   {SFX_TYPE_EXPLOAD, MTRUK16},
                                                                   {SFX_TYPE_POWER_CONSUMPTION_START, MTRUK17}}};

static const std::vector<SoundElement> UnitInfo_SfxGoldTruck = {{{SFX_TYPE_IDLE, GTRUK1},
                                                                 {SFX_TYPE_DRIVE, GTRUK5},
                                                                 {SFX_TYPE_STOP, GTRUK7},
                                                                 {SFX_TYPE_HIT, GTRUK15},
                                                                 {SFX_TYPE_EXPLOAD, GTRUK16},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_START, GTRUK17}}};

static const std::vector<SoundElement> UnitInfo_SfxEngineer = {{{SFX_TYPE_IDLE, ENGIN1},
                                                                {SFX_TYPE_DRIVE, ENGIN5},
                                                                {SFX_TYPE_STOP, ENGIN7},
                                                                {SFX_TYPE_BUILDING, ENGIN10},
                                                                {SFX_TYPE_HIT, ENGIN15},
                                                                {SFX_TYPE_EXPLOAD, ENGIN16}}};

static const std::vector<SoundElement> UnitInfo_SfxBulldozer = {{{SFX_TYPE_IDLE, BULL1},
                                                                 {SFX_TYPE_DRIVE, BULL5},
                                                                 {SFX_TYPE_STOP, BULL7},
                                                                 {SFX_TYPE_BUILDING, BULL10},
                                                                 {SFX_TYPE_HIT, BULL15},
                                                                 {SFX_TYPE_EXPLOAD, BULL16}}};

static const std::vector<SoundElement> UnitInfo_SfxRepairUnit = {{{SFX_TYPE_IDLE, REPAIR1},
                                                                  {SFX_TYPE_DRIVE, REPAIR5},
                                                                  {SFX_TYPE_STOP, REPAIR7},
                                                                  {SFX_TYPE_HIT, REPAIR15},
                                                                  {SFX_TYPE_EXPLOAD, REPAIR16},
                                                                  {SFX_TYPE_POWER_CONSUMPTION_START, REPAIR17}}};

static const std::vector<SoundElement> UnitInfo_SfxFuelTruck = {{{SFX_TYPE_IDLE, FTRUK1},
                                                                 {SFX_TYPE_DRIVE, FTRUK5},
                                                                 {SFX_TYPE_STOP, FTRUK7},
                                                                 {SFX_TYPE_HIT, FTRUK15},
                                                                 {SFX_TYPE_EXPLOAD, FTRUK16},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_START, FTRUK17}}};

static const std::vector<SoundElement> UnitInfo_SfxArmouredPersonnelCarrier = {{{SFX_TYPE_IDLE, APC1},
                                                                                {SFX_TYPE_DRIVE, APC5},
                                                                                {SFX_TYPE_STOP, APC7},
                                                                                {SFX_TYPE_HIT, APC15},
                                                                                {SFX_TYPE_EXPLOAD, APC16}}};

static const std::vector<SoundElement> UnitInfo_SfxInfiltrator = {
    {{SFX_TYPE_DRIVE, INFIL5}, {SFX_TYPE_FIRE, INFIL14}, {SFX_TYPE_HIT, INFIL15}, {SFX_TYPE_EXPLOAD, INFIL16}}};

static const std::vector<SoundElement> UnitInfo_SfxInfantry = {
    {{SFX_TYPE_DRIVE, INFAN5}, {SFX_TYPE_FIRE, INFAN14}, {SFX_TYPE_HIT, INFAN15}, {SFX_TYPE_EXPLOAD, INFAN16}}};

static const std::vector<SoundElement> UnitInfo_SfxEscort = {{{SFX_TYPE_IDLE, ESCRT2},
                                                              {SFX_TYPE_DRIVE, ESCRT6},
                                                              {SFX_TYPE_STOP, ESCRT8},
                                                              {SFX_TYPE_FIRE, ESCRT14},
                                                              {SFX_TYPE_HIT, ESCRT15},
                                                              {SFX_TYPE_EXPLOAD, ESCRT16}}};

static const std::vector<SoundElement> UnitInfo_SfxCorvette = {{{SFX_TYPE_IDLE, CORVT2},
                                                                {SFX_TYPE_DRIVE, CORVT6},
                                                                {SFX_TYPE_STOP, CORVT8},
                                                                {SFX_TYPE_FIRE, CORVT14},
                                                                {SFX_TYPE_HIT, CORVT15},
                                                                {SFX_TYPE_EXPLOAD, CORVT16}}};

static const std::vector<SoundElement> UnitInfo_SfxGunBoat = {{{SFX_TYPE_IDLE, GUNBT2},
                                                               {SFX_TYPE_DRIVE, GUNBT6},
                                                               {SFX_TYPE_STOP, GUNBT8},
                                                               {SFX_TYPE_FIRE, GUNBT14},
                                                               {SFX_TYPE_HIT, GUNBT15},
                                                               {SFX_TYPE_EXPLOAD, GUNBT16}}};

static const std::vector<SoundElement> UnitInfo_SfxSubmarine = {{{SFX_TYPE_IDLE, SUB2},
                                                                 {SFX_TYPE_DRIVE, SUB6},
                                                                 {SFX_TYPE_STOP, SUB8},
                                                                 {SFX_TYPE_FIRE, SUB14},
                                                                 {SFX_TYPE_HIT, SUB15},
                                                                 {SFX_TYPE_EXPLOAD, SUB16}}};

static const std::vector<SoundElement> UnitInfo_SfxSeaTransport = {{{SFX_TYPE_IDLE, STRANS2},
                                                                    {SFX_TYPE_DRIVE, STRANS6},
                                                                    {SFX_TYPE_STOP, STRANS8},
                                                                    {SFX_TYPE_HIT, STRANS15},
                                                                    {SFX_TYPE_EXPLOAD, STRANS16}}};

static const std::vector<SoundElement> UnitInfo_SfxMissileCruiser = {{{SFX_TYPE_IDLE, MSLCR2},
                                                                      {SFX_TYPE_DRIVE, MSLCR6},
                                                                      {SFX_TYPE_STOP, MSLCR8},
                                                                      {SFX_TYPE_FIRE, MSLCR14},
                                                                      {SFX_TYPE_HIT, MSLCR15},
                                                                      {SFX_TYPE_EXPLOAD, MSLCR16}}};

static const std::vector<SoundElement> UnitInfo_SfxSeaMineLayer = {{{SFX_TYPE_IDLE, SMINE2},
                                                                    {SFX_TYPE_DRIVE, SMINE6},
                                                                    {SFX_TYPE_STOP, SMINE8},
                                                                    {SFX_TYPE_HIT, SMINE15},
                                                                    {SFX_TYPE_EXPLOAD, SMINE16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, SMINE17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, SMINE18}}};

static const std::vector<SoundElement> UnitInfo_SfxCargoShip = {{{SFX_TYPE_IDLE, CSHIP2},
                                                                 {SFX_TYPE_DRIVE, CSHIP6},
                                                                 {SFX_TYPE_STOP, CSHIP8},
                                                                 {SFX_TYPE_HIT, CSHIP15},
                                                                 {SFX_TYPE_POWER_CONSUMPTION_START, CSHIP17},
                                                                 {SFX_TYPE_EXPLOAD, CSHIP16}}};

static const std::vector<SoundElement> UnitInfo_SfxFighter = {{{SFX_TYPE_IDLE, FIGHT1},
                                                               {SFX_TYPE_DRIVE, FIGHT5},
                                                               {SFX_TYPE_STOP, FIGHT7},
                                                               {SFX_TYPE_FIRE, FIGHT14},
                                                               {SFX_TYPE_HIT, FIGHT15},
                                                               {SFX_TYPE_EXPLOAD, FIGHT16},
                                                               {SFX_TYPE_LAND, FIGHT19},
                                                               {SFX_TYPE_TAKE, FIGHT20}}};

static const std::vector<SoundElement> UnitInfo_SfxGroundAttackPlane = {{{SFX_TYPE_IDLE, ATACK1},
                                                                         {SFX_TYPE_DRIVE, ATACK5},
                                                                         {SFX_TYPE_STOP, ATACK7},
                                                                         {SFX_TYPE_FIRE, ATACK14},
                                                                         {SFX_TYPE_HIT, ATACK15},
                                                                         {SFX_TYPE_EXPLOAD, ATACK16},
                                                                         {SFX_TYPE_LAND, ATACK19},
                                                                         {SFX_TYPE_TAKE, ATACK20}}};

static const std::vector<SoundElement> UnitInfo_SfxAirTransport = {{{SFX_TYPE_IDLE, ATRANS1},
                                                                    {SFX_TYPE_DRIVE, ATRANS5},
                                                                    {SFX_TYPE_STOP, ATRANS7},
                                                                    {SFX_TYPE_HIT, ATRANS15},
                                                                    {SFX_TYPE_EXPLOAD, ATRANS16},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_START, ATRANS17},
                                                                    {SFX_TYPE_POWER_CONSUMPTION_END, ATRANS18},
                                                                    {SFX_TYPE_LAND, ATRANS19},
                                                                    {SFX_TYPE_TAKE, ATRANS20}}};

static const std::vector<SoundElement> UnitInfo_SfxAwac = {{{SFX_TYPE_IDLE, AWAC1},
                                                            {SFX_TYPE_DRIVE, AWAC5},
                                                            {SFX_TYPE_STOP, AWAC7},
                                                            {SFX_TYPE_FIRE, AWAC14},
                                                            {SFX_TYPE_HIT, AWAC15},
                                                            {SFX_TYPE_EXPLOAD, AWAC16},
                                                            {SFX_TYPE_LAND, AWAC19},
                                                            {SFX_TYPE_TAKE, AWAC20}}};

static const std::vector<SoundElement> UnitInfo_SfxAlienGunBoat = {{{SFX_TYPE_IDLE, JUGGR1},
                                                                    {SFX_TYPE_DRIVE, JUGGR5},
                                                                    {SFX_TYPE_STOP, JUGGR7},
                                                                    {SFX_TYPE_FIRE, JUGGR14},
                                                                    {SFX_TYPE_HIT, JUGGR15},
                                                                    {SFX_TYPE_EXPLOAD, JUGGR16}}};

static const std::vector<SoundElement> UnitInfo_SfxAlienTank = {{{SFX_TYPE_IDLE, ALNTK1},
                                                                 {SFX_TYPE_DRIVE, ALNTK5},
                                                                 {SFX_TYPE_STOP, ALNTK7},
                                                                 {SFX_TYPE_FIRE, ALNTK14},
                                                                 {SFX_TYPE_HIT, ALNTK15},
                                                                 {SFX_TYPE_EXPLOAD, ALNTK16}}};

static const std::vector<SoundElement> UnitInfo_SfxAlienAssaultGun = {{{SFX_TYPE_IDLE, ALNAG1},
                                                                       {SFX_TYPE_DRIVE, ALNAG5},
                                                                       {SFX_TYPE_STOP, ALNAG7},
                                                                       {SFX_TYPE_FIRE, ALNAG14},
                                                                       {SFX_TYPE_HIT, ALNAG15},
                                                                       {SFX_TYPE_EXPLOAD, ALNAG16}}};

static const std::vector<SoundElement> UnitInfo_SfxAlienAttackPlane = {{{SFX_TYPE_IDLE, ALNPL1},
                                                                        {SFX_TYPE_DRIVE, ALNPL5},
                                                                        {SFX_TYPE_STOP, ALNPL7},
                                                                        {SFX_TYPE_FIRE, ALNPL14},
                                                                        {SFX_TYPE_HIT, ALNPL15},
                                                                        {SFX_TYPE_EXPLOAD, ALNPL16},
                                                                        {SFX_TYPE_LAND, ALNPL19},
                                                                        {SFX_TYPE_TAKE, ALNPL20}}};

const uint8_t UnitInfo::ExpResearchTopics[] = {RESEARCH_TOPIC_ATTACK, RESEARCH_TOPIC_SHOTS, RESEARCH_TOPIC_RANGE,
                                               RESEARCH_TOPIC_ARMOR, RESEARCH_TOPIC_HITS};

static void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file);
static void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file);

void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file) {
    ResourceID unit_type;

    build_list->Clear();

    for (int32_t count = file.ReadObjectCount(); count > 0; --count) {
        file.Read(unit_type);
        build_list->PushBack(&unit_type);
    }
}

void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file) {
    ResourceID unit_type;
    int32_t count;

    count = build_list->GetCount();

    file.WriteObjectCount(count);

    for (int32_t i = 0; i < count; ++i) {
        unit_type = *((*build_list)[i]);

        file.Write(unit_type);
    }
}

UnitInfo::UnitInfo()
    : name(nullptr),
      sound(SFX_TYPE_INVALID),
      unit_type(INVALID_ID),
      popup(nullptr),
      sound_table(nullptr),
      flags(0),
      x(-1),
      y(-1),
      grid_x(-1),
      grid_y(-1),
      color_cycling_lut(nullptr),
      team(-1),
      unit_id(0),
      brightness(UINT8_MAX),
      angle(0),
      max_velocity(0),
      velocity(0),
      scaler_adjust(0),
      turret_angle(0),
      turret_offset_x(0),
      turret_offset_y(0),
      total_images(0),
      image_base(0),
      turret_image_base(0),
      firing_image_base(0),
      connector_image_base(0),
      image_index_max(0),
      orders(ORDER_AWAIT),
      state(ORDER_STATE_INIT),
      prior_orders(ORDER_AWAIT),
      prior_state(ORDER_STATE_INIT),
      target_grid_x(0),
      target_grid_y(0),
      build_time(0),
      total_mining(0),
      raw_mining(0),
      fuel_mining(0),
      gold_mining(0),
      raw_mining_max(0),
      gold_mining_max(0),
      fuel_mining_max(0),
      hits(0),
      speed(0),
      group_speed(0),
      shots(0),
      move_and_fire(0),
      storage(0),
      ammo(0),
      targeting_mode(0),
      enter_mode(0),
      cursor(0),
      recoil_delay(0),
      delayed_reaction(0),
      damaged_this_turn(false),
      disabled_reaction_fire(false),
      auto_survey(false),
      research_topic(false),
      moved(0),
      bobbed(false),
      engine(0),
      weapon(0),
      comm(0),
      fuel_distance(0),
      move_fraction(0),
      connectors(0),
      shake_effect_state(0),
      build_rate(0),
      repeat_build(0),
      energized(false),
      id(0),
      unit_list(nullptr),
      in_transit(false),
      pin_count(0),
      field_165(false),
      laying_state(0),
      visible_to_team(),
      spotted_by_team(),
      sprite_bounds(),
      shadow_bounds(),
      image_index(0),
      turret_image_index(0),
      field_221(0) {}

UnitInfo::UnitInfo(ResourceID unit_type, uint16_t team, uint16_t id, uint8_t angle)
    : orders(ORDER_AWAIT),
      state(ORDER_STATE_EXECUTING_ORDER),
      prior_orders(ORDER_AWAIT),
      prior_state(ORDER_STATE_EXECUTING_ORDER),
      laying_state(0),
      target_grid_x(0),
      target_grid_y(0),
      total_mining(0),
      raw_mining(0),
      fuel_mining(0),
      gold_mining(0),
      raw_mining_max(0),
      gold_mining_max(0),
      fuel_mining_max(0),
      popup(nullptr),
      sound_table(nullptr),
      in_transit(false),
      pin_count(0),
      name(nullptr),
      unit_type(unit_type),
      team(team),
      id(id),
      disabled_reaction_fire(false),
      cursor(CURSOR_HIDDEN),
      base_values(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)),
      unit_id(0),
      build_rate(1),
      field_221(0),
      energized(false),
      build_time(0),
      flags(UnitsManager_BaseUnits[unit_type].flags | UnitsManager_TeamInfo[team].team_units->hash_team_id),
      color_cycling_lut(UnitsManager_TeamInfo[team].team_units->color_index_table),
      brightness(0xFF),
      delayed_reaction(0),
      damaged_this_turn(false),
      repeat_build(0),
      field_165(1),
      move_fraction(0),
      connectors(0),
      scaler_adjust(0),
      research_topic(0),
      velocity(0),
      sound(SFX_TYPE_INVALID),
      unit_list(nullptr),
      group_speed(0),
      shadow_offset(0, 0),
      auto_survey(false),
      angle(angle) {
    BaseUnit* unit;

    rect_init(&sprite_bounds, 0, 0, 0, 0);
    rect_init(&shadow_bounds, 0, 0, 0, 0);

    Init();

    unit = &UnitsManager_BaseUnits[unit_type];

    if (unit->sprite) {
        total_images = reinterpret_cast<struct ImageMultiHeader*>(unit->sprite)->image_count;

    } else {
        total_images = 0;
    }

    image_base = reinterpret_cast<struct BaseUnitDataFile*>(unit->data_buffer)->image_base;
    turret_image_base = reinterpret_cast<struct BaseUnitDataFile*>(unit->data_buffer)->turret_image_base;
    firing_image_base = reinterpret_cast<struct BaseUnitDataFile*>(unit->data_buffer)->firing_image_base;
    connector_image_base = reinterpret_cast<struct BaseUnitDataFile*>(unit->data_buffer)->connector_image_base;

    if (unit_type == MININGST) {
        image_base = (UnitsManager_TeamInfo[team].team_clan - 1) * sizeof(uint16_t);
    }

    image_index = image_base + angle;
    turret_image_index = turret_image_base + angle;

    if (unit_type == COMMANDO) {
        image_index_max = 103;
    } else if (unit_type == INFANTRY) {
        image_index_max = 103;
    } else {
        image_index_max = reinterpret_cast<struct BaseUnitDataFile*>(unit->data_buffer)->image_count + image_index - 1;
    }

    hits = base_values->GetAttribute(ATTRIB_HITS);
    speed = base_values->GetAttribute(ATTRIB_SPEED);
    shots = base_values->GetAttribute(ATTRIB_ROUNDS);
    move_and_fire = base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE);
    ammo = base_values->GetAttribute(ATTRIB_AMMO);
    storage = base_values->GetAttribute(ATTRIB_STORAGE);
    engine = 2;

    if (base_values->GetAttribute(ATTRIB_ATTACK)) {
        weapon = 2;
    } else {
        weapon = 0;
    }

    comm = 2;
    moved = 0;
    bobbed = false;
    shake_effect_state = 0;

    InitStealthStatus();

    if ((flags & (CONNECTOR_UNIT | BUILDING | STANDALONE)) && !(flags & GROUND_COVER) && id != 0xFFFF) {
        AttachComplex(CreateComplex(team));
    }
}

UnitInfo::UnitInfo(const UnitInfo& other)
    : unit_type(other.unit_type),
      popup(other.popup),
      sound_table(other.sound_table),
      flags(other.flags),
      x(other.x),
      y(other.y),
      grid_x(other.grid_x),
      grid_y(other.grid_y),
      attack_site(other.attack_site),
      color_cycling_lut(other.color_cycling_lut),
      team(other.team),
      unit_id(other.unit_id),
      brightness(other.brightness),
      angle(other.angle),
      max_velocity(other.max_velocity),
      velocity(other.velocity),
      sound(other.sound),
      scaler_adjust(other.scaler_adjust),
      turret_angle(other.turret_angle),
      turret_offset_x(other.turret_offset_x),
      turret_offset_y(other.turret_offset_y),
      total_images(other.total_images),
      image_base(other.image_base),
      turret_image_base(other.turret_image_base),
      firing_image_base(other.firing_image_base),
      connector_image_base(other.connector_image_base),
      image_index_max(other.image_index_max),
      orders(other.orders),
      state(other.state),
      prior_orders(other.prior_orders),
      prior_state(other.prior_state),
      target_grid_x(other.target_grid_x),
      target_grid_y(other.target_grid_y),
      build_time(other.build_time),
      total_mining(other.total_mining),
      raw_mining(other.raw_mining),
      fuel_mining(other.fuel_mining),
      gold_mining(other.gold_mining),
      raw_mining_max(other.raw_mining_max),
      gold_mining_max(other.gold_mining_max),
      fuel_mining_max(other.fuel_mining_max),
      hits(other.hits),
      speed(other.speed),
      group_speed(other.group_speed),
      shots(other.shots),
      move_and_fire(other.move_and_fire),
      storage(other.storage),
      ammo(other.ammo),
      targeting_mode(other.targeting_mode),
      enter_mode(other.enter_mode),
      cursor(other.cursor),
      recoil_delay(other.recoil_delay),
      delayed_reaction(other.delayed_reaction),
      damaged_this_turn(other.damaged_this_turn),
      disabled_reaction_fire(other.disabled_reaction_fire),
      auto_survey(other.auto_survey),
      research_topic(other.research_topic),
      moved(other.moved),
      bobbed(other.bobbed),
      engine(other.engine),
      weapon(other.weapon),
      comm(other.comm),
      fuel_distance(other.fuel_distance),
      move_fraction(other.move_fraction),
      path(other.path),
      connectors(other.connectors),
      shake_effect_state(other.shake_effect_state),
      base_values(other.base_values),
      complex(other.complex),
      build_list(other.build_list),
      build_rate(other.build_rate),
      repeat_build(other.repeat_build),
      energized(other.energized),
      id(other.id),
      unit_list(other.unit_list),
      parent_unit(other.GetParent()),
      enemy_unit(other.enemy_unit),
      tasks(other.tasks),
      in_transit(other.in_transit),
      last_target(other.last_target),
      pin_count(other.pin_count),
      field_165(other.field_165),
      laying_state(other.laying_state),
      name(nullptr),
      sprite_bounds(other.sprite_bounds),
      shadow_bounds(other.shadow_bounds),
      image_index(other.image_index),
      turret_image_index(other.turret_image_index),
      shadow_offset(other.shadow_offset),
      field_221(other.field_221) {
    memcpy(visible_to_team, other.visible_to_team, sizeof(visible_to_team));
    memcpy(spotted_by_team, other.spotted_by_team, sizeof(spotted_by_team));
}

UnitInfo::~UnitInfo() { delete[] name; }

FileObject* UnitInfo::Allocate() noexcept { return new (std::nothrow) UnitInfo(); }

static uint16_t UnitInfo_TypeIndex;
static RegisterClass UnitInfo_ClassRegister("UnitInfo", &UnitInfo_TypeIndex, &UnitInfo::Allocate);

uint16_t UnitInfo::GetTypeIndex() const { return UnitInfo_TypeIndex; }

void UnitInfo::Init() {
    BaseUnit* base_unit;
    uint32_t data_size;

    base_unit = &UnitsManager_BaseUnits[unit_type];

    if (!base_unit->sprite) {
        base_unit->sprite = ResourceManager_LoadResource(UnitsManager_AbstractUnits[unit_type].sprite);
        base_unit->shadows = ResourceManager_LoadResource(UnitsManager_AbstractUnits[unit_type].shadows);

        if (ResourceManager_DisableEnhancedGraphics) {
            if (base_unit->sprite) {
                base_unit->sprite = Gfx_RescaleSprite(base_unit->sprite, &data_size, 0, 2);

                ResourceManager_Realloc(UnitsManager_AbstractUnits[unit_type].sprite, base_unit->sprite, data_size);
            }

            if (base_unit->shadows) {
                base_unit->shadows = Gfx_RescaleSprite(base_unit->shadows, &data_size, 1, 2);

                ResourceManager_Realloc(UnitsManager_AbstractUnits[unit_type].shadows, base_unit->shadows, data_size);
            }
        }
    }

    switch (unit_type) {
        case COMMTWR: {
            popup = &UnitsManager_PopupCallbacks[22];
        } break;

        case POWERSTN:
        case POWGEN: {
            popup = &UnitsManager_PopupCallbacks[19];
        } break;

        case BARRACKS:
        case DEPOT:
        case HANGAR:
        case DOCK: {
            popup = &UnitsManager_PopupCallbacks[12];
        } break;

        case ADUMP: {
            popup = &UnitsManager_PopupCallbacks[9];
        } break;

        case FDUMP: {
            popup = &UnitsManager_PopupCallbacks[10];
        } break;

        case SHIPYARD:
        case LIGHTPLT:
        case LANDPLT:
        case AIRPLT:
        case TRAINHAL: {
            popup = &UnitsManager_PopupCallbacks[14];
        } break;

        case RESEARCH: {
            popup = &UnitsManager_PopupCallbacks[18];
        } break;

        case GREENHSE: {
            popup = &UnitsManager_PopupCallbacks[17];
        } break;

        case RECCENTR: {
            popup = &UnitsManager_PopupCallbacks[15];
        } break;

        case GUNTURRT:
        case ANTIAIR:
        case ARTYTRRT:
        case ANTIMSSL:
        case SCOUT:
        case TANK:
        case ARTILLRY:
        case ROCKTLCH:
        case MISSLLCH:
        case SP_FLAK:
        case INFANTRY:
        case FASTBOAT:
        case CORVETTE:
        case BATTLSHP:
        case SUBMARNE:
        case MSSLBOAT:
        case FIGHTER:
        case BOMBER:
        case JUGGRNT:
        case ALNTANK:
        case ALNASGUN:
        case ALNPLANE: {
            popup = &UnitsManager_PopupCallbacks[3];
        } break;

        case MININGST: {
            popup = &UnitsManager_PopupCallbacks[16];
        } break;

        case MASTER: {
            popup = &UnitsManager_PopupCallbacks[21];
        } break;

        case CONSTRCT:
        case ENGINEER: {
            popup = &UnitsManager_PopupCallbacks[13];
        } break;

        case MINELAYR:
        case SEAMNLYR: {
            popup = &UnitsManager_PopupCallbacks[6];
        } break;

        case SURVEYOR: {
            popup = &UnitsManager_PopupCallbacks[2];
        } break;

        case SCANNER:
        case GOLDTRCK: {
            popup = &UnitsManager_PopupCallbacks[1];
        } break;

        case SPLYTRCK:
        case CARGOSHP: {
            popup = &UnitsManager_PopupCallbacks[5];
        } break;

        case BULLDOZR: {
            popup = &UnitsManager_PopupCallbacks[20];
        } break;

        case REPAIR: {
            popup = &UnitsManager_PopupCallbacks[8];
        } break;

        case FUELTRCK: {
            popup = &UnitsManager_PopupCallbacks[7];
        } break;

        case CLNTRANS:
        case SEATRANS:
        case AIRTRANS: {
            popup = &UnitsManager_PopupCallbacks[11];
        } break;

        case COMMANDO: {
            popup = &UnitsManager_PopupCallbacks[4];
        } break;

        default: {
            popup = &UnitsManager_PopupCallbacks[0];
        } break;
    }

    switch (unit_type) {
        case COMMTWR: {
            sound_table = &UnitInfo_SfxMonopoleMine;
        } break;

        case POWERSTN: {
            sound_table = &UnitInfo_SfxPowerStation;
        } break;

        case POWGEN: {
            sound_table = &UnitInfo_SfxPowerGenerator;
        } break;

        case BARRACKS: {
            sound_table = &UnitInfo_SfxBarracks;
        } break;

        case SHIELDGN: {
            sound_table = &UnitInfo_SfxGoldRefinery;
        } break;

        case RADAR: {
            sound_table = &UnitInfo_SfxRadar;
        } break;

        case ADUMP: {
            sound_table = &UnitInfo_SfxMaterialStorage;
        } break;

        case FDUMP: {
            sound_table = &UnitInfo_SfxFuelStorage;
        } break;

        case GOLDSM: {
            sound_table = &UnitInfo_SfxGoldVault;
        } break;

        case DEPOT: {
            sound_table = &UnitInfo_SfxDepot;
        } break;

        case HANGAR: {
            sound_table = &UnitInfo_SfxHangar;
        } break;

        case DOCK: {
            sound_table = &UnitInfo_SfxDock;
        } break;

        case ROAD: {
            sound_table = &UnitInfo_SfxRoad;
        } break;

        case LANDPAD: {
            sound_table = &UnitInfo_SfxLandingPad;
        } break;

        case SHIPYARD: {
            sound_table = &UnitInfo_SfxShipyard;
        } break;

        case LIGHTPLT: {
            sound_table = &UnitInfo_SfxLightVehiclePlant;
        } break;

        case LANDPLT: {
            sound_table = &UnitInfo_SfxHeavyVehiclePlant;
        } break;

        case AIRPLT: {
            sound_table = &UnitInfo_SfxAirUnitsPlant;
        } break;

        case HABITAT: {
            sound_table = &UnitInfo_SfxHabitat;
        } break;

        case RESEARCH: {
            sound_table = &UnitInfo_SfxResearchCentre;
        } break;

        case GREENHSE: {
            sound_table = &UnitInfo_SfxEcoSphere;
        } break;

        case TRAINHAL: {
            sound_table = &UnitInfo_SfxTrainingHall;
        } break;

        case WTRPLTFM: {
            sound_table = &UnitInfo_SfxWaterPlatform;
        } break;

        case GUNTURRT: {
            sound_table = &UnitInfo_SfxGunTurret;
        } break;

        case ANTIAIR: {
            sound_table = &UnitInfo_SfxAntiAir;
        } break;

        case ARTYTRRT: {
            sound_table = &UnitInfo_SfxArtillery;
        } break;

        case ANTIMSSL: {
            sound_table = &UnitInfo_SfxMissileLauncher;
        } break;

        case BLOCK: {
            sound_table = &UnitInfo_SfxConcreteBlock;
        } break;

        case BRIDGE: {
            sound_table = &UnitInfo_SfxBridge;
        } break;

        case MININGST: {
            sound_table = &UnitInfo_SfxMiningStation;
        } break;

        case LANDMINE: {
            sound_table = &UnitInfo_SfxLandMine;
        } break;

        case SEAMINE: {
            sound_table = &UnitInfo_SfxSeaMine;
        } break;

        case HITEXPLD: {
            sound_table = &UnitInfo_SfxHitExplosion;
        } break;

        case MASTER: {
            sound_table = &UnitInfo_SfxMasterBuilder;
        } break;

        case CONSTRCT: {
            sound_table = &UnitInfo_SfxConstructor;
        } break;

        case SCOUT: {
            sound_table = &UnitInfo_SfxScout;
        } break;

        case TANK: {
            sound_table = &UnitInfo_SfxTank;
        } break;

        case ARTILLRY: {
            sound_table = &UnitInfo_SfxAssaultGun;
        } break;

        case ROCKTLCH: {
            sound_table = &UnitInfo_SfxRocketLauncher;
        } break;

        case MISSLLCH: {
            sound_table = &UnitInfo_SfxMissileCrawler;
        } break;

        case SP_FLAK: {
            sound_table = &MobileAntiAir;
        } break;

        case MINELAYR: {
            sound_table = &UnitInfo_SfxMineLayer;
        } break;

        case SURVEYOR: {
            sound_table = &UnitInfo_SfxSurveyor;
        } break;

        case SCANNER: {
            sound_table = &UnitInfo_SfxScanner;
        } break;

        case SPLYTRCK: {
            sound_table = &UnitInfo_SfxSupplyTruck;
        } break;

        case GOLDTRCK: {
            sound_table = &UnitInfo_SfxGoldTruck;
        } break;

        case ENGINEER: {
            sound_table = &UnitInfo_SfxEngineer;
        } break;

        case BULLDOZR: {
            sound_table = &UnitInfo_SfxBulldozer;
        } break;

        case REPAIR: {
            sound_table = &UnitInfo_SfxRepairUnit;
        } break;

        case FUELTRCK: {
            sound_table = &UnitInfo_SfxFuelTruck;
        } break;

        case CLNTRANS: {
            sound_table = &UnitInfo_SfxArmouredPersonnelCarrier;
        } break;

        case COMMANDO: {
            sound_table = &UnitInfo_SfxInfiltrator;
        } break;

        case INFANTRY: {
            sound_table = &UnitInfo_SfxInfantry;
        } break;

        case FASTBOAT: {
            sound_table = &UnitInfo_SfxEscort;
        } break;

        case CORVETTE: {
            sound_table = &UnitInfo_SfxCorvette;
        } break;

        case BATTLSHP: {
            sound_table = &UnitInfo_SfxGunBoat;
        } break;

        case SUBMARNE: {
            sound_table = &UnitInfo_SfxSubmarine;
        } break;

        case SEATRANS: {
            sound_table = &UnitInfo_SfxSeaTransport;
        } break;

        case MSSLBOAT: {
            sound_table = &UnitInfo_SfxMissileCruiser;
        } break;

        case SEAMNLYR: {
            sound_table = &UnitInfo_SfxSeaMineLayer;
        } break;

        case CARGOSHP: {
            sound_table = &UnitInfo_SfxCargoShip;
        } break;

        case FIGHTER: {
            sound_table = &UnitInfo_SfxFighter;
        } break;

        case BOMBER: {
            sound_table = &UnitInfo_SfxGroundAttackPlane;
        } break;

        case AIRTRANS: {
            sound_table = &UnitInfo_SfxAirTransport;
        } break;

        case AWAC: {
            sound_table = &UnitInfo_SfxAwac;
        } break;

        case JUGGRNT: {
            sound_table = &UnitInfo_SfxAlienGunBoat;
        } break;

        case ALNTANK: {
            sound_table = &UnitInfo_SfxAlienTank;
        } break;

        case ALNASGUN: {
            sound_table = &UnitInfo_SfxAlienAssaultGun;
        } break;

        case ALNPLANE: {
            sound_table = &UnitInfo_SfxAlienAttackPlane;
        } break;

        default: {
            sound_table = &UnitInfo_SfxDefaultUnit;
        } break;
    }
}

bool UnitInfo::IsVisibleToTeam(uint16_t team) const { return visible_to_team[team]; }

void UnitInfo::SetEnemy(UnitInfo* enemy) { enemy_unit = enemy; }

UnitInfo* UnitInfo::GetEnemy() const { return &*enemy_unit; }

uint16_t UnitInfo::GetId() const { return id; }

UnitInfo* UnitInfo::GetFirstFromUnitList() const {
    UnitInfo* result;

    if (unit_list != nullptr) {
        result = &*(unit_list->Begin());
    } else {
        result = nullptr;
    }

    return result;
}

SmartList<UnitInfo>* UnitInfo::GetUnitList() const { return unit_list; }

uint32_t UnitInfo::GetField221() const { return field_221; }

void UnitInfo::SetField221(uint32_t value) { field_221 = value; }

void UnitInfo::ChangeField221(uint32_t flags, bool mode) {
    if (mode) {
        field_221 |= flags;

    } else {
        field_221 &= ~flags;
    }
}

uint16_t UnitInfo::GetImageIndex() const { return image_index; }

void UnitInfo::AddTask(Task* task) {
    SmartPointer<Task> old_task(GetTask());
    char text[100];

    AiLog log("Adding task to %s %i: %s", UnitsManager_BaseUnits[unit_type].singular_name, unit_id,
              task->WriteStatusLog(text));

    tasks.PushFront(*task);

    if (old_task) {
        log.Log("Old topmost task: %s", old_task->WriteStatusLog(text));
    }
}

void UnitInfo::ScheduleDelayedTasks(bool priority) {
    if (field_165) {
        for (SmartList<Task>::Iterator it = delayed_tasks.Begin(); it != delayed_tasks.End(); ++it) {
            (*it).RemindTurnEnd(true);
        }
    }

    if (GetTask() && field_165) {
        Task_RemindMoveFinished(this, priority);
    }
}

Task* UnitInfo::GetTask() const {
    Task* task;

    if (tasks.GetCount()) {
        task = &tasks[0];
    } else {
        task = nullptr;
    }

    return task;
}

SmartList<Task>& UnitInfo::GetTasks() { return tasks; }

void UnitInfo::SetBaseValues(UnitValues* unit_values) { base_values = unit_values; }

UnitValues* UnitInfo::GetBaseValues() const { return &*base_values; }

bool UnitInfo::IsDetectedByTeam(uint16_t team) const { return (spotted_by_team[team] || visible_to_team[team]); }

Complex* UnitInfo::GetComplex() const { return &*complex; }

SmartPointer<UnitInfo> UnitInfo::MakeCopy() {
    SmartPointer<UnitInfo> copy = new (std::nothrow) UnitInfo(*this);
    copy->path = nullptr;

    return copy;
}

void UnitInfo::OffsetDrawZones(int32_t offset_x, int32_t offset_y) {
    x += offset_x;
    sprite_bounds.ulx += offset_x;
    sprite_bounds.lrx += offset_x;
    shadow_bounds.ulx += offset_x;
    shadow_bounds.lrx += offset_x;

    y += offset_y;
    sprite_bounds.uly += offset_y;
    sprite_bounds.lry += offset_y;
    shadow_bounds.uly += offset_y;
    shadow_bounds.lry += offset_y;
}

void UnitInfo::UpdateUnitDrawZones() {
    BaseUnit* base_unit;

    base_unit = &UnitsManager_BaseUnits[unit_type];

    if (base_unit->sprite) {
        Point position;
        int32_t unit_size;

        position.x = x;
        position.y = y;

        if (flags & BUILDING) {
            unit_size = GFX_MAP_TILE_SIZE;

        } else {
            unit_size = GFX_MAP_TILE_SIZE / 2;
        }

        sprite_bounds.ulx = position.x - unit_size;
        sprite_bounds.uly = position.y - unit_size;
        sprite_bounds.lrx = position.x + unit_size - 1;
        sprite_bounds.lry = position.y + unit_size - 1;

        UpdateSpriteFrameBounds(&sprite_bounds, position.x, position.y,
                                reinterpret_cast<struct ImageMultiHeader*>(base_unit->sprite), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&sprite_bounds, position.x + turret_offset_x, position.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(base_unit->sprite), turret_image_index);
        }

        shadow_bounds.ulx = 32000;
        shadow_bounds.uly = 32000;
        shadow_bounds.lrx = -32000;
        shadow_bounds.lry = -32000;

        position -= shadow_offset;

        UpdateSpriteFrameBounds(&shadow_bounds, position.x, position.y,
                                reinterpret_cast<struct ImageMultiHeader*>(base_unit->shadows), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&shadow_bounds, position.x + turret_offset_x, position.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(base_unit->shadows), turret_image_index);
        }

        if (shadow_bounds.ulx > shadow_bounds.lrx || shadow_bounds.uly > shadow_bounds.lry ||
            shadow_bounds.ulx - 16 < sprite_bounds.ulx || shadow_bounds.uly - 16 < sprite_bounds.uly) {
            shadow_bounds.ulx = std::min(shadow_bounds.ulx, sprite_bounds.ulx);
            shadow_bounds.uly = std::min(shadow_bounds.uly, sprite_bounds.uly);
            shadow_bounds.lrx = std::max(shadow_bounds.lrx, sprite_bounds.lrx);
            shadow_bounds.lry = std::max(shadow_bounds.lry, sprite_bounds.lry);
        }
    }
}

void UnitInfo::GetName(char* const text, const size_t size) const noexcept {
    if (text && size > 0) {
        if (name) {
            SDL_utf8strlcpy(text, name, size);
        } else {
            const auto name_length{
                snprintf(nullptr, 0, "%s %i", UnitsManager_BaseUnits[unit_type].singular_name, unit_id)};

            if (name_length > 0) {
                auto buffer{new (std::nothrow) char[name_length + sizeof(char)]};

                snprintf(buffer, name_length + sizeof(char), "%s %i", UnitsManager_BaseUnits[unit_type].singular_name,
                         unit_id);
                SDL_utf8strlcpy(text, buffer, size);

                delete[] buffer;

            } else {
                text[0] = '\0';
            }
        }
    }
}

void UnitInfo::GetDisplayName(char* text, const size_t size) const noexcept {
    if (text && size > 0) {
        char unit_name[40];
        char unit_mark[20];

        GetName(unit_name, sizeof(unit_name));
        GetVersion(unit_mark, base_values->GetVersion());

        const auto name_length{snprintf(nullptr, 0, "%s %s %s", _(660b), unit_mark, unit_name)};

        if (name_length > 0) {
            auto buffer{new (std::nothrow) char[name_length + sizeof(char)]};

            snprintf(buffer, name_length + sizeof(char), "%s %s %s", _(660b), unit_mark, unit_name);
            SDL_utf8strlcpy(text, buffer, size);

            delete[] buffer;

        } else {
            text[0] = '\0';
        }
    }
}

void UnitInfo::CalcRomanDigit(char* text, int32_t value, const char* digit1, const char* digit2, const char* digit3) {
    if (value == 9) {
        strcat(text, digit1);
        strcat(text, digit3);
    } else {
        if (value >= 5) {
            strcat(text, digit2);
            value -= 5;
        }

        if (value >= 4) {
            strcat(text, digit1);
            strcat(text, digit2);
            value -= 4;
        }

        for (int32_t i = 0; i < value; ++i) {
            strcat(text, digit1);
        }
    }
}

Complex* UnitInfo::CreateComplex(uint16_t team) { return UnitsManager_TeamInfo[team].team_units->CreateComplex(); }

struct ImageMultiFrameHeader* UnitInfo::GetSpriteFrame(struct ImageMultiHeader* sprite, uint16_t image_index) {
    uintptr_t offset;

    SDL_assert(sprite);
    SDL_assert(image_index >= 0 && image_index < sprite->image_count);

    offset =
        reinterpret_cast<int32_t*>(&(reinterpret_cast<uint8_t*>(sprite)[sizeof(sprite->image_count)]))[image_index];

    return reinterpret_cast<ImageMultiFrameHeader*>(&(reinterpret_cast<uint8_t*>(sprite)[offset]));
}

void UnitInfo::UpdateSpriteFrameBounds(Rect* bounds, int32_t x, int32_t y, struct ImageMultiHeader* sprite,
                                       uint16_t image_index) {
    if (sprite) {
        struct ImageMultiFrameHeader* frame;
        int32_t scaling_factor;
        Rect frame_bounds;

        frame = GetSpriteFrame(sprite, image_index);

        if (frame->width > 0 && frame->height > 0) {
            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor = 2;
            } else {
                scaling_factor = 1;
            }

            frame_bounds.ulx = x - (frame->hotx * scaling_factor);
            frame_bounds.uly = y - (frame->hoty * scaling_factor);
            frame_bounds.lrx = frame->width * scaling_factor + frame_bounds.ulx - 1;
            frame_bounds.lry = frame->height * scaling_factor + frame_bounds.uly - 1;

            bounds->ulx = std::min(bounds->ulx, frame_bounds.ulx);
            bounds->uly = std::min(bounds->uly, frame_bounds.uly);
            bounds->lrx = std::max(bounds->lrx, frame_bounds.lrx);
            bounds->lry = std::max(bounds->lry, frame_bounds.lry);
        }
    }
}

void UnitInfo::UpdateSpriteFrame(uint16_t image_base, uint16_t image_index_max) {
    this->image_base = image_base;
    this->image_index_max = image_index_max;
    DrawSpriteFrame(image_base + angle);
}

void UnitInfo::DrawSpriteFrame(uint16_t image_index) {
    if (this->image_index != image_index) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        this->image_index = image_index;
        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

void UnitInfo::DrawSpriteTurretFrame(uint16_t turret_image_index) {
    if (this->turret_image_index != turret_image_index) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        this->turret_image_index = turret_image_index;
        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

void UnitInfo::GetVersion(char* text, int32_t version) {
    text[0] = '\0';

    CalcRomanDigit(text, version / 100, "C", "D", "M");
    version %= 100;

    CalcRomanDigit(text, version / 10, "X", "L", "C");
    version %= 10;

    CalcRomanDigit(text, version, "I", "V", "X");
}

void UnitInfo::SetName(const char* const text) noexcept {
    delete[] name;

    if (text && strlen(text)) {
        name = new (std::nothrow) char[strlen(text) + 1];
        strcpy(name, text);

    } else {
        name = nullptr;
    }
}

int32_t UnitInfo::GetRaw() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_RAW) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.raw;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetRawFreeCapacity() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_RAW) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.raw - materials.raw;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferRaw(int32_t amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_RAW) {
        storage += amount;

        if (complex != nullptr) {
            const int32_t storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->material += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->material -= amount;

                complex->Transfer(amount, 0, 0);
            }
        }
    }
}

int32_t UnitInfo::GetFuel() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_FUEL) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.fuel;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetFuelFreeCapacity() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_FUEL) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.fuel - materials.fuel;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferFuel(int32_t amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_FUEL) {
        storage += amount;

        if (complex != nullptr) {
            int32_t storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->fuel += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->fuel -= amount;

                complex->Transfer(0, amount, 0);
            }
        }
    }
}

int32_t UnitInfo::GetGold() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_GOLD) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.gold;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetGoldFreeCapacity() {
    int32_t result;

    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_GOLD) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.gold - materials.gold;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferGold(int32_t amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_GOLD) {
        storage += amount;

        if (complex != nullptr) {
            int32_t storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->gold += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->gold -= amount;

                complex->Transfer(0, 0, amount);
            }
        }
    }
}

int32_t UnitInfo::GetTurnsToRepair() {
    int32_t hits_damage;
    int32_t base_hits;

    base_hits = base_values->GetAttribute(ATTRIB_HITS);
    hits_damage = base_hits - hits;

    return (base_hits * 4 + GetNormalRateBuildCost() * hits_damage - 1) / (base_hits * 4);
}

void UnitInfo::RefreshScreen() {
    GameManager_AddDrawBounds(&shadow_bounds);
    GameManager_AddDrawBounds(&sprite_bounds);
}

void UnitInfo::UpdateAngle(uint16_t image_index) {
    int32_t image_diff;

    image_diff = image_index - angle;

    if (image_diff) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        angle = image_index & 0x07;
        this->image_index = (this->image_index & 0xF8) + angle;

        if (flags & TURRET_SPRITE) {
            UpdateTurretAngle((turret_angle + image_diff) & 0x07);
            turret_image_index = turret_image_base + turret_angle;
        }

        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

int32_t UnitInfo::GetDrawLayer(ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case TORPEDO:
        case TRPBUBLE: {
            result = 1;
        } break;

        case WTRPLTFM: {
            result = 2;
        } break;

        case ROAD:
        case BRIDGE:
        case WALDO:
        case LRGSLAB:
        case SMLSLAB: {
            result = 3;
        } break;

        case SMLRUBLE:
        case LRGRUBLE: {
            result = 4;
        } break;

        case LANDMINE:
        case SEAMINE: {
            result = 5;
        } break;

        default: {
            result = 6;
        } break;
    }

    return result;
}

void UnitInfo::AddToDrawList(uint32_t override_flags) {
    uint32_t unit_flags;

    if (override_flags) {
        unit_flags = override_flags;
    } else {
        unit_flags = flags;
    }

    if (unit_flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        UnitsManager_MobileLandSeaUnits.PushFront(*this);

    } else if (unit_flags & STATIONARY) {
        if (unit_flags & GROUND_COVER) {
            int32_t layer_index;
            SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();

            layer_index = GetDrawLayer(unit_type);

            for (; it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if (GetDrawLayer((*it).unit_type) >= layer_index) {
                    break;
                }
            }

            UnitsManager_GroundCoverUnits.InsertBefore(it, *this);

        } else {
            int32_t reference_y;
            SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();

            reference_y = y;

            for (; it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).y >= reference_y) {
                    break;
                }
            }

            UnitsManager_StationaryUnits.InsertBefore(it, *this);
        }

    } else if (unit_flags & MOBILE_AIR_UNIT) {
        SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();

        for (; it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).flags & HOVERING) {
                break;
            }
        }

        UnitsManager_MobileAirUnits.InsertBefore(it, *this);

    } else if (unit_flags & MISSILE_UNIT) {
        if (unit_flags & GROUND_COVER) {
            UnitsManager_GroundCoverUnits.PushFront(*this);

        } else {
            SmartList<UnitInfo>::Iterator it = UnitsManager_ParticleUnits.Begin();

            for (; it != UnitsManager_ParticleUnits.End(); ++it) {
                if (((*it).flags & EXPLODING) && (*it).unit_type != RKTSMOKE) {
                    break;
                }
            }

            UnitsManager_ParticleUnits.InsertBefore(it, *this);
        }
    }
}

void UnitInfo::SetPosition(int32_t grid_x, int32_t grid_y, bool skip_map_status_update) {
    x = grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;
    y = grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;

    this->grid_x = x / GFX_MAP_TILE_SIZE;
    this->grid_y = y / GFX_MAP_TILE_SIZE;

    if (flags & BUILDING) {
        x += (GFX_MAP_TILE_SIZE / 2) - 1;
        y += (GFX_MAP_TILE_SIZE / 2) - 1;
    }

    UpdateUnitDrawZones();
    RefreshScreen();

    Hash_MapHash.Add(this, flags & GROUND_COVER);

    if (!skip_map_status_update && ((GetId() != 0xFFFF) || (flags & (EXPLODING | MISSILE_UNIT)))) {
        Access_UpdateMapStatus(this, true);
    }

    AddToDrawList();
}

void UnitInfo::UpdatePinCount(int32_t grid_x, int32_t grid_y, int32_t pin_units) {
    if (base_values->GetAttribute(ATTRIB_ATTACK_RADIUS)) {
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_GroundCoverUnits, pin_units);
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_StationaryUnits, pin_units);
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_MobileLandSeaUnits, pin_units);

    } else {
        SmartPointer<UnitInfo> target(Access_GetAttackTarget(this, grid_x, grid_y));

        if (target) {
            target->pin_count += pin_units;
        }
    }
}

void UnitInfo::RemoveTasks() {
    SmartList<Task> backup_tasks(tasks);

    tasks.Clear();

    for (SmartList<Task>::Iterator it = backup_tasks.Begin(); it != backup_tasks.End(); ++it) {
        (*it).RemoveUnit(*this);
    }
}

void UnitInfo::MoveInTransitUnitInMapHash(int32_t grid_x, int32_t grid_y) {
    RemoveInTransitUnitFromMapHash();

    in_transit = true;

    last_target.x = grid_x;
    last_target.y = grid_y;

    {
        int32_t backup_grid_x;
        int32_t backup_grid_y;

        backup_grid_x = this->grid_x;
        backup_grid_y = this->grid_y;

        this->grid_x = grid_x;
        this->grid_y = grid_y;

        Hash_MapHash.Add(this);

        this->grid_x = backup_grid_x;
        this->grid_y = backup_grid_y;
    }
}

void UnitInfo::RemoveInTransitUnitFromMapHash() {
    if (in_transit) {
        int32_t backup_grid_x;
        int32_t backup_grid_y;

        backup_grid_x = this->grid_x;
        backup_grid_y = this->grid_y;

        this->grid_x = last_target.x;
        this->grid_y = last_target.y;

        Hash_MapHash.Remove(this);

        this->grid_x = backup_grid_x;
        this->grid_y = backup_grid_y;

        in_transit = false;
    }
}

void UnitInfo::BuildOrder() {
    if (orders != ORDER_HALT_BUILDING && orders != ORDER_HALT_BUILDING_2) {
        build_time = BuildMenu_GetTurnsToBuild(GetConstructedUnitType(), team);
    }

    orders = ORDER_AWAIT;
    state = ORDER_STATE_EXECUTING_ORDER;

    UnitsManager_SetNewOrder(this, ORDER_BUILD, ORDER_STATE_INIT);
}

void UnitInfo::GetBounds(Rect* bounds) {
    bounds->ulx = x / GFX_MAP_TILE_SIZE;
    bounds->uly = y / GFX_MAP_TILE_SIZE;
    bounds->lrx = bounds->ulx + 1;
    bounds->lry = bounds->uly + 1;

    if (flags & BUILDING) {
        ++bounds->lrx;
        ++bounds->lry;
    }
}

bool UnitInfo::IsUpgradeAvailable() {
    bool result;

    if (flags & STATIONARY) {
        UnitValues* current_values = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type);

        if (base_values != current_values) {
            result = (base_values->GetAttribute(ATTRIB_HITS) < current_values->GetAttribute(ATTRIB_HITS)) ||
                     (base_values->GetAttribute(ATTRIB_ATTACK) < current_values->GetAttribute(ATTRIB_ATTACK)) ||
                     (base_values->GetAttribute(ATTRIB_ARMOR) < current_values->GetAttribute(ATTRIB_ARMOR)) ||
                     (base_values->GetAttribute(ATTRIB_SPEED) < current_values->GetAttribute(ATTRIB_SPEED)) ||
                     (base_values->GetAttribute(ATTRIB_RANGE) < current_values->GetAttribute(ATTRIB_RANGE)) ||
                     (base_values->GetAttribute(ATTRIB_ROUNDS) < current_values->GetAttribute(ATTRIB_ROUNDS)) ||
                     (base_values->GetAttribute(ATTRIB_SCAN) < current_values->GetAttribute(ATTRIB_SCAN)) ||
                     (base_values->GetAttribute(ATTRIB_AMMO) < current_values->GetAttribute(ATTRIB_AMMO));

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void UnitInfo::Redraw() {
    bool team_visibility = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

    if (team_visibility) {
        RefreshScreen();
    }

    int32_t offset_x = (grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) - x;
    int32_t offset_y = (grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) - y;

    OffsetDrawZones(offset_x, offset_y);

    if ((flags & MOBILE_AIR_UNIT) && (flags & HOVERING)) {
        shadow_offset.x = -GFX_MAP_TILE_SIZE;
        shadow_offset.y = -GFX_MAP_TILE_SIZE;

    } else {
        shadow_offset.x = 0;
        shadow_offset.y = 0;
    }

    UpdateUnitDrawZones();

    if (team_visibility) {
        RefreshScreen();
    }
}

void UnitInfo::GainExperience(int32_t experience) {
    if (flags & REGENERATING_UNIT) {
        storage += experience;

        if (storage >= 15) {
            int32_t upgrade_topic;
            int32_t upgrade_cost;
            int32_t upgrade_level;
            bool is_upgraded;

            SmartPointer<UnitValues> unit_values =
                UnitsManager_TeamInfo[team].team_units->GetCurrentUnitValues(unit_type);

            is_upgraded = false;

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *base_values);

            upgrade_topic = ExpResearchTopics[(dos_rand() * sizeof(ExpResearchTopics)) >> 15];
            upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);

            while (upgrade_cost <= storage) {
                upgrade_level = TeamUnits_UpgradeOffsetFactor(team, unit_type, upgrade_topic);

                if (upgrade_topic == RESEARCH_TOPIC_HITS || upgrade_topic == RESEARCH_TOPIC_SPEED) {
                    auto current_level = base_values->GetAttribute(upgrade_topic);

                    if (current_level == UINT8_MAX) {
                        upgrade_topic = ExpResearchTopics[(dos_rand() * sizeof(ExpResearchTopics)) >> 15];
                        upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);

                        continue;
                    }

                    if (current_level + upgrade_level > UINT8_MAX) {
                        upgrade_cost = static_cast<float>(upgrade_cost) * static_cast<float>(upgrade_level) /
                                           (UINT8_MAX - current_level) +
                                       0.5f;
                        upgrade_level = UINT8_MAX - current_level;
                    }
                }

                storage -= upgrade_cost;

                if (!is_upgraded) {
                    base_values = new UnitValues(*base_values);
                    is_upgraded = true;
                }

                base_values->AddAttribute(upgrade_topic, upgrade_level);

                upgrade_topic = ExpResearchTopics[(dos_rand() * sizeof(ExpResearchTopics)) >> 15];
                upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);
            }

            if (is_upgraded) {
                if (team == GameManager_PlayerTeam) {
                    SmartString string;

                    string.Sprintf(80, _(d6a7), UnitsManager_BaseUnits[unit_type].singular_name, grid_x + 1,
                                   grid_y + 1);
                    MessageManager_DrawMessage(string.GetCStr(), 0, this, Point(grid_x, grid_y));
                }

                base_values->UpdateVersion();
                base_values->SetUnitsBuilt(1);

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_20(this);
                }

                if (GameManager_SelectedUnit == this) {
                    GameManager_MenuUnitSelect(this);
                }
            }

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *unit_values);
        }
    }
}

void UnitInfo::RemoveDelayedTasks() {
    for (SmartList<Task>::Iterator it = delayed_tasks.Begin(); it != delayed_tasks.End(); ++it) {
        (*it).RemoveUnit(*this);
    }

    delayed_tasks.Clear();
}

void UnitInfo::AttackUnit(UnitInfo* enemy, int32_t attack_potential, int32_t direction) {
    int32_t attack_damage = UnitsManager_GetAttackDamage(enemy, this, attack_potential);

    if (hits > 0) {
        damaged_this_turn = true;

        if (!enemy->IsVisibleToTeam(team)) {
            Ai_UnitSpotted(enemy, team);
        }

        --pin_count;

        if (pin_count < 0) {
            pin_count = 0;
        }

        if (attack_damage > hits) {
            attack_damage = hits;
        }

        hits -= attack_damage;

        if (hits > 0) {
            ScheduleDelayedTasks(true);

            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                Ai_AddUnitToTrackerList(this);
            }
        }

        if (hits == 0) {
            if (flags & SELECTABLE) {
                ++UnitsManager_TeamInfo[team].casualties[unit_type];
            }

            UnitsManager_DelayedAttackTargets[team].Remove(*this);
            UnitsManager_PendingAttacks.Remove(*this);

            RemoveTasks();
            RemoveDelayedTasks();

            Ai_RemoveUnit(this);

            if (unit_type == INFANTRY || unit_type == COMMANDO) {
                angle = (direction + 5) & 0x07;
            }
        }

        if (team == GameManager_PlayerTeam) {
            if (enemy->team != team) {
                GameManager_NotifyEvent(this, true);
            }

        } else if (IsVisibleToTeam(GameManager_PlayerTeam) && hits == 0) {
            const char* formats[] = {_(e6d7), _(2962), _(01c2)};

            BaseUnit* base_unit = &UnitsManager_BaseUnits[unit_type];
            Point position(grid_x, grid_y);
            SmartString message;

            message.Sprintf(80, formats[base_unit->gender], base_unit->singular_name, grid_x + 1, grid_y + 1);

            MessageManager_DrawMessage(message.GetCStr(), 0, this, position);
        }

        UnitsManager_CheckIfUnitDestroyed(this);

        if (hits == 0 || (orders != ORDER_EXPLODE && state != ORDER_STATE_DESTROY)) {
            UnitsManager_SetNewOrderInt(this, ORDER_EXPLODE, ORDER_STATE_INIT);
        }

        FollowUnit();

        if (UnitsManager_TeamInfo[enemy->team].team_type != TEAM_TYPE_REMOTE &&
            UnitsManager_TeamInfo[enemy->team].team_type != TEAM_TYPE_ELIMINATED) {
            enemy->GainExperience(
                (GetBaseValues()->GetAttribute(ATTRIB_TURNS) * attack_damage * 5) /
                (enemy->GetBaseValues()->GetAttribute(ATTRIB_TURNS) * GetBaseValues()->GetAttribute(ATTRIB_HITS)));
        }

        if (GameManager_SelectedUnit == this && hits > 0) {
            GameManager_UpdateInfoDisplay(this);
        }
    }
}

bool UnitInfo::ExecuteMove() {
    bool result;

    RefreshScreen();

    if (pin_count > 0) {
        result = false;
    } else {
        if (path) {
            if ((flags & MOBILE_AIR_UNIT) && !(flags & HOVERING)) {
                UnitsManager_SetNewOrderInt(this, ORDER_TAKE_OFF, ORDER_STATE_INIT);
                Ai_SetTasksPendingFlag("plane takeoff");

                result = false;

            } else {
                if (speed) {
                    Ai_SetTasksPendingFlag("moving");
                }

                result = path->Execute(this);
            }

        } else {
            if (flags & MISSILE_UNIT) {
                SmartPointer<UnitInfo> parent;

                parent = GetParent();
                parent->Attack(grid_x, grid_y);

                UnitsManager_DestroyUnit(this);

                result = false;

            } else {
                orders = ORDER_AWAIT;
                state = ORDER_STATE_EXECUTING_ORDER;
                result = false;
            }
        }
    }

    return result;
}

void UnitInfo::ClearBuildListAndPath() {
    path = nullptr;

    if (GetParent() != nullptr) {
        orders = ORDER_BUILD;
        state = ORDER_STATE_UNIT_READY;
    } else {
        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    build_list.Clear();
}

void UnitInfo::Build() {
    ResourceID build_unit_type;

    build_unit_type = *build_list[0];
    orders = ORDER_BUILD;

    if (path != nullptr) {
        if (Builder_IssueBuildOrder(this, &grid_x, &grid_y, build_unit_type)) {
            target_grid_x = grid_x;
            target_grid_y = grid_y;

            build_time = BuildMenu_GetTurnsToBuild(build_unit_type, team);

            StartBuilding();

        } else {
            SetParent(nullptr);

            ClearBuildListAndPath();
            SoundManager_PlaySfx(this, SFX_TYPE_IDLE);
        }

    } else {
        ClearBuildListAndPath();
    }

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
        GameManager_UpdateDrawBounds();
    }
}

void UnitInfo::Move() {
    SmartPointer<UnitInfo> unit;
    bool team_visibility;

    do {
        if (pin_count > 0) {
            return;
        }

        RefreshScreen();

        if (velocity < max_velocity) {
            velocity += GameManager_FastMovement ? 2 : 1;
        }

        if (velocity > max_velocity) {
            velocity = max_velocity;
        }

        if (GameManager_SelectedUnit == this) {
            if (speed > 0 && !path->IsEndStep()) {
                if (GetSfxType() != SFX_TYPE_DRIVE && GetSfxType() != SFX_TYPE_STOP) {
                    SoundManager_PlaySfx(this, SFX_TYPE_DRIVE);
                }

            } else {
                SoundManager_PlaySfx(this, SFX_TYPE_STOP);
            }
        }

        int32_t unit_velocity = velocity;

        if (unit_velocity + moved >= GFX_MAP_TILE_SIZE - (unit_velocity / 2)) {
            unit_velocity = GFX_MAP_TILE_SIZE - moved;

            if (unit_type != COMMANDO && unit_type != INFANTRY) {
                if (speed == 0 || path->IsEndStep()) {
                    unit_velocity /= 2;

                    if (unit_velocity == 0) {
                        unit_velocity = 1;
                    }

                    velocity = unit_velocity;
                    max_velocity = unit_velocity;
                }
            }
        }

        moved += unit_velocity;

        int32_t step_x = Paths_8DirPointsArrayX[angle];
        int32_t step_y = Paths_8DirPointsArrayY[angle];

        int32_t distance_x = step_x * unit_velocity;
        int32_t distance_y = step_y * unit_velocity;

        int32_t offset_x = ((x + distance_x - step_x) / GFX_MAP_TILE_SIZE) - grid_x;
        int32_t offset_y = ((y + distance_y - step_y) / GFX_MAP_TILE_SIZE) - grid_y;

        if (offset_x || offset_y) {
            unit = MakeCopy();

            Hash_MapHash.Remove(this);
            ClearInTransitFlag();
        }

        team_visibility = visible_to_team[GameManager_PlayerTeam];

        OffsetDrawZones(distance_x, distance_y);

        if (offset_x || offset_y) {
            grid_x = x / GFX_MAP_TILE_SIZE;
            grid_y = y / GFX_MAP_TILE_SIZE;

            if (path) {
                GroundPath* ground_path = dynamic_cast<GroundPath*>(&*path);
                SmartObjectArray<PathStep> path_steps = ground_path->GetSteps();
                int32_t path_step_index = ground_path->GetPathStepIndex();

                path_steps[path_step_index]->x -= offset_x;
                path_steps[path_step_index]->y -= offset_y;
            }

            Access_UpdateMapStatus(this, true);
            Access_UpdateMapStatus(&*unit, false);
        }

        if (visible_to_team[GameManager_PlayerTeam]) {
            team_visibility = true;
        }

        if (team_visibility || GameManager_MaxSpy) {
            RefreshScreen();
        }

        if (unit_type == COMMANDO || unit_type == INFANTRY) {
            if (image_index + 8 > image_index_max) {
                DrawSpriteFrame(image_base + angle);

            } else {
                DrawSpriteFrame(image_index + 8);
            }
        }

        if (moved == GFX_MAP_TILE_SIZE) {
            path->UpdateUnitAngle(this);

            if (orders == ORDER_BUILD && build_list.GetCount() > 0 &&
                Access_IsAccessible(*build_list[0], team, grid_x, grid_y, 0x01)) {
                Build();

            } else {
                state = ORDER_STATE_IN_PROGRESS;
            }

            if (orders == ORDER_MOVE || orders == ORDER_MOVE_TO_UNIT || orders == ORDER_MOVE_TO_ATTACK) {
                FollowUnit();
            }

            if (GameManager_SelectedUnit == this) {
                SoundManager_UpdateSfxPosition(this);
            }

            Ai_MarkMineMapPoint(Point(grid_x, grid_y), team);

            SmartPointer<UnitInfo> mine(Access_GetEnemyMineOnSentry(team, grid_x, grid_y));

            if (mine) {
                mine->SetOrder(ORDER_EXPLODE);
                mine->state = ORDER_STATE_EXPLODE;
                mine->visible_to_team[team] = true;

                Ai_UnitSpotted(&*mine, team);

                if (orders == ORDER_MOVE_TO_ATTACK) {
                    orders = ORDER_AWAIT;
                }

                BlockedOnPathRequest();

                return;
            }

            if (laying_state == 2) {
                PlaceMine();
            }

            if (laying_state == 1) {
                PickUpMine();
            }
        }

        if (Remote_IsNetworkGame) {
            return;
        }

        if (state == ORDER_STATE_IN_PROGRESS && !team_visibility) {
            ExecuteMove();
        }

    } while (state == ORDER_STATE_IN_TRANSITION && !team_visibility);
}

void UnitInfo::AllocateUnitList() { unit_list = new SmartList<UnitInfo>(); }

void UnitInfo::AssignUnitList(UnitInfo* unit) {
    unit_list = unit->GetUnitList();

    if (!unit_list) {
        unit->AllocateUnitList();
        unit->AssignUnitList(unit);
        unit_list = unit->GetUnitList();
    }

    unit_list->PushBack(*this);
}

void UnitInfo::ClearUnitList() {
    if (unit_list && unit_list->Remove(*this)) {
        if (unit_list->GetCount()) {
            if (unit_list->GetCount() == 1) {
                (*unit_list->Begin()).ClearUnitList();
            } else {
                Access_ProcessNewGroupOrder(this);
            }

        } else {
            delete unit_list;
        }

        unit_list = nullptr;
        group_speed = 0;
    }
}

void UnitInfo::MoveToFrontInUnitList() {
    unit_list->Remove(*this);
    unit_list->PushFront(*this);
}

UnitInfo* UnitInfo::GetConnectedBuilding(uint32_t connector) {
    int32_t grid_size;
    UnitInfo* result{nullptr};

    if (flags & BUILDING) {
        grid_size = 2;
    } else {
        grid_size = 1;
    }

    switch (connector) {
        case CONNECTOR_NORTH_LEFT: {
            result = Access_GetTeamBuilding(team, grid_x, grid_y - 1);
        } break;

        case CONNECTOR_NORTH_RIGHT: {
            result = Access_GetTeamBuilding(team, grid_x + 1, grid_y - 1);
        } break;

        case CONNECTOR_EAST_TOP: {
            result = Access_GetTeamBuilding(team, grid_x + grid_size, grid_y);
        } break;

        case CONNECTOR_EAST_BOTTOM: {
            result = Access_GetTeamBuilding(team, grid_x + grid_size, grid_y + 1);
        } break;

        case CONNECTOR_SOUTH_LEFT: {
            result = Access_GetTeamBuilding(team, grid_x, grid_y + grid_size);
        } break;

        case CONNECTOR_SOUTH_RIGHT: {
            result = Access_GetTeamBuilding(team, grid_x + 1, grid_y + grid_size);
        } break;

        case CONNECTOR_WEST_TOP: {
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y);
        } break;

        case CONNECTOR_WEST_BOTTOM: {
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y + 1);
        } break;
    }

    return result;
}

void UnitInfo::AttachComplex(Complex* complex) {
    if (this->complex != complex) {
        complex->Grow(*this);

        if (this->complex != nullptr) {
            this->complex->Shrink(*this);
        }

        this->complex = complex;

        if (connectors & CONNECTOR_NORTH_LEFT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_EAST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_WEST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }
    }
}

void UnitInfo::AttachToPrimaryComplex() {
    SmartPointer<Complex> unit_complex;

    if (complex) {
        complex->Shrink(*this);
        complex = nullptr;
    }

    if (connectors & CONNECTOR_NORTH_LEFT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_NORTH_RIGHT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_EAST_TOP) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_EAST_BOTTOM) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_SOUTH_RIGHT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_SOUTH_LEFT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_WEST_BOTTOM) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_WEST_TOP) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (!unit_complex) {
        unit_complex = CreateComplex(team);
    }

    AttachComplex(&*unit_complex);
}

void UnitInfo::TestConnections() {
    if (!(connectors & CONNECTION_BEING_TESTED)) {
        UnitInfo* building = nullptr;

        connectors |= CONNECTION_BEING_TESTED;

        if (connectors & CONNECTOR_NORTH_LEFT) {
            building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

            if (building) {
                building->TestConnections();
            }
        }
    }
}

UnitInfo* UnitInfo::GetFirstUntestedConnection() {
    UnitInfo* building;

    if (connectors & CONNECTOR_NORTH_LEFT) {
        building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_NORTH_RIGHT) {
        building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_EAST_TOP) {
        building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_EAST_BOTTOM) {
        building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_SOUTH_RIGHT) {
        building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_SOUTH_LEFT) {
        building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_WEST_BOTTOM) {
        building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_WEST_TOP) {
        building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    return nullptr;
}

void UnitInfo::DetachComplex() {
    complex->Shrink(*this);
    complex = nullptr;

    UnitInfo* building = GetFirstUntestedConnection();

    connectors |= CONNECTION_BEING_TESTED;

    if (building) {
        SmartPointer<Complex> building_complex(building->GetComplex());

        building->TestConnections();

        do {
            building = GetFirstUntestedConnection();

            if (building) {
                SmartPointer<Complex> new_complex(CreateComplex(team));

                complex = new_complex;

                building->TestConnections();
                building->AttachComplex(&*new_complex);

                Access_UpdateResourcesTotal(&*new_complex);

                complex = nullptr;
            }

        } while (building);

        Access_UpdateResourcesTotal(&*building_complex);
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        (*it).connectors &= ~CONNECTION_BEING_TESTED;
    }
}

void UnitInfo::FileLoad(SmartFileReader& file) noexcept {
    file.Read(unit_type);
    file.Read(id);
    file.Read(flags);
    file.Read(x);
    file.Read(y);
    file.Read(grid_x);
    file.Read(grid_y);

    SDL_assert(grid_x >= 0 && grid_x < ResourceManager_MapSize.x);
    SDL_assert(grid_y >= 0 && grid_y < ResourceManager_MapSize.y);

    delete[] name;

    uint16_t name_length;

    file.Read(name_length);

    if (name_length) {
        name = new (std::nothrow) char[name_length + 1];

        file.Read(name, name_length);

        name[name_length] = '\0';

    } else {
        name = nullptr;
    }

    file.Read(shadow_offset);
    file.Read(team);
    file.Read(unit_id);
    file.Read(brightness);
    file.Read(angle);
    file.Read(visible_to_team);
    file.Read(spotted_by_team);
    file.Read(max_velocity);
    file.Read(velocity);
    file.Read(sound);
    file.Read(scaler_adjust);
    file.Read(sprite_bounds);
    file.Read(shadow_bounds);
    file.Read(turret_angle);
    file.Read(turret_offset_x);
    file.Read(turret_offset_y);
    file.Read(total_images);
    file.Read(image_base);
    file.Read(turret_image_base);
    file.Read(firing_image_base);
    file.Read(connector_image_base);
    file.Read(image_index);
    file.Read(turret_image_index);
    file.Read(image_index_max);
    file.Read(orders);
    file.Read(state);
    file.Read(prior_orders);
    file.Read(prior_state);
    file.Read(laying_state);
    file.Read(target_grid_x);
    file.Read(target_grid_y);
    file.Read(build_time);
    file.Read(total_mining);
    file.Read(raw_mining);
    file.Read(fuel_mining);
    file.Read(gold_mining);
    file.Read(raw_mining_max);
    file.Read(gold_mining_max);
    file.Read(fuel_mining_max);
    file.Read(hits);
    file.Read(speed);
    file.Read(shots);
    file.Read(move_and_fire);
    file.Read(storage);
    file.Read(ammo);
    file.Read(targeting_mode);
    file.Read(enter_mode);
    file.Read(cursor);
    file.Read(recoil_delay);
    file.Read(delayed_reaction);
    file.Read(damaged_this_turn);
    file.Read(research_topic);
    file.Read(moved);
    file.Read(bobbed);
    file.Read(shake_effect_state);
    file.Read(engine);
    file.Read(weapon);
    file.Read(comm);
    file.Read(fuel_distance);
    file.Read(move_fraction);
    file.Read(energized);
    file.Read(repeat_build);
    file.Read(build_rate);
    file.Read(disabled_reaction_fire);
    file.Read(auto_survey);
    file.Read(field_221);

    field_221 &= ~0x100;

    if (build_rate == 0) {
        build_rate = 1;
    }

    path = dynamic_cast<UnitPath*>(file.ReadObject());
    file.Read(connectors);
    base_values = dynamic_cast<UnitValues*>(file.ReadObject());
    complex = dynamic_cast<Complex*>(file.ReadObject());
    SetParent(dynamic_cast<UnitInfo*>(file.ReadObject()));
    enemy_unit = dynamic_cast<UnitInfo*>(file.ReadObject());

    UnitInfo_BuildList_FileLoad(&build_list, file);

    if (state == ORDER_STATE_NEW_ORDER || state == ORDER_STATE_MOVE_GETTING_PATH || state == ORDER_STATE_ISSUING_PATH ||
        state == ORDER_STATE_IN_TRANSITION || state == ORDER_STATE_IN_PROGRESS) {
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    Init();

    UpdateUnitDrawZones();
}

void UnitInfo::FileSave(SmartFileWriter& file) noexcept {
    SDL_assert(grid_x >= 0 && grid_x < ResourceManager_MapSize.x);
    SDL_assert(grid_y >= 0 && grid_y < ResourceManager_MapSize.y);

    file.Write(unit_type);
    file.Write(id);
    file.Write(flags);
    file.Write(x);
    file.Write(y);
    file.Write(grid_x);
    file.Write(grid_y);

    if (name) {
        uint16_t name_length = strlen(name);

        file.Write(name_length);
        file.Write(name, name_length);

    } else {
        uint16_t name_length = 0;

        file.Write(name_length);
    }

    file.Write(shadow_offset);
    file.Write(team);
    file.Write(unit_id);
    file.Write(brightness);
    file.Write(angle);
    file.Write(visible_to_team);
    file.Write(spotted_by_team);
    file.Write(max_velocity);
    file.Write(velocity);
    file.Write(sound);
    file.Write(scaler_adjust);
    file.Write(sprite_bounds);
    file.Write(shadow_bounds);
    file.Write(turret_angle);
    file.Write(turret_offset_x);
    file.Write(turret_offset_y);
    file.Write(total_images);
    file.Write(image_base);
    file.Write(turret_image_base);
    file.Write(firing_image_base);
    file.Write(connector_image_base);
    file.Write(image_index);
    file.Write(turret_image_index);
    file.Write(image_index_max);
    file.Write(orders);
    file.Write(state);
    file.Write(prior_orders);
    file.Write(prior_state);
    file.Write(laying_state);
    file.Write(target_grid_x);
    file.Write(target_grid_y);
    file.Write(build_time);
    file.Write(total_mining);
    file.Write(raw_mining);
    file.Write(fuel_mining);
    file.Write(gold_mining);
    file.Write(raw_mining_max);
    file.Write(gold_mining_max);
    file.Write(fuel_mining_max);
    file.Write(hits);
    file.Write(speed);
    file.Write(shots);
    file.Write(move_and_fire);
    file.Write(storage);
    file.Write(ammo);
    file.Write(targeting_mode);
    file.Write(enter_mode);
    file.Write(cursor);
    file.Write(recoil_delay);
    file.Write(delayed_reaction);
    file.Write(damaged_this_turn);
    file.Write(research_topic);
    file.Write(moved);
    file.Write(bobbed);
    file.Write(shake_effect_state);
    file.Write(engine);
    file.Write(weapon);
    file.Write(comm);
    file.Write(fuel_distance);
    file.Write(move_fraction);
    file.Write(energized);
    file.Write(repeat_build);
    file.Write(build_rate);
    file.Write(disabled_reaction_fire);
    file.Write(auto_survey);
    file.Write(field_221);
    file.WriteObject(&*path);
    file.Write(connectors);
    file.WriteObject(&*base_values);
    file.WriteObject(&*complex);
    file.WriteObject(GetParent());
    file.WriteObject(&*enemy_unit);

    UnitInfo_BuildList_FileSave(&build_list, file);
}

void UnitInfo::WritePacket(NetPacket& packet) {
    packet << team;
    packet << state;
    packet << repeat_build;
    packet << build_time;
    packet << build_rate;
    packet << target_grid_x;
    packet << target_grid_y;

    packet << build_list.GetCount();

    for (int32_t i = 0; i < build_list.GetCount(); ++i) {
        packet << *build_list[i];
    }
}

void UnitInfo::ReadPacket(NetPacket& packet) {
    uint16_t unit_count;
    ResourceID list_item;

    packet >> team;
    packet >> state;
    packet >> repeat_build;
    packet >> build_time;
    packet >> build_rate;
    packet >> target_grid_x;
    packet >> target_grid_y;

    build_list.Clear();

    packet >> unit_count;

    for (int32_t i = 0; i < unit_count; ++i) {
        packet >> list_item;

        build_list.PushBack(&list_item);
    }

    StartBuilding();
}

void UnitInfo::UpdateTurretAngle(int32_t turret_angle_, bool redraw) {
    BaseUnit* base_unit = &UnitsManager_BaseUnits[unit_type];

    turret_angle = turret_angle_;

    PathStep offset = reinterpret_cast<struct BaseUnitDataFile*>(base_unit->data_buffer)->angle_offsets[angle];

    turret_offset_x = offset.x;
    turret_offset_y = offset.y;

    if (redraw) {
        DrawSpriteTurretFrame(turret_image_base + turret_angle_);
    }
}

void UnitInfo::Attack(int32_t grid_x, int32_t grid_y) {
    SmartPointer<UnitInfo> target(Access_GetAttackTarget(this, grid_x, grid_y));
    UnitInfo* enemy = nullptr;

    if (!target) {
        target = Access_GetAttackTarget2(this, grid_x, grid_y);

        if (target) {
            target->visible_to_team[team] = true;

        } else {
            enemy = Access_GetTeamUnit(grid_x, grid_y, team, SELECTABLE);

            if (!enemy && unit_type != ANTIAIR && unit_type != SP_FLAK && unit_type != FASTBOAT) {
                SmartPointer<UnitInfo> explosion =
                    UnitsManager_DeployUnit(HITEXPLD, team, nullptr, grid_x, grid_y, 0, true);

                if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_LAND) {
                    SoundManager_PlaySfx(&*explosion, SFX_TYPE_HIT);

                } else {
                    SoundManager_PlaySfx(&*explosion, SFX_TYPE_EXPLOAD);
                }
            }
        }
    }

    int32_t target_angle;

    if (flags & MISSILE_UNIT) {
        target_angle = angle / 2;

    } else {
        target_angle = UnitsManager_GetTargetAngle(grid_x - this->grid_x, grid_y - this->grid_y);
    }

    if (GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS)) {
        FindTarget(grid_x, grid_y, &UnitsManager_GroundCoverUnits);
        FindTarget(grid_x, grid_y, &UnitsManager_StationaryUnits);
        FindTarget(grid_x, grid_y, &UnitsManager_MobileLandSeaUnits);

    } else if (target) {
        target->AttackUnit(this, 0, target_angle);

    } else if (enemy && Access_IsValidAttackTarget(this, enemy, Point(grid_x, grid_y))) {
        enemy->AttackUnit(this, 0, target_angle);
    }

    if (!shots) {
        targeting_mode = 0;
    }
}

void UnitInfo::StartBuilding() {
    SDL_assert(build_list.GetCount() > 0);

    ResourceID unit_to_build = *build_list[0];

    if (flags & STATIONARY) {
        complex->material -= Cargo_GetRawConsumptionRate(unit_type, GetMaxAllowedBuildRate());
        complex->power -= Cargo_GetPowerConsumptionRate(unit_type);
        complex->workers -= Cargo_GetLifeConsumptionRate(unit_type);

        state = ORDER_STATE_EXECUTING_ORDER;

        if (GameManager_PlayerTeam == team) {
            GameManager_OptimizeProduction(team, &*complex, true, true);
        }

        DrawSpriteFrame(image_base + 1);

        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(&*GameManager_SelectedUnit);

            SoundManager_PlaySfx(this, SFX_TYPE_POWER_CONSUMPTION_START);
        }

    } else {
        DeployConstructionSiteUtilities(unit_to_build);
        Redraw();

        moved = 0;
        state = ORDER_STATE_BUILD_IN_PROGRESS;

        if (unit_type == CONSTRCT) {
            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_INIT);

        } else {
            PrepareConstructionSite(unit_to_build);
            DrawSpriteFrame(image_index + 16);
        }

        ClearUnitList();
    }
}

void UnitInfo::InitStealthStatus() {
    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        if (unit_type == LANDMINE || unit_type == SEAMINE || unit_type == COMMANDO || unit_type == SUBMARNE) {
            visible_to_team[i] = 0;
        } else {
            visible_to_team[i] = GameManager_AllVisible;
        }

        spotted_by_team[i] = 0;
    }

    visible_to_team[team] = 1;
}

void UnitInfo::SpotByTeam(uint16_t team) {
    if (this->team != team && orders != ORDER_IDLE && !visible_to_team[team]) {
        visible_to_team[team] = true;
        spotted_by_team[team] = true;

        if (UnitsManager_TeamInfo[this->team].team_type == TEAM_TYPE_COMPUTER) {
            Ai_AddUnitToTrackerList(this);
        }

        RefreshScreen();

        if (unit_type == COMMANDO && orders == ORDER_AWAIT) {
            UnitsManager_DrawBustedCommando(this);
        }

        if (unit_type == SUBMARNE || unit_type == CLNTRANS) {
            image_base = 8;

            DrawSpriteFrame(image_base + angle);
        }

        Ai_UnitSpotted(this, team);

        if (team == GameManager_PlayerTeam) {
            RadarPing();

        } else if (unit_type == SUBMARNE && this->team == GameManager_PlayerTeam) {
            SoundManager_PlayVoice(V_M201, V_F201);
        }
    }
}

void UnitInfo::Draw(uint16_t team) {
    if (this->team != team &&
        (visible_to_team[team] ||
         ((unit_type == COMMANDO || unit_type == SUBMARNE || unit_type == CLNTRANS) && image_base >= 8))) {
        visible_to_team[team] = false;

        if (unit_type == COMMANDO || unit_type == SUBMARNE || unit_type == CLNTRANS) {
            for (team = PLAYER_TEAM_RED;
                 (team < PLAYER_TEAM_MAX - 1) &&
                 (this->team == team || UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_NONE ||
                  !visible_to_team[team]);
                 ++team) {
            }
        }

        if (team == PLAYER_TEAM_ALIEN) {
            if (UnitsManager_IsUnitUnderWater(this)) {
                image_base = 0;

                DrawSpriteFrame(image_base + angle);

            } else if (unit_type == COMMANDO && orders == ORDER_AWAIT && image_base == 0) {
                DrawSpriteFrame(angle);
            }
        }

        RefreshScreen();
    }
}

void UnitInfo::DrawStealth(uint16_t team) {
    if (spotted_by_team[team] || (!UnitsManager_IsUnitUnderWater(this) && unit_type != COMMANDO &&
                                  unit_type != LANDMINE && unit_type != SEAMINE)) {
        SpotByTeam(team);

    } else if ((UnitsManager_IsUnitUnderWater(this) &&
                !UnitsManager_TeamInfo[team].heat_map_stealth_sea[grid_y * ResourceManager_MapSize.x + grid_x]) ||
               (unit_type == COMMANDO &&
                !UnitsManager_TeamInfo[team].heat_map_stealth_land[grid_y * ResourceManager_MapSize.x + grid_x])) {
        Draw(team);
    }
}

void UnitInfo::Resupply() {
    if ((flags & STATIONARY) && base_values->GetAttribute(ATTRIB_ATTACK) > 0 &&
        base_values->GetAttribute(ATTRIB_ROUNDS) > ammo) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw > 0) {
            ammo = base_values->GetAttribute(ATTRIB_AMMO);
            complex->Transfer(-1, 0, 0);
        }
    }

    Regenerate();
}

int32_t UnitInfo::GetRawConsumptionRate() { return Cargo_GetRawConsumptionRate(unit_type, GetMaxAllowedBuildRate()); }

void UnitInfo::UpdateProduction() {
    energized = false;

    if (orders == ORDER_BUILD && state != ORDER_STATE_UNIT_READY && (flags & MOBILE_LAND_UNIT)) {
        int32_t maximum_build_rate = BuildMenu_GetMaxPossibleBuildRate(unit_type, build_time, storage);

        build_rate = std::min(static_cast<int32_t>(build_rate), maximum_build_rate);

        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER ||
            ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_MASTER ||
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], *build_list[0])
                    ->GetAttribute(ATTRIB_TURNS) > 1) {
            storage -= Cargo_GetRawConsumptionRate(unit_type, build_rate);
        }
    }

    if (unit_type == COMMTWR && orders == ORDER_POWER_ON) {
        TeamUnits* team_units = UnitsManager_TeamInfo[team].team_units;
        int32_t gold_reserves = team_units->GetGold();

        team_units->SetGold(Cargo_GetGoldConsumptionRate(unit_type) + gold_reserves);

        if (!gold_reserves && GameManager_PlayerTeam == team && team_units->GetGold() > 0) {
            SoundManager_PlayVoice(V_M276, V_F276);
        }
    }

    if (unit_type == GREENHSE && orders == ORDER_POWER_ON) {
        ++storage;
        ++UnitsManager_TeamInfo[team].team_points;

        if (GameManager_PlayerTeam != team && UnitsManager_TeamInfo[team].team_points == 1) {
            SoundManager_PlayVoice(static_cast<ResourceID>(V_M098 + team * 2),
                                   static_cast<ResourceID>(V_M098 + team * 2 + 1));
        }
    }

    if (base_values->GetAttribute(ATTRIB_ROUNDS) > 0) {
        if (base_values->GetAttribute(ATTRIB_ROUNDS) > ammo) {
            shots = ammo;

            if (GameManager_SelectedUnit == this && GameManager_PlayerTeam == team) {
                SoundManager_PlayVoice(V_M270, V_F271);
            }

        } else {
            if (base_values->GetAttribute(ATTRIB_ROUNDS) != shots) {
                shots = base_values->GetAttribute(ATTRIB_ROUNDS);
            }
        }
    }

    speed = base_values->GetAttribute(ATTRIB_SPEED);

    if (speed > 0) {
        for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
            spotted_by_team[i] = false;
        }
    }

    if (orders == ORDER_DISABLE || (orders == ORDER_IDLE && prior_orders == ORDER_DISABLE)) {
        if (team != PLAYER_TEAM_ALIEN) {
            --recoil_delay;

            if (recoil_delay <= 0) {
                SmartPointer<UnitInfo> unit_copy = MakeCopy();

                recoil_delay = 0;

                if (orders == ORDER_IDLE) {
                    prior_orders = ORDER_AWAIT;
                    prior_state = ORDER_STATE_EXECUTING_ORDER;

                } else {
                    RestoreOrders();
                    Access_UpdateMapStatus(this, true);
                    Access_UpdateMapStatus(&*unit_copy, false);

                    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                        GameManager_RenderMinimapDisplay = true;
                    }
                }
            }
        }
    }

    if (state == ORDER_STATE_READY_TO_EXECUTE_ORDER) {
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    if (complex) {
        switch (UnitsManager_BaseUnits[unit_type].cargo_type) {
            case CARGO_TYPE_RAW: {
                storage = std::min(static_cast<int32_t>(complex->material), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->material -= storage;
            } break;

            case CARGO_TYPE_FUEL: {
                storage = std::min(static_cast<int32_t>(complex->fuel), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->fuel -= storage;
            } break;

            case CARGO_TYPE_GOLD: {
                storage = std::min(static_cast<int32_t>(complex->gold), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->gold -= storage;
            } break;
        }
    }
}

ResourceID UnitInfo::GetConstructedUnitType() const {
    SDL_assert(build_list.GetCount() > 0);

    return *build_list[0];
}

bool UnitInfo::IsBridgeElevated() const { return (unit_type == BRIDGE) && (image_index != image_base); }

bool UnitInfo::IsInGroupZone(UnitInfoGroup* group) {
    bool result;

    if (shadow_bounds.ulx < group->GetBounds1()->lrx && shadow_bounds.lrx >= group->GetBounds1()->ulx &&
        shadow_bounds.uly < group->GetBounds1()->lry && shadow_bounds.lry >= group->GetBounds1()->uly) {
        result = true;

    } else if (sprite_bounds.ulx < group->GetBounds1()->lrx && sprite_bounds.lrx >= group->GetBounds1()->ulx &&
               sprite_bounds.uly < group->GetBounds1()->lry && sprite_bounds.lry >= group->GetBounds1()->uly) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitInfo::RenderShadow(Point point, int32_t image_id, Rect* bounds) {
    if (UnitsManager_BaseUnits[unit_type].shadows) {
        uint32_t scaling_factor;
        uint32_t zoom_level;
        struct ImageMultiFrameHeader* frame;

        scaling_factor = 1 << (scaler_adjust + 1);

        zoom_level = (2 * Gfx_ZoomLevel) / scaling_factor;

        if (zoom_level >= 8) {
            Gfx_ResourceBuffer = UnitsManager_BaseUnits[unit_type].shadows;

            frame = GetSpriteFrame(reinterpret_cast<struct ImageMultiHeader*>(Gfx_ResourceBuffer), image_id);

            point -= shadow_offset;

            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor /= 2;
            }

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<uint8_t*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<uint32_t*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;

                Gfx_DecodeShadow();
            }
        }
    }
}

void UnitInfo::RenderAirShadow(Rect* bounds) { RenderShadow(Point(x, y), image_index, bounds); }

void UnitInfo::RenderSprite(Point point, int32_t image_base, Rect* bounds) {
    if (UnitsManager_BaseUnits[unit_type].sprite) {
        uint32_t scaling_factor;
        uint32_t zoom_level;
        struct ImageMultiFrameHeader* frame;

        scaling_factor = 1 << (scaler_adjust + 1);

        zoom_level = (2 * Gfx_ZoomLevel) / scaling_factor;

        if (zoom_level >= 4) {
            Gfx_ResourceBuffer = UnitsManager_BaseUnits[unit_type].sprite;

            frame = GetSpriteFrame(reinterpret_cast<struct ImageMultiHeader*>(Gfx_ResourceBuffer), image_base);

            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor /= 2;
            }

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<uint8_t*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<uint32_t*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;
                Gfx_UnitBrightnessBase = brightness;

                if (zoom_level < 8) {
                    if (flags & HASH_TEAM_RED) {
                        Gfx_TeamColorIndexBase = COLOR_RED;

                    } else if (flags & HASH_TEAM_GREEN) {
                        Gfx_TeamColorIndexBase = COLOR_GREEN;

                    } else if (flags & HASH_TEAM_BLUE) {
                        Gfx_TeamColorIndexBase = COLOR_BLUE;

                    } else if (flags & HASH_TEAM_GRAY) {
                        Gfx_TeamColorIndexBase = 0xFF;

                    } else {
                        Gfx_TeamColorIndexBase = COLOR_YELLOW;
                    }

                } else {
                    Gfx_TeamColorIndexBase = COLOR_BLACK;
                }

                Gfx_DecodeSprite();
            }
        }
    }
}

void UnitInfo::Render(Rect* bounds) {
    Point position(x, y);

    RenderSprite(position, image_index, bounds);

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        position.x += turret_offset_x;
        position.y += turret_offset_y;

        RenderSprite(position, turret_image_index, bounds);
    }
}

void UnitInfo::RenderWithConnectors(Rect* bounds) {
    Point position(x, y);

    RenderShadow(position, image_index, bounds);
    RenderSprite(position, image_index, bounds);

    if (connectors) {
        if (connectors & CONNECTOR_NORTH_LEFT) {
            RenderShadow(position, connector_image_base, bounds);
            RenderSprite(position, connector_image_base, bounds);
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            RenderShadow(position, connector_image_base + 4, bounds);
            RenderSprite(position, connector_image_base + 4, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            RenderShadow(position, connector_image_base + 2, bounds);
            RenderSprite(position, connector_image_base + 2, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            RenderShadow(position, connector_image_base + 6, bounds);
            RenderSprite(position, connector_image_base + 6, bounds);
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            RenderShadow(position, connector_image_base + 1, bounds);
            RenderSprite(position, connector_image_base + 1, bounds);
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            RenderShadow(position, connector_image_base + 5, bounds);
            RenderSprite(position, connector_image_base + 5, bounds);
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            RenderShadow(position, connector_image_base + 3, bounds);
            RenderSprite(position, connector_image_base + 3, bounds);
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            RenderShadow(position, connector_image_base + 7, bounds);
            RenderSprite(position, connector_image_base + 7, bounds);
        }
    }

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        position.x += turret_offset_x;
        position.y += turret_offset_y;

        RenderShadow(position, turret_image_index, bounds);
        RenderSprite(position, turret_image_index, bounds);
    }
}

int32_t UnitInfo::GetMaxAllowedBuildRate() {
    int32_t result;

    if (flags & MOBILE_LAND_UNIT) {
        result = build_rate;

    } else {
        result = std::min(static_cast<int32_t>(build_rate),
                          BuildMenu_GetMaxPossibleBuildRate(unit_type, build_time, storage));
    }

    return result;
}

void UnitInfo::StopMovement() {
    AiLog log("%s at [%i,%i]: Emergency Stop", UnitsManager_BaseUnits[unit_type].singular_name, grid_x + 1, grid_y + 1);

    if (orders == ORDER_MOVE && path != nullptr) {
        if (state == ORDER_STATE_IN_PROGRESS || state == ORDER_STATE_IN_TRANSITION) {
            path->CancelMovement(this);

        } else {
            log.Log("Not in move / turn state.");
        }

    } else {
        log.Log("Not moving.");
    }
}

int32_t UnitInfo::GetLayingState() const { return laying_state; }

void UnitInfo::SetLayingState(int32_t state) { laying_state = state; }

void UnitInfo::ClearPins() { pin_count = 0; }

bool UnitInfo::AttemptSideStep(int32_t grid_x, int32_t grid_y, int32_t angle) {
    bool result;

    if (orders == ORDER_AWAIT || orders == ORDER_SENTRY || orders == ORDER_MOVE) {
        if (speed > 0 || UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            if (this->grid_x != grid_x || this->grid_y != grid_y) {
                int32_t backup_grid_x = this->grid_x;
                int32_t backup_grid_y = this->grid_y;

                this->grid_x = grid_x;
                this->grid_y = grid_y;

                Hash_MapHash.Remove(this);

                this->grid_x = backup_grid_x;
                this->grid_y = backup_grid_y;

                BlockedOnPathRequest(true);

                result = true;

            } else if (orders == ORDER_MOVE &&
                       (state == ORDER_STATE_IN_PROGRESS || state == ORDER_STATE_IN_TRANSITION)) {
                result = true;

            } else {
                Point position;
                Point best_site;
                int32_t step_cost;
                int32_t best_cost{0};
                int32_t unit_angle{8};
                int32_t best_angle{0};

                for (int32_t direction = 0; direction < 8; ++direction) {
                    for (int32_t scan_range = 0; scan_range < 2; ++scan_range) {
                        position.x = this->grid_x + Paths_8DirPointsArray[direction].x;
                        position.y = this->grid_y + Paths_8DirPointsArray[direction].y;

                        unit_angle = (direction - angle + 8) & 0x03;

                        if (unit_angle > 2) {
                            unit_angle = 4 - unit_angle;
                        }

                        if (best_cost == 0 || unit_angle > best_angle) {
                            step_cost = Access_IsAccessible(unit_type, team, position.x, position.y, 0x02);

                            if (direction & 0x01) {
                                step_cost = (step_cost * 3) / 2;
                            }

                            if (step_cost) {
                                if ((step_cost <= speed * 4 + move_fraction) ||
                                    UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                                    best_angle = unit_angle;
                                    best_cost = step_cost;
                                    best_site = position;
                                }
                            }
                        }
                    }
                }

                if (best_cost) {
                    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                        speed += (best_cost + 3) / 4;
                    }

                    Redraw();
                    RemoveInTransitUnitFromMapHash();

                    if (IsVisibleToTeam(GameManager_PlayerTeam)) {
                        if (GameManager_SelectedUnit == this || GameManager_DisplayButtonRange ||
                            GameManager_DisplayButtonScan) {
                            GameManager_UpdateDrawBounds();
                        }
                    }

                    SmartPointer<GroundPath> ground_path(new (std::nothrow) GroundPath(best_site.x, best_site.y));

                    ground_path->AddStep(best_site.x - this->grid_x, best_site.y - this->grid_y);

                    path = &*ground_path;

                    orders = ORDER_MOVE;
                    state = ORDER_STATE_IN_PROGRESS;

                    result = true;

                } else {
                    result = false;
                }
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t UnitInfo::GetTurnsToBuild(ResourceID unit_type, int32_t build_speed_multiplier, int32_t* turns_to_build) {
    int32_t result;
    int32_t turns;
    int32_t local_storage;

    if (build_time > 0 && build_list.GetCount() > 0 && *build_list[0] == unit_type) {
        turns = build_time;

    } else {
        turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_TURNS);
    }

    if (flags & MOBILE_LAND_UNIT) {
        local_storage = storage;

    } else {
        local_storage = INT16_MAX;
    }

    *turns_to_build = 0;
    result = 0;

    while (turns > 0) {
        int32_t build_speed_limit = BuildMenu_GetMaxPossibleBuildRate(this->unit_type, turns, local_storage);

        build_speed_multiplier = std::min(build_speed_multiplier, build_speed_limit);

        result += Cargo_GetRawConsumptionRate(this->unit_type, build_speed_multiplier);

        ++*turns_to_build;

        local_storage -= Cargo_GetRawConsumptionRate(this->unit_type, build_speed_multiplier);

        turns -= build_speed_multiplier;
    }

    return result;
}

void UnitInfo::SetBuildRate(int32_t value) { build_rate = value; }

int32_t UnitInfo::GetBuildRate() const { return build_rate; }

void UnitInfo::SetRepeatBuildState(bool value) { repeat_build = value; }

bool UnitInfo::GetRepeatBuildState() const { return repeat_build; }

void UnitInfo::SpawnNewUnit() {
    if (unit_type == BULLDOZR) {
        while (state == ORDER_STATE_IN_TRANSITION) {
            GameManager_ProcessTick(false);
        }

        const int32_t limit_x = GetParent()->unit_type == LRGRUBLE ? grid_x + 1 : grid_x;
        const int32_t limit_y = GetParent()->unit_type == LRGRUBLE ? grid_y + 1 : grid_y;

        for (int32_t pos_x{grid_x}; pos_x <= limit_x; ++pos_x) {
            for (int32_t pos_y{grid_y}; pos_y <= limit_y; ++pos_y) {
                const auto units = Hash_MapHash[Point(pos_x, pos_y)];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).unit_type == SMLRUBLE || (*it).unit_type == LRGRUBLE) {
                            storage += (*it).storage;

                            UnitsManager_DestroyUnit(it->Get());
                        }
                    }
                }
            }
        }

        Access_DestroyUtilities(grid_x, grid_y, false, false, false, false);
        storage = std::min<int32_t>(storage, GetBaseValues()->GetAttribute(ATTRIB_STORAGE));

        DrawSpriteFrame(image_index - 8);

        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;

        if (GetParent()->flags & BUILDING) {
            target_grid_x = grid_x;
            target_grid_y = grid_y;

            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_DEINIT);
        }

        SetParent(nullptr);

        if (GameManager_SelectedUnit == this) {
            SoundManager_PlaySfx(this, SFX_TYPE_IDLE);
        }

    } else {
        if (flags & STATIONARY) {
            SmartPointer<UnitInfo> factory_unit;
            int32_t position_x = this->grid_x;
            int32_t position_y = this->grid_y;

            factory_unit = UnitsManager_DeployUnit(GetConstructedUnitType(), team, GetComplex(), position_x, position_y,
                                                   0, false, true);

            factory_unit->SetParent(this);
            factory_unit->SetOrder(ORDER_IDLE);
            factory_unit->SetOrderState(ORDER_STATE_STORE);
            factory_unit->scaler_adjust = 4;

            ++UnitsManager_TeamInfo[team].stats_units_built;

            build_list.Remove(0);

            if (repeat_build) {
                build_list.PushBack(&factory_unit->unit_type);
            }

            SetParent(factory_unit.Get());

            storage = 1;
            orders = ORDER_BUILD;
            state = ORDER_STATE_UNIT_READY;

            DrawSpriteFrame(image_base);
            ScheduleDelayedTasks(true);

        } else {
            SmartPointer<UnitInfo> utility_unit;
            int32_t position_x = this->grid_x;
            int32_t position_y = this->grid_y;

            if (GameManager_SelectedUnit == this) {
                SoundManager_PlaySfx(this, SFX_TYPE_IDLE);
            }

            utility_unit = Access_GetConstructionUtility(team, position_x, position_y);

            SDL_assert(utility_unit != nullptr);

            utility_unit = UnitsManager_DeployUnit(GetConstructedUnitType(), team, nullptr, utility_unit->grid_x,
                                                   utility_unit->grid_y, 0, false, true);

            if (!path) {
                build_list.Clear();
            }

            this->SetParent(&*utility_unit);
            utility_unit->SetParent(this);

            UnitsManager_SetNewOrderInt(&*utility_unit, ORDER_IDLE, ORDER_STATE_BUILDING_READY);

            if (unit_type == ENGINEER) {
                DrawSpriteFrame(image_index - 16);

            } else {
                DrawSpriteFrame(angle);
            }

            if (utility_unit->unit_type == ROAD || utility_unit->unit_type == BRIDGE ||
                utility_unit->unit_type == WTRPLTFM || utility_unit->unit_type == CNCT_4W) {
                Access_DestroyUtilities(position_x, position_y, false, false, false, false);

                SetParent(nullptr);
                utility_unit->SetOrder(ORDER_AWAIT);
                utility_unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                UnitsManager_UpdateConnectors(&*utility_unit);
                Access_UpdateMapStatus(&*utility_unit, true);

                GameManager_RenderMinimapDisplay = true;

                Hash_MapHash.Remove(this);
                Hash_MapHash.Add(this);

                if (path) {
                    state = ORDER_STATE_UNIT_READY;

                } else {
                    orders = ORDER_AWAIT;
                    state = ORDER_STATE_EXECUTING_ORDER;
                }

                DrawSpriteFrame(angle);

                if (GetTask()) {
                    GetTask()->AddUnit(*utility_unit);
                }

            } else {
                orders = ORDER_BUILD;
                state = ORDER_STATE_UNIT_READY;

                if (GetTask()) {
                    utility_unit->AddTask(GetTask());
                }
            }
        }
    }
}

void UnitInfo::FollowUnit() {
    if (ini_get_setting(INI_FOLLOW_UNIT) && GameManager_SelectedUnit == this) {
        if (GameManager_PlayMode == PLAY_MODE_TURN_BASED || GameManager_PlayerTeam == team ||
            UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn) {
            if (visible_to_team[GameManager_PlayerTeam] && !GameManager_IsInsideMapView(this)) {
                GameManager_UpdateMainMapView(MAP_VIEW_CENTER, grid_x, grid_y);
            }
        }
    }
}

int32_t UnitInfo::GetExperience() {
    int32_t result = sqrt(storage * 10) - 2.0;

    if (result < 0) {
        result = 0;
    }

    return result;
}

void UnitInfo::BlockedOnPathRequest(bool mode, bool skip_notification) {
    path = nullptr;

    if (orders != ORDER_MOVE_TO_ATTACK || state == ORDER_STATE_NEW_ORDER) {
        orders = ORDER_AWAIT;
    }

    state = ORDER_STATE_EXECUTING_ORDER;

    if (!ini_get_setting(INI_DISABLE_FIRE)) {
        delayed_reaction = true;
    }

    MoveFinished(mode);

    if (Remote_IsNetworkGame) {
        if (!skip_notification) {
            Remote_SendNetPacket_41(this, mode);
        }
    }
}

void UnitInfo::MoveFinished(bool mode) {
    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
        GameManager_AutoSelectNext(this);
        SoundManager_PlaySfx(this, SFX_TYPE_IDLE);
    }

    velocity = 0;
    moved = 0;

    ScheduleDelayedTasks(true);

    if (unit_type == INFANTRY) {
        UpdateSpriteFrame(0, image_index_max);

    } else if (unit_type == COMMANDO) {
        UpdateSpriteFrame(0, image_index_max);
        UnitsManager_TestBustedCommando(this);
    }

    if (IsVisibleToTeam(GameManager_PlayerTeam)) {
        if (GameManager_SelectedUnit == this || GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
            GameManager_UpdateDrawBounds();
        }

        GameManager_RenderMinimapDisplay = true;
    }

    RemoveInTransitUnitFromMapHash();
    Redraw();

    if (unit_type == MINELAYR || unit_type == SEAMNLYR) {
        Ai_CheckMines(this);
    }

    if (mode && !ini_get_setting(INI_DISABLE_FIRE)) {
        delayed_reaction = true;

        UnitsManager_AddToDelayedReactionList(this);

        if (ammo > 0) {
            Ai_EvaluateAttackTargets(this);
        }
    }
}

void UnitInfo::RadarPing() {
    if (orders != ORDER_BUILD && orders != ORDER_CLEAR &&
        (!(flags & STATIONARY) || unit_type == LANDMINE || unit_type == SEAMINE) && (flags & SELECTABLE)) {
        SoundManager_PlaySfx(RADRPING);
        GameManager_NotifyEvent(this, 0);
    }
}

int32_t UnitInfo::GetNormalRateBuildCost() const {
    int32_t result;

    if (flags & STATIONARY) {
        result = Cargo_GetRawConsumptionRate(CONSTRCT, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
        result = Cargo_GetRawConsumptionRate(TRAINHAL, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    } else {
        result = Cargo_GetRawConsumptionRate(LANDPLT, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);
    }

    return result;
}

SmartObjectArray<ResourceID> UnitInfo::GetBuildList() { return build_list; }

void UnitInfo::RemoveTask(Task* task, bool mode) {
    SmartPointer<Task> unit_task(GetTask());
    char text[100];

    AiLog log("Removing task from %s %i: %s", UnitsManager_BaseUnits[unit_type].singular_name, unit_id,
              task->WriteStatusLog(text));

    if (unit_task) {
        SmartPointer<Task> reference_task;

        tasks.Remove(*task);

        reference_task = GetTask();

        if (unit_task == reference_task) {
            log.Log("No change in top task (%s)", unit_task->WriteStatusLog(text));

        } else if (tasks.GetCount() > 0) {
            log.Log("New topmost task: %s", reference_task->WriteStatusLog(text));

            if (mode && Task_IsReadyToTakeOrders(this)) {
                Task_RemindMoveFinished(this);
            }
        }

    } else {
        SDL_assert(tasks.GetCount() == 0);

        log.Log("Unit has no tasks!");
    }
}

bool UnitInfo::IsReadyForOrders(Task* task) {
    bool result;

    if (Task_IsReadyToTakeOrders(this) && GetTask() == task) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitInfo::RestoreOrders() {
    if (prior_orders == orders && unit_type != COMMTWR && unit_type != MININGST && unit_type != HABITAT &&
        unit_type != POWGEN && unit_type != POWERSTN && unit_type != RESEARCH) {
        prior_orders = ORDER_AWAIT;
        prior_state = ORDER_STATE_EXECUTING_ORDER;
    }

    orders = prior_orders;
    state = prior_state;
}

void UnitInfo::AddDelayedTask(Task* task) { delayed_tasks.PushBack(*task); }

void UnitInfo::RemoveDelayedTask(Task* task) { delayed_tasks.Remove(*task); }

bool UnitInfo::AreTherePins() { return pin_count > 0; }

void UnitInfo::DeployConstructionSiteUtilities(ResourceID unit_type) {
    int32_t unit_angle = 0;
    ResourceID unit_type1;
    ResourceID unit_type2;

    if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
        unit_type1 = LRGTAPE;
        unit_type2 = LRGCONES;

    } else {
        unit_type1 = SMLTAPE;
        unit_type2 = SMLCONES;
    }

    if (Access_GetModifiedSurfaceType(target_grid_x, target_grid_y) & (SURFACE_TYPE_WATER | SURFACE_TYPE_COAST)) {
        ++unit_angle;

    } else {
        UnitsManager_DeployUnit(unit_type2, team, nullptr, target_grid_x, target_grid_y, 0);
    }

    SmartPointer<UnitInfo> unit =
        UnitsManager_DeployUnit(unit_type1, team, nullptr, target_grid_x, target_grid_y, unit_angle);

    unit->SetParent(this);
}

bool UnitInfo::Land() {
    bool result;

    if (moved == 0 && GameManager_SelectedUnit == this) {
        SoundManager_PlaySfx(this, SFX_TYPE_LAND);
    }

    RefreshScreen();

    moved = (moved + 1) & 0x3F;

    if (moved > 8) {
        flags &= ~HOVERING;

        Redraw();

        result = true;

    } else {
        shadow_offset.x += 8;
        shadow_offset.y += 8;

        UpdateUnitDrawZones();
        RefreshScreen();

        result = false;
    }

    return result;
}

void UnitInfo::ClearInTransitFlag() { in_transit = false; }

bool UnitInfo::Take() {
    bool result;

    if (moved == 0 && GameManager_SelectedUnit == this) {
        SoundManager_PlaySfx(this, SFX_TYPE_TAKE);
    }

    RefreshScreen();

    moved = (moved + 1) & 0x3F;

    if (moved > 4) {
        flags |= HOVERING;

        result = true;

    } else {
        shadow_offset.x -= 16;
        shadow_offset.y -= 16;

        UpdateUnitDrawZones();
        RefreshScreen();

        result = false;
    }

    return result;
}

void UnitInfo::SpinningTurretAdvanceAnimation() {
    if (turret_image_index + 1 > image_index_max) {
        DrawSpriteTurretFrame(turret_image_base);

    } else {
        DrawSpriteTurretFrame(turret_image_index + 1);
    }
}

int32_t UnitInfo::GetAttackRange() {
    int32_t result;

    if (base_values->GetAttribute(ATTRIB_ROUNDS) > 0) {
        if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            result = base_values->GetAttribute(ATTRIB_RANGE) + base_values->GetAttribute(ATTRIB_SPEED) / 2;

        } else {
            result = ((base_values->GetAttribute(ATTRIB_ROUNDS) - 1) * base_values->GetAttribute(ATTRIB_SPEED)) /
                         base_values->GetAttribute(ATTRIB_ROUNDS) +
                     base_values->GetAttribute(ATTRIB_RANGE);
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::UpdatePinsFromLists(int32_t grid_x, int32_t grid_y, SmartList<UnitInfo>* units, int32_t pin_units) {
    int32_t attack_radius = base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).flags & SELECTABLE) && Access_IsWithinAttackRange(&*it, grid_x, grid_y, attack_radius) &&
            Access_IsValidAttackTarget(this, &*it, Point(grid_x, grid_y))) {
            (*it).pin_count += pin_units;
        }
    }
}

void UnitInfo::FindTarget(int32_t grid_x, int32_t grid_y, SmartList<UnitInfo>* units) {
    int32_t attack_radius = base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).flags & SELECTABLE) && Access_IsWithinAttackRange(&*it, grid_x, grid_y, attack_radius) &&
            Access_IsValidAttackTarget(this, &*it, Point(grid_x, grid_y))) {
            if (((*it).unit_type != BRIDGE && (*it).unit_type != WTRPLTFM) || !Access_IsUnitBusyAtLocation(&*it)) {
                (*it).AttackUnit(this, TaskManager_GetDistance((*it).grid_x - grid_x, (*it).grid_y - grid_y),
                                 UnitsManager_GetTargetAngle((*it).grid_x - grid_x, (*it).grid_y - grid_y));
            }
        }
    }
}

void UnitInfo::UpgradeInt() {
    SmartPointer<UnitValues> values(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type));
    SmartPointer<UnitInfo> copy = MakeCopy();

    hits += values->GetAttribute(ATTRIB_HITS) - base_values->GetAttribute(ATTRIB_HITS);

    UnitsManager_CheckIfUnitDestroyed(this);

    base_values = values;
    base_values->SetUnitsBuilt(1);

    Access_UpdateMapStatus(this, true);
    Access_UpdateMapStatus(&*copy, false);
}

void UnitInfo::Regenerate() {
    if (!damaged_this_turn && (flags & STATIONARY) && base_values->GetAttribute(ATTRIB_HITS) != hits) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw == 0) {
            return;
        }

        if (materials.raw > 1) {
            materials.raw = 1;
        }

        int32_t turns_to_repair = Repair(materials.raw);

        complex->Transfer(-turns_to_repair, 0, 0);
    }

    if ((flags & REGENERATING_UNIT) && base_values->GetAttribute(ATTRIB_HITS) != hits) {
        Repair(1);
    }

    damaged_this_turn = false;
}

void UnitInfo::StepMoveUnit(Point position) {
    ++position.y;

    for (int32_t range_limit = 3;; range_limit += 2) {
        --position.x;
        ++position.y;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t i = 0; i < range_limit; ++i) {
                position += Paths_8DirPointsArray[direction];

                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y &&
                    Access_IsAccessible(unit_type, team, position.x, position.y, 0x12)) {
                    SmartPointer<UnitInfo> unit_copy = MakeCopy();

                    RemoveInTransitUnitFromMapHash();

                    Hash_MapHash.Remove(this);

                    RefreshScreen();

                    grid_x = position.x;
                    grid_y = position.y;

                    Hash_MapHash.Add(this);

                    Redraw();

                    Access_UpdateMapStatus(this, true);
                    Access_UpdateMapStatus(&*unit_copy, false);

                    if (orders == ORDER_MOVE || orders == ORDER_MOVE_TO_UNIT || orders == ORDER_MOVE_TO_ATTACK) {
                        BlockedOnPathRequest();
                    }

                    return;
                }
            }
        }
    }
}

void UnitInfo::PrepareConstructionSite(ResourceID unit_type) {
    uint32_t unit_flags = UnitsManager_BaseUnits[unit_type].flags;
    SmartPointer<UnitInfo> utility_unit(Access_GetConstructionUtility(team, grid_x, grid_y));
    Point position(utility_unit->grid_x, utility_unit->grid_y);

    if (unit_flags & BUILDING) {
        Point site;

        for (site.x = position.x; site.x < position.x + 2; ++site.x) {
            for (site.y = position.y; site.y < position.y + 2; ++site.y) {
                const auto units = Hash_MapHash[site];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).unit_type == COMMANDO || (*it).unit_type == SUBMARNE || (*it).unit_type == CLNTRANS) {
                            (*it).StepMoveUnit(position);
                        }
                    }
                }
            }
        }
    }

    if (unit_flags & REQUIRES_SLAB) {
        ResourceID slab_type = (unit_flags & BUILDING) ? LRGSLAB : SMLSLAB;

        SmartPointer<UnitInfo> unit = UnitsManager_DeployUnit(
            slab_type, team, nullptr, position.x, position.y,
            (dos_rand() *
             reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[slab_type].data_buffer)->image_count) >>
                15);

        unit->scaler_adjust = 4;

        UnitsManager_ScaleUnit(&*unit, ORDER_STATE_EXPAND);
    }
}

int32_t UnitInfo::GetTargetUnitAngle() {
    int32_t result;

    if (state == ORDER_STATE_TAPE_POSITIONING_ENTER) {
        SmartPointer<UnitInfo> utility_unit(Access_GetConstructionUtility(team, grid_x, grid_y));

        if (grid_x == utility_unit->grid_x) {
            if (grid_y == utility_unit->grid_y) {
                result = UNIT_ANGLE_SE;

            } else {
                result = UNIT_ANGLE_NE;
            }

        } else {
            if (grid_y == utility_unit->grid_y) {
                result = UNIT_ANGLE_SW;

            } else {
                result = UNIT_ANGLE_NW;
            }
        }

    } else {
        int32_t target_x = target_grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;
        int32_t target_y = target_grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;

        if (target_x > x) {
            if (target_y > y) {
                result = UNIT_ANGLE_SE;

            } else {
                result = UNIT_ANGLE_NE;
            }

        } else {
            if (target_y > y) {
                result = UNIT_ANGLE_SW;

            } else {
                result = UNIT_ANGLE_NW;
            }
        }
    }

    return result;
}

void UnitInfo::UpdateInfoDisplay() {
    int32_t base_speed = GetBaseValues()->GetAttribute(ATTRIB_SPEED);
    int32_t base_rounds = GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
    int32_t unit_speed = speed;

    unit_speed -= (((shots + 1) * base_speed) / base_rounds) - ((shots * base_speed) / base_rounds);

    if (unit_speed < 0) {
        unit_speed = 0;
    }

    speed = unit_speed;

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }
}

int32_t UnitInfo::Repair(int32_t materials) {
    int32_t hits_damage_level = base_values->GetAttribute(ATTRIB_HITS) - hits;
    int32_t repair_cost = GetTurnsToRepair();

    if (repair_cost > materials) {
        hits_damage_level = (base_values->GetAttribute(ATTRIB_HITS) * 4 * materials) / GetNormalRateBuildCost();
        repair_cost = (base_values->GetAttribute(ATTRIB_HITS) * 4 + GetNormalRateBuildCost() * hits_damage_level - 1) /
                      (base_values->GetAttribute(ATTRIB_HITS) * 4);
    }

    hits += hits_damage_level;

    UnitsManager_CheckIfUnitDestroyed(this);

    return repair_cost;
}

void UnitInfo::CancelBuilding() {
    if (flags & STATIONARY) {
        if (orders == ORDER_BUILD) {
            if (state == ORDER_STATE_BUILD_CANCEL) {
                orders = ORDER_HALT_BUILDING;

            } else {
                orders = ORDER_HALT_BUILDING_2;
            }

            state = ORDER_STATE_EXECUTING_ORDER;

            Cargo_UpdateResourceLevels(this, 1);

        } else {
            orders = ORDER_AWAIT;
            state = ORDER_STATE_EXECUTING_ORDER;

            build_list.Clear();

            build_time = 0;
        }

        if (GameManager_SelectedUnit == this) {
            SoundManager_PlaySfx(this, SFX_TYPE_POWER_CONSUMPTION_END);

            GameManager_UpdateInfoDisplay(this);
        }

        DrawSpriteFrame(image_base);

    } else {
        build_list.Clear();

        build_time = 0;

        Access_DestroyUtilities(grid_x, grid_y, true, false, false, false);

        path = nullptr;

        moved = 0;

        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;

        if (unit_type == CONSTRCT || (unit_type == BULLDOZR && (GetParent()->flags & BUILDING))) {
            target_grid_x = grid_x;
            target_grid_y = grid_y;

            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_DEINIT);

            DrawSpriteFrame(angle);

        } else if (unit_type == ENGINEER && image_index >= 16) {
            DrawSpriteFrame(image_index - 16);

        } else if (unit_type == BULLDOZR && image_index >= 8) {
            DrawSpriteFrame(image_index - 8);
        }

        if (GameManager_SelectedUnit == this) {
            SoundManager_PlaySfx(this, SFX_TYPE_IDLE);
        }
    }
}

void UnitInfo::Refuel(UnitInfo* parent) {
    if (!energized && unit_type == FUELTRCK && storage > 0) {
        --storage;
        energized = true;
    }

    RestoreOrders();

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }

    if (parent->GetTask()) {
        parent->GetTask()->Task_vfunc20(*parent);
    }
}

void UnitInfo::Reload(UnitInfo* parent) {
    bool need_action = false;

    if (complex) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw > 0) {
            need_action = true;

            complex->Transfer(-1, 0, 0);
        }

    } else {
        if (storage > 0) {
            need_action = true;

            --storage;
        }
    }

    if (need_action) {
        if (GameManager_SelectedUnit == this) {
            SoundManager_PlaySfx(this, SFX_TYPE_POWER_CONSUMPTION_START, true);
        }

        parent->ammo = parent->GetBaseValues()->GetAttribute(ATTRIB_AMMO);
    }

    RestoreOrders();

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }
}

bool UnitInfo::Upgrade(UnitInfo* parent) {
    bool result{false};

    if (parent->GetBaseValues() != UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], parent->unit_type) &&
        complex) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        int32_t materials_cost = parent->GetNormalRateBuildCost() / 4;

        if (materials.raw >= materials_cost) {
            parent->UpgradeInt();

            complex->Transfer(-materials_cost, 0, 0);

            if (GameManager_PlayerTeam == team && state != ORDER_STATE_EXECUTING_ORDER) {
                char unit_mark[10];
                SmartString message;

                GetVersion(unit_mark, parent->GetBaseValues()->GetVersion());

                message.Sprintf(80, _(d23e), UnitsManager_BaseUnits[parent->unit_type].singular_name, unit_mark,
                                materials_cost);

                MessageManager_DrawMessage(message.GetCStr(), 0, parent, Point(parent->grid_x, parent->grid_y));
            }

            result = true;

        } else if (GameManager_PlayerTeam == team && state != ORDER_STATE_EXECUTING_ORDER) {
            SmartString message;

            message.Sprintf(80, _(e3e0), materials_cost);

            MessageManager_DrawMessage(message.GetCStr(), 2, 0);
        }
    }

    RestoreOrders();

    SetParent(nullptr);

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }

    if (GameManager_SelectedUnit == parent) {
        if (GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
            GameManager_UpdateDrawBounds();
        }

        GameManager_MenuUnitSelect(parent);
    }

    parent->ScheduleDelayedTasks(result);

    return result;
}

void UnitInfo::BusyWaitOrder() {
    bool last_state = field_165;

    field_165 = false;

    while (orders != ORDER_AWAIT) {
        GameManager_ProcessTick(false);
        MouseEvent::ProcessInput();
    }

    field_165 = last_state;
}

void UnitInfo::PositionInTape() {
    Ai_SetTasksPendingFlag("Positioning in tape");

    if (state == ORDER_STATE_TAPE_POSITIONING_INIT) {
        moved = 0;
        state = ORDER_STATE_TAPE_POSITIONING_ENTER;

    } else if (state == ORDER_STATE_TAPE_POSITIONING_DEINIT) {
        moved = 0;
        state = ORDER_STATE_TAPE_POSITIONING_LEAVE;
    }

    RefreshScreen();

    if (moved >= 4) {
        uint8_t old_state = state;

        RestoreOrders();

        moved = 0;

        if (unit_type == MASTER) {
            PrepareConstructionSite(MININGST);
            UnitsManager_ScaleUnit(this, ORDER_STATE_SHRINK);

        } else if (unit_type == CONSTRCT && old_state == ORDER_STATE_TAPE_POSITIONING_ENTER) {
            PrepareConstructionSite(GetConstructedUnitType());
            Paths_UpdateAngle(this, (angle + 1) & 0x07);
            DrawSpriteFrame(image_index + 16);

        } else if (old_state != ORDER_STATE_TAPE_POSITIONING_ENTER) {
            SmartPointer<UnitInfo> copy = MakeCopy();

            Hash_MapHash.Remove(this);

            grid_x = x / GFX_MAP_TILE_SIZE;
            grid_y = y / GFX_MAP_TILE_SIZE;

            Hash_MapHash.Add(this);

            Access_UpdateMapStatus(this, true);
            Access_UpdateMapStatus(&*copy, false);
        }

    } else {
        int32_t target_angle = GetTargetUnitAngle();

        if (moved || !Paths_UpdateAngle(this, target_angle)) {
            ++moved;

            OffsetDrawZones(Paths_8DirPointsArrayX[target_angle] * 8, Paths_8DirPointsArrayY[target_angle] * 8);

            RefreshScreen();
        }
    }
}

void UnitInfo::PlaceMine() {
    if ((unit_type == MINELAYR || unit_type == SEAMNLYR) && storage > 0) {
        ResourceID mine_type{(unit_type == SEAMNLYR) ? SEAMINE : LANDMINE};
        bool is_found{false};

        {
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if ((*it).unit_type == mine_type) {
                        is_found = true;
                        break;
                    }
                }
            }
        }

        if (!is_found) {
            SmartPointer<UnitInfo> new_unit(UnitsManager_DeployUnit(mine_type, team, nullptr, grid_x, grid_y, 0));
            Rect bounds;
            Point site;

            bounds.ulx = std::max(0, grid_x - 1);
            bounds.uly = std::max(0, grid_y - 1);
            bounds.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), grid_x + 2);
            bounds.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), grid_y + 2);

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    const auto units = Hash_MapHash[site];

                    if (units) {
                        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                            if ((*it).unit_type == SURVEYOR) {
                                new_unit->SpotByTeam((*it).team);
                            }
                        }
                    }
                }
            }

            --storage;

            if (!storage) {
                laying_state = 0;
            }

            if (GameManager_SelectedUnit == this) {
                GameManager_UpdateInfoDisplay(this);

                SoundManager_PlaySfx(this, SFX_TYPE_POWER_CONSUMPTION_START, true);
            }
        }
    }
}

void UnitInfo::PickUpMine() {
    if ((unit_type == MINELAYR || unit_type == SEAMNLYR) && storage < GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
        SmartPointer<UnitInfo> mine;
        ResourceID mine_type{(unit_type == SEAMNLYR) ? SEAMINE : LANDMINE};
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).unit_type == mine_type) {
                    mine = *it;
                    break;
                }
            }
        }

        if (mine) {
            UnitsManager_DestroyUnit(mine.Get());
            ++storage;

            if (storage == GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                laying_state = 0;

                if (GameManager_SelectedUnit == this) {
                    GameManager_UpdateInfoDisplay(this);

                    SoundManager_PlaySfx(this, SFX_TYPE_POWER_CONSUMPTION_END, true);
                }
            }
        }
    }
}

bool UnitInfo::ShakeWater() {
    if (UnitsManager_byte_17947D) {
        bobbed = false;
    }

    int32_t distance = moved / GFX_MAP_TILE_SIZE;
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (moved >= distance * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) {
        if ((moved & 0x1F) == 0) {
            offset_x = -Paths_8DirPointsArrayX[distance];
            offset_y = -Paths_8DirPointsArrayY[distance];
        }

    } else {
        if ((moved & 0x1F) == 0) {
            offset_x = Paths_8DirPointsArrayX[distance];
            offset_y = Paths_8DirPointsArrayY[distance];
        }
    }

    if (offset_x || offset_y) {
        if (bobbed || !UnitsManager_EffectCounter) {
            return false;
        }

        RefreshScreen();
        OffsetDrawZones(offset_x, offset_y);

        shadow_offset.x -= offset_x;
        shadow_offset.y -= offset_y;

        UpdateUnitDrawZones();
        RefreshScreen();

        bobbed = true;
    }

    ++moved;

    return offset_x || offset_y;
}

bool UnitInfo::ShakeAir() {
    if (UnitsManager_byte_17947D) {
        bobbed = false;
    }

    int32_t distance = moved / 8;
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (moved >= distance * 8 + 4) {
        if ((moved & 0x01) == 0) {
            offset_x = -Paths_8DirPointsArrayX[distance];
            offset_y = -Paths_8DirPointsArrayY[distance];
        }

    } else {
        if ((moved & 0x01) == 0) {
            offset_x = Paths_8DirPointsArrayX[distance];
            offset_y = Paths_8DirPointsArrayY[distance];
        }
    }

    if (offset_x || offset_y) {
        if (bobbed || !UnitsManager_EffectCounter) {
            return false;
        }

        RefreshScreen();
        OffsetDrawZones(offset_x, offset_y);

        shadow_offset.x -= offset_x * 2;
        shadow_offset.y -= offset_y * 2;

        UpdateUnitDrawZones();
        RefreshScreen();

        bobbed = true;
    }

    moved = (moved + 1) & 0x3F;

    return offset_x || offset_y;
}

void UnitInfo::ShakeSabotage() {
    ++shake_effect_state;

    shake_effect_state &= 0x0F;

    if (!(flags & BUILDING)) {
        int32_t offset_x = Paths_8DirPointsArrayX[shake_effect_state / 2];
        int32_t offset_y = Paths_8DirPointsArrayY[shake_effect_state / 2];

        RefreshScreen();

        if (shake_effect_state & 0x01) {
            OffsetDrawZones(offset_x, offset_y);

        } else {
            OffsetDrawZones(-offset_x, -offset_y);
        }

        RefreshScreen();
    }
}

void UnitInfo::PrepareFire() {
    int32_t unit_angle = angle;
    bool team_visibility = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

    recoil_delay = 3;

    if (unit_type == ANTIAIR) {
        recoil_delay *= 5;

    } else if (unit_type == SP_FLAK || unit_type == FASTBOAT) {
        recoil_delay *= 2;
    }

    if (team_visibility || UnitsManager_TeamInfo[GameManager_PlayerTeam]
                               .heat_map_complete[target_grid_y * ResourceManager_MapSize.x + target_grid_x]) {
        SoundManager_PlaySfx(this, SFX_TYPE_FIRE);
    }

    if (flags & HAS_FIRING_SPRITE) {
        if (flags & TURRET_SPRITE) {
            DrawSpriteTurretFrame(turret_angle + firing_image_base);

        } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
            if (unit_type == COMMANDO) {
                recoil_delay = 8;

                DrawSpriteFrame(unit_angle + 104);

            } else {
                recoil_delay = 8;

                DrawSpriteFrame(unit_angle + 104);
            }

        } else {
            DrawSpriteFrame(unit_angle + firing_image_base);
        }
    }

    if (flags & FIRES_MISSILES) {
        SmartPointer<UnitInfo> new_unit;
        ResourceID particle_unit;

        switch (unit_type) {
            case SUBMARNE:
            case CORVETTE: {
                particle_unit = TORPEDO;
            } break;

            case ALNTANK: {
                particle_unit = ALNTBALL;
            } break;

            case ALNASGUN:
            case JUGGRNT: {
                particle_unit = ALNABALL;
            } break;

            case ALNPLANE: {
                particle_unit = ALNMISSL;
            } break;

            default: {
                particle_unit = ROCKET;
            } break;
        }

        if (particle_unit == ALNTBALL || particle_unit == ALNABALL) {
            unit_angle = 0;

        } else {
            unit_angle = UnitsManager_GetFiringAngle(target_grid_x - grid_x, target_grid_y - grid_y);

            if (particle_unit == ALNMISSL) {
                unit_angle *= 2;
            }
        }

        new_unit = UnitsManager_DeployUnit(particle_unit, team, nullptr, grid_x, grid_y, unit_angle, true);

        new_unit->target_grid_x = target_grid_x;
        new_unit->target_grid_y = target_grid_y;

        new_unit->SetParent(this);

        new_unit->SetOrder(ORDER_MOVE);
        new_unit->SetOrderState(ORDER_STATE_INIT);

        UnitsManager_AddToDelayedReactionList(this);
    }

    --ammo;

    if (GameManager_RealTime) {
        if (shots > ammo) {
            shots = ammo;
        }

    } else {
        --shots;

        if (!GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            UpdateInfoDisplay();
        }
    }

    state = ORDER_STATE_FIRE_IN_PROGRESS;
}

void UnitInfo::ProgressFire() {
    --recoil_delay;

    if (recoil_delay) {
        if (unit_type == ANTIAIR || unit_type == SP_FLAK || unit_type == FASTBOAT) {
            DrawSpriteTurretFrame(turret_image_index + ((recoil_delay & 1) ? -8 : 8));

        } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
            DrawSpriteFrame(image_index + 8);
        }

    } else {
        if (flags & HAS_FIRING_SPRITE) {
            if (flags & TURRET_SPRITE) {
                DrawSpriteTurretFrame(turret_image_base + turret_angle);

            } else {
                DrawSpriteFrame(image_base + angle);
            }

            if (unit_type == COMMANDO) {
                UnitsManager_TestBustedCommando(this);
            }
        }

        RestoreOrders();

        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(this);
        }

        if (!(flags & FIRES_MISSILES)) {
            Attack(target_grid_x, target_grid_y);

            UnitsManager_AddToDelayedReactionList(this);
        }

        if (speed > 0) {
            ScheduleDelayedTasks(true);
        }
    }
}

void UnitInfo::ChangeTeam(uint16_t target_team) {
    uint16_t old_team = team;

    if (target_team != old_team) {
        PathsManager_RemoveRequest(this);
    }

    UnitsManager_DelayedAttackTargets[team].Remove(*this);

    ClearUnitList();
    SetParent(nullptr);
    SetEnemy(nullptr);
    path = nullptr;
    RemoveTasks();
    Ai_RemoveUnit(this);
    Access_UpdateMapStatus(this, true);

    if (UnitsManager_TeamInfo[team].heat_map_complete) {
        visible_to_team[team] =
            UnitsManager_TeamInfo[team].heat_map_complete[grid_y * ResourceManager_MapSize.x + grid_x];

    } else {
        visible_to_team[team] = false;
    }

    flags &= ~UnitsManager_TeamInfo[team].team_units->hash_team_id;
    team = target_team;
    auto_survey = false;
    flags |= UnitsManager_TeamInfo[target_team].team_units->hash_team_id;
    color_cycling_lut = UnitsManager_TeamInfo[target_team].team_units->color_index_table;

    if (orders == ORDER_DISABLE) {
        RestoreOrders();

        recoil_delay = 0;
    }

    Access_UpdateMapStatus(this, true);

    visible_to_team[target_team] = true;

    Ai_UpdateTerrain(this);
    Ai_UnitSpotted(this, old_team);

    RefreshScreen();

    GameManager_RenderMinimapDisplay = true;
}

[[nodiscard]] UnitInfo* UnitInfo::GetParent() const noexcept { return parent_unit.Get(); }

void UnitInfo::SetParent(UnitInfo* const parent) noexcept { parent_unit = parent; }

[[nodiscard]] uint8_t UnitInfo::GetSfxType() const noexcept { return sound; }

uint8_t UnitInfo::SetSfxType(uint8_t sound) noexcept {
    auto previous_sound{this->sound};

    SDL_assert(sound < SFX_TYPE_LIMIT);

    this->sound = sound;

    return previous_sound;
}

[[nodiscard]] ResourceID UnitInfo::GetUnitType() const noexcept { return unit_type; }

void UnitInfo::SetUnitType(const ResourceID unit_type) noexcept {
    SDL_assert(unit_type < UNIT_END);

    this->unit_type = unit_type;
}

[[nodiscard]] uint8_t UnitInfo::GetOrder() const noexcept { return orders; }

uint8_t UnitInfo::SetOrder(const uint8_t order) noexcept {
    auto previous_order{this->orders};

    this->orders = order;

    return previous_order;
}

[[nodiscard]] uint8_t UnitInfo::GetPriorOrder() const noexcept { return prior_orders; }

uint8_t UnitInfo::SetPriorOrder(const uint8_t order) noexcept {
    auto previous_order{this->prior_orders};

    this->prior_orders = order;

    return previous_order;
}

uint8_t UnitInfo::GetOrderState() const noexcept { return state; }

uint8_t UnitInfo::SetOrderState(const uint8_t order_state) noexcept {
    auto previous_order_state{this->state};

    this->state = order_state;

    return previous_order_state;
}

uint8_t UnitInfo::GetPriorOrderState() const noexcept { return prior_state; }

uint8_t UnitInfo::SetPriorOrderState(const uint8_t order_state) noexcept {
    auto previous_order_state{this->prior_state};

    this->prior_state = order_state;

    return previous_order_state;
}
