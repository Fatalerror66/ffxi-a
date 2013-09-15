-----------------------------------------
--	ID: 4164
--	Prism Powder
--	When applied, it makes things invisible.
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
	return 0;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	local duration = math.random(60, 180);
	duration = duration + (duration * target:getMod(MOD_INVIS_DUR));
	if (not target:hasStatusEffect(EFFECT_INVISIBLE)) then
		target:addStatusEffect(EFFECT_INVISIBLE,0,10,duration);
	end
end;
