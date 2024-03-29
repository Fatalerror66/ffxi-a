-----------------------------------
-- Area: Ifrit's Cauldron
-- NPC:  ??? claustrum final stage
-----------------------------------
package.loaded["scripts/zones/Ifrits_Cauldron/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/zones/Ifrits_Cauldron/TextIDs");
-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	
	if(player:getVar("relic") == 1) then
	    if(trade:hasItemQty(18329,1) and trade:hasItemQty(1584,1) and trade:hasItemQty(1589,1) and trade:hasItemQty(1451,1) and trade:getItemCount() == 4) then
			player:startEvent(0x0020,18330);
		end
	end
	
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)	
player:messageSpecial(NOTHING_OUT_OF_ORDINARY);
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

    if(csid == 0x0020)then
	    if(player:getFreeSlotsCount() == 1) then
		   player:messageSpecial(ITEM_CANNOT_BE_OBTAINED,18330);
		   player:messageSpecial(ITEM_CANNOT_BE_OBTAINEDX,1450,30);
	    else
		   player:tradeComplete();
		   player:setVar("relic",0);
	       player:addItem(18330);
	       player:addItem(1450,30);
		   player:messageSpecial(ITEM_OBTAINED,18330);
	       player:messageSpecial(ITEM_OBTAINEDX,1450,30);    
		end
	end
end;