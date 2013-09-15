-----------------------------------------
--	ID: 5871
--	Item: Ruszor Quiver
--	When used, you will obtain one stack of Ruszor Arrows
-----------------------------------------

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
result = 0;
	if (target:getFreeSlotsCount() == 0) then
		result = 308;
	end
return result;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addItem(19182,99);
end;