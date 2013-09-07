-------------------------------------------------
--	Author: Ezekyel
--	Excavation functions
--  Info from: 
--      http://wiki.ffxiclopedia.org/wiki/Excavation
-------------------------------------------------

require("scripts/globals/keyitems");
require("scripts/globals/quests");
require("scripts/globals/settings");

-------------------------------------------------
-- npcid and drop by zone
-------------------------------------------------

-- Zone, {npcid,npcid,npcid,..}
npcid = {7,{16806335,16806336,16806337,16806338,16806339,16806340},
		 68,{17056226,17056227,17056228},
		 117,{17257044,17257045,17257046,17257047,17257048,17257049},
		 173,{17486246,17486247,17486248,17486249,17486250,17486251},
		 198,{17588767,17588768,17588769,17588770,17588771,17588772}};
-- Zone, {itemid,drop rate,itemid,drop rate,..}
drop = {7,{0x0370,0.2220,0x0382,0.4440,0x037B,0.5660,0x43F4,0.6880,0x0381,0.7600,0x0380,0.8320,0x09C7,0.8740,0x04D4,0.9160,0x05C1,0.9580,0x0301,1.0000},
		68,{},
		117,{0x0370,0.2820,0x0382,0.4840,0x037B,0.6160,0x037D,0.7660,0x43F4,0.8870,0x0381,0.9270,0x0380,0.9550,0x0375,0.9830,0x0760,1.0000},
		173,{0x03A8,0.1820,0x0378,0.3170,0x0371,0.3570,0x0360,0.5390,0x43F3,0.7210,0x43F5,0.8560,0x0377,0.8680,0x023D,0.8800,0x0375,0.9200,0x023F,0.9600,0x07C1,1.0000},
		198,{0x0370,0.3060,0x037B,0.4870,0x43F4,0.6680,0x037D,0.7870,0x0381,0.8430,0x0380,0.8990,0x02BF,0.9220,0x0301,0.9440,0x0760,1.0000}};
-- Define array of Colored Rocks, Do not reorder this array or rocks.
rocks = {0x0301,0x0302,0x0303,0x0304,0x0305,0x0306,0x0308,0x0307};

function startExcavation(player,zone,npc,trade,csid)
	
	if(trade:hasItemQty(605,1) and trade:getItemCount() == 1) then
		
		broke = pickaxeBreak(player,trade);
		item = getItem(player,zone);
		
		if(player:getFreeSlotsCount() == 0) then
			full = 1;
		else
			full = 0;
		end
		
		player:startEvent(csid,item,broke,full);
		
		if(item ~= 0 and full == 0) then
			player:addItem(item);
			SetServerVariable("[EXCAVATION]Zone "..zone,GetServerVariable("[EXCAVATION]Zone "..zone) + 1);
		end
		
		if(GetServerVariable("[EXCAVATION]Zone "..zone) >= 3) then
			getNewPositionNPC(player,npc,zone);
		end
	
		if(broke ~= 1 and player:getQuestStatus(AHT_URHGAN,OLDUUM) == QUEST_ACCEPTED and player:hasKeyItem(ELECTROCELL) == false and player:hasKeyItem(ELECTROPOT) == false and player:hasKeyItem(ELECTROLOCOMOTIVE) == false and zone == 68) then
			local randPick = math.random(0,2);
			
			if randPick == 1 then
				player:addKeyItem(ELECTROCELL);
				player:messageSpecial(KEYITEM_OBTAINED,ELECTROCELL);
			elseif randPick == 2 then
				player:addKeyItem(ELECTROPOT);
				player:messageSpecial(KEYITEM_OBTAINED,ELECTROPOT);
			else
				player:addKeyItem(ELECTROLOCOMOTIVE);
				player:messageSpecial(KEYITEM_OBTAINED,ELECTROLOCOMOTIVE);

			end
		end
	else
		player:messageSpecial(MINING_IS_POSSIBLE_HERE,605);
	end
	
end

-----------------------------------
-- Determine if Pickaxe breaks
-----------------------------------

function pickaxeBreak(player,trade)
	

	
	local broke = 0;
	pickaxebreak = math.random();
	
	if(pickaxebreak < EXCAVATION_BREAK_CHANCE) then
		broke = 1;
		player:tradeComplete();
	end
	
	return broke;
	
end

-----------------------------------
-- Get an item
-----------------------------------

function getItem(player,zone)	
	
	Rate = math.random();
	
	for zon = 1, table.getn(drop), 2 do
		if(drop[zon] == zone) then
			for itemlist = 1, table.getn(drop[zon + 1]), 2 do
				if(Rate <= drop[zon + 1][itemlist + 1]) then
					item = drop[zon + 1][itemlist];		
					break;			
				end
			end
			break;
		end
	end
	
	--------------------
	-- Determine result if Colored Rock
	--------------------
	
	if (item == 769) then
		day = VanadielDayElement();
		item = rocks[day + 1];
	end
	
	--------------------
	-- Determine chance of no item mined
	-- Default rate is 50%
	--------------------
	
	Rate = math.random();
	
	if(Rate <= (1 - EXCAVATION_RATE)) then
		item = 0;
	end
	
	return item;
	
end

-----------------------------------------
-- After 3 items he change the position
-----------------------------------------

function getNewPositionNPC(player,npc,zone)
	
	local newnpcid = npc:getID();
	
	for u = 1, table.getn(npcid), 2 do
		if(npcid[u] == zone) then
			nbNPC = table.getn(npcid[u + 1]);
			while newnpcid == npc:getID() do
				newnpcid = math.random(1,nbNPC);
				newnpcid = npcid[u + 1][newnpcid];
			end
			break;
		end
	end
	
	npc:setStatus(2);
	GetNPCByID(newnpcid):setStatus(0);
	SetServerVariable("[EXCAVATION]Zone "..zone,0);
	
end