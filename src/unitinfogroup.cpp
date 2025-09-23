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

#include "unitinfogroup.hpp"

#include "drawmap.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "units_manager.hpp"

UnitInfoArray::UnitInfoArray(uint16_t growth_factor) : array(growth_factor) {}

UnitInfoArray::~UnitInfoArray() {}

void UnitInfoArray::Insert(UnitInfo& unit) {
    for (uint32_t i = 0, count = array.GetCount(); i < count; ++i) {
        if (&array[i] == &unit) {
            return;
        }
    }

    array.Insert(&unit, array.GetCount());
}

int32_t UnitInfoArray::GetCount() const { return array.GetCount(); }

UnitInfo& UnitInfoArray::operator[](int32_t index) const { return array[index]; }

void UnitInfoArray::Release() { array.Release(); }

UnitInfoGroup::UnitInfoGroup() {}

UnitInfoGroup::~UnitInfoGroup() {}

Rect* UnitInfoGroup::GetBounds1() { return &bounds1; }

Rect* UnitInfoGroup::GetBounds2() { return &bounds2; }

bool UnitInfoGroup::IsRelevant(UnitInfo* unit, UnitInfoGroup* group) {
    bool result;

    if ((unit->IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) &&
        (Gfx_ZoomLevel >= 8 || !(unit->flags & GROUND_COVER)) && (unit->GetOrder() != ORDER_IDLE)) {
        result = unit->IsInGroupZone(group);
    } else {
        result = false;
    }

    return result;
}

bool UnitInfoGroup::Populate() {
    bool result;
    Point point;
    int32_t unit_count;
    Rect bounds;

    unit_count = UnitsManager_GroundCoverUnits.GetCount() + UnitsManager_MobileLandSeaUnits.GetCount() +
                 UnitsManager_StationaryUnits.GetCount() + UnitsManager_MobileAirUnits.GetCount() +
                 UnitsManager_ParticleUnits.GetCount();

    bounds.ulx = std::max(bounds1.ulx / 64 - 2, 0);
    bounds.uly = std::max(bounds1.uly / 64 - 2, 0);
    bounds.lrx = std::min((bounds1.lrx + 63) / 64 + 1, static_cast<int32_t>(ResourceManager_MapSize.x));
    bounds.lry = std::min((bounds1.lry + 63) / 64 + 1, static_cast<int32_t>(ResourceManager_MapSize.y));

    if ((bounds.lrx - bounds.ulx) * (bounds.lry - bounds.uly) <= (unit_count / 2)) {
        for (point.y = bounds.uly; point.y < bounds.lry; ++point.y) {
            for (point.x = bounds.ulx; point.x < bounds.lrx; ++point.x) {
                const auto units = Hash_MapHash[point];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if (UnitInfoGroup::IsRelevant(&(*it), this)) {
                            if ((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                                sea_land_units.Insert((*it));

                            } else if ((*it).flags & GROUND_COVER) {
                                switch ((*it).GetUnitType()) {
                                    case BRIDGE: {
                                        if ((*it).IsBridgeElevated()) {
                                            elevated_bridge.Insert((*it));

                                        } else {
                                            passeable_ground_cover.Insert((*it));
                                        }
                                    } break;

                                    case TORPEDO:
                                    case TRPBUBLE: {
                                        torpedos.Insert((*it));
                                    } break;

                                    case SMLSLAB:
                                    case LRGSLAB:
                                    case ROAD:
                                    case WALDO: {
                                        passeable_ground_cover.Insert((*it));
                                    } break;

                                    case LANDMINE:
                                    case SEAMINE: {
                                        sea_land_mines.Insert((*it));
                                    } break;

                                    case WTRPLTFM: {
                                        water_platform.Insert((*it));
                                    } break;

                                    case SMLRUBLE:
                                    case LRGRUBLE: {
                                        land_rubbles.Insert((*it));
                                    } break;

                                    default: {
                                        ground_covers.Insert((*it));
                                    } break;
                                }

                            } else if ((*it).flags & STATIONARY) {
                                if ((*it).GetUnitType() == LANDPAD) {
                                    ground_covers.Insert((*it));

                                } else {
                                    elevated_bridge.Insert((*it));
                                }

                            } else if ((*it).flags & MISSILE_UNIT) {
                                if (((*it).flags & EXPLODING) && (*it).GetUnitType() != RKTSMOKE) {
                                    air_particles_explosions.Insert((*it));

                                } else {
                                    rockets.Insert((*it));
                                }

                            } else if ((*it).flags & MOBILE_AIR_UNIT) {
                                if (((*it).flags & HOVERING) || (*it).GetOrder() == ORDER_MOVE) {
                                    air_units_in_air.Insert((*it));

                                } else {
                                    air_units_on_ground.Insert((*it));
                                }
                            }
                        }
                    }
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitInfoGroup::RenderGroups() {
    for (int32_t i = torpedos.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &torpedos[i]);
    }

    for (int32_t i = water_platform.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &water_platform[i]);
    }

    for (int32_t i = passeable_ground_cover.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &passeable_ground_cover[i]);
    }

    for (int32_t i = land_rubbles.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &land_rubbles[i]);
    }

    for (int32_t i = sea_land_mines.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &sea_land_mines[i]);
    }

    for (int32_t i = ground_covers.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &ground_covers[i]);
    }

    for (int32_t i = sea_land_units.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &sea_land_units[i]);
    }

    for (int32_t i = elevated_bridge.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &elevated_bridge[i]);
    }

    for (int32_t i = air_units_on_ground.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderAirShadow(this, &air_units_on_ground[i]);
    }

    for (int32_t i = air_units_on_ground.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &air_units_on_ground[i], false);
    }

    for (int32_t i = air_units_in_air.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderAirShadow(this, &air_units_in_air[i]);
    }

    for (int32_t i = rockets.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &rockets[i]);
    }

    for (int32_t i = air_units_in_air.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &air_units_in_air[i], false);
    }

    for (int32_t i = air_particles_explosions.GetCount() - 1; i >= 0; --i) {
        DrawMap_RenderUnit(this, &air_particles_explosions[i]);
    }
}

void UnitInfoGroup::RenderList(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (UnitInfoGroup::IsRelevant(&(*it), this)) {
            DrawMap_RenderUnit(this, &(*it));
        }
    }
}

void UnitInfoGroup::RenderLists() {
    RenderList(&UnitsManager_GroundCoverUnits);
    RenderList(&UnitsManager_MobileLandSeaUnits);
    RenderList(&UnitsManager_StationaryUnits);
    SmartList<UnitInfo>::Iterator it2 = UnitsManager_ParticleUnits.End();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if (IsRelevant(&(*it), this)) {
            DrawMap_RenderAirShadow(this, &(*it));

            if (!((*it).flags & HOVERING)) {
                DrawMap_RenderUnit(this, &(*it), false);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_ParticleUnits.Begin(); it != UnitsManager_ParticleUnits.End();
         ++it) {
        if (((*it).flags & EXPLODING) && (*it).GetUnitType() != RKTSMOKE) {
            it2 = it;
            break;
        }

        if (IsRelevant(&(*it), this)) {
            DrawMap_RenderUnit(this, &(*it), false);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if (((*it).flags & HOVERING) && IsRelevant(&(*it), this)) {
            DrawMap_RenderUnit(this, &(*it), false);
        }
    }

    for (; it2 != UnitsManager_ParticleUnits.End(); ++it2) {
        if (IsRelevant(&(*it2), this)) {
            DrawMap_RenderUnit(this, &(*it2), false);
        }
    }
}

void UnitInfoGroup::ProcessDirtyZone(Rect* bounds) {
    bounds1 = *bounds;

    bounds2.ulx = ((bounds1.ulx << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
    bounds2.uly = ((bounds1.uly << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly;
    bounds2.lrx = ((((bounds1.lrx - 1) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx) + 1;
    bounds2.lry = ((((bounds1.lry - 1) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly) + 1;

    if (Populate()) {
        RenderGroups();

    } else {
        RenderLists();
    }

    torpedos.Release();
    water_platform.Release();
    passeable_ground_cover.Release();
    land_rubbles.Release();
    sea_land_mines.Release();
    ground_covers.Release();
    sea_land_units.Release();
    elevated_bridge.Release();
    air_units_in_air.Release();
    air_units_on_ground.Release();
    rockets.Release();
    air_particles_explosions.Release();
}
