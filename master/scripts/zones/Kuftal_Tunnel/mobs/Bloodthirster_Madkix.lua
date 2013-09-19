----------------------------------	
-- Area: Kuftal Tunnel	
--   NM: Bloodthirster Madkix
-----------------------------------	
  
-----------------------------------	
-- onMobDeath	
-----------------------------------	
	
function onMobDeath(mob,killer)	
  
    -- Set Bloodthirster Madkix's Window Open Time
    wait = math.random((3600),(14400)); -- 1-4 hours
    SetServerVariable("[POP]Bloodthirster_Madkix", os.time(t) + wait);
    DeterMob(mob:getID(), true);

    -- Set PH back to normal, then set to respawn spawn
    PH = GetServerVariable("[PH]Bloodthirster_Madkix");
    SetServerVariable("[PH]Bloodthirster_Madkix", 0);
    DeterMob(PH, false);
    GetMobByID(PH):setRespawnTime(GetMobRespawnTime(PH));
  
end;