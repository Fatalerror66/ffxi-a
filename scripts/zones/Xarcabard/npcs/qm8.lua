-----------------------------------
-- Area: Xarcabard
-- NPC:  ???
-- Involved in Quests: The Three Magi
-- Involoved in Mission: Breaking
-- Barriers (Sandoria 9-1)
-- @zone 112
-- @pos 243, -27, 101
-----------------------------------
package.loaded["scripts/zones/Xarcabard/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/Xarcabard/TextIDs");
require("scripts/globals/quests");
require("scripts/globals/missions");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	if(player:getCurrentMission(SANDORIA) == BREAKING_BARRIERS and player:getVar("MissionStatus") == 2) then
		player:addKeyItem(FIGURE_OF_GARUDA);
		player:messageSpecial(KEYITEM_OBTAINED,FIGURE_OF_GARUDA);
		player:setVar("MissionStatus",3);
	else
		player:messageSpecial(NOTHING_OUT_OF_ORDINARY);
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