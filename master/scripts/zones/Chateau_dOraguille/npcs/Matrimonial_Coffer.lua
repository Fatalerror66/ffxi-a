-----------------------------------
--	[Command name]: shop
--	[Author      ]: 
--	[Description ]: 
-----------------------------------
--http://ffxi.gamerescape.com/wiki/Matrimonial_Coffer
-----------------------------------
-- Action
-----------------------------------

function onTrigger(player,npc)
stock =
	{
	--Gender: Female Only ?
	  --Wedding Dress Set 400,000
		13933,100000,		--bridal corsage,
		14386,100000,		--wedding dress,
		14251,100000,		--wedding hose,
		14126,100000,		--wedding boots,
		15848,100000,		--matrimony band,
	
	--Gender: Male Only ?
	  --Benedight Coat Set 200,000
		16364,100000,		--benedight hose
		11309,100000,		--benedight coat,
		15847,80000,		--matrimony ring,
	};
	showShop(player, STATIC, stock);
end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;