/* Copyright (c) 2022 M.A.X. Port Team
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

#ifndef AI_PLAYER_HPP
#define AI_PLAYER_HPP

#include "taskpathrequest.hpp"
#include "terrainmap.hpp"
#include "threatmap.hpp"
#include "transportorder.hpp"
#include "unitinfo.hpp"
#include "weighttable.hpp"

class AiPlayer {
public:
    AiPlayer();
    ~AiPlayer();

    void SetInfoMapPoint(Point point);
    void UpdateMineMap(Point point);
    void MarkMineMapPoint(Point point);
    void ChangeTasksPendingFlag(bool value);
    bool SelectStrategy();
    void PlanMinefields();
    void GuessEnemyAttackDirections();
    bool CreateBuilding(ResourceID unit_type, Point position, Task* task);
    bool IsUpgradeNeeded(UnitInfo* unit);
    bool MatchPath(TaskPathRequest* request);
    void ClearZone(Zone* zone);
    unsigned char** GetInfoMap();
    unsigned short** GetDamagePotentialMap(UnitInfo* unit, int caution_level, unsigned char flags);
    unsigned short** GetDamagePotentialMap(ResourceID unit_type, int caution_level, unsigned char flags);
    int GetDamagePotential(UnitInfo* unit, Point point, int caution_level, unsigned char flags);
    void AddTransportOrder(TransportOrder* transport_order);
    Task* FindManager(Point site);
    unsigned char** GetMineMap();
    unsigned char GetMineMapEntry(Point site);
    WeightTable GetFilteredWeightTable(ResourceID unit_type, unsigned char flags);
    int GetPredictedAttack(UnitInfo* unit, int caution_level);
    unsigned short GetTargetTeam() const;
};

extern int AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level);

extern AiPlayer AiPlayer_Teams[4];
extern TerrainMap AiPlayer_TerrainMap;
extern ThreatMap AiPlayer_ThreatMaps[10];

#endif /* AI_PLAYER_HPP */
