
-----------------------------------
--
-- Zone: Windurst_Waters_[S] (94)
--
-----------------------------------

package.loaded["scripts/zones/Windurst_Waters_[S]/TextIDs"] = nil;
require("scripts/globals/settings");
require("scripts/zones/Windurst_Waters_[S]/TextIDs");
require("scripts/globals/quests");
require("scripts/globals/titles");
require("scripts/globals/keyitems");


-----------------------------------
-- onInitialize
-----------------------------------

function onInitialize(zone)
end;

-----------------------------------		
-- onZoneIn		
-----------------------------------		

function onZoneIn(player,prevZone)		
	cs = -1;	
	if(player:getQuestStatus(CRYSTAL_WAR,SNAKE_ON_THE_PLAINS) == QUEST_COMPLETED)then
		if(player:getQuestStatus(CRYSTAL_WAR,THE_TIGRESS_STIRS) == QUEST_AVAILABLE)then
			cs = 0x0082;
		end	
	end
	if ((player:getXPos() == 0) and (player:getYPos() == 0) 	and (player:getZPos() == 0)) then
		player:setPos(-39.996,-7.64,235,64);
	end
	return cs;	
end;		

-----------------------------------		
-- onRegionEnter		
-----------------------------------		

function onRegionEnter(player,region)	
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
	if(csid == 0x0082) then
		player:addKeyItem(959);
		player:messageSpecial(KEYITEM_OBTAINED,INKY_BLACK_YAGUDO_FEATHER);
		player:setVar("tigress",1);
		player:setPos(-39.996,-7.64,235,64);
	end
end;	