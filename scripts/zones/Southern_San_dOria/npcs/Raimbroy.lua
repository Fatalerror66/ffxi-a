-----------------------------------
-- Area: Southern San d'Oria
-- NPC:  Raimbroy
-- Starts and Finishes Quest: The Sweetest Things
-- @zone 230
-- @pos 
-------------------------------------
package.loaded["scripts/zones/Southern_San_dOria/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/titles");
require("scripts/globals/shop");
require("scripts/globals/quests");
require("scripts/zones/Southern_San_dOria/TextIDs");

----------------------------------- 
-- onTrade Action 
----------------------------------- 

function onTrade(player,npc,trade)
	-- "Flyers for Regine" conditional script
	FlyerForRegine = player:getQuestStatus(SANDORIA,FLYERS_FOR_REGINE);
	-- "The Sweetest Things" quest status var
	theSweetestThings = player:getQuestStatus(SANDORIA,THE_SWEETEST_THINGS);
   
	if(theSweetestThings ~= QUEST_AVAILABLE) then
		if(trade:hasItemQty(4370,5) and trade:getItemCount() == 5) then
			player:startEvent(0x0217,GIL_RATE*400);
		else
			player:startEvent(0x020a);
		end
	end
   
	if (FlyerForRegine == 1) then
		count = trade:getItemCount();
		MagicFlyer = trade:hasItemQty(532,1);
		if (MagicFlyer == true and count == 1) then
			player:messageSpecial(FLYER_REFUSED);
		end
	end
	
end;

----------------------------------- 
-- onTrigger Action 
-----------------------------------

function onTrigger(player,npc) 
   
	theSweetestThings = player:getQuestStatus(SANDORIA, THE_SWEETEST_THINGS);
   
	-- "The Sweetest Things" Quest Dialogs
	if(player:getFameLevel(SANDORIA) >= 2 and theSweetestThings == QUEST_AVAILABLE) then
		theSweetestThingsVar = player:getVar("theSweetestThings");
		if(theSweetestThingsVar == 1) then
			player:startEvent(0x0215);
		elseif(theSweetestThingsVar == 2) then
			player:startEvent(0x0216);
		else
			player:startEvent(0x0214);
		end
	elseif(theSweetestThings == QUEST_ACCEPTED) then
		player:startEvent(0x0218);
	elseif(theSweetestThings == QUEST_COMPLETED) then
		player:startEvent(0x0219);
	else
		player:showText(npc,AVELINE_SHOP_DIALOG);
		
		stock = {940,500,		-- Revival Root
		 	 3398,5000,		-- Odious Root
			 1472,5000,		-- Gardenia Seed
			 4505,500,		-- Sunflower Seeds
			 2948,1000,		-- Acidic Humus
			 2920,1000,		-- Alkaline Humus
			 2728,1000,		-- Cernunnos Bulb
			 1263,1000,		-- Leshonki Bulb
			 1158,1000,		-- Wandering Bulb
			 953,1000,		-- Treank Bulb
			 3244,500,		-- Giant Mistletoe
			 918,500,		-- Mistletoe
			 2554,500,		-- Asphodel
			 2729,500,		-- Hydrangea
			 2507,500,		-- Lycopodium Flower
			 5907,500}		-- Winterflower
					  
		showShop(player, STATIC, stock);
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

	-- "The Sweetest Things" ACCEPTED
	if(csid == 0x0214) then
		player:setVar("theSweetestThings", 1);
	elseif(csid == 0x0215) then
		if(option == 0) then
			player:addQuest(SANDORIA,THE_SWEETEST_THINGS);
			player:setVar("theSweetestThings", 0);
		else
			player:setVar("theSweetestThings", 2);
		end
	elseif(csid == 0x0216 and option == 0) then
		player:addQuest(SANDORIA, THE_SWEETEST_THINGS);
		player:setVar("theSweetestThings", 0);
	elseif(csid == 0x0217) then
		player:tradeComplete();
		player:addTitle(APIARIST);
		player:addGil(GIL_RATE*400);
		if(player:getQuestStatus(SANDORIA, THE_SWEETEST_THINGS) == QUEST_ACCEPTED) then
			player:addFame(SANDORIA,SAN_FAME*30);
			player:completeQuest(SANDORIA, THE_SWEETEST_THINGS);
		else
			player:addFame(SANDORIA, SAN_FAME*5);
		end
	end
	
end;