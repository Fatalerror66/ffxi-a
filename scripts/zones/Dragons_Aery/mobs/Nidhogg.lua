-----------------------------------
-- Area: Dragons Aery
--  HNM: Nidhogg
-----------------------------------

require("scripts/globals/titles");
require("scripts/globals/status");

-----------------------------------
-- onMobInitialize Action
-----------------------------------

function onMobInitialize(mob)
end;

-----------------------------------
-- onMobEngaged Action
-----------------------------------

function onMobEngaged(mob,target)
	Nidhogg_Engaged = os.time(t);
end;

-----------------------------------
-- onMobFight Action
-----------------------------------

function onMobFight(mob,target)

	if (mob:getBattleTime() % 60 == 0) then -- Check every minute to reduce load
		if(os.time(t) >= (Nidhogg_Engaged + 3600)) then
			mob:rageMode(); -- Stats = Stats * 10
		end
	end

end;

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)

    killer:addTitle(NIDHOGG_SLAYER);

    -- Set Nidhogg's Window Open Time
    wait = 8 * 3600
    SetServerVariable("[POP]Nidhogg", os.time(t) + wait); -- 8 hours
    DeterMob(mob:getID(), true);
    
    -- Set Fafnir's spawnpoint and respawn time (1-4 hours)
    Fafnir = 17408018;
    SetServerVariable("[PH]Nidhogg", 0);
    DeterMob(Fafnir, false);
    UpdateNMSpawnPoint(Fafnir);
    GetMobByID(Fafnir):setRespawnTime(math.random((3600),(14400)));

end;