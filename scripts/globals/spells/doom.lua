-----------------------------------------
-- Spell: Doom
-- Gives you 30 seconds to live.
-----------------------------------------

require("scripts/globals/magic");
require("scripts/globals/status");

-----------------------------------------
-- OnSpellCast
-----------------------------------------

function OnMagicCastingCheck(caster,target,spell)
	return 0;
end;

function onSpellCast(caster,target,spell)
    local effect = EFFECT_DOOM;
    if(target:hasStatusEffect(effect) == false) then
    	if(math.random(0,100) >= target:getMod(MOD_DOOMRES)) then
    	    spell:setMsg(237); -- gains effect
    	    target:addStatusEffect(effect,10,3,30);
    	end
    else
        spell:setMsg(75) -- no effect
    end

    return effect;
end;