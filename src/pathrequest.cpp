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

#include "pathrequest.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "remote.hpp"
#include "sound_manager.hpp"
#include "units_manager.hpp"

const char* PathRequest_CautionLevels[] = {"none", "avoid reaction fire", "avoid next turn's fire", "avoid all damage"};

PathRequest::PathRequest(UnitInfo* unit, int32_t mode, Point point) : client(unit), point(point), flags(mode) {
    AiLog log("Path request for %s at [%i,%i].", UnitsManager_BaseUnits[client->GetUnitType()].singular_name,
              client->grid_x + 1, client->grid_y + 1);

    max_cost = INT16_MAX;
    minimum_distance = 0;
    caution_level = 0;
    board_transport = 0;
    optimize = 1;
}

PathRequest::~PathRequest() {}

UnitInfo* PathRequest::GetClient() const { return &*client; }

UnitInfo* PathRequest::GetTransporter() const { return &*transporter; }

Point PathRequest::GetDestination() const { return point; }

uint8_t PathRequest::GetCautionLevel() const { return caution_level; }

bool PathRequest::PathRequest_Vfunc1() { return false; }

void PathRequest::Cancel() {
    if (client->hits &&
        (client->orders == ORDER_MOVE || client->orders == ORDER_MOVE_TO_UNIT ||
         client->orders == ORDER_MOVE_TO_ATTACK) &&
        client->state == ORDER_STATE_NEW_ORDER) {
        UnitsManager_SetNewOrder(&*client, client->orders, ORDER_STATE_12);
    }

    Access_ProcessNewGroupOrder(&*client);
}

void PathRequest::SetMaxCost(int32_t value) {
    AiLog log("Max cost: %i.", value);

    max_cost = value;
}

void PathRequest::SetMinimumDistance(int32_t value) {
    AiLog log("Minimum distance: %i.", value);

    minimum_distance = value;
}

void PathRequest::SetCautionLevel(int32_t value) {
    AiLog log("Caution level: %s.", PathRequest_CautionLevels[value]);

    caution_level = value;
}

void PathRequest::SetBoardTransport(bool value) {
    AiLog log("Board transport: %s.", value ? "True" : "False");

    board_transport = value;
}

void PathRequest::SetOptimizeFlag(bool value) {
    AiLog log("Optimize: %s", value ? "True" : "False");

    optimize = value;
}

void PathRequest::CreateTransport(ResourceID unit_type) {
    if (unit_type != INVALID_ID) {
        AiLog log("Use %s.", UnitsManager_BaseUnits[unit_type].singular_name);

        transporter = new (std::nothrow) UnitInfo(unit_type, client->team, 0xFFFF);

    } else {
        transporter = nullptr;
    }
}

uint8_t PathRequest::GetFlags() const { return flags; }

uint16_t PathRequest::GetMaxCost() const { return max_cost; }

uint8_t PathRequest::GetBoardTransport() const { return board_transport; }

uint16_t PathRequest::GetMinimumDistance() const { return minimum_distance; }

void PathRequest::AssignGroundPath(UnitInfo* unit, GroundPath* path) {
    Point destination(unit->target_grid_x, unit->target_grid_y);
    SmartObjectArray<PathStep> steps = path->GetSteps();
    SmartPointer<GroundPath> ground_path(new (std::nothrow) GroundPath(destination.x, destination.y));
    int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
    Point position(unit->grid_x, unit->grid_y);
    int32_t index;

    range = range * range;

    for (index = 0; index < steps.GetCount() && Access_GetDistance(position, destination) > range; ++index) {
        position.x += steps[index]->x;
        position.y += steps[index]->y;

        ground_path->AddStep(steps[index]->x, steps[index]->y);
    }

    if (index > 0) {
        unit->path = &*ground_path;

        if (unit->flags & MOBILE_AIR_UNIT) {
            unit->path->SetEndXY(position.x, position.y);
            unit->path = Paths_GetAirPath(unit);
        }

    } else {
        unit->path = nullptr;
    }
}

void PathRequest::Finish(GroundPath* path) {
    if (client->hits > 0 &&
        (client->orders == ORDER_MOVE || client->orders == ORDER_MOVE_TO_UNIT || client->orders == ORDER_BUILD ||
         client->orders == ORDER_MOVE_TO_ATTACK) &&
        (client->state == ORDER_STATE_NEW_ORDER || client->state == ORDER_STATE_29)) {
        uint8_t unit1_order_state = client->state;

        client->path = path;

        if (client->orders == ORDER_MOVE_TO_ATTACK && path != nullptr) {
            AssignGroundPath(&*client, path);
        }

        if (client->path == nullptr) {
            if (client->orders == ORDER_MOVE_TO_ATTACK) {
                int32_t range = client->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

                range = range * range;

                if (Access_GetDistance(&*client, Point(client->target_grid_x, client->target_grid_y)) <= range) {
                    UnitsManager_SetNewOrder(&*client, ORDER_MOVE_TO_ATTACK, ORDER_STATE_1);
                    client->RefreshScreen();
                    Access_ProcessNewGroupOrder(&*client);

                    return;
                }
            }

            client->BlockedOnPathRequest(false);

            if (client->GetUnitList() && unit1_order_state == ORDER_STATE_NEW_ORDER) {
                client->ClearUnitList();
            }

            if (GameManager_SelectedUnit == client) {
                SoundManager_PlayVoice(V_M094, V_F095);
            }

        } else {
            if (client->orders == ORDER_BUILD) {
                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(&*client);
                }

                UnitsManager_StartBuild(&*client);

            } else if (client->state == ORDER_STATE_29) {
                if (client->orders == ORDER_MOVE_TO_ATTACK) {
                    client->state = ORDER_STATE_1;

                } else {
                    client->RestoreOrders();
                }

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(&*client);
                }

                GameManager_UpdateDrawBounds();

            } else if (client->GetUnitList()) {
                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(&*client);
                }

                Access_ProcessNewGroupOrder(&*client);

            } else {
                client->Redraw();
                client->state = ORDER_STATE_IN_PROGRESS;

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(&*client);
                }
            }
        }
    }
}
