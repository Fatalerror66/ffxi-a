-----------------------------------
--	Area: Buburimu Peninsula
--	NPC:  Song Runes
--  Finishes Quest: The Old Monument
-----------------------------------
package.loaded["scripts/zones/Buburimu_Peninsula/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/titles");
require("scripts/globals/quests");
require("scripts/globals/teleports");
require("scripts/zones/Buburimu_Peninsula/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
			
	if (player:getVar("TheOldMonument_Event") == 3) then
		if (trade:hasItemQty(917,1) == true and trade:getItemCount() == 1 and trade:getGil() == 0) then
			player:tradeComplete();
			player:completeQuest(JEUNO,THE_OLD_MONUMENT);
			player:addItem(634,1);
			player:messageSpecial(ITEM_OBTAINED, 634);
			player:addTitle(RESEARCHER_OF_CLASSICS);
			player:addFame(BASTOK,BAS_FAME*10);
			player:addFame(SANDORIA,SAN_FAME*10);
			player:addFame(WINDURST,WIN_FAME*10);
			player:setVar("TheOldMonument_Event",0);
		end
	end		
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	
	if (player:getVar("TheOldMonument_Event") == 2) then
		player:startEvent(0x0000);
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
	
	if (csid == 0x0000) then
		player:setVar("TheOldMonument_Event",3);
	end
end;

