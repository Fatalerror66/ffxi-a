-----------------------------------
-- Area: Dynamis Tavnazia
-- NPC:  Diabolos Diamond
-----------------------------------
-----------------------------------

require("scripts/globals/titles");
require("scripts/globals/keyitems");
-----------------------------------
-- onMobSpawn Action
-----------------------------------

function OnMobSpawn(mob)
end;

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)
     	if(killer:hasKeyItem(DYNAMIS_TAVNAZIA_SLIVER ) == false)then
	    	killer:addKeyItem(DYNAMIS_TAVNAZIA_SLIVER);
		    killer:messageSpecial(KEYITEM_OBTAINED,DYNAMIS_TAVNAZIA_SLIVER);
		end
	killer:addTitle(NIGHTMARE_AWAKENER);
end;