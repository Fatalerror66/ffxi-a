-----------------------------------	
-- Area: East Sarutabaruta	
-- MOB:  Spiny Spipi	
-----------------------------------	
	
-----------------------------------	
-- onMobDeath	
-----------------------------------	
	
function onMobDeath(mob,killer)	

    -- Set Spiny_Spipi's Window Open Time
    wait = math.random((1440),(3600))
    SetServerVariable("[POP]Spiny_Spipi", os.time(t) + wait); -- 24min-1hr
    DeterMob(mob:getID(), true);
    
    -- Set PH back to normal, then set to respawn spawn
    PH = GetServerVariable("[PH]Spiny_Spipi");
    SetServerVariable("[PH]Spiny_Spipi", 0);
    DeterMob(PH, false);
    GetMobByID(PH):setRespawnTime(GetMobRespawnTime(PH));
    
end;	
