-----------------------------------
-- Area: Southern San d'Oria
-- NPC: Synergy Furnace
-- Type: Synergy NPC
-- @zone 230
-- @pos many
-----------------------------------
package.loaded["scripts/zones/Southern_San_dOria/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/Southern_San_dOria/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	player:startEvent(0x1194);
end; 

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;