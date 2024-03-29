-----------------------------------
--  Area: Ru'Lude Gardens
--  NPC:  Nomad Moogle
--  Type: Adventurer's Assistant
--  @pos 10.012 1.453 121.883 243
-----------------------------------
package.loaded["scripts/zones/RuLude_Gardens/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/keyitems");
require("scripts/globals/quests");
require("scripts/zones/RuLude_Gardens/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	if(player:hasKeyItem(LIMIT_BREAKER) == false and player:getMainLvl() >= 75) then
		player:startEvent(0x2797);
	else
		player:startEvent(0x005E);
	end

end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);
	
	if(csid == 0x2797) then
		player:addKeyItem(LIMIT_BREAKER);
		player:messageSpecial(KEYITEM_OBTAINED,LIMIT_BREAKER);
	end
	
end;