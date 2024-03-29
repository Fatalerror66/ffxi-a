-----------------------------------
-- Area: Apollyon CS
-- NPC:  Na Qba Chirurgeon
-----------------------------------
package.loaded["scripts/zones/Apollyon/TextIDs"] = nil;

-----------------------------------
require("scripts/globals/limbus");
require("scripts/zones/Apollyon/TextIDs");

-----------------------------------
-- onMobSpawn Action
-----------------------------------

function onMobSpawn(mob)
	mob:setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
end;

-----------------------------------
-- onMobEngaged
-----------------------------------

function onMobEngaged(mob,target)
	local mobID = mob:getID();
	local X = mob:getXPos();
	local Y = mob:getYPos();
	local Z = mob:getZPos();
								SpawnMob(16933139):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
								SpawnMob(16933140):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
								SpawnMob(16933138):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
end;
-----------------------------------
-- onMobFight Action
-----------------------------------

function onMobFight(mob,target)
	local mobID = mob:getID();
	local X = mob:getXPos();
	local Y = mob:getYPos();
	local Z = mob:getZPos();
local lifepourcent= ((mob:getHP()/mob:getMaxHP())*100); 
local instancetime = target:getSpecialInstanceLeftTime(5);

    if(lifepourcent < 50 and GetNPCByID(16933246):getAnimation() == 8)then
								SpawnMob(16933142):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
								SpawnMob(16933143):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
								SpawnMob(16933141):setMobMod(MOBMOD_SUPERLINK, mob:getShortID());
								GetNPCByID(16933246):setAnimation(9);
	end
	
	if(instancetime < 13)then

	   if(IsMobDead(16933129)==false)then  
	       GetMobByID(16933129):updateEnmity(target);
	   elseif(IsMobDead(16933144)==false)then  
	       GetMobByID(16933144):updateEnmity(target);
	   end
	
	end
end;
-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob,killer)
    if((IsMobDead(16933129)==false or IsMobDead(16933144)==false) and alreadyReceived(killer,2,GetInstanceRegion(1294)) == false)then		  
		     killer:addTimeToSpecialInstance(5,5);
	         addLimbusList(killer,2,GetInstanceRegion(1294));
	end
end;
