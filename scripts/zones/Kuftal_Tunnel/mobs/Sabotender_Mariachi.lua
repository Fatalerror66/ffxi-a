----------------------------------	
-- Area: Kuftal Tunnel	
--   NM: Sabotender Mariachi
-- ToDo: Auto-Regen during the day
-----------------------------------	
  
-----------------------------------	
-- onMobDeath	
-----------------------------------	
	
function onMobDeath(mob,killer)	

    -- Set Sabotender Mariachi's Window Open Time
    wait = math.random((3600),(14400)); -- 1-4 hours
    SetServerVariable("[POP]Sabotender_Mariachi", os.time(t) + wait); -- 1-4 hours
    DeterMob(mob:getID(), true);

    -- Set PH back to normal, then set to respawn spawn
    PH = GetServerVariable("[PH]Sabotender_Mariachi");
    SetServerVariable("[PH]Sabotender_Mariachi", 0);
    DeterMob(PH, false);
    GetMobByID(PH):setRespawnTime(GetMobRespawnTime(PH));
  
end;