------------------------------------- Area: Jugner Forest [S]--  NM: Cobraclaw Buchzvotch-- @pos -6.100 0.237 -294.975-----------------------------------require("scripts/globals/titles");require("scripts/globals/status");------------------------------------- onMobEngaged Action-----------------------------------function onMobEngaged(mob,target)end;------------------------------------- onMobFight Action-----------------------------------function onMobFight(mob,target)end;------------------------------------- onMobDeath-----------------------------------function onMobDeath(mob, killer)	killer:setVar("WrathOfGriffonProg",3); despawnMob(17113464);end;