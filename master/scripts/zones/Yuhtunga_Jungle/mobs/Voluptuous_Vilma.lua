-----------------------------------
-- Area: Yuhtunga Jungle
-- MOB:  Voluptuous Vilma
-----------------------------------
-----------------------------------

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)	
	
	local Rose_Garden = 17281357;
	local Rose_Garden_PH = GetServerVariable("Rose_Garden_PH");
	local Voluptuous_Vilma = 17281358;
		
	GetMobByID(Rose_Garden):setExtraVar(os.time() + math.random((3600),(14400)));
	SetServerVariable("Voluptuous_Vilma_PH", 0);
	DeterMob(Voluptuous_Vilma, true);
	DeterMob(Rose_Garden_PH, false);
	SpawnMob(Rose_Garden_PH, "", GetMobRespawnTime(Rose_Garden_PH));
	
end;