-----------------------------------------
-- Spell: Aero IV
-- Deals wind damage to an enemy.
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
	--doElementalNuke(V,M,caster,spell,target,hasMultipleTargetReduction,resistBonus)
	local dmg = doElementalNuke(440,2,caster,spell,target,false,1.0);
	return dmg;
end;