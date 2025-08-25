max_builder_capability_list = {
    [MAX_UNIT.CONSTRCT] = {
        MAX_UNIT.MININGST, MAX_UNIT.POWERSTN, MAX_UNIT.LIGHTPLT, MAX_UNIT.LANDPLT, MAX_UNIT.AIRPLT,
        MAX_UNIT.SHIPYARD, MAX_UNIT.COMMTWR, MAX_UNIT.DEPOT, MAX_UNIT.HANGAR, MAX_UNIT.DOCK,
        MAX_UNIT.HABITAT, MAX_UNIT.RESEARCH, MAX_UNIT.GREENHSE, MAX_UNIT.TRAINHAL, MAX_UNIT.BARRACKS
    },
    [MAX_UNIT.ENGINEER] = {
        MAX_UNIT.ADUMP, MAX_UNIT.FDUMP, MAX_UNIT.GOLDSM, MAX_UNIT.POWGEN, MAX_UNIT.CNCT_4W,
        MAX_UNIT.RADAR, MAX_UNIT.GUNTURRT, MAX_UNIT.ANTIAIR, MAX_UNIT.ARTYTRRT, MAX_UNIT.ANTIMSSL,
        MAX_UNIT.LANDPAD, MAX_UNIT.BRIDGE, MAX_UNIT.WTRPLTFM, MAX_UNIT.BLOCK, MAX_UNIT.ROAD
    },
    [MAX_UNIT.LIGHTPLT] = {
        MAX_UNIT.SCOUT, MAX_UNIT.SURVEYOR, MAX_UNIT.ENGINEER, MAX_UNIT.REPAIR, MAX_UNIT.SPLYTRCK,
        MAX_UNIT.FUELTRCK, MAX_UNIT.GOLDTRCK, MAX_UNIT.SP_FLAK, MAX_UNIT.MINELAYR, MAX_UNIT.BULLDOZR, MAX_UNIT.CLNTRANS
    },
    [MAX_UNIT.LANDPLT] = {
        MAX_UNIT.CONSTRCT, MAX_UNIT.SCANNER, MAX_UNIT.TANK, MAX_UNIT.ARTILLRY, MAX_UNIT.ROCKTLCH, MAX_UNIT.MISSLLCH
    },
    [MAX_UNIT.AIRPLT] = {
        MAX_UNIT.FIGHTER, MAX_UNIT.BOMBER, MAX_UNIT.AIRTRANS, MAX_UNIT.AWAC
    },
    [MAX_UNIT.SHIPYARD] = {
        MAX_UNIT.FASTBOAT, MAX_UNIT.CORVETTE, MAX_UNIT.BATTLSHP, MAX_UNIT.SUBMARNE,
        MAX_UNIT.SEATRANS, MAX_UNIT.MSSLBOAT, MAX_UNIT.SEAMNLYR, MAX_UNIT.CARGOSHP
    },
    [MAX_UNIT.TRAINHAL] = {
        MAX_UNIT.COMMANDO, MAX_UNIT.INFANTRY
    }
}

function max_get_builder_type(unit_type)
    for builder, buildable_list in pairs(max_builder_capability_list) do
        for _, buildable in ipairs(buildable_list) do
            if buildable == unit_type then
                return builder
            end
        end
    end
    return MAX_UNIT.INVALID_ID
end

function max_get_buildable_units(builder_type)
    local buildable_units = {}
    if max_builder_capability_list[builder_type] then
        return max_builder_capability_list[builder_type]
    end
    return buildable_units
end

function max_is_buildable(unit_type)
    for builder, buildable_list in pairs(max_builder_capability_list) do
        for _, buildable in ipairs(buildable_list) do
            if buildable == unit_type then
                return true
            end
        end
    end
    return false
end
