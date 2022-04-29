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

#include "registerarray.hpp"
#include "resource_manager.hpp"
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

UnitInfo::UnitInfo() : build_list(0) {}

UnitInfo::UnitInfo(const UnitInfo& other) {}

UnitInfo::~UnitInfo() {}

TextFileObject* UnitInfo::Allocate() { return new (std::nothrow) UnitInfo(); }

static unsigned short UnitInfo_TypeIndex;
static MAXRegisterClass UnitInfo_ClassRegister("UnitInfo", &UnitInfo_TypeIndex, &UnitInfo::Allocate);

unsigned short UnitInfo::GetTypeIndex() const { return UnitInfo_TypeIndex; }

void UnitInfo::FileLoad(SmartFileReader& file) {}

void UnitInfo::FileSave(SmartFileWriter& file) {}

void UnitInfo::TextLoad(TextStructure& object) {}

void UnitInfo::TextSave(SmartTextfileWriter& file) {}

void UnitInfo::Init() {
    switch (unit_type) {
        case COMMTWR:
            sound_table = &UnitInfo_SfxMonopoleMine;
            break;
        case POWERSTN:
            sound_table = &UnitInfo_SfxPowerStation;
            break;
        case POWGEN:
            sound_table = &UnitInfo_SfxPowerGenerator;
            break;
        case BARRACKS:
            sound_table = &UnitInfo_SfxBarracks;
            break;
        case SHIELDGN:
            sound_table = &UnitInfo_SfxGoldRefinery;
            break;
        case RADAR:
            sound_table = &UnitInfo_SfxRadar;
            break;
        case ADUMP:
            sound_table = &UnitInfo_SfxMaterialStorage;
            break;
        case FDUMP:
            sound_table = &UnitInfo_SfxFuelStorage;
            break;
        case GOLDSM:
            sound_table = &UnitInfo_SfxGoldVault;
            break;
        case DEPOT:
            sound_table = &UnitInfo_SfxDepot;
            break;
        case HANGAR:
            sound_table = &UnitInfo_SfxHangar;
            break;
        case DOCK:
            sound_table = &UnitInfo_SfxDock;
            break;
        case ROAD:
            sound_table = &UnitInfo_SfxRoad;
            break;
        case LANDPAD:
            sound_table = &UnitInfo_SfxLandingPad;
            break;
        case SHIPYARD:
            sound_table = &UnitInfo_SfxShipyard;
            break;
        case LIGHTPLT:
            sound_table = &UnitInfo_SfxLightVehiclePlant;
            break;
        case LANDPLT:
            sound_table = &UnitInfo_SfxHeavyVehiclePlant;
            break;
        case AIRPLT:
            sound_table = &UnitInfo_SfxAirUnitsPlant;
            break;
        case HABITAT:
            sound_table = &UnitInfo_SfxHabitat;
            break;
        case RESEARCH:
            sound_table = &UnitInfo_SfxResearchCentre;
            break;
        case GREENHSE:
            sound_table = &UnitInfo_SfxEcoSphere;
            break;
        case TRAINHAL:
            sound_table = &UnitInfo_SfxTrainingHall;
            break;
        case WTRPLTFM:
            sound_table = &UnitInfo_SfxWaterPlatform;
            break;
        case GUNTURRT:
            sound_table = &UnitInfo_SfxGunTurret;
            break;
        case ANTIAIR:
            sound_table = &UnitInfo_SfxAntiAir;
            break;
        case ARTYTRRT:
            sound_table = &UnitInfo_SfxArtillery;
            break;
        case ANTIMSSL:
            sound_table = &UnitInfo_SfxMissileLauncher;
            break;
        case BLOCK:
            sound_table = &UnitInfo_SfxConcreteBlock;
            break;
        case BRIDGE:
            sound_table = &UnitInfo_SfxBridge;
            break;
        case MININGST:
            sound_table = &UnitInfo_SfxMiningStation;
            break;
        case LANDMINE:
            sound_table = &UnitInfo_SfxLandMine;
            break;
        case SEAMINE:
            sound_table = &UnitInfo_SfxSeaMine;
            break;
        case HITEXPLD:
            sound_table = &UnitInfo_SfxHitExplosion;
            break;
        case MASTER:
            sound_table = &UnitInfo_SfxMasterBuilder;
            break;
        case CONSTRCT:
            sound_table = &UnitInfo_SfxConstructor;
            break;
        case SCOUT:
            sound_table = &UnitInfo_SfxScout;
            break;
        case TANK:
            sound_table = &UnitInfo_SfxTank;
            break;
        case ARTILLRY:
            sound_table = &UnitInfo_SfxAssaultGun;
            break;
        case ROCKTLCH:
            sound_table = &UnitInfo_SfxRocketLauncher;
            break;
        case MISSLLCH:
            sound_table = &UnitInfo_SfxMissileCrawler;
            break;
        case SP_FLAK:
            sound_table = &MobileAntiAir;
            break;
        case MINELAYR:
            sound_table = &UnitInfo_SfxMineLayer;
            break;
        case SURVEYOR:
            sound_table = &UnitInfo_SfxSurveyor;
            break;
        case SCANNER:
            sound_table = &UnitInfo_SfxScanner;
            break;
        case SPLYTRCK:
            sound_table = &UnitInfo_SfxSupplyTruck;
            break;
        case GOLDTRCK:
            sound_table = &UnitInfo_SfxGoldTruck;
            break;
        case ENGINEER:
            sound_table = &UnitInfo_SfxEngineer;
            break;
        case BULLDOZR:
            sound_table = &UnitInfo_SfxBulldozer;
            break;
        case REPAIR:
            sound_table = &UnitInfo_SfxRepairUnit;
            break;
        case FUELTRCK:
            sound_table = &UnitInfo_SfxFuelTruck;
            break;
        case CLNTRANS:
            sound_table = &UnitInfo_SfxArmouredPersonnelCarrier;
            break;
        case COMMANDO:
            sound_table = &UnitInfo_SfxInfiltrator;
            break;
        case INFANTRY:
            sound_table = &UnitInfo_SfxInfantry;
            break;
        case FASTBOAT:
            sound_table = &UnitInfo_SfxEscort;
            break;
        case CORVETTE:
            sound_table = &UnitInfo_SfxCorvette;
            break;
        case BATTLSHP:
            sound_table = &UnitInfo_SfxGunBoat;
            break;
        case SUBMARNE:
            sound_table = &UnitInfo_SfxSubmarine;
            break;
        case SEATRANS:
            sound_table = &UnitInfo_SfxSeaTransport;
            break;
        case MSSLBOAT:
            sound_table = &UnitInfo_SfxMissileCruiser;
            break;
        case SEAMNLYR:
            sound_table = &UnitInfo_SfxSeaMineLayer;
            break;
        case CARGOSHP:
            sound_table = &UnitInfo_SfxCargoShip;
            break;
        case FIGHTER:
            sound_table = &UnitInfo_SfxFighter;
            break;
        case BOMBER:
            sound_table = &UnitInfo_SfxGroundAttackPlane;
            break;
        case AIRTRANS:
            sound_table = &UnitInfo_SfxAirTransport;
            break;
        case AWAC:
            sound_table = &UnitInfo_SfxAwac;
            break;
        case JUGGRNT:
            sound_table = &UnitInfo_SfxAlienGunBoat;
            break;
        case ALNTANK:
            sound_table = &UnitInfo_SfxAlienTank;
            break;
        case ALNASGUN:
            sound_table = &UnitInfo_SfxAlienAssaultGun;
            break;
        case ALNPLANE:
            sound_table = &UnitInfo_SfxAlienAttackPlane;
            break;
        default:
            sound_table = &UnitInfo_SfxDefaultUnit;
            break;
    }
}

bool UnitInfo::IsVisibleToTeam(unsigned short team) const { return visible_to_team[team]; }

unsigned short UnitInfo::GetId() const { return id; }

unsigned int UnitInfo::GetField221() const { return field_221; }

unsigned short UnitInfo::GetImageIndex() const { return image_index; }

void UnitInfo::ClearUnitList() {
    if (unit_list) {
        /// \todo Implement method
    }
}

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

UnitInfo* UnitInfo::GetParent() const { return &*parent_unit; }

void UnitInfo::SetEnemy(UnitInfo* enemy) { enemy_unit = enemy; }

UnitValues* UnitInfo::GetBaseValues() const { return &*base_values; }

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

void UnitInfo::GetVersion(char* text, int version) {
    text[0] = '\0';

    CalcRomanDigit(text, version / 100, "C", "D", "M");
    version %= 100;

    CalcRomanDigit(text, version / 10, "X", "L", "C");
    version %= 10;

    CalcRomanDigit(text, version / 100, "I", "V", "X");
}

void UnitInfo::Setname(char* text) {
    delete[] name;

    if (text && strlen(text)) {
        name = new (std::nothrow) char[strlen(text) + 1];
        strcpy(name, text);
    } else {
        name = nullptr;
    }
}

unsigned short* UnitInfo::GetAttribute(char index) {
    /// \todo
}

void UnitInfo::RefreshScreen() {
    /// \todo
}
