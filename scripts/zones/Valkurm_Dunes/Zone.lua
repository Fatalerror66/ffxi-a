-----------------------------------
--
-- Zone: Valkurm_Dunes (103)
--
-----------------------------------

package.loaded["scripts/zones/Valkurm_Dunes/TextIDs"] = nil;

require("scripts/zones/Valkurm_Dunes/TextIDs");
require("scripts/globals/icanheararainbow");
require("scripts/globals/status");
require("scripts/globals/weather");

-----------------------------------
-- onInitialize
-----------------------------------

function onInitialize(zone)		
end;		

-----------------------------------		
-- onZoneIn		
-----------------------------------		

function onZoneIn( player, prevZone)

	cs = -1;

	if( player:getXPos() == 0 and player:getYPos() == 0 and player:getZPos() == 0) then
		player:setPos( 60.989, -4.898, -151.001, 198);
	end

	if( triggerLightCutscene( player)) then -- Quest: I Can Hear A Rainbow
		cs = 0x0003;
	end
	
	return cs;		
end;			

-----------------------------------			
-- onRegionEnter			
-----------------------------------			

function onRegionEnter( player, region)			
end;			

-----------------------------------			
-- onEventUpdate			
-----------------------------------			

function onEventUpdate( player, csid, option)			
--printf("CSID: %u",csid);		
--printf("RESULT: %u",option);

	if( csid == 0x0003) then		
		lightCutsceneUpdate( player);  -- Quest: I Can Hear A Rainbow
	end		
end;			

-----------------------------------			
-- onEventFinish			
-----------------------------------			

function onEventFinish( player, csid, option)			
--printf("CSID: %u",csid);		
--printf("RESULT: %u",option);

	if( csid == 0x0003) then		
		lightCutsceneFinish( player);  -- Quest: I Can Hear A Rainbow
	end		
end;

function OnZoneWeatherChange(weather)
	local qm1 = GetNPCByID(17199692); -- Quest: An Empty Vessel
	if(weather == WEATHER_DUST_STORM) then
		qm1:setStatus(STATUS_NORMAL);
	else
		qm1:setStatus(STATUS_DISAPPEAR);
	end
end;