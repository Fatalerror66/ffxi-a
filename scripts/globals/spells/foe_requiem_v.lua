-----------------------------------------
-- Spell: Foe Requiem V
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
    local effect = EFFECT_REQUIEM;
    local duration = 127;
    local power = 5;

	local bonus = AffinityBonus(caster, spell:getElement());
    local pCHR = caster:getStat(MOD_CHR);
    local mCHR = target:getStat(MOD_CHR);
    local dCHR = (pCHR - mCHR);
    local resm = applyResistance(caster,spell,target,dCHR,SINGING_SKILL,bonus);
    if(resm < 0.5) then
        spell:setMsg(85);--resist message
        return 1;
    end
	duration = duration + (duration * (caster:getMod(MOD_SONG_DURATION)/100));
 	duration = duration + (duration * ((caster:getMod(MOD_ALL_SONGS) * 10)/100));
	duration = duration + (duration * ((caster:getMod(MOD_REQUIEM) * 10)/100));
	
	power = power + caster:getMod(MOD_REQUIEM) + caster:getMod(MOD_ALL_SONGS);

    -- Try to overwrite weaker slow / haste
    if(canOverwrite(target, effect, power)) then
        -- overwrite them
        target:delStatusEffect(effect);
        target:addStatusEffect(effect,power,3,duration);
        spell:setMsg(237);
    else
        spell:setMsg(75); -- no effect
    end

    return effect;
end;