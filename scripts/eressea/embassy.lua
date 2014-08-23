-- Muschelplateau

local embassy = {}
local home = nil

-- global exports (use item)
function use_seashell(u, amount)
-- Muschelplateau...
    local visit = u.faction.objects:get("embassy_muschel")
    if visit and u.region~= home then
        local turns = get_turn() - visit
        local msg = message.create('msg_event')
        msg:set_string("string", u.name .. "(" .. itoa36(u.id) .. ") erzählt den Bewohnern von " .. u.region.name .. " von Muschelplateau, das die Partei " .. u.faction.name .. " vor " .. turns .. " Wochen besucht hat." )
        msg:send_region(u.region)
        return 0
    end
    return -4
end

function embassy.init()
    home = get_region(165,30)
    if home==nil then
        eressea.log.error("cannot find embassy region 'Muschelplateau'")
    end
end

function embassy.update()
-- Muschelplateau
    eressea.log.debug("updating embassies in " .. tostring(home))
    local u
    for u in home.units do
        if u.faction.objects:get('embassy_muschel')==nil then
            if (u.faction:add_item('seashell', 1)>0) then
                eressea.log.debug("new seashell for " .. tostring(u.faction))
                u.faction.objects:set('embassy_muschel', get_turn())
            end
        end
    end
end

return embassy
