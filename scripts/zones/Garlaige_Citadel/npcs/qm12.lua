-----------------------------------
-- Area: Garlaige Citadel
-- NPC:  qm12 (???)
-- used in quest - hitting The Marquisate thf af3
-- 3rd ???
-- @zone 200
-----------------------------------
package.loaded["scripts/zones/Garlaige_Citadel/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/keyitems");
require("scripts/zones/Garlaige_Citadel/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	hittingTheMarquisateHagainCS = player:getVar("hittingTheMarquisateHagainCS");

	if (hittingTheMarquisateHagainCS == 4) then 
		player:messageSpecial(PRESENCE_FROM_CEILING);
		player:setVar("hittingTheMarquisateHagainCS",5);		
	end
	
end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
--printf("CSID2: %u",csid);
--printf("RESULT2: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;
