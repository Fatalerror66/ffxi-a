-----------------------------------
-- Area: Windurst Woods
-- NPC: Kuzah Hpirohpon
-- Guild Merchant NPC: Clothcrafting Guild 
-- @zone: 241
-- @pos: -80.068 -3.25 -127.686
-----------------------------------
package.loaded["scripts/zones/Windurst_Woods/TextIDs"] = nil;
-----------------------------------
require("scripts/globals/settings");
require("scripts/zones/Windurst_Woods/TextIDs");
require("scripts/globals/festivals");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	onHalloweenTrade(player,trade,npc);
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	if(player:sendGuild(515,6,21,0)) then
		player:showText(npc,KUZAH_HPIROHPON_DIALOG);
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

