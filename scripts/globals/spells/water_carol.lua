-----------------------------------------
-- Spell: Water Carol
-- MOD_WATERRES
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
	return handleCarol(caster, target, spell, 40, 120, MOD_WATERRES);
end;
