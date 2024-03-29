---------------------------------------------
--  Great Wheel
--
--  Description: Damages enemies in an area of effect. Additional effect: Enmity Reset
--  Type: Physical
--  Wipes Shadows
--  Range: Melee range radial
---------------------------------------------
require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------
function OnMobSkillCheck(target,mob,skill)
    return 0;
end;

function OnMobWeaponSkill(target, mob, skill)

-- TODO: Knockback

	local numhits = 1;
	local accmod = 1;
	local dmgmod = 2.0;
	local info = MobPhysicalMove(mob,target,skill,numhits,accmod,dmgmod,TP_NO_EFFECT);
	local dmg = MobFinalAdjustments(info.dmg,mob,skill,target,MOBSKILL_PHYSICAL,MOBPARAM_SLASH,MOBPARAM_3_SHADOW);

	target:delHP(dmg);
   mob:resetEnmity(target);
	return dmg;
end;
