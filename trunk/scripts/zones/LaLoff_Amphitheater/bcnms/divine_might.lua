-----------------------------------
-- Area: LaLoff Amphitheater
-- Name: Divine Might
-- 
-----------------------------------
package.loaded["scripts/zones/LaLoff_Amphitheater/TextIDs"] = nil;
-------------------------------------
require("scripts/zones/LaLoff_Amphitheater/TextIDs");
require("scripts/globals/missions");
require("scripts/globals/quests");
require("scripts/globals/keyitems");

-- Death cutscenes:

-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,0,0); -- Hume
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,1,0); -- taru
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,2,0); -- mithra
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,3,0); -- elvan
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,4,0); -- galka
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,5,0); -- divine might
-- player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,6,0); -- skip ending cs


-- After registering the BCNM via bcnmRegister(bcnmid)
function OnBcnmRegister(player,instance)
end;

-- Physically entering the BCNM via bcnmEnter(bcnmid)
function OnBcnmEnter(player,instance)
	player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,1,1);
end;

-- Leaving the BCNM by every mean possible, given by the LeaveCode
-- 1=Select Exit on circle
-- 2=Winning the BC
-- 3=Disconnected or warped out
-- 4=Losing the BC
-- via bcnmLeave(1) or bcnmLeave(2). LeaveCodes 3 and 4 are called
-- from the core when a player disconnects or the time limit is up, etc

function OnBcnmLeave(player,instance,leavecode)
print("leave code "..leavecode);

	if(leavecode == 2) then -- play end CS. Need time and battle id for record keeping + storage
		if(player:hasCompletedMission(ZILART,ARK_ANGELS)) then
			player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,5,1);		-- winning CS (allow player to skip)
		else
			player:startEvent(0x7d01,1,1,1,instance:getTimeInside(),1,5,0);		-- winning CS
		end

	elseif(leavecode == 4) then
		player:startEvent(0x7d02);	-- player lost
	end
end;

function onEventUpdate(player,csid,option)
-- print("bc update csid "..csid.." and option "..option);

end;

function onEventFinish(player,csid,option)
-- print("bc finish csid "..csid.." and option "..option);

   if(csid == 0x7d01) then
      if (player:getQuestStatus(OUTLANDS,DIVINE_MIGHT) == QUEST_ACCEPTED) then
         player:setVar("DivineMight",2); -- Used to use 2 to track completion, so that's preserved to maintain compatibility
         for i=SHARD_OF_APATHY, SHARD_OF_RAGE do
            player:addKeyItem(i);
            player:messageSpecial(KEYITEM_OBTAINED,i);
         end
         if (player:getCurrentMission(ZILART) == ARK_ANGELS) then
            player:completeMission(ZILART,ARK_ANGELS);
            player:addMission(ZILART,THE_SEALED_SHRINE);
            player:setVar("ZilartStatus",0);
         end
      elseif (player:getQuestStatus(OUTLANDS,DIVINE_MIGHT_REPEAT) == QUEST_ACCEPTED and player:hasKeyItem(MOONLIGHT_ORE) == true) then
         player:setVar("DivineMight",2);
      end
   end
end;