local team = ...
local team_status

if
    max_has_attack_power(team, MAX_UNIT_CLASS.MOBILE_LAND_SEA) or
    max_has_attack_power(team, MAX_UNIT_CLASS.MOBILE_AIR) or
    max_has_attack_power(team, MAX_UNIT_CLASS.STATIONARY) or
    max_can_rebuild_complex(team) or
    max_can_rebuild_builders(team)
then
    team_status = MAX_VICTORY_STATE.PENDING
else
    team_status = MAX_VICTORY_STATE.LOST
end

if team == MAX_TEAM.RED
then
    if team_status ~= MAX_VICTORY_STATE.LOST
    then
        if max_count_ready_units(MAX_TEAM.GREEN, MAX_UNIT.RESEARCH) == 0
        then
            team_status = MAX_VICTORY_STATE.LOST
        else
            if max_count_ready_units(MAX_TEAM.GREEN, MAX_UNIT.GREENHSE) > 0
            then
                team_status = MAX_VICTORY_STATE.PENDING
            else
                team_status = MAX_VICTORY_STATE.WON
            end
        end
    end
else
    if team_status ~= MAX_VICTORY_STATE.LOST
    then
       team_status = MAX_VICTORY_STATE.GENERIC
    end
end

return team_status
