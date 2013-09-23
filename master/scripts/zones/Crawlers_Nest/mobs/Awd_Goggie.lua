-----------------------------------
-- Area: Crawler's Nest
-- NPC:  Awd Goggie
-- @zone 197
-- @pos -253.026, -1.867, 253.055
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
	killer:addTitle(BOGEYDOWNER);
	GetNPCByID(17584458):hideNPC(900);
end;