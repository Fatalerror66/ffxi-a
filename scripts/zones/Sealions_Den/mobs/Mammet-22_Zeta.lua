-----------------------------------
-- Area: 
--  mob Mammet-22_Zeta
-----------------------------------


require("scripts/globals/status");

-----------------------------------
-- onMobInitialize Action
-----------------------------------

function onMobInitialize(mob)	
end;

-----------------------------------
-- onMobEngaged Action
-----------------------------------

function onMobEngaged(mob,target)
end;

-----------------------------------
-- onMobFight Action
-----------------------------------

function onMobFight(mob,target)
end;

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)
 local MammetID=mob:getID();
   -- printf("MammetID dead: %u",MammetID);
	
local mammet_1_1=GetMobAction(16908289);
local mammet_1_2=GetMobAction(16908290);
local mammet_1_3=GetMobAction(16908291);
local mammet_1_4=GetMobAction(16908292);
local mammet_1_5=GetMobAction(16908293);

local mammet_2_1=GetMobAction(16908296);
local mammet_2_2=GetMobAction(16908297);
local mammet_2_3=GetMobAction(16908298);
local mammet_2_4=GetMobAction(16908299);
local mammet_2_5=GetMobAction(16908300);

local mammet_3_1=GetMobAction(16908303);
local mammet_3_2=GetMobAction(16908304);
local mammet_3_3=GetMobAction(16908305);
local mammet_3_4=GetMobAction(16908306);
local mammet_3_5=GetMobAction(16908307);



	if(MammetID==16908289 or MammetID==16908290 or MammetID==16908291 or MammetID==16908292 or MammetID==16908293)then
	   if((mammet_1_1 == 0 or mammet_1_1 == 21 or mammet_1_1 == 22 or mammet_1_1 == 23)and
	      (mammet_1_2 == 0 or mammet_1_2 == 21 or mammet_1_2 == 22 or mammet_1_2 == 23)and
		  (mammet_1_3 == 0 or mammet_1_3 == 21 or mammet_1_3 == 22 or mammet_1_3 == 23)and
		  (mammet_1_4 == 0 or mammet_1_4 == 21 or mammet_1_4 == 22 or mammet_1_4 == 23)and
		  (mammet_1_5 == 0 or mammet_1_5 == 21 or mammet_1_5 == 22 or mammet_1_5 == 23))then		  
		 killer:setPos(-779, -103, -80);
		 killer:startEvent(0x000b);
	     SpawnMob(16908294); --omega1
		 	killer:addTitle(BRANDED_BY_LIGHTNING);
	   end

	elseif(MammetID==16908297 or MammetID==16908298 or MammetID==16908299 or MammetID==16908300 or MammetID==16908296)then
		   if((mammet_2_1 == 0 or mammet_2_1 == 21 or mammet_2_1 == 22 or mammet_2_1 == 23)and
	      (mammet_2_2 == 0 or mammet_2_2 == 21 or mammet_2_2 == 22 or mammet_2_2 == 23)and
		  (mammet_2_3 == 0 or mammet_2_3 == 21 or mammet_2_3 == 22 or mammet_2_3 == 23)and
		  (mammet_2_4 == 0 or mammet_2_4 == 21 or mammet_2_4 == 22 or mammet_2_4 == 23)and
		  (mammet_2_5 == 0 or mammet_2_5 == 21 or mammet_2_5 == 22 or mammet_2_5 == 23))then
		  killer:setPos(-140, -23, -440);
		killer:startEvent(0x000b);
	     SpawnMob(16908301); --omega2
		 	killer:addTitle(BRANDED_BY_LIGHTNING);
         end
	elseif(MammetID==16908303 or MammetID==16908304 or MammetID==16908305 or MammetID==16908306 or MammetID==16908307)then
		  if((mammet_3_1 == 0 or mammet_3_1 == 21 or mammet_3_1 == 22 or mammet_3_1 == 23)and
	      (mammet_3_2 == 0 or mammet_3_2 == 21 or mammet_3_2 == 22 or mammet_3_2 == 23)and
		  (mammet_3_3 == 0 or mammet_3_3 == 21 or mammet_3_3 == 22 or mammet_3_3 == 23)and
		  (mammet_3_4 == 0 or mammet_3_4 == 21 or mammet_3_4 == 22 or mammet_3_4 == 23)and
		  (mammet_3_5 == 0 or mammet_3_5 == 21 or mammet_3_5 == 22 or mammet_3_5 == 23))then
		   killer:setPos(499, 56, -802);
		killer:startEvent(0x000b);
	     SpawnMob(16908308); --omega 3
		 	killer:addTitle(BRANDED_BY_LIGHTNING);
		 end
	end
	
	
	

end;

