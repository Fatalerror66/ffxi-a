-----------------------------------
-- Area: 
-- NPC:  Steelfleece_Baldarich
-----------------------------------

require("scripts/globals/titles");

-----------------------------------
-- onMobSpawn Action
-----------------------------------

function OnMobSpawn(mob)
end;

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)
	killer:addTitle(THE_HORNSPLITTER);
end;