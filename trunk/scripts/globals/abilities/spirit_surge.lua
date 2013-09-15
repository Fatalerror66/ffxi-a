-----------------------------------
-- Ability: Spirit Surge
-----------------------------------

require("scripts/globals/status");

-----------------------------------
-- OnUseAbility
-----------------------------------

function OnAbilityCheck(player,target,ability)
	-- The wyvern must be present in order to use Spirit Surge
	if (target:getPet() == nil) then
		return MSGBASIC_REQUIRES_A_PET,0;
	else
		return 0,0;
	end
end;

function OnUseAbility(player, target, ability)
	-- Spirit Surge gives a Strength boost dependant of DRG level
	local strBoost = 0;
	if(target:getMainJob()==JOB_DRG) then
		strBoost = (1 + target:getMainLvl()/5);	-- Use Mainjob Lvl
	elseif(target:getSubJob()==JOB_DRG) then
		strBoost = (1 + target:getSubLvl()/5);	-- Use Subjob Lvl
	end
	
	-- Spirit Surge lasts 60 seconds, or 20 more if Wyrm Mail+2 is equipped
	local duration = 60;
	if(target:getEquipID(SLOT_BODY)==10683) then
		duration = duration + 20;
	end
	
	target:addStatusEffect(EFFECT_SPIRIT_SURGE, strBoost, 0, duration);
end;