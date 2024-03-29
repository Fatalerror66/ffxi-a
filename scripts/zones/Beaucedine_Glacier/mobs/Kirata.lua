-----------------------------------
--  Area: Beaucedine Glacier (111)
--   Mob: Kirata
-----------------------------------

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob,killer)	

    -- Set Kirata's Window Open Time
    wait = math.random((1440),(3600));
    SetServerVariable("[POP]Kirata", os.time(t) + wait); -- 24min-1hr
    DeterMob(mob:getID(), true);
    
    -- Set PH back to normal, then set to respawn spawn
    PH = GetServerVariable("[PH]Kirata");
    SetServerVariable("[PH]Kirata", 0);
    DeterMob(PH, false);
    GetMobByID(PH):setRespawnTime(GetMobRespawnTime(PH));

end;

