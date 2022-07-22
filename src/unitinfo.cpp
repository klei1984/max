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
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "message_manager.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "unitinfogroup.hpp"
#include "units_manager.hpp"

struct SoundTable UnitInfo_SfxDefaultUnit = {20,
                                             {{SFX_TYPE_IDLE, GEN_IDLE},
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

struct SoundTable UnitInfo_SfxMonopoleMine = {5,
                                              {{SFX_TYPE_BUILDING, MONOP10},
                                               {SFX_TYPE_HIT, MONOP15},
                                               {SFX_TYPE_EXPLOAD, MONOP16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, MONOP17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, MONOP18}}};

struct SoundTable UnitInfo_SfxPowerStation = {5,
                                              {{SFX_TYPE_BUILDING, POWST10},
                                               {SFX_TYPE_HIT, POWST15},
                                               {SFX_TYPE_EXPLOAD, POWST16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, POWST17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, POWST18}}};

struct SoundTable UnitInfo_SfxPowerGenerator = {5,
                                                {{SFX_TYPE_BUILDING, POWGN10},
                                                 {SFX_TYPE_HIT, POWGN15},
                                                 {SFX_TYPE_EXPLOAD, POWGN16},
                                                 {SFX_TYPE_POWER_CONSUMPTION_START, POWGN17},
                                                 {SFX_TYPE_POWER_CONSUMPTION_END, POWGN18}}};

struct SoundTable UnitInfo_SfxBarracks = {2, {{SFX_TYPE_HIT, BARRA15}, {SFX_TYPE_EXPLOAD, BARRA16}}};

struct SoundTable UnitInfo_SfxGoldRefinery = {5,
                                              {{SFX_TYPE_BUILDING, GOLDR10},
                                               {SFX_TYPE_HIT, GOLDR15},
                                               {SFX_TYPE_EXPLOAD, GOLDR16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, GOLDR17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, GOLDR18}}};

struct SoundTable UnitInfo_SfxRadar = {
    3, {{SFX_TYPE_IDLE, RADAR13}, {SFX_TYPE_HIT, RADAR15}, {SFX_TYPE_EXPLOAD, RADAR16}}};

struct SoundTable UnitInfo_SfxMaterialStorage = {2, {{SFX_TYPE_HIT, SSTOR15}, {SFX_TYPE_EXPLOAD, SSTOR16}}};

struct SoundTable UnitInfo_SfxFuelStorage = {2, {{SFX_TYPE_HIT, SFUEL15}, {SFX_TYPE_EXPLOAD, SFUEL16}}};

struct SoundTable UnitInfo_SfxGoldVault = {2, {{SFX_TYPE_HIT, SGOLD15}, {SFX_TYPE_EXPLOAD, SGOLD16}}};

struct SoundTable UnitInfo_SfxDepot = {2, {{SFX_TYPE_HIT, DEPOT15}, {SFX_TYPE_EXPLOAD, DEPOT16}}};

struct SoundTable UnitInfo_SfxHangar = {2, {{SFX_TYPE_HIT, HANGR15}, {SFX_TYPE_EXPLOAD, HANGR16}}};

struct SoundTable UnitInfo_SfxDock = {2, {{SFX_TYPE_HIT, DOCK15}, {SFX_TYPE_EXPLOAD, DOCK16}}};

struct SoundTable UnitInfo_SfxRoad = {2, {{SFX_TYPE_HIT, ROAD15}, {SFX_TYPE_EXPLOAD, ROAD16}}};

struct SoundTable UnitInfo_SfxLandingPad = {2, {{SFX_TYPE_HIT, LPAD15}, {SFX_TYPE_EXPLOAD, LPAD16}}};

struct SoundTable UnitInfo_SfxShipyard = {5,
                                          {{SFX_TYPE_BUILDING, SUNIT10},
                                           {SFX_TYPE_HIT, SUNIT15},
                                           {SFX_TYPE_EXPLOAD, SUNIT16},
                                           {SFX_TYPE_POWER_CONSUMPTION_START, SUNIT17},
                                           {SFX_TYPE_POWER_CONSUMPTION_END, SUNIT18}}};

struct SoundTable UnitInfo_SfxLightVehiclePlant = {5,
                                                   {{SFX_TYPE_BUILDING, LVP10},
                                                    {SFX_TYPE_HIT, LVP15},
                                                    {SFX_TYPE_EXPLOAD, LVP16},
                                                    {SFX_TYPE_POWER_CONSUMPTION_START, LVP17},
                                                    {SFX_TYPE_POWER_CONSUMPTION_END, LVP18}}};

struct SoundTable UnitInfo_SfxHeavyVehiclePlant = {5,
                                                   {{SFX_TYPE_BUILDING, HVP10},
                                                    {SFX_TYPE_HIT, HVP15},
                                                    {SFX_TYPE_EXPLOAD, HVP16},
                                                    {SFX_TYPE_POWER_CONSUMPTION_START, HVP17},
                                                    {SFX_TYPE_POWER_CONSUMPTION_END, HVP18}}};

struct SoundTable UnitInfo_SfxAirUnitsPlant = {5,
                                               {{SFX_TYPE_BUILDING, AUNIT10},
                                                {SFX_TYPE_HIT, AUNIT15},
                                                {SFX_TYPE_EXPLOAD, AUNIT16},
                                                {SFX_TYPE_POWER_CONSUMPTION_START, AUNIT17},
                                                {SFX_TYPE_POWER_CONSUMPTION_END, AUNIT18}}};

struct SoundTable UnitInfo_SfxHabitat = {
    3, {{SFX_TYPE_BUILDING, DORMI10}, {SFX_TYPE_HIT, DORMI15}, {SFX_TYPE_EXPLOAD, DORMI16}}};

struct SoundTable UnitInfo_SfxResearchCentre = {5,
                                                {{SFX_TYPE_BUILDING, RESEAR10},
                                                 {SFX_TYPE_HIT, RESEAR15},
                                                 {SFX_TYPE_EXPLOAD, RESEAR16},
                                                 {SFX_TYPE_POWER_CONSUMPTION_START, RESEAR17},
                                                 {SFX_TYPE_POWER_CONSUMPTION_END, RESEAR18}}};

struct SoundTable UnitInfo_SfxEcoSphere = {5,
                                           {{SFX_TYPE_BUILDING, ECOSP10},
                                            {SFX_TYPE_HIT, ECOSP15},
                                            {SFX_TYPE_EXPLOAD, ECOSP16},
                                            {SFX_TYPE_POWER_CONSUMPTION_START, ECOSP17},
                                            {SFX_TYPE_POWER_CONSUMPTION_END, ECOSP18}}};

struct SoundTable UnitInfo_SfxTrainingHall = {5,
                                              {{SFX_TYPE_BUILDING, TRAIN10},
                                               {SFX_TYPE_HIT, TRAIN15},
                                               {SFX_TYPE_EXPLOAD, TRAIN16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, TRAIN17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, TRAIN18}}};

struct SoundTable UnitInfo_SfxWaterPlatform = {2, {{SFX_TYPE_HIT, WPLAT15}, {SFX_TYPE_EXPLOAD, WPLAT16}}};

struct SoundTable UnitInfo_SfxGunTurret = {
    3, {{SFX_TYPE_FIRE, FGUN14}, {SFX_TYPE_HIT, FGUN15}, {SFX_TYPE_EXPLOAD, FGUN16}}};

struct SoundTable UnitInfo_SfxArtillery = {
    3, {{SFX_TYPE_FIRE, FARTY14}, {SFX_TYPE_HIT, FARTY15}, {SFX_TYPE_EXPLOAD, FARTY16}}};

struct SoundTable UnitInfo_SfxAntiAir = {
    3, {{SFX_TYPE_FIRE, FANTI14}, {SFX_TYPE_HIT, FANTI15}, {SFX_TYPE_EXPLOAD, FANTI16}}};

struct SoundTable UnitInfo_SfxMissileLauncher = {
    3, {{SFX_TYPE_FIRE, FROCK14}, {SFX_TYPE_HIT, FROCK15}, {SFX_TYPE_EXPLOAD, FROCK16}}};

struct SoundTable UnitInfo_SfxConcreteBlock = {2, {{SFX_TYPE_HIT, BLOCK15}, {SFX_TYPE_EXPLOAD, BLOCK16}}};

struct SoundTable UnitInfo_SfxBridge = {2, {{SFX_TYPE_HIT, BRIDG15}, {SFX_TYPE_EXPLOAD, BRIDG16}}};

struct SoundTable UnitInfo_SfxMiningStation = {5,
                                               {{SFX_TYPE_BUILDING, MSTAT10},
                                                {SFX_TYPE_HIT, MSTAT15},
                                                {SFX_TYPE_EXPLOAD, MSTAT16},
                                                {SFX_TYPE_POWER_CONSUMPTION_START, MSTAT17},
                                                {SFX_TYPE_POWER_CONSUMPTION_END, MSTAT18}}};

struct SoundTable UnitInfo_SfxLandMine = {1, {{SFX_TYPE_EXPLOAD, LMINE16}}};

struct SoundTable UnitInfo_SfxSeaMine = {1, {{SFX_TYPE_EXPLOAD, CMINE16}}};

struct SoundTable UnitInfo_SfxHitExplosion = {2, {{SFX_TYPE_HIT, EMPTYLND}, {SFX_TYPE_EXPLOAD, EMPTYWTR}}};

struct SoundTable UnitInfo_SfxMasterBuilder = {6,
                                               {{SFX_TYPE_IDLE, MASTR1},
                                                {SFX_TYPE_DRIVE, MASTR5},
                                                {SFX_TYPE_STOP, MASTR7},
                                                {SFX_TYPE_TRANSFORM, MASTR15},
                                                {SFX_TYPE_HIT, MASTR9},
                                                {SFX_TYPE_EXPLOAD, MASTR16}}};

struct SoundTable UnitInfo_SfxConstructor = {6,
                                             {{SFX_TYPE_IDLE, CONST1},
                                              {SFX_TYPE_DRIVE, CONST5},
                                              {SFX_TYPE_STOP, CONST7},
                                              {SFX_TYPE_BUILDING, CONST10},
                                              {SFX_TYPE_HIT, CONST15},
                                              {SFX_TYPE_EXPLOAD, CONST16}}};

struct SoundTable UnitInfo_SfxScout = {6,
                                       {{SFX_TYPE_IDLE, SCOUT1},
                                        {SFX_TYPE_DRIVE, SCOUT5},
                                        {SFX_TYPE_STOP, SCOUT7},
                                        {SFX_TYPE_FIRE, SCOUT14},
                                        {SFX_TYPE_HIT, SCOUT15},
                                        {SFX_TYPE_EXPLOAD, SCOUT16}}};

struct SoundTable UnitInfo_SfxTank = {6,
                                      {{SFX_TYPE_IDLE, TANK1},
                                       {SFX_TYPE_DRIVE, TANK5},
                                       {SFX_TYPE_STOP, TANK7},
                                       {SFX_TYPE_FIRE, TANK14},
                                       {SFX_TYPE_HIT, TANK15},
                                       {SFX_TYPE_EXPLOAD, TANK16}}};

struct SoundTable UnitInfo_SfxAssaultGun = {6,
                                            {{SFX_TYPE_IDLE, ASGUN1},
                                             {SFX_TYPE_DRIVE, ASGUN5},
                                             {SFX_TYPE_STOP, ASGUN7},
                                             {SFX_TYPE_FIRE, ASGUN14},
                                             {SFX_TYPE_HIT, ASGUN15},
                                             {SFX_TYPE_EXPLOAD, ASGUN16}}};

struct SoundTable UnitInfo_SfxRocketLauncher = {6,
                                                {{SFX_TYPE_IDLE, RLNCH1},
                                                 {SFX_TYPE_DRIVE, RLNCH5},
                                                 {SFX_TYPE_STOP, RLNCH7},
                                                 {SFX_TYPE_FIRE, RLNCH14},
                                                 {SFX_TYPE_HIT, RLNCH15},
                                                 {SFX_TYPE_EXPLOAD, RLNCH16}}};

struct SoundTable UnitInfo_SfxMissileCrawler = {6,
                                                {{SFX_TYPE_IDLE, MSLNC1},
                                                 {SFX_TYPE_DRIVE, MSLNC5},
                                                 {SFX_TYPE_STOP, MSLNC7},
                                                 {SFX_TYPE_FIRE, MSLNC14},
                                                 {SFX_TYPE_HIT, MSLNC15},
                                                 {SFX_TYPE_EXPLOAD, MSLNC16}}};

struct SoundTable MobileAntiAir = {6,
                                   {{SFX_TYPE_IDLE, MANTI1},
                                    {SFX_TYPE_DRIVE, MANTI5},
                                    {SFX_TYPE_STOP, MANTI7},
                                    {SFX_TYPE_FIRE, MANTI14},
                                    {SFX_TYPE_HIT, MANTI15},
                                    {SFX_TYPE_EXPLOAD, MANTI16}}};

struct SoundTable UnitInfo_SfxMineLayer = {7,
                                           {{SFX_TYPE_IDLE, MLAYER1},
                                            {SFX_TYPE_DRIVE, MLAYER5},
                                            {SFX_TYPE_STOP, MLAYER7},
                                            {SFX_TYPE_HIT, MLAYER15},
                                            {SFX_TYPE_EXPLOAD, MLAYER16},
                                            {SFX_TYPE_POWER_CONSUMPTION_START, MLAYER17},
                                            {SFX_TYPE_POWER_CONSUMPTION_END, MLAYER18}}};

struct SoundTable UnitInfo_SfxSurveyor = {5,
                                          {{SFX_TYPE_IDLE, SURVY1},
                                           {SFX_TYPE_DRIVE, SURVY5},
                                           {SFX_TYPE_STOP, SURVY7},
                                           {SFX_TYPE_HIT, SURVY15},
                                           {SFX_TYPE_EXPLOAD, SURVY16}}};

struct SoundTable UnitInfo_SfxScanner = {5,
                                         {{SFX_TYPE_IDLE, SCAN1},
                                          {SFX_TYPE_DRIVE, SCAN5},
                                          {SFX_TYPE_STOP, SCAN7},
                                          {SFX_TYPE_HIT, SCAN15},
                                          {SFX_TYPE_EXPLOAD, SCAN16}}};

struct SoundTable UnitInfo_SfxSupplyTruck = {6,
                                             {{SFX_TYPE_IDLE, MTRUK1},
                                              {SFX_TYPE_DRIVE, MTRUK5},
                                              {SFX_TYPE_STOP, MTRUK7},
                                              {SFX_TYPE_HIT, MTRUK15},
                                              {SFX_TYPE_EXPLOAD, MTRUK16},
                                              {SFX_TYPE_POWER_CONSUMPTION_START, MTRUK17}}};

struct SoundTable UnitInfo_SfxGoldTruck = {6,
                                           {{SFX_TYPE_IDLE, GTRUK1},
                                            {SFX_TYPE_DRIVE, GTRUK5},
                                            {SFX_TYPE_STOP, GTRUK7},
                                            {SFX_TYPE_HIT, GTRUK15},
                                            {SFX_TYPE_EXPLOAD, GTRUK16},
                                            {SFX_TYPE_POWER_CONSUMPTION_START, GTRUK17}}};

struct SoundTable UnitInfo_SfxEngineer = {6,
                                          {{SFX_TYPE_IDLE, ENGIN1},
                                           {SFX_TYPE_DRIVE, ENGIN5},
                                           {SFX_TYPE_STOP, ENGIN7},
                                           {SFX_TYPE_BUILDING, ENGIN10},
                                           {SFX_TYPE_HIT, ENGIN15},
                                           {SFX_TYPE_EXPLOAD, ENGIN16}}};

struct SoundTable UnitInfo_SfxBulldozer = {6,
                                           {{SFX_TYPE_IDLE, BULL1},
                                            {SFX_TYPE_DRIVE, BULL5},
                                            {SFX_TYPE_STOP, BULL7},
                                            {SFX_TYPE_BUILDING, BULL10},
                                            {SFX_TYPE_HIT, BULL15},
                                            {SFX_TYPE_EXPLOAD, BULL16}}};

struct SoundTable UnitInfo_SfxRepairUnit = {6,
                                            {{SFX_TYPE_IDLE, REPAIR1},
                                             {SFX_TYPE_DRIVE, REPAIR5},
                                             {SFX_TYPE_STOP, REPAIR7},
                                             {SFX_TYPE_HIT, REPAIR15},
                                             {SFX_TYPE_EXPLOAD, REPAIR16},
                                             {SFX_TYPE_POWER_CONSUMPTION_START, REPAIR17}}};

struct SoundTable UnitInfo_SfxFuelTruck = {6,
                                           {{SFX_TYPE_IDLE, FTRUK1},
                                            {SFX_TYPE_DRIVE, FTRUK5},
                                            {SFX_TYPE_STOP, FTRUK7},
                                            {SFX_TYPE_HIT, FTRUK15},
                                            {SFX_TYPE_EXPLOAD, FTRUK16},
                                            {SFX_TYPE_POWER_CONSUMPTION_START, FTRUK17}}};

struct SoundTable UnitInfo_SfxArmouredPersonnelCarrier = {5,
                                                          {{SFX_TYPE_IDLE, APC1},
                                                           {SFX_TYPE_DRIVE, APC5},
                                                           {SFX_TYPE_STOP, APC7},
                                                           {SFX_TYPE_HIT, APC15},
                                                           {SFX_TYPE_EXPLOAD, APC16}}};

struct SoundTable UnitInfo_SfxInfiltrator = {
    4, {{SFX_TYPE_DRIVE, INFIL5}, {SFX_TYPE_FIRE, INFIL14}, {SFX_TYPE_HIT, INFIL15}, {SFX_TYPE_EXPLOAD, INFIL16}}};

struct SoundTable UnitInfo_SfxInfantry = {
    4, {{SFX_TYPE_DRIVE, INFAN5}, {SFX_TYPE_FIRE, INFAN14}, {SFX_TYPE_HIT, INFAN15}, {SFX_TYPE_EXPLOAD, INFAN16}}};

struct SoundTable UnitInfo_SfxEscort = {6,
                                        {{SFX_TYPE_IDLE, ESCRT2},
                                         {SFX_TYPE_DRIVE, ESCRT6},
                                         {SFX_TYPE_STOP, ESCRT8},
                                         {SFX_TYPE_FIRE, ESCRT14},
                                         {SFX_TYPE_HIT, ESCRT15},
                                         {SFX_TYPE_EXPLOAD, ESCRT16}}};

struct SoundTable UnitInfo_SfxCorvette = {6,
                                          {{SFX_TYPE_IDLE, CORVT2},
                                           {SFX_TYPE_DRIVE, CORVT6},
                                           {SFX_TYPE_STOP, CORVT8},
                                           {SFX_TYPE_FIRE, CORVT14},
                                           {SFX_TYPE_HIT, CORVT15},
                                           {SFX_TYPE_EXPLOAD, CORVT16}}};

struct SoundTable UnitInfo_SfxGunBoat = {6,
                                         {{SFX_TYPE_IDLE, GUNBT2},
                                          {SFX_TYPE_DRIVE, GUNBT6},
                                          {SFX_TYPE_STOP, GUNBT8},
                                          {SFX_TYPE_FIRE, GUNBT14},
                                          {SFX_TYPE_HIT, GUNBT15},
                                          {SFX_TYPE_EXPLOAD, GUNBT16}}};

struct SoundTable UnitInfo_SfxSubmarine = {6,
                                           {{SFX_TYPE_IDLE, SUB2},
                                            {SFX_TYPE_DRIVE, SUB6},
                                            {SFX_TYPE_STOP, SUB8},
                                            {SFX_TYPE_FIRE, SUB14},
                                            {SFX_TYPE_HIT, SUB15},
                                            {SFX_TYPE_EXPLOAD, SUB16}}};

struct SoundTable UnitInfo_SfxSeaTransport = {5,
                                              {{SFX_TYPE_IDLE, STRANS2},
                                               {SFX_TYPE_DRIVE, STRANS6},
                                               {SFX_TYPE_STOP, STRANS8},
                                               {SFX_TYPE_HIT, STRANS15},
                                               {SFX_TYPE_EXPLOAD, STRANS16}}};

struct SoundTable UnitInfo_SfxMissileCruiser = {6,
                                                {{SFX_TYPE_IDLE, MSLCR2},
                                                 {SFX_TYPE_DRIVE, MSLCR6},
                                                 {SFX_TYPE_STOP, MSLCR8},
                                                 {SFX_TYPE_FIRE, MSLCR14},
                                                 {SFX_TYPE_HIT, MSLCR15},
                                                 {SFX_TYPE_EXPLOAD, MSLCR16}}};

struct SoundTable UnitInfo_SfxSeaMineLayer = {7,
                                              {{SFX_TYPE_IDLE, SMINE2},
                                               {SFX_TYPE_DRIVE, SMINE6},
                                               {SFX_TYPE_STOP, SMINE8},
                                               {SFX_TYPE_HIT, SMINE15},
                                               {SFX_TYPE_EXPLOAD, SMINE16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, SMINE17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, SMINE18}}};

struct SoundTable UnitInfo_SfxCargoShip = {6,
                                           {{SFX_TYPE_IDLE, CSHIP2},
                                            {SFX_TYPE_DRIVE, CSHIP6},
                                            {SFX_TYPE_STOP, CSHIP8},
                                            {SFX_TYPE_HIT, CSHIP15},
                                            {SFX_TYPE_POWER_CONSUMPTION_START, CSHIP17},
                                            {SFX_TYPE_EXPLOAD, CSHIP16}}};

struct SoundTable UnitInfo_SfxFighter = {8,
                                         {{SFX_TYPE_IDLE, FIGHT1},
                                          {SFX_TYPE_DRIVE, FIGHT5},
                                          {SFX_TYPE_STOP, FIGHT7},
                                          {SFX_TYPE_FIRE, FIGHT14},
                                          {SFX_TYPE_HIT, FIGHT15},
                                          {SFX_TYPE_EXPLOAD, FIGHT16},
                                          {SFX_TYPE_LAND, FIGHT19},
                                          {SFX_TYPE_TAKE, FIGHT20}}};

struct SoundTable UnitInfo_SfxGroundAttackPlane = {8,
                                                   {{SFX_TYPE_IDLE, ATACK1},
                                                    {SFX_TYPE_DRIVE, ATACK5},
                                                    {SFX_TYPE_STOP, ATACK7},
                                                    {SFX_TYPE_FIRE, ATACK14},
                                                    {SFX_TYPE_HIT, ATACK15},
                                                    {SFX_TYPE_EXPLOAD, ATACK16},
                                                    {SFX_TYPE_LAND, ATACK19},
                                                    {SFX_TYPE_TAKE, ATACK20}}};

struct SoundTable UnitInfo_SfxAirTransport = {9,
                                              {{SFX_TYPE_IDLE, ATRANS1},
                                               {SFX_TYPE_DRIVE, ATRANS5},
                                               {SFX_TYPE_STOP, ATRANS7},
                                               {SFX_TYPE_HIT, ATRANS15},
                                               {SFX_TYPE_EXPLOAD, ATRANS16},
                                               {SFX_TYPE_POWER_CONSUMPTION_START, ATRANS17},
                                               {SFX_TYPE_POWER_CONSUMPTION_END, ATRANS18},
                                               {SFX_TYPE_LAND, ATRANS19},
                                               {SFX_TYPE_TAKE, ATRANS20}}};

struct SoundTable UnitInfo_SfxAwac = {8,
                                      {{SFX_TYPE_IDLE, AWAC1},
                                       {SFX_TYPE_DRIVE, AWAC5},
                                       {SFX_TYPE_STOP, AWAC7},
                                       {SFX_TYPE_FIRE, AWAC14},
                                       {SFX_TYPE_HIT, AWAC15},
                                       {SFX_TYPE_EXPLOAD, AWAC16},
                                       {SFX_TYPE_LAND, AWAC19},
                                       {SFX_TYPE_TAKE, AWAC20}}};

struct SoundTable UnitInfo_SfxAlienGunBoat = {6,
                                              {{SFX_TYPE_IDLE, JUGGR1},
                                               {SFX_TYPE_DRIVE, JUGGR5},
                                               {SFX_TYPE_STOP, JUGGR7},
                                               {SFX_TYPE_FIRE, JUGGR14},
                                               {SFX_TYPE_HIT, JUGGR15},
                                               {SFX_TYPE_EXPLOAD, JUGGR16}}};

struct SoundTable UnitInfo_SfxAlienTank = {6,
                                           {{SFX_TYPE_IDLE, ALNTK1},
                                            {SFX_TYPE_DRIVE, ALNTK5},
                                            {SFX_TYPE_STOP, ALNTK7},
                                            {SFX_TYPE_FIRE, ALNTK14},
                                            {SFX_TYPE_HIT, ALNTK15},
                                            {SFX_TYPE_EXPLOAD, ALNTK16}}};

struct SoundTable UnitInfo_SfxAlienAssaultGun = {6,
                                                 {{SFX_TYPE_IDLE, ALNAG1},
                                                  {SFX_TYPE_DRIVE, ALNAG5},
                                                  {SFX_TYPE_STOP, ALNAG7},
                                                  {SFX_TYPE_FIRE, ALNAG14},
                                                  {SFX_TYPE_HIT, ALNAG15},
                                                  {SFX_TYPE_EXPLOAD, ALNAG16}}};

struct SoundTable UnitInfo_SfxAlienAttackPlane = {8,
                                                  {{SFX_TYPE_IDLE, ALNPL1},
                                                   {SFX_TYPE_DRIVE, ALNPL5},
                                                   {SFX_TYPE_STOP, ALNPL7},
                                                   {SFX_TYPE_FIRE, ALNPL14},
                                                   {SFX_TYPE_HIT, ALNPL15},
                                                   {SFX_TYPE_EXPLOAD, ALNPL16},
                                                   {SFX_TYPE_LAND, ALNPL19},
                                                   {SFX_TYPE_TAKE, ALNPL20}}};

const unsigned char UnitInfo::ExpResearchTopics[] = {RESEARCH_TOPIC_ATTACK, RESEARCH_TOPIC_SHOTS, RESEARCH_TOPIC_RANGE,
                                                     RESEARCH_TOPIC_ARMOR, RESEARCH_TOPIC_HITS};

static void UnitInfo_TransferCargo(UnitInfo* unit, int* cargo);

static void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file);
static void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file);
static void UnitInfo_BuildList_TextLoad(SmartObjectArray<ResourceID>* build_list, TextStructure& object);
static void UnitInfo_BuildList_TextSave(SmartObjectArray<ResourceID>* build_list, SmartTextfileWriter& file);

void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file) {
    ResourceID unit_type;

    build_list->Clear();

    for (int count = file.ReadObjectCount(); count > 0; --count) {
        file.Read(unit_type);
        build_list->PushBack(&unit_type);
    }
}

void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file) {
    ResourceID unit_type;
    int count;

    count = build_list->GetCount();

    file.WriteObjectCount(count);

    for (int i = 0; i < count; ++i) {
        unit_type = *((*build_list)[i]);

        file.Write(unit_type);
    }
}

void UnitInfo_BuildList_TextLoad(SmartObjectArray<ResourceID>* build_list, TextStructure& object) {
    unsigned short value;
    ResourceID unit_type;

    build_list->Clear();

    while (object.GetField("unit", Enums_UnitType, &value)) {
        unit_type = static_cast<ResourceID>(value);
        build_list->PushBack(&unit_type);
    }
}

void UnitInfo_BuildList_TextSave(SmartObjectArray<ResourceID>* build_list, SmartTextfileWriter& file) {
    ResourceID unit_type;
    int count;

    count = build_list->GetCount();

    for (int i = 0; i < count; ++i) {
        unit_type = *((*build_list)[i]);

        file.WriteEnum("unit", Enums_UnitType, unit_type);
    }
}

UnitInfo::UnitInfo()
    : unit_type(INVALID_ID),
      popup(nullptr),
      sound_table(nullptr),
      field_158(false),
      pin_count(0),
      field_165(true),
      unit_list(nullptr),
      group_speed(0),
      name(nullptr),
      build_rate(1),
      field_221(0),
      disabled_reaction_fire(false),
      auto_survey(false) {}

UnitInfo::UnitInfo(ResourceID unit_type, unsigned short team, unsigned short id, unsigned char angle)
    : orders(ORDER_AWAITING),
      state(ORDER_STATE_1),
      prior_orders(ORDER_AWAITING),
      prior_state(ORDER_STATE_1),
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
      field_158(0),
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
      damaged_this_turn(0),
      repeat_build(0),
      field_165(1),
      move_fraction(0),
      connectors(0),
      scaler_adjust(0),
      research_topic(0),
      velocity(0),
      sound(0),
      unit_list(nullptr),
      group_speed(0),
      shadow_offset(0, 0),
      auto_survey(false),
      angle(angle) {
    BaseUnit* unit;

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
        image_base = (UnitsManager_TeamInfo[team].team_clan - 1) * sizeof(unsigned short);
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
    bobbed = 0;
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
      point(other.point),
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
      parent_unit(other.parent_unit),
      enemy_unit(other.enemy_unit),
      task_list1(other.task_list1),
      field_158(other.field_158),
      unknown_point(other.unknown_point),
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

TextFileObject* UnitInfo::Allocate() { return new (std::nothrow) UnitInfo(); }

static unsigned short UnitInfo_TypeIndex;
static MAXRegisterClass UnitInfo_ClassRegister("UnitInfo", &UnitInfo_TypeIndex, &UnitInfo::Allocate);

unsigned short UnitInfo::GetTypeIndex() const { return UnitInfo_TypeIndex; }

void UnitInfo::Init() {
    BaseUnit* base_unit;
    unsigned int data_size;

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
                base_unit->shadows = Gfx_RescaleSprite(base_unit->shadows, &data_size, 0, 2);

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

bool UnitInfo::IsVisibleToTeam(unsigned short team) const { return visible_to_team[team]; }

void UnitInfo::SetEnemy(UnitInfo* enemy) { enemy_unit = enemy; }

UnitInfo* UnitInfo::GetEnemy() const { return &*enemy_unit; }

UnitInfo* UnitInfo::GetParent() const { return &*parent_unit; }

unsigned short UnitInfo::GetId() const { return id; }

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

unsigned int UnitInfo::GetField221() const { return field_221; }

unsigned short UnitInfo::GetImageIndex() const { return image_index; }

void UnitInfo::PushFrontTask1List(Task* task) { task_list1.PushFront(*task); }

void UnitInfo::ClearTask1List() {
    /// \todo Implement method
}

Task* UnitInfo::GetTask1ListFront() const {
    Task* task;

    if (task_list1.GetCount()) {
        task = &task_list1[0];
    } else {
        task = nullptr;
    }

    return task;
}

void UnitInfo::SetParent(UnitInfo* parent) { parent_unit = parent; }

void UnitInfo::SetBaseValues(UnitValues* unit_values) { base_values = unit_values; }

UnitValues* UnitInfo::GetBaseValues() const { return &*base_values; }

bool UnitInfo::IsDetectedByTeam(unsigned short team) const { return (spotted_by_team[team] || visible_to_team[team]); }

Complex* UnitInfo::GetComplex() const { return &*complex; }

bool UnitInfo::UnitInfo_sub_430A2(short grid_x, short grid_y) {
    /// \todo Implement method
    return false;
}

SmartPointer<UnitInfo> UnitInfo::MakeCopy() {
    SmartPointer<UnitInfo> copy = new (std::nothrow) UnitInfo(*this);
    copy->path = nullptr;

    return copy;
}

void UnitInfo::OffsetDrawZones(int offset_x, int offset_y) {
    x = offset_x;
    sprite_bounds.ulx += offset_x;
    sprite_bounds.lrx += offset_x;
    shadow_bounds.ulx += offset_x;
    shadow_bounds.lrx += offset_x;

    x = offset_y;
    sprite_bounds.uly += offset_y;
    sprite_bounds.lry += offset_y;
    sprite_bounds.uly += offset_y;
    sprite_bounds.lry += offset_y;
}

void UnitInfo::UpdateUnitDrawZones() {
    BaseUnit* base_unit;

    base_unit = &UnitsManager_BaseUnits[unit_type];

    if (base_unit->sprite) {
        Point point;
        int unit_size;

        point.x = x;
        point.y = y;

        if (flags & BUILDING) {
            unit_size = 64;
        } else {
            unit_size = 32;
        }

        sprite_bounds.ulx = point.x - unit_size;
        sprite_bounds.uly = point.y - unit_size;
        sprite_bounds.lrx = point.x + unit_size - 1;
        sprite_bounds.lry = point.y + unit_size - 1;

        UpdateSpriteFrameBounds(&sprite_bounds, point.x, point.y,
                                reinterpret_cast<struct ImageMultiHeader*>(base_unit->sprite), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&sprite_bounds, point.x + turret_offset_x, point.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(base_unit->sprite), turret_image_index);
        }

        shadow_bounds.ulx = 32000;
        shadow_bounds.uly = -32000;
        shadow_bounds.lrx = 32000;
        shadow_bounds.lry = -32000;

        point -= shadow_offset;

        UpdateSpriteFrameBounds(&shadow_bounds, point.x, point.y,
                                reinterpret_cast<struct ImageMultiHeader*>(base_unit->shadows), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&sprite_bounds, point.x + turret_offset_x, point.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(base_unit->shadows), turret_image_index);
        }

        if (shadow_bounds.ulx > shadow_bounds.lrx || shadow_bounds.uly > shadow_bounds.lry ||
            shadow_bounds.ulx - 16 < sprite_bounds.ulx || shadow_bounds.uly - 16 < sprite_bounds.uly) {
            shadow_bounds.ulx = std::min(shadow_bounds.ulx, sprite_bounds.ulx);
            shadow_bounds.uly = std::min(shadow_bounds.uly, sprite_bounds.uly);
            shadow_bounds.lrx = std::min(shadow_bounds.lrx, sprite_bounds.lrx);
            shadow_bounds.lry = std::min(shadow_bounds.lry, sprite_bounds.lry);
        }
    }
}

void UnitInfo::GetName(char* text) const {
    if (name) {
        strcpy(text, name);
    } else {
        sprintf(text, "%s %i", UnitsManager_BaseUnits[unit_type].singular_name, unit_id);
    }
}

void UnitInfo::GetDisplayName(char* text) const {
    char name[40];
    char mark[20];

    GetName(name);
    GetVersion(mark, base_values->GetVersion());
    strcpy(text, "Mk ");
    strcat(text, mark);
    strcat(text, " ");
    strcat(text, name);
}

void UnitInfo::CalcRomanDigit(char* text, int value, const char* digit1, const char* digit2, const char* digit3) {
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

        for (int i = 0; i < value; ++i) {
            strcat(text, digit1);
        }
    }
}

Complex* UnitInfo::CreateComplex(unsigned short team) {
    return UnitsManager_TeamInfo[team].team_units->CreateComplex();
}

struct ImageMultiFrameHeader* UnitInfo::GetSpriteFrame(struct ImageMultiHeader* sprite, unsigned short image_index) {
    uintptr_t offset;

    SDL_assert(sprite);
    SDL_assert(image_index >= 0 && image_index < sprite->image_count);

    offset =
        reinterpret_cast<int*>(&(reinterpret_cast<unsigned char*>(sprite)[sizeof(sprite->image_count)]))[image_index];

    return reinterpret_cast<ImageMultiFrameHeader*>(&(reinterpret_cast<unsigned char*>(sprite)[offset]));
}

void UnitInfo::UpdateSpriteFrameBounds(Rect* bounds, int x, int y, struct ImageMultiHeader* sprite,
                                       unsigned short image_index) {
    if (sprite) {
        struct ImageMultiFrameHeader* frame;
        int scaling_factor;
        Rect frame_bounds;

        frame = GetSpriteFrame(sprite, image_index);

        if (frame->width > 0 && frame->height > 0) {
            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor = 2;
            } else {
                scaling_factor = 1;
            }
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

void UnitInfo::UpdateSpriteFrame(unsigned short image_base, unsigned short image_index_max) {
    this->image_base = image_base;
    this->image_index_max = image_index_max;
    DrawSpriteFrame(image_base + angle);
}

void UnitInfo::DrawSpriteFrame(unsigned short image_index) {
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

void UnitInfo::DrawSpriteTurretFrame(unsigned short turret_image_index) {
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

void UnitInfo::GetVersion(char* text, int version) {
    text[0] = '\0';

    CalcRomanDigit(text, version / 100, "C", "D", "M");
    version %= 100;

    CalcRomanDigit(text, version / 10, "X", "L", "C");
    version %= 10;

    CalcRomanDigit(text, version, "I", "V", "X");
}

void UnitInfo::SetName(char* text) {
    delete[] name;

    if (text && strlen(text)) {
        name = new (std::nothrow) char[strlen(text) + 1];
        strcpy(name, text);
    } else {
        name = nullptr;
    }
}

void UnitInfo_TransferCargo(UnitInfo* unit, int* cargo) {
    if (*cargo >= 0) {
        int free_capacity;

        free_capacity = unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - unit->storage;

        if (*cargo <= free_capacity) {
            unit->storage += *cargo;
            *cargo = 0;

        } else {
            unit->storage += free_capacity;
            *cargo -= free_capacity;
        }

        if (GameManager_SelectedUnit == unit) {
            GameManager_UpdateInfoDisplay(unit);
        }

    } else if (-(*cargo) <= unit->storage) {
        unit->storage += *cargo;
        *cargo = 0;

    } else {
        *cargo += unit->storage;
        unit->storage = 0;
    }
}

void UnitInfo_Transfer(Complex* complex, int raw, int fuel, int gold) {
    complex->gold += gold;
    complex->fuel += fuel;
    complex->material += raw;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if (raw || fuel || gold) {
            if ((*it).GetComplex() == complex) {
                switch (UnitsManager_BaseUnits[(*it).unit_type].cargo_type) {
                    case CARGO_TYPE_RAW: {
                        UnitInfo_TransferCargo(&*it, &raw);
                    } break;

                    case CARGO_TYPE_FUEL: {
                        UnitInfo_TransferCargo(&*it, &fuel);
                    } break;

                    case CARGO_TYPE_GOLD: {
                        UnitInfo_TransferCargo(&*it, &gold);
                    } break;
                }
            }
        }
    }
}

int UnitInfo::GetRaw() {
    int result;

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

int UnitInfo::GetRawFreeCapacity() {
    int result;

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

void UnitInfo::TransferRaw(int amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_RAW) {
        storage += amount;

        if (complex != nullptr) {
            int storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->material += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->material -= amount;

                UnitInfo_Transfer(&*complex, amount, 0, 0);
            }
        }
    }
}

int UnitInfo::GetFuel() {
    int result;

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

int UnitInfo::GetFuelFreeCapacity() {
    int result;

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

void UnitInfo::TransferFuel(int amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_FUEL) {
        storage += amount;

        if (complex != nullptr) {
            int storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->fuel += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->fuel -= amount;

                UnitInfo_Transfer(&*complex, 0, amount, 0);
            }
        }
    }
}

int UnitInfo::GetGold() {
    int result;

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

int UnitInfo::GetGoldFreeCapacity() {
    int result;

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

void UnitInfo::TransferGold(int amount) {
    if (UnitsManager_BaseUnits[unit_type].cargo_type == CARGO_TYPE_GOLD) {
        storage += amount;

        if (complex != nullptr) {
            int storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->gold += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->gold -= amount;

                UnitInfo_Transfer(&*complex, 0, 0, amount);
            }
        }
    }
}

int UnitInfo::GetTurnsToRepair() {
    int hits_damage;
    int base_hits;

    base_hits = base_values->GetAttribute(ATTRIB_HITS);
    hits_damage = base_hits - hits;

    return (base_hits * 4 + GetNormalRateBuildCost() * hits_damage - 1) / (base_hits * 4);
}

unsigned short* UnitInfo::GetAttribute(char index) {
    /// \todo
}

void UnitInfo::RefreshScreen() {
    GameManager_AddDrawBounds(&shadow_bounds);
    GameManager_AddDrawBounds(&sprite_bounds);
}

void UnitInfo::UpdateAngle(unsigned short image_index) {
    int image_diff;

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

int UnitInfo::GetDrawLayer(ResourceID unit_type) {
    int result;

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

void UnitInfo::AddToDrawList(unsigned int override_flags) {
    unsigned int unit_flags;

    if (override_flags) {
        unit_flags = override_flags;
    } else {
        unit_flags = flags;
    }

    if (unit_flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        UnitsManager_MobileLandSeaUnits.PushFront(*this);

    } else if (unit_flags & STATIONARY) {
        if (unit_flags & GROUND_COVER) {
            int layer_index;
            SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();

            layer_index = GetDrawLayer(unit_type);

            for (; it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if (GetDrawLayer((*it).unit_type) >= layer_index) {
                    break;
                }
            }

            UnitsManager_GroundCoverUnits.InsertAfter(it, *this);

        } else {
            int reference_y;
            SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();

            reference_y = y;

            for (; it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).y >= reference_y) {
                    break;
                }
            }

            UnitsManager_StationaryUnits.InsertAfter(it, *this);
        }

    } else if (unit_flags & MOBILE_AIR_UNIT) {
        SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();

        for (; it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).flags & HOVERING) {
                break;
            }
        }

        UnitsManager_MobileAirUnits.InsertAfter(it, *this);

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

            UnitsManager_ParticleUnits.InsertAfter(it, *this);
        }
    }
}

void UnitInfo::SetPosition(int grid_x, int grid_y, bool skip_map_status_update) {
    x = grid_x * 64 + 32;
    y = grid_y * 64 + 32;

    this->grid_x = x / 64;
    this->grid_y = y / 64;

    if (flags & BUILDING) {
        x += 31;
        y += 31;
    }

    UpdateUnitDrawZones();
    RefreshScreen();

    Hash_MapHash.Add(this, flags & GROUND_COVER);

    if (!skip_map_status_update && ((GetId() != 0xFFFF) || (flags & (EXPLODING | MISSILE_UNIT)))) {
        Access_UpdateMapStatus(this, true);
    }

    AddToDrawList();
}

void UnitInfo::UpdatePinCount(int grid_x, int grid_y, int pin_units) {
    /// \todo
}

void UnitInfo::ClearFromTaskLists() {
    /// \todo
}

void UnitInfo::ReaddUnknown(int grid_x, int grid_y) {
    /// \todo
}

void UnitInfo::RemoveUnknown() {
    /// \todo
}

void UnitInfo::BuildOrder() {
    if (orders != ORDER_AWAITING_21 && orders != ORDER_BUILDING_HALTED) {
        build_time = BuildMenu_GetTurnsToBuild(GetConstructedUnitType(), team);
    }

    orders = ORDER_AWAITING;
    state = ORDER_STATE_1;

    UnitsManager_SetNewOrder(this, ORDER_BUILDING, ORDER_STATE_0);
}

bool UnitInfo::IsUpgradeAvailable() {
    /// \todo
}

void UnitInfo::Redraw() {
    /// \todo
}

void UnitInfo::GainExperience(int experience) {
    if (flags & REGENERATING_UNIT) {
        storage += experience;

        if (storage >= 15) {
            int upgrade_topic;
            int upgrade_cost;
            bool is_upgraded;

            SmartPointer<UnitValues> unit_values =
                UnitsManager_TeamInfo[team].team_units->GetCurrentUnitValues(unit_type);

            is_upgraded = false;

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *base_values);

            upgrade_topic = ExpResearchTopics[(dos_rand() * sizeof(ExpResearchTopics)) >> 15];
            upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);

            while (upgrade_cost <= storage) {
                storage -= upgrade_cost;

                if (!is_upgraded) {
                    base_values = new UnitValues(*base_values);
                }

                base_values->SetAttribute(upgrade_topic, TeamUnits_UpgradeOffsetFactor(team, unit_type, upgrade_topic));

                upgrade_topic = ExpResearchTopics[(dos_rand() * sizeof(ExpResearchTopics)) >> 15];
                upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);
                is_upgraded = true;
            }

            if (is_upgraded) {
                if (team == GameManager_PlayerTeam) {
                    SmartString string;

                    string.Sprintf(80, "%s at [%i,%i] has increased in experience.",
                                   UnitsManager_BaseUnits[team].singular_name, grid_x + 1, grid_y + 1);
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

void UnitInfo::ProcessTaskList() {
    for (SmartList<Task>::Iterator it = task_list2.Begin(); it != task_list2.End(); ++it) {
        (*it).Remove(*this);
    }

    task_list2.Clear();
}

void UnitInfo::AttackUnit(UnitInfo* enemy, int attack_potential, int direction) {
    /// \todo
}

bool UnitInfo::ExpectAttack() {
    bool result;

    RefreshScreen();

    if (pin_count) {
        result = false;
    } else {
        if (path != nullptr) {
            if ((flags & MOBILE_AIR_UNIT) && !(flags & HOVERING)) {
                UnitsManager_SetNewOrderInt(this, ORDER_TAKING_OFF, ORDER_STATE_0);
                Ai_SetTasksPendingFlag("plane takeoff");

                result = false;

            } else {
                if (speed) {
                    Ai_SetTasksPendingFlag("moving");
                }

                result = path->Path_vfunc10(this);
            }

        } else {
            if (flags & MISSILE_UNIT) {
                SmartPointer<UnitInfo> parent;

                parent = GetParent();
                parent->Attack(grid_x, grid_y);

                UnitsManager_DestroyUnit(this);

                result = false;

            } else {
                orders = ORDER_AWAITING;
                state = ORDER_STATE_1;
                result = false;
            }
        }
    }

    return result;
}

void UnitInfo::ClearBuildListAndPath() {
    path = nullptr;

    if (parent_unit != nullptr) {
        orders = ORDER_BUILDING;
        state = ORDER_STATE_UNIT_READY;
    } else {
        orders = ORDER_AWAITING;
        state = ORDER_STATE_1;
    }

    build_list.Clear();
}

void UnitInfo::Build() {
    ResourceID build_unit_type;

    build_unit_type = *build_list[0];
    orders = ORDER_BUILDING;

    if (path != nullptr) {
        if (Builder_IssueBuildOrder(this, &grid_x, &grid_y, build_unit_type)) {
            target_grid_x = grid_x;
            target_grid_y = grid_y;

            build_time = BuildMenu_GetTurnsToBuild(build_unit_type, team);

            StartBuilding();

        } else {
            parent_unit = nullptr;

            ClearBuildListAndPath();
            SoundManager.PlaySfx(this, SFX_TYPE_IDLE);
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
    /// \todo
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

UnitInfo* UnitInfo::GetConnectedBuilding(unsigned int connector) {
    int grid_size;
    UnitInfo* result;

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
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y);
        } break;

        case CONNECTOR_SOUTH_RIGHT: {
            result = Access_GetTeamBuilding(team, grid_x + 1, grid_y + grid_size);
        } break;

        case CONNECTOR_WEST_TOP: {
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y);
        } break;

        case CONNECTOR_WEST_BOTTOM: {
            result = Access_GetTeamBuilding(team, grid_x, grid_y + grid_size);
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
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_EAST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            UnitInfo* unit;

            unit = GetConnectedBuilding(CONNECTOR_WEST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }
    }
}

void UnitInfo::AttachToPrimaryComplex() {
    /// \todo
}

void UnitInfo::TestConnections() {
    /// \todo
}

UnitInfo* UnitInfo::GetFirstUntestedConnection() {
    /// \todo
}

void UnitInfo::DetachComplex() {
    /// \todo
}

void UnitInfo::FileLoad(SmartFileReader& file) {
    /// \todo
}

void UnitInfo::FileSave(SmartFileWriter& file) {
    /// \todo
}

void UnitInfo::TextLoad(TextStructure& object) {
    /// \todo
}

void UnitInfo::TextSave(SmartTextfileWriter& file) {
    /// \todo
}

void UnitInfo::UpdateTurretAngle(int turrent_angle, bool redraw) {
    /// \todo
}

void UnitInfo::Attack(int grid_x, int grid_y) {
    /// \todo
}

void UnitInfo::StartBuilding() {
    /// \todo
}

void UnitInfo::InitStealthStatus() {
    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        if (unit_type == LANDMINE || unit_type == SEAMINE || unit_type == COMMANDO || unit_type == SUBMARNE) {
            visible_to_team[i] = 0;
        } else {
            visible_to_team[i] = GameManager_AllVisible;
        }

        spotted_by_team[i] = 0;
    }

    visible_to_team[team] = 1;
}

void UnitInfo::SpotByTeam(unsigned short team) {
    if (this->team != team && orders != ORDER_IDLE && !visible_to_team[team]) {
        visible_to_team[team] = true;
        spotted_by_team[team] = true;

        if (UnitsManager_TeamInfo[this->team].team_type == TEAM_TYPE_COMPUTER) {
            Ai_AddUnitToTrackerList(this);
        }

        RefreshScreen();

        if (unit_type == COMMANDO && orders == ORDER_AWAITING) {
            UnitsManager_DrawBustedCommando(this);
        }

        if (unit_type == SUBMARNE || unit_type == CLNTRANS) {
            image_base = 8;

            DrawSpriteFrame(image_base + angle);
        }

        /// \todo Ai_sub_49060(this, team);

        if (team == GameManager_PlayerTeam) {
            RadarPing();

        } else if (unit_type == SUBMARNE && this->team == GameManager_PlayerTeam) {
            SoundManager.PlayVoice(V_M201, V_F201);
        }
    }
}

void UnitInfo::Draw(unsigned short team) {
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

            } else if (unit_type == COMMANDO && orders == ORDER_AWAITING && image_base == 0) {
                DrawSpriteFrame(angle);
            }
        }

        RefreshScreen();
    }
}

void UnitInfo::DrawStealth(unsigned short team) {
    /// \todo
}

void UnitInfo::Resupply() {
    /// \todo
}

int UnitInfo::GetRawConsumptionRate() { return Cargo_GetRawConsumptionRate(unit_type, GetMaxAllowedBuildRate()); }

void UnitInfo::UpdateProduction() {
    /// \todo
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

void UnitInfo::RenderShadow(Point point, int image_id, Rect* bounds) {
    if (UnitsManager_BaseUnits[unit_type].shadows) {
        unsigned int scaling_factor;
        unsigned int zoom_level;
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

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<unsigned char*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<unsigned int*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;

                Gfx_DecodeShadow();
            }
        }
    }
}

void UnitInfo::RenderAirShadow(Rect* bounds) { RenderShadow(Point(grid_x, grid_y), image_index, bounds); }

void UnitInfo::RenderSprite(Point point, int image_base, Rect* bounds) {
    if (UnitsManager_BaseUnits[unit_type].sprite) {
        unsigned int scaling_factor;
        unsigned int zoom_level;
        struct ImageMultiFrameHeader* frame;

        scaling_factor = 1 << (scaler_adjust + 1);

        zoom_level = (2 * Gfx_ZoomLevel) / scaling_factor;

        if (zoom_level >= 4) {
            Gfx_ResourceBuffer = UnitsManager_BaseUnits[unit_type].sprite;

            frame = GetSpriteFrame(reinterpret_cast<struct ImageMultiHeader*>(Gfx_ResourceBuffer), image_base);

            point -= shadow_offset;

            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor /= 2;
            }

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<unsigned char*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<unsigned int*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;
                Gfx_UnitBrightnessBase = brightness;

                if (zoom_level < 8) {
                    if (flags & HASH_TEAM_RED) {
                        Gfx_TeamColorIndexBase = 0x01;

                    } else if (flags & HASH_TEAM_GREEN) {
                        Gfx_TeamColorIndexBase = 0x02;

                    } else if (flags & HASH_TEAM_BLUE) {
                        Gfx_TeamColorIndexBase = 0x03;

                    } else if (flags & HASH_TEAM_GRAY) {
                        Gfx_TeamColorIndexBase = 0xFF;

                    } else {
                        Gfx_TeamColorIndexBase = 0x04;
                    }

                } else {
                    Gfx_TeamColorIndexBase = 0x00;
                }

                Gfx_DecodeSprite();
            }
        }
    }
}

void UnitInfo::Render(Rect* bounds) {
    Point point(grid_x, grid_y);

    RenderSprite(point, image_index, bounds);

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        point.x = turret_offset_x;
        point.y = turret_offset_y;

        RenderSprite(point, turret_image_index, bounds);
    }
}

void UnitInfo::RenderWithConnectors(Rect* bounds) {
    Point point(x, y);

    RenderShadow(point, image_index, bounds);
    RenderSprite(point, image_index, bounds);

    if (connectors) {
        if (connectors & CONNECTOR_NORTH_LEFT) {
            RenderShadow(point, connector_image_base, bounds);
            RenderSprite(point, connector_image_base, bounds);
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            RenderShadow(point, connector_image_base + 4, bounds);
            RenderSprite(point, connector_image_base + 4, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            RenderShadow(point, connector_image_base + 2, bounds);
            RenderSprite(point, connector_image_base + 2, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            RenderShadow(point, connector_image_base + 6, bounds);
            RenderSprite(point, connector_image_base + 6, bounds);
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            RenderShadow(point, connector_image_base + 1, bounds);
            RenderSprite(point, connector_image_base + 1, bounds);
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            RenderShadow(point, connector_image_base + 5, bounds);
            RenderSprite(point, connector_image_base + 5, bounds);
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            RenderShadow(point, connector_image_base + 3, bounds);
            RenderSprite(point, connector_image_base + 3, bounds);
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            RenderShadow(point, connector_image_base + 7, bounds);
            RenderSprite(point, connector_image_base + 7, bounds);
        }
    }

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        point.x = turret_offset_x;
        point.y = turret_offset_y;

        RenderShadow(point, turret_image_index, bounds);
        RenderSprite(point, turret_image_index, bounds);
    }
}

int UnitInfo::GetMaxAllowedBuildRate() {
    /// \todo
}

void UnitInfo::TakePathStep() {
    if (orders == ORDER_MOVING && path != nullptr && (state == ORDER_STATE_5 || state == ORDER_STATE_6)) {
        path->Path_vfunc8(this);
    }
}

int UnitInfo::GetLayingState() const { return laying_state; }

void UnitInfo::SetLayingState(int state) { laying_state = state; }

void UnitInfo::ClearPins() { pin_count = 0; }

bool UnitInfo::AttemptSideStep(int grid_x, int grid_y, int angle) {
    /// \todo
}

int UnitInfo::GetTurnsToBuild(ResourceID unit_type, int build_speed_multiplier, int* turns_to_build) {
    /// \todo
}

void UnitInfo::SetBuildRate(int value) { build_rate = value; }

int UnitInfo::GetBuildRate() const { return build_rate; }

void UnitInfo::SetRepeatBuildState(bool value) { repeat_build = value; }

bool UnitInfo::GetRepeatBuildState() const { return repeat_build; }

void UnitInfo::SpawnNewUnit() {
    /// \todo
}

void UnitInfo::FollowUnit() {
    /// \todo
}

int UnitInfo::GetExperience() {
    /// \todo
}

void UnitInfo::BlockedOnPathRequest(bool mode) {
    /// \todo
}

void UnitInfo::MoveFinished(bool mode) {
    /// \todo
}

void UnitInfo::RadarPing() {
    /// \todo
}

int UnitInfo::GetNormalRateBuildCost() const {
    int result;

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
