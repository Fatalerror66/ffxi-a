-----------------------------------------
-- ID: 5877
-- Item: Phial of Terror Screen
-- Item Effect: Temporarily provides immunity to Terror
-- Duration: 2 Mins
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
result = 0;
	if (target:hasStatusEffect(EFFECT_MEDICINE)) then
		result = 111;
	end
return result;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addStatusEffect(EFFECT_MEDICINE,0,0,120);
	if(target:addStatusEffect(EFFECT_NEGATE_TERROR,0,0,120)) then
		target:messageBasic(205);
	else
        	target:messageBasic(423); -- no effect
        end
end;

-----------------------------------------
-- onEffectGain Action
-----------------------------------------

function onEffectGain(target,effect)
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
end;