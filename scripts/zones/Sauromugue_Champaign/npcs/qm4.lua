-----------------------------------
--  Area: Sauromugue Champaign
--   NPC: qm4 (Tower 4) 
--  Type: Standard NPC
--
-- THF Quest "As Thick As Thieves"
-- @zone: 120
-- @where x122, y0, z-230
-----------------------------------
package.loaded["scripts/zones/Sauromugue_Champaign/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/quests");
require("scripts/globals/keyitems");
require("scripts/zones/Sauromugue_Champaign/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	thickAsThieves = player:getQuestStatus(WINDURST,AS_THICK_AS_THIEVES);
	thickAsThievesGrapplingCS = player:getVar("thickAsThievesGrapplingCS");
	
	if(thickAsThievesGrapplingCS == 4) then
		SpawnMob(17269107,120):updateEnmity(player); -- Climbpix Highrise
		setMobPos(17269107,122,0,230,0);		
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
end;