-----------------------------------
-- Area: Lufaise Meadows
-- MOB:  Defoliate Leshy
-----------------------------------
-----------------------------------

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)	
	
	local Colorful_Leshy = 16875762;
	local Colorful_Leshy_PH = GetServerVariable("Colorful_Leshy_PH");
	local Defoliate_Leshy = 16875763;
		
	GetMobByID(Colorful_Leshy):setExtraVar(os.time() + math.random((3600),(14400)));
	SetServerVariable("Defoliate_Leshy_PH", 0);
	DeterMob(Defoliate_Leshy, true);
	DeterMob(Colorful_Leshy_PH, false);
	SpawnMob(Colorful_Leshy_PH, "", GetMobRespawnTime(Colorful_Leshy_PH));
	
end;