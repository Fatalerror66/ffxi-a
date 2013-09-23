-----------------------------------
-- Area: Davoi
-- NPC:  Disused Well
-- Involved in Quest: A Knight's Test
-- @zone 149
-- @pos -221 2 -293
-----------------------------------
package.loaded["scripts/zones/Davoi/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/keyitems");
require("scripts/globals/teleports");
require("scripts/zones/Davoi/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	if (player:hasKeyItem(KNIGHTS_SOUL) == false and player:hasKeyItem(BOOK_OF_TASKS) and player:hasKeyItem(BOOK_OF_THE_WEST) and player:hasKeyItem(BOOK_OF_THE_EAST)) then
		player:addKeyItem(KNIGHTS_SOUL);
		player:messageSpecial(KEYITEM_OBTAINED, KNIGHTS_SOUL);
		debugTeleport(player, 17719359); -- Balasiel, zone 230, -138 x
	else
		player:messageSpecial(A_WELL);
	end
	
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