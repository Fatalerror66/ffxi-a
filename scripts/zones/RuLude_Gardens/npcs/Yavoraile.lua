-----------------------------------
--	Area: Ru'Lud Gardens
--	NPC:  Yavoraile
--	Standard Info NPC
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/quests");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	local WildcatJeuno = player:getVar("WildcatJeuno");
	if (player:getQuestStatus(JEUNO,LURE_OF_THE_WILDCAT_JEUNO) == QUEST_ACCEPTED and player:getMaskBit(WildcatJeuno,4) == false) then
		player:startEvent(10092);
	else
		player:startEvent(0x0076);
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
	if (csid == 10092) then
		player:setMaskBit(player:getVar("WildcatJeuno"),"WildcatJeuno",4,true)
	end
end;
