-----------------------------------------
-- ID: 4485
-- Item: Noble Lady
-- Food Effect: 5 Min, Mithra only
-----------------------------------------
-- Dexterity 3
-- Mind -5
-- Charisma 3
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
local result = 0;
	if (target:getRace() ~= 7) then
		result = 247;
	end
	if(target:getMod(MOD_EAT_RAW_FISH) == 1) then
		result = 0;
	end
	if (target:hasStatusEffect(EFFECT_FOOD) == true) then
		result = 246;
	end
return result;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addStatusEffect(EFFECT_FOOD,0,0,300,4485);
end;

-----------------------------------
-- onEffectGain Action
-----------------------------------

function onEffectGain(target,effect)
	target:addMod(MOD_DEX, 3);
	target:addMod(MOD_MND, -5);
	target:addMod(MOD_CHR, 3);
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
	target:delMod(MOD_DEX, 3);
	target:delMod(MOD_MND, -5);
	target:delMod(MOD_CHR, 3);
end;
