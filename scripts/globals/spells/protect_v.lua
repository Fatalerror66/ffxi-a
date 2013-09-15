-----------------------------------------
-- Spell: Protect V
-----------------------------------------

require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------------
-- OnSpellCast
-----------------------------------------

function OnMagicCastingCheck(caster,target,spell)
	return 0;
end;

function onSpellCast(caster,target,spell)
    local power = 175;
    local duration = 1800;

    duration = calculateDurationForLvl(duration, 76, target:getMainLvl());

	-- Estoqueurs Bonus
	duration = duration + (duration * caster:getMod(MOD_ENHANCING_DUR));
	
	if (target:getEquipID(SLOT_EAR1) == 11039 or target:getEquipID(SLOT_EAR2) == 11039) then -- Brachyura Earring
		power = power + 10;
	end
	if (target:getEquipID(SLOT_RING1) == 10764 or target:getEquipID(SLOT_RING2) == 10764) then -- Sheltered Ring
		power = power + 10;
	end
	
    local typeEffect = EFFECT_PROTECT;
    if(target:addStatusEffect(typeEffect, power, 0, duration)) then
        spell:setMsg(230);
    else
        spell:setMsg(75); -- no effect
    end

    return typeEffect;
end;
