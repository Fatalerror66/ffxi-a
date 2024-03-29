-----------------------------------------
-- Spell: Dia V
-- Lowers an enemy's defense and gradually
-- deals light elemental damage.
-----------------------------------------

require("scripts/globals/settings");
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------------
-- OnSpellCast
-----------------------------------------

function OnMagicCastingCheck(caster,target,spell)
	return 0;
end;

function onSpellCast(caster,target,spell)

	--calculate raw damage
	local basedmg = caster:getSkillLevel(ENFEEBLING_MAGIC_SKILL);
	local dmg = calculateMagicDamage(basedmg,5,caster,spell,target,ENFEEBLING_MAGIC_SKILL,MOD_INT,false);

	-- Softcaps at 512, should always do at least 1

	dmg = utils.clamp(dmg, 1, 512);

	--get resist multiplier (1x if no resist)
	local resist = applyResistance(caster,spell,target,caster:getStat(MOD_INT)-target:getStat(MOD_INT),ENFEEBLING_MAGIC_SKILL,1.0);
	--get the resisted damage
	dmg = dmg*resist;
	--add on bonuses (staff/day/weather/jas/mab/etc all go in this function)
	dmg = addBonuses(caster,spell,target,dmg);
	--add in target adjustment
	dmg = adjustForTarget(target,dmg);
	--add in final adjustments including the actual damage dealt
	local final = finalMagicAdjustments(caster,target,spell,dmg);

	-- Calculate duration.
	local duration = 120;
	local diaPowerMod = 0;
		
	if(caster:getEquipID(SLOT_MAIN) == 17466 or caster:getEquipID(SLOT_SUB) == 17466) then -- Dia Wand
		diaPowerMod = 1;
	end
	
	if (caster:hasStatusEffect(EFFECT_SABOTEUR) == true) then
		duration = duration + (duration * (1 + (caster:getMod(MOD_SABOTEUR)/100)));
		diaPowerMod = diaPowerMod + 5;
		caster:delStatusEffect(EFFECT_SABOTEUR);
    	end

	-- Check for Bio.
	bio = target:getStatusEffect(EFFECT_BIO);

	-- Do it!
	if(bio == nil or (DIA_OVERWRITE == 0 and bio:getPower() <= 5) or (DIA_OVERWRITE == 1 and bio:getPower() < 5)) then
		target:addStatusEffect(EFFECT_DIA,5,3,duration, 0, 25, diaPowerMod);
		spell:setMsg(2);
	else
		spell:setMsg(75);
	end

	-- Try to kill same tier Bio
	if(BIO_OVERWRITE == 1 and bio ~= nil) then
		if(bio:getPower() <= 5) then
			target:delStatusEffect(EFFECT_BIO);
		end
	end

	return final;

end;
