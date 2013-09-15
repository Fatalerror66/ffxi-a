-----------------------------------
-- Area: Yuhtunga Jungle
-- NPC:  Cermet Headstone
-- Involved in Mission: ZM5 
-- Headstone Pilgrimage (Fire Frag.)
-- Quests: Wrath of the Opo Opo's
-- @zone 123
-- @pos 491 20 301
-----------------------------------
package.loaded["scripts/zones/Yuhtunga_Jungle/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/quests");
require("scripts/globals/keyitems");
require("scripts/globals/titles");
require("scripts/globals/missions");
require("scripts/zones/Yuhtunga_Jungle/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	local count = trade:getItemCount();
	local Garnet  = trade:hasItemQty(790,1);
	if(player:getQuestStatus(OUTLANDS,WRATH_OF_THE_OPO_OPOS) == QUEST_AVALIABLE and player:getCurrentMission(ZILART) >= HEADSTONE_PILGRIMAGE) then
		if(Garnet == true and count == 1) then
			player:addQuest(OUTLANDS,WRATH_OF_THE_OPO_OPOS);
			player:tradeComplete();
			player:startEvent(0x00CA);
		end
	end
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	if(player:getCurrentMission(ZILART) == HEADSTONE_PILGRIMAGE) then
		-- if requirements are met and 15 mins have passed since mobs were last defeated, spawn them
		if(player:hasKeyItem(FIRE_FRAGMENT) == false and GetServerVariable("[ZM4]Fire_Headstone_Active") < os.time()) then
			player:startEvent(0x00C8,FIRE_FRAGMENT);
		-- if 15 min window is open and requirements are met, recieve key item
		elseif(player:hasKeyItem(FIRE_FRAGMENT) == false and GetServerVariable("[ZM4]Fire_Headstone_Active") > os.time()) then
			player:addKeyItem(FIRE_FRAGMENT);
			-- Check and see if all fragments have been found (no need to check fire and dark frag)
			if(player:hasKeyItem(ICE_FRAGMENT) and player:hasKeyItem(EARTH_FRAGMENT) and player:hasKeyItem(WATER_FRAGMENT) and 
			   player:hasKeyItem(WIND_FRAGMENT) and player:hasKeyItem(LIGHTNING_FRAGMENT) and player:hasKeyItem(LIGHT_FRAGMENT)) then
				player:messageSpecial(FOUND_ALL_FRAGS,FIRE_FRAGMENT);
				player:addTitle(BEARER_OF_THE_EIGHT_PRAYERS);
				player:completeMission(ZILART,HEADSTONE_PILGRIMAGE);
				player:addMission(ZILART,THROUGH_THE_QUICKSAND_CAVES);
			else
				player:messageSpecial(KEYITEM_OBTAINED,FIRE_FRAGMENT);
			end
		else
			player:messageSpecial(ALREADY_OBTAINED_FRAG,FIRE_FRAGMENT);
		end
	elseif(player:hasCompletedMission(ZILART,HEADSTONE_PILGRIMAGE)) then
		player:messageSpecial(ZILART_MONUMENT);
	else
		player:messageSpecial(CANNOT_REMOVE_FRAG);
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
	
	if(csid == 0x00CA) then
		player:addItem(13143);
		player:messageSpecial(ITEM_OBTAINED,13143);
		player:completeQuest(OUTLANDS,WRATH_OF_THE_OPO_OPOS);
		player:addFame(OUTLANDS,30);
		player:addTitle(282);
	elseif(csid == 0x00C8 and option == 1) then
		SpawnMob(17281031,300):updateEnmity(player); -- Carthi
		SpawnMob(17281030,300):updateEnmity(player); -- Tipha
		SetServerVariable("[ZM4]Fire_Headstone_Active",0);
	end
	
end;