-----------------------------------	
-- Atonement
-- Sword weapon skill	
-- Skill Level: NA	
-- Delivers a twofold attack. Damage varies with TP. Burtgang: Aftermath effect varies with TP.		
-- In order to obtain Atonement, the quest Unlocking a Myth must be completed.	
-- Will stack with Sneak Attack.	
-- Aligned with the Aqua Gorget, Flane Gorget & Light Gorget	
-- Aligned with the Aqua Belt, Flame Belt & Light Belt.	
-- Element: None	
-- Modifiers: Related to accumulated Eminity	
-- 100%TP    200%TP    300%TP	
-- 1.00      1.75      2.50	
-----------------------------------	
	
require("scripts/globals/status");	
require("scripts/globals/settings");	
require("scripts/globals/weaponskills");	
-----------------------------------	
	
function OnUseWeaponSkill(player, target, wsID)	
	
	local params = {};
	params.numHits = 2;
	params.ftp100 = 1; params.ftp200 = 1.75; params.ftp300 = 2.5;
	params.str_wsc = 0.3; params.dex_wsc = 0.0; params.vit_wsc = 0.0; params.agi_wsc = 0.0; params.int_wsc = 0.0; params.mnd_wsc = 0.5; params.chr_wsc = 0.0;
	params.crit100 = 0.0; params.crit200 = 0.0; params.crit300 = 0.0;
	params.canCrit = false;
	params.acc100 = 0.0; params.acc200= 0.0; params.acc300= 0.0;
	params.atkmulti = 1;
	local damage, criticalHit, tpHits, extraHits = doPhysicalWeaponskill(player, target, params);

	local main = player:getEquipID(SLOT_MAIN);
	local sub = player:getEquipID(SLOT_SUB);
	local aftermath = 0;
	local tp = player:getTP();
	local duration = 0;
	local subpower = 0;
	
	if (main == 18997 or sub == 18997) then
		aftermath = 1;
	elseif (main == 19066 or sub == 19066) then
		aftermath = 1;
	elseif (main == 19086 or sub == 19086) then
		aftermath = 1;
	elseif (main == 19618 or sub == 19618) then
		aftermath = 1;
		damage = damage * 1.15;
	elseif (main == 19716 or sub == 19716) then
		aftermath = 1;
		damage = damage * 1.15;
	elseif (main == 19825 or sub == 19825) then
		aftermath = 1;
		damage = damage * 1.3;
	elseif (main == 19954 or sub == 19954) then
		aftermath = 1;
		damage = damage * 1.3;
	end
		
	if (aftermath == 1) then
		if (tp == 300) then
			player:delStatusEffect(EFFECT_AFTERMATH_LV1);
			player:delStatusEffect(EFFECT_AFTERMATH_LV2);
			player:delStatusEffect(EFFECT_AFTERMATH_LV3);
			if (main == 18997 or sub == 18997) then
				duration = 120;
				player:addStatusEffect(EFFECT_AFTERMATH_LV3,14,0,duration,0,40);
			elseif ((main == 19066 or sub == 19066) or (main == 19086 or sub == 19086) or (main == 19618  or sub == 19618)) then
				duration = 180;
				player:addStatusEffect(EFFECT_AFTERMATH_LV3,14,0,duration,0,60);
			elseif ((main == 19716 or sub == 19716) or (main == 19825 or sub == 19825) or (main == 19954 or sub == 19954)) then
				duration = 180;
				player:addStatusEffect(EFFECT_AFTERMATH_LV3,15,0,duration,0,20);
			end
		elseif (tp >= 200) then
			if (player:hasStatusEffect(EFFECT_AFTERMATH_LV3) == false) then
				player:delStatusEffect(EFFECT_AFTERMATH_LV1);
				player:delStatusEffect(EFFECT_AFTERMATH_LV2);
				if (main == 18997 or sub == 18997) then
					duration = 90;
					subpower = math.floor(2 * (tp / 5) - 60);
				elseif ((main == 19066 or sub == 19066) or (main == 19086 or sub == 19086) or (main == 19618  or sub == 19618)) then
					duration = 120;
					subpower = math.floor(3 * (tp / 5) - 90);
				elseif ((main == 19716 or sub == 19716) or (main == 19825 or sub == 19825) or (main == 19954 or sub == 19954)) then
					duration = 120;
					subpower = math.floor((tp * .6) - 80);
				end
				player:addStatusEffect(EFFECT_AFTERMATH_LV2,14,0,duration,0,subpower);
			end
		else
			if (player:hasStatusEffect(EFFECT_AFTERMATH_LV3) == false) then
				if (player:hasStatusEffect(EFFECT_AFTERMATH_LV2) == false) then
					player:delStatusEffect(EFFECT_AFTERMATH_LV1);
					if (main == 18997 or sub == 18997) then
						duration = 60;
						subpower = math.floor(tp / 10);
					elseif ((main == 19066 or sub == 19066) or (main == 19086 or sub == 19086) or (main == 19618  or sub == 19618)) then
						duration = 90;
						subpower = math.floor(3 * (tp / 20));
					elseif ((main == 19716 or sub == 19716) or (main == 19825 or sub == 19825) or (main == 19954 or sub == 19954)) then
						duration = 90;
						subpower = math.floor((tp / 10) + 20);
					end
					player:addStatusEffect(EFFECT_AFTERMATH_LV1,14,0,duration,0,subpower);
				end
			end
		end
	end
	
	return tpHits, extraHits, criticalHit, damage;
	
end	
