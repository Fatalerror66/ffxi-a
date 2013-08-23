﻿/*
===========================================================================

  Copyright (c) 2010-2012 Darkstar Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

  This file is part of DarkStar-server source code.

===========================================================================
*/

#include "../../common/showmsg.h"
#include "../../common/timer.h"
#include "../../common/utils.h"

#include <math.h>

#include "lua_baseentity.h"
#include "lua_statuseffect.h"
#include "lua_trade_container.h"
#include "luautils.h"

#include "../packets/action.h"
#include "../packets/auction_house.h"
#include "../packets/char_abilities.h"
#include "../packets/char_appearance.h"
#include "../packets/char_jobs.h"
#include "../packets/char_job_extra.h"
#include "../packets/char_equip.h"
#include "../packets/char_health.h"
#include "../packets/char_skills.h"
#include "../packets/char_spells.h"
#include "../packets/char_stats.h"
#include "../packets/char_sync.h"
#include "../packets/char_update.h"
#include "../packets/chat_message.h"
#include "../packets/chat_message_string.h"
#include "../packets/send_box.h"
#include "../packets/entity_update.h"
#include "../packets/event.h"
#include "../packets/event_string.h"
#include "../packets/event_update.h"
#include "../packets/guild_menu.h"
#include "../packets/guild_menu_buy.h"
#include "../packets/inventory_finish.h"
#include "../packets/inventory_modify.h"
#include "../packets/inventory_size.h"
#include "../packets/key_items.h"
#include "../packets/menu_mog.h"
#include "../packets/menu_merit.h"
#include "../packets/menu_raisetractor.h"
#include "../packets/message_basic.h"
#include "../packets/message_special.h"
#include "../packets/message_standard.h"
#include "../packets/message_system.h"
#include "../packets/message_text.h"
#include "../packets/position.h"
#include "../packets/quest_mission_log.h"
#include "../packets/release.h"
#include "../packets/server_ip.h"
#include "../packets/shop_items.h"
#include "../packets/shop_menu.h"
#include "../packets/conquest_map.h"
#include "../packets/weather.h"

#include "../ability.h"
#include "../utils/battleutils.h"
#include "../utils/blueutils.h"
#include "../utils/charutils.h"
#include "../utils/itemutils.h"
#include "../utils/guildutils.h"
#include "../utils/puppetutils.h"
#include "../map.h"
#include "../alliance.h"
#include "../entities/mobentity.h"
#include "../mobskill.h"
#include "../entities/npcentity.h"
#include "../entities/petentity.h"
#include "../utils/petutils.h"
#include "../spell.h"
#include "../trade_container.h"
#include "../utils/zoneutils.h"



CLuaBaseEntity::CLuaBaseEntity(lua_State* L)
{
	if( !lua_isnil(L,-1) )
	{
		m_PBaseEntity = (CBaseEntity*)lua_touserdata(L,-1);
		lua_pop(L,1);
	}else{
		m_PBaseEntity = NULL;
	}
}

//======================================================//

CLuaBaseEntity::CLuaBaseEntity(CBaseEntity* PEntity)
{
	m_PBaseEntity = PEntity;
}

//======================================================//

inline int32 CLuaBaseEntity::leavegame(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	((CCharEntity*)m_PBaseEntity)->status = STATUS_SHUTDOWN;
	((CCharEntity*)m_PBaseEntity)->pushPacket(new CServerIPPacket((CCharEntity*)m_PBaseEntity,1));

	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::warp(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	((CCharEntity*)m_PBaseEntity)->loc.boundary = 0;
	((CCharEntity*)m_PBaseEntity)->loc.p = ((CCharEntity*)m_PBaseEntity)->profile.home_point.p;
    ((CCharEntity*)m_PBaseEntity)->loc.destination = ((CCharEntity*)m_PBaseEntity)->profile.home_point.destination;

	((CCharEntity*)m_PBaseEntity)->status = STATUS_DISAPPEAR;
	((CCharEntity*)m_PBaseEntity)->animation = ANIMATION_NONE;

	((CCharEntity*)m_PBaseEntity)->clearPacketList();
	((CCharEntity*)m_PBaseEntity)->pushPacket(new CServerIPPacket((CCharEntity*)m_PBaseEntity,2));

	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->health.hp );
	return 1;
}

inline int32 CLuaBaseEntity::getHPP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetHPP() );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::addHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;

	int32 result = PBattle->addHP(lua_tointeger(L,-1));

	// will always remove sleep effect
    PBattle->StatusEffectContainer->DelStatusEffect(EFFECT_SLEEP);
    PBattle->StatusEffectContainer->DelStatusEffect(EFFECT_SLEEP_II);
    PBattle->StatusEffectContainer->DelStatusEffect(EFFECT_LULLABY);

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
    lua_pushinteger( L, result );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::restoreHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	if(m_PBaseEntity->animation != ANIMATION_DEATH)
	{
		int32 result = ((CBattleEntity*)m_PBaseEntity)->addHP(lua_tointeger(L,-1));

		if( result != 0 && m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status != STATUS_DISAPPEAR)
		{
			charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
		}
		lua_pushinteger( L, result );
		return 1;
	}
	lua_pushinteger( L, 0 );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::delHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	int32 result = ((CBattleEntity*)m_PBaseEntity)->addHP(-lua_tointeger(L,-1));

    if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::setHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	((CBattleEntity*)m_PBaseEntity)->health.hp = 0;
	int32 value = lua_tointeger(L,-1);
	int32 result = ((CBattleEntity*)m_PBaseEntity)->addHP(value);

	if(m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getPet(lua_State* L)
{
	if(((CBattleEntity*)m_PBaseEntity)->PPet != NULL)
	{
		//uint32 petid = (uint32);

		CBattleEntity* PPet = ((CBattleEntity*)m_PBaseEntity)->PPet;

		lua_pushstring(L,CLuaBaseEntity::className);
		lua_gettable(L,LUA_GLOBALSINDEX);
		lua_pushstring(L,"new");
		lua_gettable(L,-2);
		lua_insert(L,-2);
		lua_pushlightuserdata(L,(void*)PPet);
		lua_pcall(L,2,1,0);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

inline int32 CLuaBaseEntity::familiar(lua_State* L)
{
	if(((CBattleEntity*)m_PBaseEntity)->PPet != NULL)
	{
		//uint32 petid = (uint32);

		CBattleEntity* PPet = ((CBattleEntity*)m_PBaseEntity)->PPet;

		petutils::Familiar(PPet);
	}

	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getPetID(lua_State* L)
{
	if ( m_PBaseEntity != NULL )
	{
		if(((CBattleEntity*)m_PBaseEntity)->PPet) {
			lua_pushinteger( L, ((CPetEntity*)(((CBattleEntity*)m_PBaseEntity)->PPet))->m_PetID );
			return 1;
		}
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->health.mp );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::addMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	int32 result = ((CBattleEntity*)m_PBaseEntity)->addMP(lua_tointeger(L,-1));

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
    lua_pushinteger( L, result );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::restoreMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	if(m_PBaseEntity->animation != ANIMATION_DEATH)
	{
		int32 result = ((CBattleEntity*)m_PBaseEntity)->addMP(lua_tointeger(L,-1));

		if( result != 0 && m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status != STATUS_DISAPPEAR)
		{
			charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
		}
		lua_pushinteger( L, result );
		return 1;
	}
	lua_pushinteger( L, 0 );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::delMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	int32 result = ((CBattleEntity*)m_PBaseEntity)->addMP(-lua_tointeger(L,-1));

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::setMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	((CBattleEntity*)m_PBaseEntity)->health.mp = 0;
	int32 value = lua_tointeger(L,-1) - ((CBattleEntity*)m_PBaseEntity)->health.mp;
	int32 result = ((CBattleEntity*)m_PBaseEntity)->addMP(value);

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getTP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushnumber( L, ((CBattleEntity*)m_PBaseEntity)->health.tp );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::addTP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint16 result = ((CBattleEntity*)m_PBaseEntity)->addTP(lua_tointeger(L,-1));

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::delTP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint16 result = ((CBattleEntity*)m_PBaseEntity)->addTP(-lua_tointeger(L,-1));

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
        charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::setTP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	float value = lua_tointeger(L,-1) - ((CBattleEntity*)m_PBaseEntity)->health.tp;
	uint16 result = ((CBattleEntity*)m_PBaseEntity)->addTP(value);

	if( result != 0 &&	m_PBaseEntity->objtype == TYPE_PC && m_PBaseEntity->status !=  STATUS_DISAPPEAR)
	{
		charutils::UpdateHealth((CCharEntity*)m_PBaseEntity);
	}
	return 0;
}

//======================================================//

inline int32 CLuaBaseEntity::getMaxHP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetMaxHP() );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::getMaxMP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetMaxMP() );
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::getXPos(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushnumber(L, m_PBaseEntity->GetXPos());
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::getYPos(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushnumber( L,  m_PBaseEntity->GetYPos());
	return 1;
}

//======================================================//

inline int32 CLuaBaseEntity::getZPos(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushnumber(L, m_PBaseEntity->GetZPos());
	return 1;
}

inline int32 CLuaBaseEntity::getRotPos(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushnumber(L, m_PBaseEntity->GetRotPos());
	return 1;
}

//======================================================//

//======================================================//

inline int32 CLuaBaseEntity::getRace(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	lua_pushinteger( L, ((CCharEntity*)m_PBaseEntity)->look.race );
	return 1;
}

/************************************************************************
*                                                                       *
*  Мгновенное перемещение сущности                                      *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setPos(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    if( m_PBaseEntity->objtype != TYPE_PC)
    {
        m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_DESPAWN));
    }

	if(lua_isnumber(L, 1))
	{

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
		m_PBaseEntity->loc.p.x = (float) lua_tonumber(L,1);
	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		m_PBaseEntity->loc.p.y = (float) lua_tonumber(L,2);
	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		m_PBaseEntity->loc.p.z = (float) lua_tonumber(L,3);
	if( !lua_isnil(L,4) && lua_isnumber(L,4) )
		m_PBaseEntity->loc.p.rotation = (uint8) lua_tointeger(L,4);
	}
	else
	{
		// its a table
		lua_rawgeti(L, 1, 1);
		m_PBaseEntity->loc.p.x = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 2);
		m_PBaseEntity->loc.p.y = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 3);
		m_PBaseEntity->loc.p.z = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 4);
		m_PBaseEntity->loc.p.rotation = (uint8)lua_tointeger(L, -1);
		lua_pop(L,1);
	}


	if( m_PBaseEntity->objtype == TYPE_PC)
	{
		if( !lua_isnil(L,5) && lua_isnumber(L,5) )
		{
            ((CCharEntity*)m_PBaseEntity)->loc.destination = (uint16)lua_tointeger(L,5);
			((CCharEntity*)m_PBaseEntity)->status = STATUS_DISAPPEAR;
			((CCharEntity*)m_PBaseEntity)->loc.boundary = 0;
			((CCharEntity*)m_PBaseEntity)->clearPacketList();
			((CCharEntity*)m_PBaseEntity)->pushPacket(new CServerIPPacket((CCharEntity*)m_PBaseEntity,2));
			//((CCharEntity*)m_PBaseEntity)->loc.zone->DecreaseZoneCounter(((CCharEntity*)m_PBaseEntity));
		}
        else
        {
			((CCharEntity*)m_PBaseEntity)->pushPacket(new CPositionPacket((CCharEntity*)m_PBaseEntity));
		}
	}
    else
    {
        m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_SPAWN));
    }
	return 0;
}

inline int32 CLuaBaseEntity::getPos(lua_State* L)
{
	lua_createtable(L, 3, 0);
    int8 newTable = lua_gettop(L);

    lua_pushnumber(L, m_PBaseEntity->loc.p.x);
    lua_rawseti(L, newTable, 1);

    lua_pushnumber(L, m_PBaseEntity->loc.p.y);
    lua_rawseti(L, newTable, 2);

    lua_pushnumber(L, m_PBaseEntity->loc.p.z);
    lua_rawseti(L, newTable, 3);

    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::addItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	uint16 itemID = (uint16)lua_tointeger(L,1);
	uint32 quantity = 1;
	uint16 augment0 = 0; uint8 augment0val = 0;
	uint16 augment1 = 0; uint8 augment1val = 0;
	uint16 augment2 = 0; uint8 augment2val = 0;
	uint16 augment3 = 0; uint8 augment3val = 0;

	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		quantity = (uint32)lua_tointeger(L,2);

	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		augment0 = (uint16)lua_tointeger(L,3);
	if( !lua_isnil(L,4) && lua_isnumber(L,4) )
		augment0val = (uint8)lua_tointeger(L,4);
	if( !lua_isnil(L,5) && lua_isnumber(L,5) )
		augment1 = (uint16)lua_tointeger(L,5);
	if( !lua_isnil(L,6) && lua_isnumber(L,6) )
		augment1val = (uint8)lua_tointeger(L,6);
	if( !lua_isnil(L,7) && lua_isnumber(L,7) )
		augment2 = (uint16)lua_tointeger(L,7);
	if( !lua_isnil(L,8) && lua_isnumber(L,8) )
		augment2val = (uint8)lua_tointeger(L,8);
	if( !lua_isnil(L,9) && lua_isnumber(L,9) )
		augment3 = (uint16)lua_tointeger(L,9);
	if( !lua_isnil(L,10) && lua_isnumber(L,10) )
		augment3val = (uint8)lua_tointeger(L,10);

	uint8 SlotID = ERROR_SLOTID;

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if (PChar->getStorage(LOC_INVENTORY)->GetFreeSlotsCount() != 0 && quantity != 0)
    {
        CItem* PItem = itemutils::GetItem(itemID);

	    if (PItem != NULL)
	    {
		    PItem->setQuantity(quantity);

            if (PItem->isType(ITEM_ARMOR))
		    {
			    if (augment0 != 0) ((CItemArmor*)PItem)->setAugment(0, augment0, augment0val);
			    if (augment1 != 0) ((CItemArmor*)PItem)->setAugment(1, augment1, augment1val);
                if (augment2 != 0) ((CItemArmor*)PItem)->setAugment(2, augment2, augment2val);
                if (augment3 != 0) ((CItemArmor*)PItem)->setAugment(3, augment3, augment3val);
		    }
            SlotID = charutils::AddItem(PChar, LOC_INVENTORY, PItem);
	    }
	    else
	    {
		    ShowWarning(CL_YELLOW"charplugin::AddItem: Item <%i> is not found in a database\n" CL_RESET, itemID);
	    }
    }
	lua_pushboolean( L, (SlotID != ERROR_SLOTID) );
	return 1;
}
//==========================================================//

inline int32 CLuaBaseEntity::resetPlayer(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1));

	const int8* charName  = lua_tostring(L, -1);
	uint32 id = 0;


	// char will not be logged in so get the id manually
	const int8* Query = "SELECT charid FROM chars WHERE charname = '%s';";
	int32 ret = Sql_Query(SqlHandle,Query,charName);

	if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		id = (int32)Sql_GetIntData(SqlHandle,0);


	// could not get player from database
	if (id == 0){
		ShowDebug("Could not get the character from database.\n");
		return 1;
	}

	// delete the account session
	Query = "DELETE FROM accounts_sessions WHERE charid = %u;";
	Sql_Query(SqlHandle, Query, id);



	// send the player to lower jeuno
	Query =
        "UPDATE chars "
        "SET "
          "pos_zone = %u,"
          "pos_prevzone = %u,"
          "pos_rot = %u,"
          "pos_x = %.3f,"
          "pos_y = %.3f,"
          "pos_z = %.3f,"
          "boundary = %u "
        "WHERE charid = %u;";

    Sql_Query(SqlHandle, Query,
        245,		// lower jeuno
        122,		// prev zone
        86,			// rotation
        33.464f,	// x
        -5.000f,	// y
		69.162f,	// z
        0,			//boundary,
        id);

	ShowDebug("Player reset was successful.\n");

	return 1;
}


/*****************************************************
wakeUp - Wakes the calling entity if necessary.
Should only be used onTick for DoTs. This checks the
ACTION_SLEEP state rather than enumerating StatusEffectContainer
using delStatusEffect, so it's a lot faster.
*******************************************************/

inline int32 CLuaBaseEntity::wakeUp(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;

    // is asleep is not working!
    // if(PEntity->isAsleep())
    // {
        //wake them up!
        PEntity->StatusEffectContainer->DelStatusEffect(EFFECT_SLEEP);
        PEntity->StatusEffectContainer->DelStatusEffect(EFFECT_SLEEP_II);
        PEntity->StatusEffectContainer->DelStatusEffect(EFFECT_LULLABY);
    // }
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::hasItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 ItemID = (uint16)lua_tointeger(L,1);

	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
	{
		uint8  locationID = LOC_INVENTORY;

		locationID = (uint8)lua_tointeger(L,2);
		locationID = (locationID < MAX_CONTAINER_ID ? locationID : LOC_INVENTORY);

		lua_pushboolean( L, PChar->getStorage(locationID)->SearchItem(ItemID) != ERROR_SLOTID );
		return 1;
	}
    lua_pushboolean( L, charutils::HasItem(PChar, ItemID) );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getFreeSlotsCount(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	uint8  locationID = LOC_INVENTORY;

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
	{
		locationID = (uint8)lua_tointeger(L,1);
		locationID = (locationID < MAX_CONTAINER_ID ? locationID : LOC_INVENTORY);
	}

	uint8 FreeSlots =((CCharEntity*)m_PBaseEntity)->getStorage(locationID)->GetFreeSlotsCount();

	lua_pushinteger( L, FreeSlots );
	return 1;
}

/************************************************************************
*  player:createWornItem(item)                                          *
*  Item cannot be used a second time								    *
*  For BCNM Item and Testimony (Maat battle)                            *
************************************************************************/

inline int32 CLuaBaseEntity::createWornItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	uint8 slotID = PChar->getStorage(LOC_INVENTORY)->SearchItem((uint16)lua_tointeger(L,1));

	if(slotID != -1)
	{
		CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);

		const int8* Query =
				"UPDATE char_inventory "
				"SET worn = 1 "
				"WHERE charid = %u AND location = %u AND slot = %u;";

		if (Sql_Query(SqlHandle, Query, PChar->id, PItem->getLocationID(), PItem->getSlotID()) != SQL_ERROR)
		{
			PItem->setWornItem(1);
		}
	}

	return 0;
}

/************************************************************************
*  player:hasWornItem(item)                                             *
*  return true if player has this worn item				                *
*  For BCNM Item and Testimony (Maat battle)                            *
************************************************************************/

inline int32 CLuaBaseEntity::hasWornItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	uint8 slotID = PChar->getStorage(LOC_INVENTORY)->SearchItem((uint16)lua_tointeger(L,1));

	if(slotID != ERROR_SLOTID)
	{
		CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);

        lua_pushboolean( L, PItem->getWornItem() == 1 );
        return 1;
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getZone(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushinteger( L, m_PBaseEntity->getZone() );
	return 1;
}

/************************************************************************
*                                                                       *
*  Получаем имя зоны, в которой находится персонаж                      *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getZoneName(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->loc.zone == NULL);

    lua_pushstring( L, m_PBaseEntity->loc.zone->GetName() );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getCurrentRegion(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushinteger( L, zoneutils::GetCurrentRegion(m_PBaseEntity->getZone()) );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getPreviousZone(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushinteger( L, m_PBaseEntity->loc.prevzone );
	return 1;
}

/************************************************************************
*                                                                       *
*  Узнаем континент, на котором находится сущность                      *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getContinentID(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushinteger( L, m_PBaseEntity->loc.zone->GetContinentID() );
	return 1;
}

/************************************************************************
*                                                                       *
*  Проверяем, посещалась ли указанная зона персонажем ранее             *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::isZoneVisited(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    lua_pushboolean( L, hasBit((uint16)lua_tointeger(L,-1), PChar->m_ZonesList, sizeof(PChar->m_ZonesList)));
	return 1;
}

/************************************************************************
*                                                                       *
*															            *
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::getWeather(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    WEATHER weather = WEATHER_NONE;
	if (m_PBaseEntity->objtype & TYPE_PC || m_PBaseEntity->objtype & TYPE_MOB)
	    weather = battleutils::GetWeather((CBattleEntity*)m_PBaseEntity, false);
    else
        weather = zoneutils::GetZone(m_PBaseEntity->getZone())->GetWeather();

	switch(weather)
    {
		case WEATHER_NONE:				lua_pushinteger(L, 0); break;
		case WEATHER_SUNSHINE:			lua_pushinteger(L, 1); break;
		case WEATHER_CLOUDS:			lua_pushinteger(L, 2); break;
		case WEATHER_FOG:				lua_pushinteger(L, 3); break;
		case WEATHER_HOT_SPELL:			lua_pushinteger(L, 4); break;
		case WEATHER_HEAT_WAVE:			lua_pushinteger(L, 5); break;
		case WEATHER_RAIN:				lua_pushinteger(L, 6); break;
		case WEATHER_SQUALL:			lua_pushinteger(L, 7); break;
		case WEATHER_DUST_STORM:		lua_pushinteger(L, 8); break;
		case WEATHER_SAND_STORM:		lua_pushinteger(L, 9); break;
		case WEATHER_WIND:				lua_pushinteger(L, 10); break;
		case WEATHER_GALES:				lua_pushinteger(L, 11); break;
		case WEATHER_SNOW:				lua_pushinteger(L, 12); break;
		case WEATHER_BLIZZARDS:			lua_pushinteger(L, 13); break;
		case WEATHER_THUNDER:			lua_pushinteger(L, 14); break;
		case WEATHER_THUNDERSTORMS:		lua_pushinteger(L, 15); break;
		case WEATHER_AURORAS:			lua_pushinteger(L, 16); break;
		case WEATHER_STELLAR_GLARE:		lua_pushinteger(L, 17); break;
		case WEATHER_GLOOM:				lua_pushinteger(L, 18); break;
		case WEATHER_DARKNESS:			lua_pushinteger(L, 19); break;
        default: lua_pushnil(L);
    }
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setWeather(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	uint8 weather = (uint8)lua_tointeger(L,1);

    if (weather < MAX_WEATHER_ID)
    {
        zoneutils::GetZone(m_PBaseEntity->getZone())->SetWeather((WEATHER)weather);
		luautils::OnZoneWeatherChange(m_PBaseEntity->getZone(), weather);
    }
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getNation(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	lua_pushinteger( L, ((CCharEntity*)m_PBaseEntity)->profile.nation );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setNation(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    PChar->profile.nation = (uint8)lua_tointeger(L,-1);
    charutils::SaveCharNation(PChar);
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getRankPoints(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	lua_pushinteger( L, ((CCharEntity*)m_PBaseEntity)->profile.rankpoints );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setRankPoints(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->profile.rankpoints = (int32)lua_tointeger(L, -1);
	charutils::SaveMissionsList(PChar);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::addRankPoints(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->profile.rankpoints += (int32)lua_tointeger(L, -1);;
	charutils::SaveMissionsList(PChar);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getRank(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity * PChar = (CCharEntity*)m_PBaseEntity;

	lua_pushinteger( L, PChar->profile.rank[PChar->profile.nation]);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setRank(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->profile.rank[PChar->profile.nation] = (int32)lua_tointeger(L, -1);;
	charutils::SaveMissionsList(PChar);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::addQuest(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint8 questID = (uint8)lua_tointeger(L,-1);
	uint8 logID   = (uint8)lua_tointeger(L,-2);

	if(logID < MAX_QUESTAREA && questID < MAX_QUESTID)
	{
		uint8 current  = PChar->m_questLog[logID].current [questID/8] & (1 << (questID % 8));
		uint8 complete = PChar->m_questLog[logID].complete[questID/8] & (1 << (questID % 8));

		if ((current == 0) && (complete == 0))
		{
			PChar->m_questLog[logID].current [questID/8] |= (1 << (questID % 8));
			PChar->pushPacket(new CQuestMissionLogPacket(PChar, logID, 1));

            charutils::SaveQuestsList(PChar);
		}
	}else{
		ShowError(CL_RED"Lua::addQuest: LogID %i or QuestID %i is invalid\n" CL_RESET, logID, questID);
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::delQuest(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint8 questID = (uint8)lua_tointeger(L,-1);
	uint8 logID   = (uint8)lua_tointeger(L,-2);

	if(logID < MAX_QUESTAREA && questID < MAX_QUESTID)
	{
		uint8 current  = PChar->m_questLog[logID].current [questID/8] & (1 << (questID % 8));
		uint8 complete = PChar->m_questLog[logID].complete[questID/8] & (1 << (questID % 8));

		if ((current != 0) || (complete != 0))
		{
			PChar->m_questLog[logID].current [questID/8] &= ~(1 << (questID % 8));
			PChar->m_questLog[logID].complete[questID/8] &= ~(1 << (questID % 8));

			PChar->pushPacket(new CQuestMissionLogPacket(PChar, logID, 1));
			PChar->pushPacket(new CQuestMissionLogPacket(PChar, logID, 2));

			charutils::SaveQuestsList(PChar);
		}
	}else{
		ShowError(CL_RED"Lua::delQuest: LogID %i or QuestID %i is invalid\n" CL_RESET, logID, questID);
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getQuestStatus(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	uint8 questID = (uint8)lua_tointeger(L,-1);
	uint8 logID   = (uint8)lua_tointeger(L,-2);

	if(logID < MAX_QUESTAREA && questID < MAX_QUESTID)
	{
		uint8 current  = ((CCharEntity*)m_PBaseEntity)->m_questLog[logID].current [questID/8] & (1 << (questID % 8));
		uint8 complete = ((CCharEntity*)m_PBaseEntity)->m_questLog[logID].complete[questID/8] & (1 << (questID % 8));

		lua_pushinteger( L, (complete != 0 ? 2 : (current != 0 ? 1 : 0)) );
		return 1;
	}else{
		ShowError(CL_RED"Lua::getQuestStatus: LogID %i or QuestID %i is invalid\n" CL_RESET, logID, questID);
	}
	lua_pushnil(L);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::completeQuest(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint8 questID = (uint8)lua_tointeger(L,-1);
	uint8 logID   = (uint8)lua_tointeger(L,-2);

	if(logID < MAX_QUESTAREA && questID < MAX_QUESTID)
	{
		uint8 complete = PChar->m_questLog[logID].complete[questID/8] & (1 << (questID % 8));

		if (complete == 0)
		{
			PChar->m_questLog[logID].current [questID/8] &= ~(1 << (questID % 8));
			PChar->m_questLog[logID].complete[questID/8] |=  (1 << (questID % 8));

			PChar->pushPacket(new CQuestMissionLogPacket(PChar, logID, 1));
			PChar->pushPacket(new CQuestMissionLogPacket(PChar, logID, 2));
		}
        charutils::SaveQuestsList(PChar);
	}else{
		ShowError(CL_RED"Lua::completeQuest: LogID %i or QuestID %i is invalid\n" CL_RESET, logID, questID);
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  Проверяем, завершил ли персонаж задачу (quest)                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasCompleteQuest(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	uint8 questID = (uint8)lua_tointeger(L,-1);
	uint8 logID   = (uint8)lua_tointeger(L,-2);

	if(logID < MAX_QUESTAREA && questID < MAX_QUESTID)
	{
		uint8 complete = ((CCharEntity*)m_PBaseEntity)->m_questLog[logID].complete[questID/8] & (1 << (questID % 8));

		lua_pushboolean( L, (complete != 0) );
		return 1;
	}
    ShowError(CL_RED"Lua::hasCompleteQuest: LogID %i or QuestID %i is invalid\n" CL_RESET, logID, questID);
	lua_pushboolean( L, false );
	return 1;
}

/************************************************************************
*                                                                       *
*  Добавляем выбранную миссию                                           *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addMission(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);


    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    uint8 LogID     = (uint8)lua_tointeger(L,1);
    uint8 MissionID = (uint8)lua_tointeger(L,2);

    if (LogID < MAX_MISSIONAREA && MissionID < MAX_MISSIONID)
    {
        CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

        if (PChar->m_missionLog[LogID].current != LogID > 2 ? 0 : -1)
        {
            ShowWarning(CL_YELLOW"Lua::addMission: player has a current mission\n" CL_RESET, LogID);
        }
        PChar->m_missionLog[LogID].current = MissionID;
		PChar->pushPacket(new CQuestMissionLogPacket(PChar, LogID+11, 1));

		charutils::SaveMissionsList(PChar);
    }
    else
    {
        ShowError(CL_RED"Lua::delMission: LogID %i or Mission %i is invalid\n" CL_RESET, LogID, MissionID);
    }
	return 0;
}

/************************************************************************
*                                                                       *
*  Удаляем выбранную миссию                                             *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::delMission(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    uint8 LogID     = (uint8)lua_tointeger(L,1);
    uint8 MissionID = (uint8)lua_tointeger(L,2);

    if (LogID < MAX_MISSIONAREA && MissionID < MAX_MISSIONID)
    {
        CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

        uint8 current  = PChar->m_missionLog[LogID].current;
		uint8 complete = PChar->m_missionLog[LogID].complete[MissionID];

		if (current == MissionID)
		{
			PChar->m_missionLog[LogID].current = LogID > 2 ? 0 : -1;
			PChar->pushPacket(new CQuestMissionLogPacket(PChar, LogID+11, 1));
		}
		if (complete != 0)
		{
			PChar->m_missionLog[LogID].complete[MissionID] = false;
			PChar->pushPacket(new CQuestMissionLogPacket(PChar, LogID+11, 2));
		}
		charutils::SaveMissionsList(PChar);
    }
    else
    {
        ShowError(CL_RED"Lua::delMission: LogID %i or Mission %i is invalid\n" CL_RESET, LogID, MissionID);
    }
	return 0;
}

/************************************************************************
*                                                                       *
*  Проверяем, завершил ли персонаж выбранную миссию                     *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasCompletedMission(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    uint8 LogID     = (uint8)lua_tointeger(L,1);
    uint8 MissionID = (uint8)lua_tointeger(L,2);

    bool complete = false;

    if (LogID < MAX_MISSIONAREA && MissionID < MAX_MISSIONID)
    {
        complete = ((CCharEntity*)m_PBaseEntity)->m_missionLog[LogID].complete[MissionID];
    }
    else
    {
        ShowError(CL_RED"Lua::completeMission: LogID %i or Mission %i is invalid\n" CL_RESET, LogID, MissionID);
    }
	lua_pushboolean( L, complete );
	return 1;
}

/************************************************************************
*                                                                       *
*  Узнаем текущую миссию                                                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getCurrentMission(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint8  LogID     = (uint8)lua_tointeger(L,1);
    uint8  MissionID = 0;

    if (LogID < MAX_MISSIONAREA)
    {
        MissionID = (uint8)((CCharEntity*)m_PBaseEntity)->m_missionLog[LogID].current;
    }
    else
    {
        ShowError(CL_RED"Lua::completeMission: LogID %i is invalid\n" CL_RESET, LogID);
    }
	lua_pushinteger( L, MissionID );
	return 1;
}

/************************************************************************
*                                                                       *
*  Завершаем выбранную миссию                                           *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::completeMission(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    uint8 LogID     = (uint8)lua_tointeger(L,1);
    uint8 MissionID = (uint8)lua_tointeger(L,2);

    if (LogID < MAX_MISSIONAREA && MissionID < MAX_MISSIONID)
    {
	    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

        if (PChar->m_missionLog[LogID].current != MissionID)
        {
            ShowWarning(CL_YELLOW"Lua::completeMission: completion of not current mission\n" CL_RESET, LogID);
        }
	    PChar->m_missionLog[LogID].current = LogID > 2 ? 0 : -1;
	    PChar->m_missionLog[LogID].complete[MissionID] = true;
	    PChar->pushPacket(new CQuestMissionLogPacket(PChar, LogID+11, 1));
	    PChar->pushPacket(new CQuestMissionLogPacket(PChar, LogID+11, 2));

	    charutils::SaveMissionsList(PChar);
    }
    else
    {
        ShowError(CL_RED"Lua::completeMission: LogID %i or Mission %i is invalid\n" CL_RESET, LogID, MissionID);
    }
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::addKeyItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 KeyItemID = (uint16)lua_tointeger(L, -1);

	if( charutils::addKeyItem(PChar,KeyItemID) )
	{
		PChar->pushPacket(new CKeyItemsPacket(PChar,(KEYS_TABLE)(KeyItemID >> 9)));

		charutils::SaveKeyItems(PChar);
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::delKeyItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 KeyItemID = (uint16)lua_tointeger(L, -1);

	if( charutils::delKeyItem(PChar,KeyItemID) )
	{
		PChar->pushPacket(new CKeyItemsPacket(PChar,(KEYS_TABLE)(KeyItemID >> 9)));

		charutils::SaveKeyItems(PChar);
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::hasKeyItem(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint16 KeyItemID = (uint16)lua_tointeger(L, -1);

	lua_pushboolean( L, (charutils::hasKeyItem((CCharEntity*)m_PBaseEntity,KeyItemID) != 0));
	return 1;
}

/************************************************************************
*																		*
*  Проверяем, было ли описание ключевого предмета прочитано				*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::seenKeyItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint16 KeyItemID = (uint16)lua_tointeger(L, -1);

	lua_pushboolean( L, (charutils::seenKeyItem((CCharEntity*)m_PBaseEntity,KeyItemID) != 0));
	return 1;
}

/************************************************************************
*																		*
*  Should remove the key item from the seen list						*
*																		*
************************************************************************/


inline int32 CLuaBaseEntity::unseenKeyItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 KeyItemID = (uint16)lua_tointeger(L, -1);

	if( charutils::unseenKeyItem(PChar,KeyItemID) )
	{
		PChar->pushPacket(new CKeyItemsPacket(PChar,(KEYS_TABLE)(KeyItemID >> 9)));

		charutils::SaveKeyItems(PChar);
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  получить текущий уровень мастерства					                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getSkillLevel(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype & TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetSkill(lua_tointeger(L,-1)));
	return 1;
}

/************************************************************************
*                                                                       *
*                                   					                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getMaxSkillLevel(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-3) || !lua_isnumber(L,-3));

    SKILLTYPE skill = (SKILLTYPE)lua_tointeger(L,-1);
    JOBTYPE job = (JOBTYPE)lua_tointeger(L,-2);
    uint8 level = lua_tointeger(L,-3);

    lua_pushinteger( L, battleutils::GetMaxSkill(skill, job, level));
	return 1;
}

/************************************************************************
*                                                                       *
*  Get craft skill Rank player:getSkillRank(SKILLID)					*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getSkillRank(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	uint8 rankID = (uint8)lua_tointeger(L,1);

	lua_pushinteger( L, PChar->RealSkills.rank[rankID]);
	return 1;
}

/************************************************************************
*                                                                       *
*  Set craft skill rank player:setSkillRank(SKILLID,newRank)			*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setSkillRank(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	uint16 skillID = (uint16)lua_tointeger(L,1);
	uint16 newrank = (uint16)lua_tointeger(L,2);

	PChar->WorkingSkills.rank[skillID] = newrank;
	PChar->WorkingSkills.skill[skillID] += 1;
	PChar->RealSkills.rank[skillID] = newrank;
	//PChar->RealSkills.skill[skillID] += 1;

	charutils::BuildingCharSkillsTable(PChar);
	charutils::SaveCharSkills(PChar, skillID);
	PChar->pushPacket(new CCharSkillsPacket(PChar));

	return 0;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getStat(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;

    switch(lua_tointeger(L,-1))
    {
        case MOD_STR:  lua_pushinteger(L, PEntity->STR()); break;
        case MOD_DEX:  lua_pushinteger(L, PEntity->DEX()); break;
        case MOD_VIT:  lua_pushinteger(L, PEntity->VIT()); break;
        case MOD_AGI:  lua_pushinteger(L, PEntity->AGI()); break;
        case MOD_INT:  lua_pushinteger(L, PEntity->INT()); break;
        case MOD_MND:  lua_pushinteger(L, PEntity->MND()); break;
        case MOD_CHR:  lua_pushinteger(L, PEntity->CHR()); break;
		case MOD_ATT:  lua_pushinteger(L, PEntity->ATT()); break;
		case MOD_DEF:  lua_pushinteger(L, PEntity->DEF()); break;
        default: lua_pushnil(L);
    }
    return 1;
}

/************************************************************************
*                                                                       *
*  Добавляем персонажу заклинание с отображением сообщения              *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addSpell(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L, 1) || !lua_isnumber(L, 1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	bool silent = false;

	uint32 n = lua_gettop(L);

    uint16 SpellID = (uint16)lua_tointeger(L, 1);
	if (n > 1)
		silent = lua_toboolean(L, 2);

    if (charutils::addSpell(PChar, SpellID))
    {
        charutils::SaveSpells(PChar);
        PChar->pushPacket(new CCharSpellsPacket(PChar));
		if(!silent)
			PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 23));
    }
    return 0;
}

/************************************************************************
*                                                                       *
*  @addallspells GM command - Adds all Valid spells only                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addAllSpells(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 elements = sizeof ValidSpells / sizeof ValidSpells[0];

		 for(uint16 i = 0; i < elements; ++i)
		 {
			if (charutils::addSpell(PChar, ValidSpells[i]))
			{
				charutils::SaveSpells(PChar);
			}
		 }
    PChar->pushPacket(new CCharSpellsPacket(PChar));
    PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 23));

	return 0;
}

/************************************************************************
*                                                                       *
*  Проверяем у персонажа наличие заклинания в списке заклинаний         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasSpell(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    uint16 SpellID = (uint16)lua_tointeger(L,-1);

    lua_pushboolean(L, (charutils::hasSpell((CCharEntity*)m_PBaseEntity, SpellID) != 0));
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::canLearnSpell(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint32 Message = 0;
	uint16 SpellID = (uint16)lua_tointeger(L,-1);

	if (charutils::hasSpell((CCharEntity*)m_PBaseEntity,SpellID))
	{
		Message = 96;
	}
	else if (!spell::CanUseSpell((CCharEntity*)m_PBaseEntity, SpellID))
	{
		Message = 95;
	}
	lua_pushinteger( L, Message );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::delSpell(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 SpellID = (uint16)lua_tointeger(L,-1);

	if (charutils::delSpell(PChar,SpellID))
	{
		charutils::SaveSpells(PChar);
		PChar->pushPacket(new CCharSpellsPacket(PChar));
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  Add learned ability (corsair roll)						            *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addLearnedAbility(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint16 AbilityID = (uint16)lua_tointeger(L,-1);

    if (charutils::addLearnedAbility(PChar, AbilityID))
    {
		charutils::addAbility(PChar, AbilityID);
        charutils::SaveLearnedAbilities(PChar);
        PChar->pushPacket(new CCharAbilitiesPacket(PChar));
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 442));
    }
    return 0;
}

/************************************************************************
*                                                                       *
*  has learned ability (corsair roll)							        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasLearnedAbility(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    uint16 AbilityID = (uint16)lua_tointeger(L,-1);

    lua_pushboolean(L, (charutils::hasLearnedAbility((CCharEntity*)m_PBaseEntity, AbilityID) != 0));
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::canLearnAbility(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint32 Message = 0;
	uint16 AbilityID = (uint16)lua_tointeger(L,-1);

	if (charutils::hasLearnedAbility((CCharEntity*)m_PBaseEntity,AbilityID))
	{
		Message = 444;
	}
	else if (!ability::CanLearnAbility((CCharEntity*)m_PBaseEntity, AbilityID))
	{
		Message = 443;
	}
	lua_pushinteger( L, Message );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::delLearnedAbility(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 AbilityID = (uint16)lua_tointeger(L,-1);

	if (charutils::delLearnedAbility(PChar,AbilityID))
	{
		charutils::SaveLearnedAbilities(PChar);
		PChar->pushPacket(new CCharAbilitiesPacket(PChar));
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getMainJob(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetMJob() );
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getMainLvl(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetMLevel() );
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getSubJob(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetSJob() );
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getSubLvl(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->GetSLevel() );
    return 1;
}

/************************************************************************
*																		*
*  Делаем доступной персонажу указанную профессию. 0 - subjob			*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::unlockJob(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    JOBTYPE JobID = (JOBTYPE)lua_tointeger(L,-1);

    if (JobID < MAX_JOBTYPE)
    {
        PChar->jobs.unlocked |= (1 << JobID);

        if (JobID == JOB_NON) JobID = JOB_WAR;
		if (PChar->jobs.job[JobID] == 0) PChar->jobs.job[JobID] = 1;

        charutils::SaveCharJob(PChar, JobID);
        PChar->pushPacket(new CCharJobsPacket(PChar));
    }
    return 0;
}

/************************************************************************
*                                                                       *
*  Изменяем ограничение максимального уровня персонажа (genkai)         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::levelCap(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    if (!lua_isnil(L,-1) && lua_isnumber(L,-1))
    {
        uint8 genkai = (uint8)lua_tointeger(L,-1);

        if (PChar->jobs.genkai != genkai)
		{
		    PChar->jobs.genkai = (uint8)lua_tointeger(L,1);

            Sql_Query(SqlHandle,"UPDATE char_jobs SET genkai = %u WHERE charid = %u LIMIT 1", PChar->jobs.genkai, PChar->id);
        }
        return 0;
	}
	lua_pushinteger(L, PChar->jobs.genkai);
    return 1;
}

/************************************************************************
*																		*
*  Устанавливаем/узнаем временное ограничение уровня. Параметр функции	*
*  является новый ограничением уровня, 0 - отмена ограничения уровня.	*
*  Функция всегда возвращается значение текущего/нового ограниченя.		*
*																		*
*  Нужно будет вынести код установки уровня в отдельную функцию			*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::levelRestriction(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
	{
		PChar->m_LevelRestriction = (uint32)lua_tointeger(L,1);

		uint8 NewMLevel = 0;

		if (PChar->m_LevelRestriction != 0 &&
			PChar->m_LevelRestriction < PChar->jobs.job[PChar->GetMJob()])
		{
			NewMLevel = PChar->m_LevelRestriction;
		}else{
			NewMLevel = PChar->jobs.job[PChar->GetMJob()];
		}

		if (PChar->GetMLevel()!= NewMLevel)
		{
			PChar->SetMLevel(NewMLevel);
			PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

            blueutils::ValidateBlueSpells(PChar);
			charutils::BuildingCharSkillsTable(PChar);
			charutils::CalculateStats(PChar);
			charutils::BuildingCharTraitsTable(PChar);
			charutils::BuildingCharAbilityTable(PChar);
			charutils::CheckValidEquipment(PChar); // Handles rebuilding weapon skills as well.
		}
	}
	lua_pushinteger( L, PChar->m_LevelRestriction );
	return 1;
}

/************************************************************************
*																		*
*  Restricts a player's subjob temporarily                              *
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::sjRestriction(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint8 job = (uint8)lua_tonumber(L,1);
	bool state = lua_toboolean(L,2);

	if(state)
		PChar->SetSJob(JOB_NON);
	else if(!state && job != JOB_NON)
	{
		PChar->SetSJob(job);
		PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);
	}

	charutils::BuildingCharSkillsTable(PChar);
	charutils::CalculateStats(PChar);
	charutils::BuildingCharAbilityTable(PChar);
	charutils::BuildingCharTraitsTable(PChar);
	charutils::CheckValidEquipment(PChar);
	charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
	PChar->health.hp = PChar->GetMaxHP();
	PChar->health.mp = PChar->GetMaxMP();

	PChar->pushPacket(new CCharJobsPacket(PChar));
	PChar->pushPacket(new CCharUpdatePacket(PChar));
	PChar->pushPacket(new CCharHealthPacket(PChar));
	PChar->pushPacket(new CCharStatsPacket(PChar));
	PChar->pushPacket(new CCharSkillsPacket(PChar));
	PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
    PChar->pushPacket(new CCharJobExtraPacket(PChar, false));
	PChar->pushPacket(new CMenuMeritPacket(PChar));
	PChar->pushPacket(new CCharSyncPacket(PChar));
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::release(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	RELEASE_TYPE releaseType = RELEASE_STANDARD;

	if (PChar->m_event.EventID != -1)
	{
		// Message: Event skipped
		releaseType = RELEASE_SKIPPING;
		PChar->pushPacket(new CMessageSystemPacket(0,0,117));
	}
	PChar->pushPacket(new CReleasePacket(PChar,releaseType));
	PChar->pushPacket(new CReleasePacket(PChar,RELEASE_EVENT));
	return 0;
}

/************************************************************************
*                                                                       *
*  Запускаем событие с указанными параметрами                           *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::startEvent(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    int32 n = lua_gettop(L);

    if (n > 10)
    {
        ShowError("CLuaBaseEntity::startEvent: Could not start event, Lack of arguments.\n");
        lua_settop(L,-n);
        return 0;
    }
    if (m_PBaseEntity->animation == ANIMATION_HEALING)
    {
        ((CCharEntity*)m_PBaseEntity)->StatusEffectContainer->DelStatusEffect(EFFECT_HEALING);
    }

    uint16 EventID = (uint16)lua_tointeger(L,1);

    uint32 param0 = 0;
    uint32 param1 = 0;
    uint32 param2 = 0;
    uint32 param3 = 0;
    uint32 param4 = 0;
    uint32 param5 = 0;
    uint32 param6 = 0;
    uint32 param7 = 0;
    int16 textTable = -1;

    if( !lua_isnil(L,2) && lua_isnumber(L,2) )
        param0 = (uint32)lua_tointeger(L,2);
    if( !lua_isnil(L,3) && lua_isnumber(L,3) )
        param1 = (uint32)lua_tointeger(L,3);
    if( !lua_isnil(L,4) && lua_isnumber(L,4) )
        param2 = (uint32)lua_tointeger(L,4);
    if( !lua_isnil(L,5) && lua_isnumber(L,5) )
        param3 = (uint32)lua_tointeger(L,5);
    if( !lua_isnil(L,6) && lua_isnumber(L,6) )
        param4 = (uint32)lua_tointeger(L,6);
    if( !lua_isnil(L,7) && lua_isnumber(L,7) )
        param5 = (uint32)lua_tointeger(L,7);
    if( !lua_isnil(L,8) && lua_isnumber(L,8) )
        param6 = (uint32)lua_tointeger(L,8);
    if( !lua_isnil(L,9) && lua_isnumber(L,9) )
        param7 = (uint32)lua_tointeger(L,9);
    if( !lua_isnil(L,10) && lua_isnumber(L,10) )
        textTable = (int16)lua_tointeger(L,10);

    ((CCharEntity*)m_PBaseEntity)->pushPacket(
        new CEventPacket(
            (CCharEntity*)m_PBaseEntity,
            EventID,
            n-1,
            param0,
            param1,
            param2,
            param3,
            param4,
            param5,
            param6,
            param7,
            textTable));

    // если требуется вернуть фиктивный результат, то делаем это
    if( !lua_isnil(L,10) && lua_isnumber(L,10) )
    {
        ((CCharEntity*)m_PBaseEntity)->m_event.Option = (int32)lua_tointeger(L,10);
    }
    return 0;
}

/************************************************************************
*                                                                       *
*  Start event with string (0x33 packet)                                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::startEventString(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    if (m_PBaseEntity->animation == ANIMATION_HEALING)
    {
        ((CCharEntity*)m_PBaseEntity)->StatusEffectContainer->DelStatusEffect(EFFECT_HEALING);
    }

    uint16 EventID = (uint16)lua_tointeger(L,1);

    string_t string0 = "";
    string_t string1 = "";
    string_t string2 = "";
    string_t string3 = "";

    uint32 param0 = 0;
    uint32 param1 = 0;
    uint32 param2 = 0;
    uint32 param3 = 0;
    uint32 param4 = 0;
    uint32 param5 = 0;
    uint32 param6 = 0;
    uint32 param7 = 0;

    if( !lua_isnil(L,2) && lua_isstring(L,2) )
        string0 = lua_tolstring(L,2,NULL);
    if( !lua_isnil(L,3) && lua_isstring(L,3) )
        string1 = lua_tolstring(L,3,NULL);
    if( !lua_isnil(L,4) && lua_isstring(L,4) )
        string2 = lua_tolstring(L,4,NULL);
    if( !lua_isnil(L,5) && lua_isstring(L,5) )
        string3 = lua_tolstring(L,5,NULL);
    if( !lua_isnil(L,6) && lua_isnumber(L,6) )
        param0 = (uint32)lua_tointeger(L,6);
    if( !lua_isnil(L,7) && lua_isnumber(L,7) )
        param1 = (uint32)lua_tointeger(L,7);
    if( !lua_isnil(L,8) && lua_isnumber(L,8) )
        param2 = (uint32)lua_tointeger(L,8);
    if( !lua_isnil(L,9) && lua_isnumber(L,9) )
        param3 = (uint32)lua_tointeger(L,9);
    if( !lua_isnil(L,10) && lua_isnumber(L,10) )
        param4 = (uint32)lua_tointeger(L,10);
    if( !lua_isnil(L,11) && lua_isnumber(L,11) )
        param5 = (uint32)lua_tointeger(L,11);
    if( !lua_isnil(L,12) && lua_isnumber(L,12) )
        param6 = (uint32)lua_tointeger(L,12);
    if( !lua_isnil(L,13) && lua_isnumber(L,13) )
        param7 = (uint32)lua_tointeger(L,13);

    ((CCharEntity*)m_PBaseEntity)->pushPacket(
        new CEventStringPacket(
            (CCharEntity*)m_PBaseEntity,
            EventID,
            string0,
            string1,
            string2,
            string3,
            param0,
            param1,
            param2,
            param3,
            param4,
            param5,
            param6,
            param7));

    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::updateEvent(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	int32 n = lua_gettop(L);

	if ( n > 8 )
	{
		ShowError("CLuaBaseEntity::updateEvent: Could not update event, Lack of arguments.\n");
		lua_settop(L,-n);
		return 0;
	}

	uint32 param0 = 0;
	uint32 param1 = 0;
	uint32 param2 = 0;
	uint32 param3 = 0;
	uint32 param4 = 0;
	uint32 param5 = 0;
	uint32 param6 = 0;
	uint32 param7 = 0;

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
		param0 = (uint32)lua_tointeger(L,1);
	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		param1 = (uint32)lua_tointeger(L,2);
	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		param2 = (uint32)lua_tointeger(L,3);
	if( !lua_isnil(L,4) && lua_isnumber(L,4) )
		param3 = (uint32)lua_tointeger(L,4);
	if( !lua_isnil(L,5) && lua_isnumber(L,5) )
		param4 = (uint32)lua_tointeger(L,5);
	if( !lua_isnil(L,6) && lua_isnumber(L,6) )
		param5 = (uint32)lua_tointeger(L,6);
	if( !lua_isnil(L,7) && lua_isnumber(L,7) )
		param6 = (uint32)lua_tointeger(L,7);
	if( !lua_isnil(L,8) && lua_isnumber(L,8) )
		param7 = (uint32)lua_tointeger(L,8);

	((CCharEntity*)m_PBaseEntity)->pushPacket(
		new CEventUpdatePacket(
			param0,
			param1,
			param2,
			param3,
			param4,
			param5,
			param6,
			param7));

	return 0;
}

/************************************************************************
*																		*
*  Получаем указатель на персонажа, начавшего событие   				*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::getEventTarget(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    if (((CCharEntity*)m_PBaseEntity)->m_event.Target == NULL)
    {
        ShowWarning(CL_YELLOW"EventTarget is empty: %s\n" CL_RESET, m_PBaseEntity->GetName());
    }
    lua_pushstring(L,CLuaBaseEntity::className);
    lua_gettable(L,LUA_GLOBALSINDEX);
    lua_pushstring(L,"new");
    lua_gettable(L,-2);
    lua_insert(L,-2);
    lua_pushlightuserdata(L,(void*)((CCharEntity*)m_PBaseEntity)->m_event.Target);
    lua_pcall(L,2,1,0);
    return 1;
}

/************************************************************************
*																		*
*  Opens the dialogue box to deliver items to players     				*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::openSendBox(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    charutils::RecoverFailedSendBox((CCharEntity*)m_PBaseEntity);
    charutils::OpenSendBox((CCharEntity*)m_PBaseEntity);

    return 0;
}

/************************************************************************
*																		*
*  Отображаем статичный текст от лица NPC								*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::showText(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	// DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	uint16 messageID = (uint16)lua_tointeger(L,2);

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);

	if(PLuaBaseEntity != NULL)
	{
		CBaseEntity* PBaseEntity = PLuaBaseEntity->GetBaseEntity();
		if (PBaseEntity->objtype == TYPE_NPC)
		{
			PBaseEntity->m_TargID = m_PBaseEntity->targid;
			PBaseEntity->loc.p.rotation = getangle(PBaseEntity->loc.p, m_PBaseEntity->loc.p);

			PBaseEntity->loc.zone->PushPacket(
				PBaseEntity,
				CHAR_INRANGE,
				new CEntityUpdatePacket(PBaseEntity,ENTITY_UPDATE));
		}

        uint32 param0 = 0;
        uint32 param1 = 0;
        uint32 param2 = 0;
        uint32 param3 = 0;

        if( !lua_isnil(L,3) && lua_isnumber(L,3) )
            param0 = (uint32)lua_tointeger(L,3);
        if( !lua_isnil(L,4) && lua_isnumber(L,4) )
            param1 = (uint32)lua_tointeger(L,4);
        if( !lua_isnil(L,5) && lua_isnumber(L,5) )
            param2 = (uint32)lua_tointeger(L,5);
        if( !lua_isnil(L,6) && lua_isnumber(L,6) )
            param3 = (uint32)lua_tointeger(L,6);

        if(m_PBaseEntity->objtype == TYPE_PC)
        {
        ((CCharEntity*)m_PBaseEntity)->pushPacket(new CMessageSpecialPacket(PBaseEntity, messageID, param0, param1, param2, param3));
	}
        else
        {
			m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity,CHAR_INRANGE,new CMessageSpecialPacket(PBaseEntity, messageID, param0, param1, param3));
        }
	}
	return 0;
}

/************************************************************************
*																		*
*                                                           			*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::sendMenu(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint32 menu = (uint32)lua_tointeger(L, -1);

	switch(menu)
	{
		case 1:
			PChar->pushPacket(new CMenuMogPacket());
			break;
		case 2:
			PChar->pushPacket(new CShopMenuPacket(PChar));
			PChar->pushPacket(new CShopItemsPacket(PChar));
			break;
        case 3:
            PChar->pushPacket(new CAuctionHousePacket(2));
            break;
		default:
			ShowDebug("Menu %i not implemented, yet.\n", menu);
		break;
	}
	return 0;
}

/************************************************************************
*																		*
*  Отправляем персонажу меню магазина гильдии							*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::sendGuild(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,3) || !lua_isnumber(L,3));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,4) || !lua_isnumber(L,4));

	uint16 GuildID = (uint16)lua_tonumber(L,1);
	uint8  open    = (uint8) lua_tonumber(L,2);
	uint8  close   = (uint8) lua_tonumber(L,3);
	uint8  holiday = (uint8) lua_tonumber(L,4);

	DSP_DEBUG_BREAK_IF(open > close);

	uint8 VanadielHour = (uint8)CVanaTime::getInstance()->getHour();
	uint8 VanadielDay  = (uint8)CVanaTime::getInstance()->getWeekday();

	GUILDSTATUS status = GUILD_OPEN;

	if(VanadielDay == holiday)
	{
		status = GUILD_HOLYDAY;
	}
	else if ((VanadielHour < open) || (VanadielHour >= close))
	{
		status = GUILD_CLOSE;
	}
    CItemContainer* PGuildShop = guildutils::GetGuildShop(GuildID);
	((CCharEntity*)m_PBaseEntity)->PGuildShop = PGuildShop;
	((CCharEntity*)m_PBaseEntity)->pushPacket(new CGuildMenuPacket(status, open, close, holiday));
    if (status == GUILD_OPEN) {
        ((CCharEntity*)m_PBaseEntity)->pushPacket(new CGuildMenuBuyPacket((CCharEntity*)m_PBaseEntity, PGuildShop));
    }

	lua_pushboolean( L, status == GUILD_OPEN );
	return 1;
}

/************************************************************************
*																		*
*  Получаем временные переменные, необходимые для логики поисков		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::getVar(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isstring(L,-1));

	int32 value = 0;

	const int8* varname  = lua_tostring(L, -1);
	const int8* fmtQuery = "SELECT value FROM char_vars WHERE charid = %u AND varname = '%s' LIMIT 1;";

	int32 ret = Sql_Query(SqlHandle,fmtQuery,m_PBaseEntity->id, varname);

	if (ret != SQL_ERROR &&
		Sql_NumRows(SqlHandle) != 0 &&
		Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	{
		value = (int32)Sql_GetIntData(SqlHandle,0);
	}

	lua_pushinteger(L, value);
	return 1;
}

/************************************************************************
*																		*
*  Сохраняем временные переменные, необходимые для логики поисков		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::setVar(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isstring(L,-2));

	const int8* varname =  lua_tostring(L,-2);
	int32 value = (int32)lua_tointeger(L,-1);

	if (value == 0)
	{
		Sql_Query(SqlHandle,"DELETE FROM char_vars WHERE charid = %u AND varname = '%s' LIMIT 1;",m_PBaseEntity->id, varname);
		return 0;
	}

	const int8* fmtQuery = "INSERT INTO char_vars SET charid = %u, varname = '%s', value = %i ON DUPLICATE KEY UPDATE value = %i;";

	Sql_Query(SqlHandle,fmtQuery,m_PBaseEntity->id, varname, value, value);

	lua_pushnil(L);
	return 1;
}

/************************************************************************
*                                                                       *
*  Увеличиваем/уменьшаем значение временной переменной                  *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addVar(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isstring(L,-2));

	const int8* varname =  lua_tostring(L,-2);
	int32 value = (int32)lua_tointeger(L,-1);

	const int8* Query = "INSERT INTO char_vars SET charid = %u, varname = '%s', value = %i ON DUPLICATE KEY UPDATE value = value + %i;";

	Sql_Query(SqlHandle, Query,
        m_PBaseEntity->id,
        varname,
        value,
        value);

	return 0;
}

inline int32 CLuaBaseEntity::setPetName(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L, 1) || !lua_isnumber(L, 1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L, 2) || !lua_isnumber(L, 2));

	int32 n = lua_gettop(L);

	uint8 petType = (uint8)lua_tointeger(L, 1);

	if (n == 2)
	{
		uint16 value = (uint16)lua_tointeger(L, 2);

		if (petType == PETTYPE_WYVERN)
		{
			Sql_Query(SqlHandle, "INSERT INTO char_pet SET charid = %u, wyvernid = %u ON DUPLICATE KEY UPDATE wyvernid = %u;", m_PBaseEntity->id, value, value);
		}
		else if (petType == PETTYPE_AUTOMATON)
		{
			Sql_Query(SqlHandle, "INSERT INTO char_pet SET charid = %u, automatonid = %u ON DUPLICATE KEY UPDATE automatonid = %u;", m_PBaseEntity->id, value, value);
		}
		/*
		else if (petType == PETTYPE_ADVENTURING_FELLOW)
		{
			Sql_Query(SqlHandle, "INSERT INTO char_pet SET charid = %u, adventuringfellowid = %u ON DUPLICATE KEY UPDATE adventuringfellowid = %u;", m_PBaseEntity->id, value, value);
		}
		*/
	}
	else if (n == 3)
	{
		if (petType == PETTYPE_CHOCOBO)
		{
			uint32 chocoboname1 = lua_tointeger(L, 2) & 0x0000FFFF;
			uint32 chocoboname2 = lua_tointeger(L, 3) << 16;

			uint32 value = chocoboname1 + chocoboname2;

			Sql_Query(SqlHandle, "INSERT INTO char_pet SET charid = %u, chocoboid = %u ON DUPLICATE KEY UPDATE chocoboid = %u;", m_PBaseEntity->id, value, value);
		}
	}
    return 0;
}

inline int32 CLuaBaseEntity::getAutomatonName(lua_State* L)
{
    lua_pushstring(L, ((CCharEntity*)m_PBaseEntity)->m_AutomatonName.c_str());
    return 1;
}

/************************************************************************
*																		*
*  Set a single bit as part of a bitmask in a database variable 		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::setMaskBit(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isboolean(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-3) || !lua_isstring(L,-3));

	const int8* varname =  lua_tostring(L,-3);
	int32 bit = (int32)lua_tointeger(L,-2);
	bool state = ( lua_toboolean(L,-1) == 0 ? false : true );

	int32 value = (int32)lua_tointeger(L,-4);

	if(state == true)
	{
		value |= (1<<bit); // добавляем
	}
	else
	{
		value &= ~(1<<bit); // удаляем
	}

	const int8* fmtQuery = "INSERT INTO char_vars SET charid = %u, varname = '%s', value = %i ON DUPLICATE KEY UPDATE value = %i;";

	Sql_Query(SqlHandle,fmtQuery,m_PBaseEntity->id, varname, value, value);

	lua_pushinteger(L, value);
	return 1;
}

/************************************************************************
*																		*
*  Get a single bit from a bitmask in a database variable 				*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::getMaskBit(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	uint8 bit = (uint8)lua_tointeger(L,-1);

    DSP_DEBUG_BREAK_IF(bit >= 32);

	lua_pushboolean(L, (uint32)lua_tointeger(L,-2) & (1 << bit));
	return 1;
}

/************************************************************************
*																		*
*  Counts the number of "true" bits in a bitmask from a variable 		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::countMaskBits(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint8  count = 0;
	uint32 value = (uint32)lua_tointeger(L,-1);

	for (uint8 bit = 0; bit < 32; bit++)
    {
		if (value & (1 << bit)) count++;
	}
	lua_pushinteger(L, count);
	return 1;
}

/************************************************************************
*																		*
*  Returns true if var of the specified size contains only set bits		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::isMaskFull(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	bool condition = false;

	int32 value = (int32)lua_tointeger(L,-2);
	int16 size = (int16)lua_tointeger(L,-1);

	condition = (value == intpow32(2, size)-1);

	lua_pushboolean(L, condition);
	return 1;
}


//==========================================================//

inline int32 CLuaBaseEntity::setHomePoint(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    PChar->profile.home_point.p = PChar->loc.p;
    PChar->profile.home_point.destination = PChar->getZone();

	const int8 *fmtQuery = "UPDATE chars \
                            SET home_zone = %u, home_rot = %u, home_x = %.3f, home_y = %.3f, home_z = %.3f \
							WHERE charid = %u;";

	Sql_Query(SqlHandle, fmtQuery,
        PChar->profile.home_point.destination,
        PChar->profile.home_point.p.rotation,
		PChar->profile.home_point.p.x,
        PChar->profile.home_point.p.y,
        PChar->profile.home_point.p.z,
        PChar->id);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::tradeComplete(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	for (uint8 slotID = 0; slotID < TRADE_CONTAINER_SIZE; ++slotID)
	{
		if(PChar->Container->getInvSlotID(slotID) != 0xFF)
		{
			uint8 invSlotID = PChar->Container->getInvSlotID(slotID);
			int32 quantity  = PChar->Container->getQuantity(slotID);

			charutils::UpdateItem(PChar, LOC_INVENTORY, invSlotID, -quantity);
		}
	}
	PChar->Container->Clean();
	PChar->pushPacket(new CInventoryFinishPacket());
	return 0;
}

/************************************************************************
*																		*
*  Used in place of traceComplete when a trade uses confirmItem to      *
*  confirm traded items.												*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::confirmTrade(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	for (uint8 slotID = 0; slotID < TRADE_CONTAINER_SIZE; ++slotID)
	{
		if(PChar->Container->getInvSlotID(slotID) != 0xFF && PChar->Container->getConfirmedStatus(slotID))
		{
			uint8 invSlotID = PChar->Container->getInvSlotID(slotID);
			int32 quantity  = PChar->Container->getQuantity(slotID);

			charutils::UpdateItem(PChar, LOC_INVENTORY, invSlotID, -quantity);
		}
	}
	PChar->Container->Clean();
	PChar->pushPacket(new CInventoryFinishPacket());
	return 0;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasTitle(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    uint16 TitleID = (uint16)lua_tointeger(L,-1);

    lua_pushboolean(L, (charutils::hasTitle((CCharEntity*)m_PBaseEntity, TitleID) != 0));
    return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getTitle(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	lua_pushinteger( L, ((CCharEntity*)m_PBaseEntity)->profile.title );
    return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setTitle(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint16 TitleID = (uint16)lua_tointeger(L,-1);

    charutils::setTitle(PChar, TitleID);

    return 0;
}

/************************************************************************
*                                                                       *
*  Добавляем персонажу новое звание                                     *
*                                                                       *
************************************************************************/

inline int CLuaBaseEntity::addTitle(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint16 TitleID = (uint16)lua_tointeger(L,-1);

    PChar->profile.title = TitleID;
    PChar->pushPacket(new CCharStatsPacket(PChar));

    charutils::addTitle(PChar, TitleID);
    charutils::SaveTitles(PChar);
    return 0;
}

/************************************************************************
*                                                                       *
*  Удаляем у персонажа звание (DEBUG ONLY)                              *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::delTitle(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 TitleID = (uint16)lua_tointeger(L,-1);

	if (charutils::delTitle(PChar, TitleID))
	{
        if (PChar->profile.title == TitleID)
        {
            PChar->profile.title = 0;
        }
		PChar->pushPacket(new CCharStatsPacket(PChar));

        charutils::SaveTitles(PChar);
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getGil(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	if( m_PBaseEntity->objtype == TYPE_PC )
	{
		CItem * item = ((CCharEntity*)m_PBaseEntity)->getStorage(LOC_INVENTORY)->GetItem(0);

		if(item == NULL) //Player has no money
		{
			lua_pushinteger( L, 0 );
			return 1;
		}
		else if(!item->isType(ITEM_CURRENCY))
		{
			ShowFatalError(CL_RED"lua::getGil : Item in currency slot is not gil!\n" CL_RESET);
			return 0;
		}

		lua_pushinteger( L, item->getQuantity() );
		return 1;
	}
	if(m_PBaseEntity->objtype == TYPE_MOB)
	{
		// TODO: Mobs should have a gil pool, until implemented mob can be mugged unlimited times.
		CMobEntity * PMob = (CMobEntity*)m_PBaseEntity;
		if(PMob->m_EcoSystem == SYSTEM_BEASTMEN || PMob->m_Type & MOBTYPE_NOTORIOUS)
		{
			lua_pushinteger(L, PMob->GetRandomGil());
			return 1;
		}
	}
	lua_pushinteger(L, 0);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::addGil(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CItem * item = ((CCharEntity*)m_PBaseEntity)->getStorage(LOC_INVENTORY)->GetItem(0);

	if(item == NULL || !item->isType(ITEM_CURRENCY))
	{
		ShowFatalError(CL_RED"lua::addGil : No Gil in currency slot\n" CL_RESET);
		return 0;
	}

	int32 gil = (int32)lua_tointeger(L, -1);

	charutils::UpdateItem((CCharEntity*)m_PBaseEntity, LOC_INVENTORY, 0, gil);
	return 0;
}

/************************************************************************
*                                                                       *
*  Отнимаем указанное количество gil у персонажа                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::delGil(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    bool result = false;

	CItem* PItem = ((CCharEntity*)m_PBaseEntity)->getStorage(LOC_INVENTORY)->GetItem(0);

    if (PItem != NULL && PItem->isType(ITEM_CURRENCY))
    {
        int32 gil = (int32)lua_tointeger(L, -1);
	    result = charutils::UpdateItem((CCharEntity*)m_PBaseEntity, LOC_INVENTORY, 0, -gil) == 0xFFFF;
    }
    else
    {
        ShowFatalError(CL_RED"lua::delGil : No Gil in currency slot\n" CL_RESET);
    }
    lua_pushboolean(L, result);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setGil(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	CItem * item = ((CCharEntity*)m_PBaseEntity)->getStorage(LOC_INVENTORY)->GetItem(0);

	if(item == NULL || !item->isType(ITEM_CURRENCY))
	{
		ShowFatalError(CL_RED"lua::setGil : No Gil in currency slot\n" CL_RESET);
		return 0;
	}

	int32 gil = (int32)lua_tointeger(L, -1) - item->getQuantity();

	charutils::UpdateItem((CCharEntity*)m_PBaseEntity, LOC_INVENTORY, 0, gil);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::messageSpecial(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint16 messageID = (uint16)lua_tointeger(L,1);

	uint32 param0 = 0;
	uint32 param1 = 0;
	uint32 param2 = 0;
	uint32 param3 = 0;

	bool showName = 0;

	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		param0 = (uint32)lua_tointeger(L,2);
	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		param1 = (uint32)lua_tointeger(L,3);
	if( !lua_isnil(L,4) && lua_isnumber(L,4) )
		param2 = (uint32)lua_tointeger(L,4);
	if( !lua_isnil(L,5) && lua_isnumber(L,5) )
		param3 = (uint32)lua_tointeger(L,5);

	if( !lua_isnil(L,6) && lua_isboolean(L,6) )
		showName = ( lua_toboolean(L,6) == 0 ? false : true );

	((CCharEntity*)m_PBaseEntity)->pushPacket(
		new CMessageSpecialPacket(
			m_PBaseEntity,
			messageID,
			param0,
			param1,
			param2,
			param3,
			showName));
	return 0;
}

/************************************************************************
*                                                                       *
*  Отправляем базовое сообщение персонажу                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::messageBasic(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint16 messageID = (uint16)lua_tointeger(L,1);

	uint32 param0 = 0;
	uint32 param1 = 0;

    if( !lua_isnil(L,2) && lua_isnumber(L,2) )
        param0 = (uint32)lua_tointeger(L,2);
    if( !lua_isnil(L,3) && lua_isnumber(L,3) )
        param1 = (uint32)lua_tointeger(L,3);

	if(m_PBaseEntity->objtype == TYPE_PC){
		((CCharEntity*)m_PBaseEntity)->pushPacket(new CMessageBasicPacket(m_PBaseEntity, m_PBaseEntity, param0, param1, messageID));
	}
	else{//broadcast in range
		m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity,CHAR_INRANGE,new CMessageBasicPacket(m_PBaseEntity, m_PBaseEntity, param0, param1, messageID));
	}
	return 0;
}

/*
	Prodcast a message to public.
	Example:
		player:messagePublic(125, mob, 41, stolen);
*/
inline int32 CLuaBaseEntity::messagePublic(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint16 messageID = (uint16)lua_tointeger(L,1);

	uint32 param0 = 0;
	uint32 param1 = 0;

    CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,2);

    if (PEntity != NULL)
    {
	    if( !lua_isnil(L,2) && lua_isnumber(L,3) )
	        param0 = (uint32)lua_tointeger(L,3);
	    if( !lua_isnil(L,3) && lua_isnumber(L,4) )
	        param1 = (uint32)lua_tointeger(L,4);

		m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity,CHAR_INRANGE_SELF,new CMessageBasicPacket(m_PBaseEntity, PEntity->GetBaseEntity(), param0, param1, messageID));
    }
	return 0;
}

inline int32 CLuaBaseEntity::clearTargID(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	m_PBaseEntity->m_TargID = 0;
    m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity,CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity,ENTITY_UPDATE));
	return 0;
}

//========================================================//

inline int32 CLuaBaseEntity::capSkill(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint8 skill = lua_tointeger(L, -1);
	if(skill < MAX_SKILLTYPE){
		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
		CItemWeapon* PItem = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT_MAIN];
		/* let's just ignore this part for the moment
		//remove modifiers if valid
		if(skill>=1 && skill<=12 && PItem!=NULL && PItem->getSkillType()==skill){
			PChar->delModifier(MOD_ATT, PChar->GetSkill(skill));
			PChar->delModifier(MOD_ACC, PChar->GetSkill(skill));
		}
		*/
		uint16 maxSkill = 10*battleutils::GetMaxSkill((SKILLTYPE)skill, PChar->GetMJob(),PChar->GetMLevel());
		PChar->RealSkills.skill[skill] = maxSkill; //set to capped
		PChar->WorkingSkills.skill[skill] = maxSkill/10;
		PChar->WorkingSkills.skill[skill] |= 0x8000; //set blue capped flag
		PChar->pushPacket(new CCharSkillsPacket(PChar));
		charutils::CheckWeaponSkill(PChar, skill);
		/* and ignore this part
		//reapply modifiers if valid
		if(skill>=1 && skill<=12 && PItem!=NULL && PItem->getSkillType()==skill){
			PChar->addModifier(MOD_ATT, PChar->GetSkill(skill));
			PChar->addModifier(MOD_ACC, PChar->GetSkill(skill));
		}
		*/
		charutils::SaveCharSkills(PChar, skill); //save to db
	}
    return 0;
}


//========================================================//

inline int32 CLuaBaseEntity::capAllSkills(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	for (uint8 i = 1; i < 43; ++i)
	{
		const int8* Query = "INSERT INTO char_skills "
						"SET "
						"charid = %u,"
						"skillid = %u,"
						"value = %u,"
						"rank = %u "
						"ON DUPLICATE KEY UPDATE value = %u, rank = %u;";

						Sql_Query(SqlHandle, Query,
						PChar->id,
						i,
						5000,
						PChar->RealSkills.rank[i],
						5000,
						PChar->RealSkills.rank[i]);

		uint16 maxSkill = 10*battleutils::GetMaxSkill((SKILLTYPE)i, PChar->GetMJob(),PChar->GetMLevel());
		PChar->RealSkills.skill[i] = maxSkill; //set to capped
		PChar->WorkingSkills.skill[i] = maxSkill/10;
		PChar->WorkingSkills.skill[i] |= 0x8000; //set blue capped flag
	}
	charutils::CheckWeaponSkill(PChar, SKILL_NON);
	PChar->pushPacket(new CCharSkillsPacket(PChar));
	return 0;
}


//==========================================================//

inline int32 CLuaBaseEntity::messageSystem(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	uint16 messageID = (uint16)lua_tointeger(L,1);

	uint32 param0 = 0;
	uint32 param1 = 0;

	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		param0 = (uint32)lua_tointeger(L,2);
	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		param1 = (uint32)lua_tointeger(L,3);

	((CCharEntity*)m_PBaseEntity)->pushPacket(new CMessageSystemPacket(param0,param1,messageID));
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::createShop(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	((CCharEntity*)m_PBaseEntity)->Container->Clean();

	if( !lua_isnil(L,-1) && lua_isnumber(L,-1) )
	{
		((CCharEntity*)m_PBaseEntity)->Container->setType((uint8)lua_tointeger(L, -1));
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::addShopItem(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	uint16 itemID = (uint16)lua_tonumber(L,-2);
	uint32 price  = (uint32)lua_tonumber(L,-1);

	uint8 slotID = ((CCharEntity*)m_PBaseEntity)->Container->getItemsCount();

	if (slotID < 16)
	{
		((CCharEntity*)m_PBaseEntity)->Container->setItem(slotID, itemID, 0, price);
	}
    return 0;
}

/************************************************************************
*                                                                       *
*  Узнаем текущее значение известности персонажа                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getFame(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

    uint8  fameArea = (uint8)lua_tointeger(L, -1);
    uint16 fame     = 0;

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    switch (fameArea)
    {
        case 0: // San d'Oria
        case 1: // Bastock
        case 2: // Windurst
            fame = PChar->profile.fame[fameArea];
        break;
        case 3: // Jeuno
            fame = (PChar->profile.fame[0] + PChar->profile.fame[1] + PChar->profile.fame[2]) / 3;
        break;
        case 4: // Selbina / Rabao
            fame = (PChar->profile.fame[0] + PChar->profile.fame[1]) / 2;
        break;
        case 5: // Norg
            fame = PChar->profile.fame[3];
        break;
    }
    lua_pushinteger( L, fame);
    return 1;
}

/************************************************************************
*                                                                       *
*  Узнаем текущий уровень известности персонажа                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getFameLevel(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    this->getFame(L);

    uint16 fame = (uint16)lua_tointeger(L, -1);
    uint8  fameLevel = 1;

    if (fame >= 2450)
        fameLevel = 9;
    else if (fame >= 2200)
        fameLevel = 8;
    else if (fame >= 1950)
        fameLevel = 7;
    else if (fame >= 1700)
        fameLevel = 6;
    else if (fame >= 1300)
        fameLevel = 5;
    else if (fame >= 900)
        fameLevel = 4;
    else if (fame >= 500)
        fameLevel = 3;
    else if (fame >= 200)
        fameLevel = 2;

    lua_pushinteger(L, fameLevel);
    return 1;
}

/************************************************************************
*                                                                       *
*  Устанавливаем известность персонажу                                  *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setFame(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

    uint8  fameArea = (uint8) lua_tointeger(L,-2);
    uint16 fame     = (uint16)lua_tointeger(L,-1);

    switch(fameArea)
    {
        case 0: // San d'Oria
        case 1: // Bastock
        case 2: // Windurst
            ((CCharEntity*)m_PBaseEntity)->profile.fame[fameArea] = fame;
        break;
        case 3: // Jeuno
            ((CCharEntity*)m_PBaseEntity)->profile.fame[0] = fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[1] = fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[2] = fame;
        break;
        case 4: // Selbina / Rabao
            ((CCharEntity*)m_PBaseEntity)->profile.fame[0] = fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[1] = fame;
        break;
        case 5: // Norg
            ((CCharEntity*)m_PBaseEntity)->profile.fame[3] = fame;
        break;
    }
    charutils::SaveFame((CCharEntity*)m_PBaseEntity);
    return 0;
}

/************************************************************************
*                                                                       *
*  Добавляем известность персонажу                                      *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addFame(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

    uint8  fameArea = (uint8) lua_tointeger(L,-2);
    uint16 fame     = (uint16)lua_tointeger(L,-1);

    switch(fameArea)
    {
        case 0: // San d'Oria
        case 1: // Bastock
        case 2: // Windurst
            ((CCharEntity*)m_PBaseEntity)->profile.fame[fameArea] += fame;
        break;
        case 3: // Jeuno
            ((CCharEntity*)m_PBaseEntity)->profile.fame[0] += fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[1] += fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[2] += fame;
        break;
        case 4: // Selbina / Rabao
            ((CCharEntity*)m_PBaseEntity)->profile.fame[0] += fame;
            ((CCharEntity*)m_PBaseEntity)->profile.fame[1] += fame;
        break;
        case 5: // Norg
            ((CCharEntity*)m_PBaseEntity)->profile.fame[3] += fame;
        break;
    }
    charutils::SaveFame((CCharEntity*)m_PBaseEntity);
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getAnimation(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushinteger(L, m_PBaseEntity->animation);
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setAnimation(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint8 animation = (uint8)lua_tointeger(L, -1);

	if (m_PBaseEntity->animation != animation)
	{
		m_PBaseEntity->animation = animation;

		if (m_PBaseEntity->objtype == TYPE_PC)
		{
			((CCharEntity*)m_PBaseEntity)->pushPacket(new CCharUpdatePacket((CCharEntity*)m_PBaseEntity));
		} else {
            m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity,ENTITY_UPDATE));
		}
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  Получаем/устанавливаем скорость передвижения сущности                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::speed(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    if( !lua_isnil(L,-1) && lua_isnumber(L,-1) )
    {
        uint8 speed = (uint8)dsp_min(lua_tointeger(L,-1), 255);

        if (m_PBaseEntity->speed != speed)
		{
            m_PBaseEntity->speed = speed;

			if (m_PBaseEntity->objtype == TYPE_PC)
			{
			    ((CCharEntity*)m_PBaseEntity)->pushPacket(new CCharUpdatePacket((CCharEntity*)m_PBaseEntity));
            } else {
                m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_UPDATE));
            }
        }
        return 0;
	}
    lua_pushinteger(L, m_PBaseEntity->speed);
    return 1;
}

/************************************************************************
*                                                                       *
*  Получаем/устанавливаем значение дополнительной анимации              *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::AnimationSub(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    if( !lua_isnil(L,-1) && lua_isnumber(L,-1) )
    {
        uint8 animationsub = (uint8)lua_tointeger(L,-1);

        if (m_PBaseEntity->animationsub != animationsub)
		{
		    m_PBaseEntity->animationsub = animationsub;

			if (m_PBaseEntity->objtype == TYPE_PC)
			{
			    ((CCharEntity*)m_PBaseEntity)->pushPacket(new CCharUpdatePacket((CCharEntity*)m_PBaseEntity));
            } else {
                m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_UPDATE));
            }
        }
        return 0;
	}
	lua_pushinteger(L, m_PBaseEntity->animationsub);
    return 1;
}

/************************************************************************
*                                                                       *
*  Получаем/устанавливаем костюм персонажу                              *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::costume(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    if( !lua_isnil(L,-1) && lua_isnumber(L,-1) )
    {
        uint16 costum = (uint16)lua_tointeger(L,-1);

        if (PChar->m_Costum != costum &&
            PChar->status   != STATUS_SHUTDOWN &&
            PChar->status   != STATUS_DISAPPEAR)
		{
            PChar->m_Costum = costum;
            PChar->status   = STATUS_UPDATE;
            PChar->pushPacket(new CCharUpdatePacket(PChar));
        }
        return 0;
	}
    lua_pushinteger(L, PChar->m_Costum);
    return 1;
}

/************************************************************************
*                                                                       *
*  Проверяем, может ли персонаж использовать костюм                     *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::canUseCostume(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    if (((CCharEntity*)m_PBaseEntity)->m_Costum != 0)
    {
        lua_pushinteger(L, 445);
        return 1;
    }
    lua_pushinteger(L, (m_PBaseEntity->loc.zone->CanUseMisc(MISC_COSTUME) ? 0 : 316));
    return 1;
}

/************************************************************************
*                                                                       *
*  Проверяем, может ли персонаж использовать chocobo                    *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::canUseChocobo(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    if (m_PBaseEntity->animation == ANIMATION_CHOCOBO || !charutils::hasKeyItem((CCharEntity*)m_PBaseEntity, 138)) //keyitem CHOCOBO_LICENSE
    {
        lua_pushinteger(L, 445);
        return 1;
    }
    lua_pushinteger(L, (m_PBaseEntity->loc.zone->CanUseMisc(MISC_CHOCOBO) ? 0 : 316));
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::setStatus(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	m_PBaseEntity->status = (STATUSTYPE)lua_tointeger(L, 1);
	m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_UPDATE));
	return 0;
}

/************************************************************************
*                                                                       *
*  Разрещение атаковать этого персонажа другим персонажам               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setPVPFlag(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    ((CCharEntity*)m_PBaseEntity)->m_PVPFlag = 0x08;

	m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_UPDATE));
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::sendTractor(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-3) || !lua_isnumber(L,-3));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-4) || !lua_isnumber(L,-4));

	// недостаточно условий, tractor можно читать только на мертвую цель

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if(PChar->m_hasTractor == 0)
    {
		PChar->m_hasTractor = 1;

		PChar->m_StartActionPos.x = (float)lua_tonumber(L, -1);
		PChar->m_StartActionPos.y = (float)lua_tonumber(L, -2);
		PChar->m_StartActionPos.z = (float)lua_tonumber(L, -3);
		PChar->m_StartActionPos.rotation = (int8)lua_tonumber(L, -4);

		PChar->pushPacket(new CRaiseTractorMenuPacket(PChar, TYPE_TRACTOR));
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  Отправляем персонажу Raise меню                                      *
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::sendReraise(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint8 RaiseLevel = (uint8)lua_tonumber(L,1);

    if (RaiseLevel == 0 || RaiseLevel > 3)
    {
        ShowDebug(CL_CYAN"lua::sendRaise raise value is not valide!\n" CL_RESET);
    }
    else if(PChar->m_hasRaise == 0)
    {
        PChar->m_hasRaise = RaiseLevel;
    }
    return 0;
}

inline int32 CLuaBaseEntity::sendRaise(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint8 RaiseLevel = (uint8)lua_tonumber(L,1);

    if (RaiseLevel == 0 || RaiseLevel > 3)
    {
        ShowDebug(CL_CYAN"lua::sendRaise raise value is not valide!\n" CL_RESET);
    }
    else if(PChar->m_hasRaise == 0)
    {
        PChar->m_hasRaise = RaiseLevel;
        PChar->pushPacket(new CRaiseTractorMenuPacket(PChar, TYPE_RAISE));
    }
    return 0;
}

/************************************************************************
*                                                                       *
*  Добавляем боевой сущности StatusEffect                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,3) || !lua_isnumber(L,3));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,4) || !lua_isnumber(L,4));

    int32 n = lua_gettop(L);

    CStatusEffect * PEffect = new CStatusEffect(
        (EFFECT)lua_tointeger(L,1),
        (uint16)lua_tointeger(L,1),
        (uint16)lua_tointeger(L,2),
        (uint16)lua_tointeger(L,3),
        (uint16)lua_tointeger(L,4),
        (n >= 5 ? (uint16)lua_tointeger(L,5) : 0),
        (n >= 6 ? (uint16)lua_tointeger(L,6) : 0),
        (n >= 7 ? (uint16)lua_tointeger(L,7) : 0));

    lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->AddStatusEffect(PEffect));

	return 1;
}

/************************************************************************
*                                                                       *
*  Добавляем боевой сущности StatusEffect                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addStatusEffectEx(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,3) || !lua_isnumber(L,3));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,4) || !lua_isnumber(L,4));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,5) || !lua_isnumber(L,5));

    int32 n = lua_gettop(L);
	bool silent = false;
	if(lua_isboolean(L,-1))
	{
		silent = lua_toboolean(L,-1);
		n--;
	}

    CStatusEffect * PEffect = new CStatusEffect(
        (EFFECT)lua_tointeger(L,1),
        (uint16)lua_tointeger(L,2),
        (uint16)lua_tointeger(L,3),
        (uint16)lua_tointeger(L,4),
        (uint16)lua_tointeger(L,5),
        (n >= 6 ? (uint16)lua_tointeger(L,6) : 0),
        (n >= 7 ? (uint16)lua_tointeger(L,7) : 0),
        (n >= 8 ? (uint16)lua_tointeger(L,8) : 0));

    lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->AddStatusEffect(PEffect,silent));
	return 1;
}

/************************************************************************
*                                                                       *
*  Gets a party or alliance member relative to the player.    			*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getPartyMember(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,-2) || !lua_isnumber(L,-2));

	uint8 member        = (uint8)lua_tonumber(L,-1);
	uint8 allianceparty = (uint8)lua_tonumber(L,-2);

	CBattleEntity* PTargetChar = NULL;

	if(allianceparty == 0 && member == 0)
			PTargetChar =((CBattleEntity*)m_PBaseEntity);
	else if(((CBattleEntity*)m_PBaseEntity)->PParty != NULL)
	{
		if(allianceparty == 0 && member <= ((CBattleEntity*)m_PBaseEntity)->PParty->members.size())
			PTargetChar =((CBattleEntity*)m_PBaseEntity)->PParty->members[member];
		else if(((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance != NULL && member <= ((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance->partyList.at(allianceparty)->members.size())
			PTargetChar =((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance->partyList.at(allianceparty)->members[member];
	}

    if (PTargetChar != NULL)
	{
		lua_pushstring(L,CLuaBaseEntity::className);
		lua_gettable(L,LUA_GLOBALSINDEX);
		lua_pushstring(L,"new");
		lua_gettable(L,-2);
		lua_insert(L,-2);
		lua_pushlightuserdata(L,(void*)PTargetChar);
		lua_pcall(L,2,1,0);
		return 1;
	}
    ShowError(CL_RED"Lua::getPartyMember :: Member or Alliance Number is not valid.\n" CL_RESET);
	lua_pushnil(L);
	return 1;
}

inline int32 CLuaBaseEntity::getPartySize(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,-1) || !lua_isnumber(L,-1));

	uint8 allianceparty = (uint8)lua_tonumber(L,-1);
	uint8 partysize = 1;

	if( ((CBattleEntity*)m_PBaseEntity)->PParty != NULL)
	{
		if( allianceparty == 0)
			partysize = ((CBattleEntity*)m_PBaseEntity)->PParty->members.size();
		else if( ((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance != NULL)
			partysize = ((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance->partyList.at(allianceparty)->members.size();
	}

	lua_pushnumber( L,partysize );
	return 1;
}

inline int32 CLuaBaseEntity::getAllianceSize(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    uint8 alliancesize = 1;

    if( ((CBattleEntity*)m_PBaseEntity)->PParty != NULL)
    {
	    if( ((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance != NULL)
		    alliancesize = ((CBattleEntity*)m_PBaseEntity)->PParty->m_PAlliance->partyList.size();
    }
    lua_pushnumber( L,alliancesize );
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::addPartyEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,3) || !lua_isnumber(L,3));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,4) || !lua_isnumber(L,4));

	int32 n = lua_gettop(L);

	CStatusEffect * PEffect = new CStatusEffect(
		(EFFECT)lua_tointeger(L,1),
		(uint16)lua_tointeger(L,2),
		(uint16)lua_tointeger(L,3),
		(uint16)lua_tointeger(L,4),
		(n >= 5 ? (uint16)lua_tointeger(L,5) : 0),
        (n >= 6 ? (uint16)lua_tointeger(L,6) : 0),
        (n >= 7 ? (uint16)lua_tointeger(L,7) : 0));

	CCharEntity* PChar = ((CCharEntity*)m_PBaseEntity);

    if (PChar->PParty != NULL)
    {
	    for (int i=0; i< PChar->PParty->members.size(); ++i)
	    {
		    if (PChar->PParty->members[i]->loc.zone == PChar->loc.zone)
		    {
			    PChar->PParty->members[i]->StatusEffectContainer->AddStatusEffect(PEffect);
		    }
	    }
    }
	return 0;
}

/************************************************************************
*                                                                       *
*  Получаем указатель на эффект по имени                                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint8 n = lua_gettop(L);

    CStatusEffect* PStatusEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->GetStatusEffect(
        (EFFECT)lua_tointeger(L,1),
        (n >= 2) ? (uint16)lua_tointeger(L,2) : 0);

    if (PStatusEffect == NULL)
    {
        lua_pushnil(L);
    }
    else
    {
        lua_pop(L,1);
        lua_pushstring(L, CLuaStatusEffect::className);
        lua_gettable(L,LUA_GLOBALSINDEX);
        lua_pushstring(L,"new");
        lua_gettable(L,-2);
        lua_insert(L,-2);
        lua_pushlightuserdata(L,(void*)PStatusEffect);

        if( lua_pcall(L,2,1,0) )
        {
            return 0;
        }
    }
    return 1;
}

/************************************************************************
*                                                                       *
*  Проверяем наличие статус-эффекта в контейнере                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::hasStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    bool hasEffect = false;

    if (lua_gettop(L) >= 2)
    {
        hasEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->HasStatusEffect(
            (EFFECT)lua_tointeger(L,1),
            (uint16)lua_tointeger(L,2));
    } else {
        hasEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->HasStatusEffect(
            (EFFECT)lua_tointeger(L,1));
    }
    lua_pushboolean(L, hasEffect);
    return 1;
}

inline int32 CLuaBaseEntity::hasBustEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    bool hasEffect = false;

    hasEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->HasBustEffect(
        (EFFECT)lua_tointeger(L,1));

    lua_pushboolean(L, hasEffect);
    return 1;
}

inline int32 CLuaBaseEntity::canGainStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    bool hasEffect = false;

    hasEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->CanGainStatusEffect(
            (EFFECT)lua_tointeger(L,1),
            (uint16)lua_tointeger(L,2));

    lua_pushboolean(L, hasEffect);
    return 1;
}

/************************************************************************
*																		*
*  Удаляем статус-эффект по его основному и дополнительному типам.		*
*  Возвращаем результат выполнения операции.							*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::delStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    bool result = false;

    if( !lua_isnil(L,1) && lua_isnumber(L,1) )
    {
        if(lua_gettop(L) >= 2)
        {
            /* Delete matching status effect with matching power */
            result = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->DelStatusEffect(
                        (EFFECT)lua_tointeger(L,1),
                        (uint16)lua_tointeger(L,2));
        }
        else
        {
            /* Delete matching status effect any power */
            result = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->DelStatusEffect((EFFECT)lua_tointeger(L,1));
        }
    }

    lua_pushboolean(L, result);
    return 1;
}

inline int32 CLuaBaseEntity::delStatusEffectSilent(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    bool result = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->DelStatusEffectSilent((EFFECT)lua_tointeger(L,1));

    lua_pushboolean(L, result);
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::removePartyEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	int32 n = lua_gettop(L);

    CCharEntity* PChar = ((CCharEntity*)m_PBaseEntity);

	for (int i=0; i< PChar->PParty->members.size(); ++i)
	{
		if (PChar->PParty->members[i]->loc.zone == PChar->loc.zone)
		{
			PChar->PParty->members[i]->StatusEffectContainer->DelStatusEffect((EFFECT)lua_tointeger(L,1));
		}
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::hasPartyEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = ((CCharEntity*)m_PBaseEntity);

	if (PChar->PParty != NULL)
	{
	    for (int i=0; i< PChar->PParty->members.size(); ++i)
	    {
		    if (PChar->PParty->members[i]->loc.zone == PChar->loc.zone)
		    {
			    if (PChar->PParty->members[i]->StatusEffectContainer->HasStatusEffect((EFFECT)lua_tointeger(L,1)))
                {
                    lua_pushboolean(L, true);
	                return 1;
                }
		    }
	    }
    }
    lua_pushboolean(L, false);
	return 1;
}

/************************************************************************
*                                                                       *
*  Adds corsair roll effect				                                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addCorsairRoll(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,3) || !lua_isnumber(L,3));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,4) || !lua_isnumber(L,4));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,5) || !lua_isnumber(L,5));

    int32 n = lua_gettop(L);

	uint8 casterJob = lua_tointeger(L, 1);
	uint8 bustDuration = lua_tointeger(L, 2);

    CStatusEffect * PEffect = new CStatusEffect(
        (EFFECT)lua_tointeger(L,3),
        (uint16)lua_tointeger(L,3),
        (uint16)lua_tointeger(L,4),
        (uint16)lua_tointeger(L,5),
        (uint16)lua_tointeger(L,6),
        (n >= 7 ? (uint16)lua_tointeger(L,7) : 0),
        (n >= 8 ? (uint16)lua_tointeger(L,8) : 0),
        (n >= 9 ? (uint16)lua_tointeger(L,9) : 0));
	uint8 maxRolls = 2;
	if (casterJob != JOB_COR)
	{
		maxRolls = 1;
	}
    lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->ApplyCorsairEffect(PEffect, maxRolls, bustDuration));
	return 1;
}

inline int32 CLuaBaseEntity::hasPartyJob(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	uint8 job = lua_tointeger(L, 1);

	if (((CCharEntity*)m_PBaseEntity)->PParty != NULL)
	{
		for (uint32 i = 0; i < ((CCharEntity*)m_PBaseEntity)->PParty->members.size(); i++)
		{
			CCharEntity* PTarget = (CCharEntity*)((CCharEntity*)m_PBaseEntity)->PParty->members[i];
			if (PTarget->GetMJob() == job)
			{
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}


/************************************************************************
*                                                                       *
*  Удаляем первый отрицательный эффект                                  *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::eraseStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->EraseStatusEffect());
    return 1;
}

/************************************************************************
*                                                                       *
*                             *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::dispelAllStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->DispelAllStatusEffect());
    return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::eraseAllStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->EraseAllStatusEffect());
    return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getStatusEffectElement(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	uint16 statusId = lua_tointeger(L,1);

	lua_pushinteger( L, effects::GetEffectElement(statusId));
	return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::stealStatusEffect(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    CStatusEffect* PStatusEffect = ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->StealStatusEffect();

    if (PStatusEffect == NULL)
        lua_pushnil(L);
    else
    {
        lua_pushstring(L, CLuaStatusEffect::className);
        lua_gettable(L,LUA_GLOBALSINDEX);
        lua_pushstring(L,"new");
        lua_gettable(L,-2);
        lua_insert(L,-2);
        lua_pushlightuserdata(L,(void*)PStatusEffect);
		lua_pcall(L,2,1,0);

        delete PStatusEffect;
    }
    return 1;
}

/************************************************************************
*                                                                       *
*  Удаляем первый положительный эффект                                  *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::dispelStatusEffect(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->StatusEffectContainer->DispelStatusEffect());
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::addMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    ((CBattleEntity*)m_PBaseEntity)->addModifier(
		lua_tointeger(L,1),
        lua_tointeger(L,2));
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    lua_pushinteger(L,((CBattleEntity*)m_PBaseEntity)->getMod(lua_tointeger(L,1)));
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::delMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    ((CBattleEntity*)m_PBaseEntity)->delModifier(
		lua_tointeger(L,1),
        lua_tointeger(L,2));
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::setMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    ((CBattleEntity*)m_PBaseEntity)->setModifier(
		lua_tointeger(L,1),
        lua_tointeger(L,2));
    return 0;
}

inline int32 CLuaBaseEntity::getMobMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(!(m_PBaseEntity->objtype & TYPE_MOB));

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    lua_pushinteger(L,((CMobEntity*)m_PBaseEntity)->getMobMod(lua_tointeger(L,1)));
    return 1;
}

inline int32 CLuaBaseEntity::setMobMod(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    // putting this in here to find elusive bug
    if(!(m_PBaseEntity->objtype & TYPE_MOB))
    {
    	// this once broke on an entity (17532673) but it could not be found
    	ShowError("CLuaBaseEntity::setMobMod Expected type mob (%d) but its a (%d)\n", m_PBaseEntity->id, m_PBaseEntity->objtype);
    	return 0;
    }

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    ((CMobEntity*)m_PBaseEntity)->setMobMod(
		lua_tointeger(L,1),
        lua_tointeger(L,2));
    return 0;
}

/************************************************************************
*                                                                       *
*  Добавляем очки опыта персонажу                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addExp(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    charutils::AddExperiencePoints(false, (CCharEntity*)m_PBaseEntity, m_PBaseEntity, (uint32)lua_tointeger(L,1),0, false);
    return 0;
}


/************************************************************************
*                                                                       *
*  GM command @changeJOB !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::changeJob(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
    JOBTYPE prevjob = PChar->GetMJob();

	PChar->resetPetZoningInfo();

	PChar->jobs.unlocked |= (1 << (uint8)lua_tointeger(L,1));
	PChar->SetMJob((uint8)lua_tointeger(L,1));

    if (lua_tointeger(L,1) == JOB_BLU)
    {
        if (prevjob != JOB_BLU)
        {
            blueutils::LoadSetSpells(PChar);
        }
    }
    else if (PChar->GetSJob() != JOB_BLU)
    {
        blueutils::UnequipAllBlueSpells(PChar);
    }
    charutils::BuildingCharSkillsTable(PChar);
	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
    PChar->PRecastContainer->ResetAbilities();
    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

    charutils::SaveCharStats(PChar);
    charutils::SaveCharJob(PChar, PChar->GetMJob());
    charutils::SaveCharExp(PChar, PChar->GetMJob());
	charutils::SaveCharPoints(PChar);
	charutils::UpdateHealth(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
    PChar->pushPacket(new CMenuMeritPacket(PChar));
    PChar->pushPacket(new CCharSyncPacket(PChar));
	return 0;
}


/************************************************************************
*                                                                       *
*  GM command @changesJOB !!! FOR DEBUG ONLY !!!                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::changesJob(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    PChar->jobs.unlocked |= (1 << (uint8)lua_tointeger(L,1));
    PChar->SetSJob((uint8)lua_tointeger(L,1));

	charutils::UpdateSubJob(PChar);

    if (lua_tointeger(L,1) == JOB_BLU)
    {
        blueutils::LoadSetSpells(PChar);
    }
    else
    {
        blueutils::UnequipAllBlueSpells(PChar);
    }

	return 0;
}



/************************************************************************
*                                                                       *
*  GM command @setslevel !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setsLevel(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
	DSP_DEBUG_BREAK_IF(lua_tointeger(L,1) > 99);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	//PChar->jobs.exp[PChar->GetSJob()] = 0;
	PChar->jobs.job[PChar->GetSJob()] = (uint8)lua_tointeger(L,1);
	PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);


    charutils::BuildingCharSkillsTable(PChar);
	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

    charutils::SaveCharStats(PChar);
    charutils::SaveCharJob(PChar, PChar->GetMJob());
    charutils::SaveCharExp(PChar, PChar->GetMJob());

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
    PChar->pushPacket(new CMenuMeritPacket(PChar));
    PChar->pushPacket(new CCharSyncPacket(PChar));
    return 0;
}


/************************************************************************
*                                                                       *
*  GM command @setlevel !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setLevel(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
	DSP_DEBUG_BREAK_IF(lua_tointeger(L,1) > 99);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->SetMLevel((uint8)lua_tointeger(L,1));
	PChar->jobs.exp[PChar->GetMJob()] = 0;
	PChar->jobs.job[PChar->GetMJob()] = (uint8)lua_tointeger(L,1);
	PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

    blueutils::ValidateBlueSpells(PChar);

	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
    charutils::BuildingCharSkillsTable(PChar);
    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

    charutils::SaveCharStats(PChar);
    charutils::SaveCharJob(PChar, PChar->GetMJob());
    charutils::SaveCharExp(PChar, PChar->GetMJob());
	charutils::SaveCharPoints(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
    PChar->pushPacket(new CMenuMeritPacket(PChar));
    PChar->pushPacket(new CCharSyncPacket(PChar));
    return 0;
}

/************************************************************************
*                                                                       *
*  GM command @setMerits !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setMerits(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->PMeritPoints->SetMeritPoints((uint8)lua_tointeger(L,1));

    PChar->pushPacket(new CMenuMeritPacket(PChar));

    charutils::SaveCharExp(PChar, PChar->GetMJob());
	return 0;
}

/************************************************************************
*                                                                       *
*  gets Merit levels for merit type				                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getMerit(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    if(m_PBaseEntity->objtype != TYPE_PC){
    	// not PC just give em max merits
    	lua_pushinteger(L, 5);
    } else {
		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

		lua_pushinteger(L, PChar->PMeritPoints->GetMeritValue((MERIT_TYPE)lua_tointeger(L,1), PChar));
	}

	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::showPosition(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    ((CCharEntity*)m_PBaseEntity)->pushPacket(new CMessageStandardPacket(
		m_PBaseEntity->loc.p.x,
		m_PBaseEntity->loc.p.y,
		m_PBaseEntity->loc.p.z,
		m_PBaseEntity->loc.p.rotation,
        239));
    return 0;
}

/************************************************************************
*                                                                       *
*  Устанавливаем персонажу указанный флаг                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::setFlag(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    ((CCharEntity*)m_PBaseEntity)->nameflags.flags ^= (uint32)lua_tointeger(L,1);
    ((CCharEntity*)m_PBaseEntity)->pushPacket(new CCharUpdatePacket((CCharEntity*)m_PBaseEntity));
    return 0;
}

/************************************************************************
*                                                                       *
*  Устанавливаем/запрашиваем флаг выхода из MogHouse                    *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::moghouseFlag(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    if (!lua_isnil(L,1) && lua_isnumber(L,1))
    {
        PChar->profile.mhflag |= (uint8)lua_tointeger(L,1);
        charutils::SaveCharStats(PChar);
        return 0;
    }
    lua_pushinteger(L, PChar->profile.mhflag);
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::injectPacket(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1));

	uint8 size = 0;
	FILE* File = fopen(lua_tostring(L,1),"rb");

	if (File)
	{
		CBasicPacket * PPacket = new CBasicPacket();

		fseek(File,1,SEEK_SET);
		uint16 returnSize = fread(&size,1,1,File);

		if (size <= 128)
		{
			fseek(File,0,SEEK_SET);
			uint16 read_elements = fread(PPacket,1,size*2,File);
			fclose(File);

			((CCharEntity*)m_PBaseEntity)->pushPacket(PPacket);
		}
	}else{
		ShowError(CL_RED"CLuaBaseEntity::injectPacket : Cannot open file\n" CL_RESET);
	}
	return 0;
}

/************************************************************************
*																		*
*  Returns the ItemId of the equipped item in the associated slot		*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::getEquipID(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC && m_PBaseEntity->objtype != TYPE_PET && m_PBaseEntity->objtype != TYPE_MOB);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	if(m_PBaseEntity->objtype == TYPE_PC)
	{
		uint8 SLOT = (uint8)lua_tointeger(L,1);

		DSP_DEBUG_BREAK_IF(SLOT > 15);

		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

		CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(PChar->equip[SLOT]);

		if((PItem != NULL) && PItem->isType(ITEM_ARMOR))
		{
			lua_pushinteger(L,PItem->getID());
			return 1;
		}
	}
	lua_pushinteger(L,0);
	return 1;
}

inline int32 CLuaBaseEntity::getShieldSize(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC && m_PBaseEntity->objtype != TYPE_PET);

	if(m_PBaseEntity->objtype == TYPE_PC)
	{
		lua_pushinteger(L,((CCharEntity*)m_PBaseEntity)->getShieldSize());
		return 1;
	}
	lua_pushinteger(L,0);
	return 1;
}

/*
Pass in an item id and it will search and equip it.

Example:
	player:equipItem(itemId)
*/
inline int32 CLuaBaseEntity::equipItem(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    uint16 itemID = (uint16)lua_tointeger(L,1);
    uint8 SLOT = PChar->getStorage(LOC_INVENTORY)->SearchItem(itemID);
    CItemArmor* PItem;

    if(SLOT != ERROR_SLOTID){
	    PItem = (CItemArmor*)PChar->getStorage(LOC_INVENTORY)->GetItem(SLOT);
	    charutils::EquipItem(PChar, SLOT, PItem->getSlotType());
		charutils::SaveCharEquip(PChar);
	}
    return 0;
}

/************************************************************************
*                                                                       *
*  блокируем ячейку экипировки                                          *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::lockEquipSlot(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint8 SLOT = (uint8)lua_tointeger(L,1);

    DSP_DEBUG_BREAK_IF(SLOT > 15);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    charutils::EquipItem(PChar, 0, SLOT);

    PChar->m_EquipBlock |= 1 << SLOT;
    PChar->pushPacket(new CCharAppearancePacket(PChar));
    PChar->pushPacket(new CEquipPacket(0, SLOT));
    PChar->pushPacket(new CCharJobsPacket(PChar));

    if (PChar->status == STATUS_NORMAL) PChar->status = STATUS_UPDATE;
    return 0;
}

/************************************************************************
*                                                                       *
*  Cнимаем блокировку с ячейки экипировки                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::unlockEquipSlot(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint8 SLOT = (uint8)lua_tointeger(L,1);

    DSP_DEBUG_BREAK_IF(SLOT > 15);

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    PChar->m_EquipBlock &= ~(1 << SLOT);
    PChar->pushPacket(new CCharJobsPacket(PChar));

    if (PChar->status == STATUS_NORMAL) PChar->status = STATUS_UPDATE;
    return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::canEquipItem(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    uint16 itemID = (uint16)lua_tointeger(L,1);
	DSP_DEBUG_BREAK_IF(itemID > MAX_ITEMID);

	bool checkLevel = false;
    if(!lua_isnil(L,2) && lua_isboolean(L,2))
		checkLevel = lua_toboolean(L,2);

	CItemArmor* PItem = (CItemArmor*)itemutils::GetItem(itemID);
	CBattleEntity* PChar = (CBattleEntity*)m_PBaseEntity;

    if (!(PItem->getJobs() & (1 << (PChar->GetMJob() -1))))
	{
		lua_pushboolean(L, false);
		return 1;
	}
	if(checkLevel && (PItem->getReqLvl() > PChar->GetMLevel()))
	{
		lua_pushboolean(L, false);
		return 1;
	}
	//ShowDebug("Item ID: %u Item Jobs: %u Player Job: %u\n",itemID,PItem->getJobs(),PChar->GetMJob());
    lua_pushboolean(L, true);
    return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getPetElement(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	if(((CBattleEntity*)m_PBaseEntity)->PPet){
		lua_pushinteger(L, ((CPetEntity*)((CBattleEntity*)m_PBaseEntity)->PPet)->m_Element);
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getPetName(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	if(((CBattleEntity*)m_PBaseEntity)->PPet){
		lua_pushstring(L, (((CBattleEntity*)m_PBaseEntity)->PPet)->name.c_str());
    } else {
        lua_pushstring(L, "");
	}
	return 1;
}

/*
Checks if the current entity has an alive pet.
*/
inline int32 CLuaBaseEntity::hasPet(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	CBattleEntity* PTarget = (CBattleEntity*)m_PBaseEntity;

	lua_pushboolean(L, (PTarget->PPet != NULL && PTarget->PPet->status != STATUS_DISAPPEAR));
	return 1;
}

/************************************************************************
*																		*
*  Сущность призывает питомца											*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::spawnPet(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	if ( m_PBaseEntity->objtype == TYPE_PC )
	{
		if( !lua_isnil(L,1) && lua_isstring(L,1) )
		{
			petutils::SpawnPet((CBattleEntity*)m_PBaseEntity, lua_tointeger(L,1), false);
		}
		else
		{
			ShowError(CL_RED"CLuaBaseEntity::spawnPet : PetID is NULL\n" CL_RESET);
		}
	}
	else if( m_PBaseEntity->objtype == TYPE_MOB)
	{
		CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

		if(PMob->PPet == NULL)
		{
			ShowError("lua_baseentity::spawnPet PMob (%d) trying to spawn pet but its NULL\n", PMob->id);
			return 0;
		}

		CMobEntity* PPet = (CMobEntity*)PMob->PPet;

		// if a number is given its an avatar or elemental spawn
		if( !lua_isnil(L,1) && lua_isstring(L,1) )
		{
			petutils::SpawnMobPet(PMob, lua_tointeger(L,1));
		}

		// always spawn on master
		PPet->m_SpawnPoint = nearPosition(PMob->loc.p, 2.2f, M_PI);

		// setup AI
		PPet->PBattleAI->SetCurrentAction(ACTION_SPAWN);

	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::petAttack(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);
	if(((CBattleEntity*)m_PBaseEntity)->PPet!=NULL){
		petutils::AttackTarget((CBattleEntity*)m_PBaseEntity,(CBattleEntity*)PEntity->GetBaseEntity());
	}
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::petRetreat(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    if(((CBattleEntity*)m_PBaseEntity)->PPet!=NULL){
	    petutils::RetreatToMaster((CBattleEntity*)m_PBaseEntity);
    }
    return 0;
}


//==========================================================//

inline int32 CLuaBaseEntity::petAbility(lua_State *L)
{
	return 0;
}

/************************************************************************
*																		*
*  Сущность освобождает питомца											*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::despawnPet(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    if(((CBattleEntity*)m_PBaseEntity)->PPet!=NULL){
	    petutils::DespawnPet((CBattleEntity*)m_PBaseEntity);
    }
	return 0;
}

/************************************************************************
*                                                                       *
*  Обновляем ненависть монстров к указанной цели                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::updateEnmity(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

  //DSP_DEBUG_BREAK_IF(lua_gettop(L) > 1);
    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

    CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);

    if (PEntity != NULL &&
        PEntity->GetBaseEntity()->objtype != TYPE_NPC)
    {
        ((CMobEntity*)m_PBaseEntity)->PEnmityContainer->AddBaseEnmity((CBattleEntity*)PEntity->GetBaseEntity());
    }
    return 0;
}

/************************************************************************
    Resets all enmity of player on mob.
    Example:
    mob:resetEnmity(target)
************************************************************************/

inline int32 CLuaBaseEntity::resetEnmity(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

  //DSP_DEBUG_BREAK_IF(lua_gettop(L) > 1);
    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

    CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);

    if (PEntity != NULL &&
        PEntity->GetBaseEntity()->objtype != TYPE_NPC)
    {
        ((CMobEntity*)m_PBaseEntity)->PEnmityContainer->LowerEnmityByPercent((CBattleEntity*)PEntity->GetBaseEntity(), 100, NULL);
    }
    return 0;
}

/************************************************************************
    Check if the mob has immunity for this type of spell
    list at mobentity.h
    return true if he has immunity, else false
************************************************************************/

inline int32 CLuaBaseEntity::hasImmunity(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->hasImmunity((uint32)lua_tointeger(L,1)));
    return 1;
}

/************************************************************************
*                                                                       *
*  Get the time in second of the battle                                 *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getBattleTime(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushinteger(L, ((CBattleEntity*)m_PBaseEntity)->PBattleAI->GetBattleTime());
    return 1;
}

/************************************************************************
*                                                                       *
*  Add the rage mode for a mob (stat x10)                               *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::rageMode(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	((CMobEntity*)m_PBaseEntity)->addRageMode();
	return 0;
}

/************************************************************************
*																		*
*  Check if the mob is an undead										*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::isUndead(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->m_EcoSystem == SYSTEM_UNDEAD);
	return 1;
}

inline int32 CLuaBaseEntity::getSystem(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	uint8 system = ((CMobEntity*)m_PBaseEntity)->m_EcoSystem;

	lua_pushinteger(L, system);
	return 1;
}

inline int32 CLuaBaseEntity::getFamily(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	uint16 family = ((CMobEntity*)m_PBaseEntity)->m_Family;

	lua_pushinteger(L, family);
	return 1;
}

 /************************************************************************
*                                                                      *
*  Returns true if mob is of passed in type                                *
*                                                                      *
************************************************************************/

inline int32 CLuaBaseEntity::isMobType(lua_State *L)
{
   DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

   if(m_PBaseEntity->objtype != TYPE_MOB)
   {
   	lua_pushboolean(L, false);
   	return 1;
   }

   DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

   CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

   lua_pushboolean(L, PMob->m_Type & lua_tointeger(L,1));
   return 1;
}

/************************************************************************
*	Change skin of a mob												*
*  	1st number: skinid in mob_change_skin.sql							*
*	mob:changeSkin(1)						*
************************************************************************/

inline int32 CLuaBaseEntity::changeSkin(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_MOB);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	PMob->SetNewSkin(lua_tointeger(L,1));

	PMob->loc.zone->PushPacket(PMob, CHAR_INRANGE, new CEntityUpdatePacket(PMob,ENTITY_UPDATE));

	return 0;
}

/************************************************************************
*  mob:getSkinID()                                                      *
*  Get the last Skin of your mob (0 for base)                           *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getSkinID(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

    lua_pushinteger(L, ((CMobEntity*)m_PBaseEntity)->GetSkinID());
    return 1;
}

/************************************************************************
			Calculates the enmity produced by the input cure and
			applies it to all on the base entity's enemies hate list
			FORMAT: phealer:(ptarget,amount)
************************************************************************/

inline int32 CLuaBaseEntity::updateEnmityFromCure(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
	DSP_DEBUG_BREAK_IF(lua_tointeger(L,2) < 0);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);
	uint32 amount = lua_tointeger(L,2);

    if (PEntity != NULL &&
        PEntity->GetBaseEntity()->objtype == TYPE_PC)
	{
		battleutils::GenerateCureEnmity((CBattleEntity*)m_PBaseEntity, (CBattleEntity*)PEntity->GetBaseEntity(), amount);
	}

	return 0;
}

/************************************************************************
*                                                                       *
*  Calculates the enmity produced by the input damage                   *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::updateEnmityFromDamage(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	// TODO: Scripters should check if the target is a monster before calling this, but for now lets do this and
	// catch it further down.
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB && m_PBaseEntity->objtype != TYPE_PC && m_PBaseEntity->objtype != TYPE_PET );
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));
	DSP_DEBUG_BREAK_IF(lua_tointeger(L,2) < 0);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	if (m_PBaseEntity->objtype == TYPE_PC || m_PBaseEntity->objtype == TYPE_PET) {
		return 0;
	}

	CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);
	uint32 damage = lua_tointeger(L,2);

    if (PEntity != NULL &&
        PEntity->GetBaseEntity()->objtype != TYPE_NPC)
	{
		((CMobEntity*)m_PBaseEntity)->PEnmityContainer->UpdateEnmityFromDamage((CBattleEntity*)PEntity->GetBaseEntity(),damage);
	}

	return 0;
}

/************************************************************************
*																		*
*  Проверяем, покидал ли персонаж зону после поднятия флага				*
*  необходимости ее покинуть. С параметром устанавливаем флаг, без		*
*  параметра узнаем результат.											*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::needToZone(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	if (!lua_isnil(L,-1) && lua_isboolean(L,-1))
	{
        m_PBaseEntity->loc.zoning = lua_toboolean(L,-1);
		return 0;
	}
    lua_pushboolean( L, m_PBaseEntity->loc.zoning );
	return 1;
}

/************************************************************************
*																		*
*	Get Container Size													*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::getContainerSize(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CCharEntity* PChar = ((CCharEntity*)m_PBaseEntity);
    lua_pushinteger(L, PChar->getStorage(lua_tointeger(L,1))->GetSize());
	return 1;
}

/************************************************************************
*                                                                       *
*  Increase/Decrease Container Size                                     *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::changeContainerSize(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	if( !lua_isnil(L,1) && lua_isnumber(L,1) &&
		!lua_isnil(L,2) && lua_isnumber(L,2))
	{
        uint8 LocationID = (uint8)lua_tointeger(L,1);

        if (LocationID < MAX_CONTAINER_ID)
        {
		    CCharEntity* PChar = ((CCharEntity*)m_PBaseEntity);

		    PChar->getStorage(LocationID)->AddBuff(lua_tointeger(L,2));
		    PChar->pushPacket(new CInventorySizePacket(PChar));
		    charutils::SaveCharInventoryCapacity(PChar);
        }
        else
        {
            ShowError(CL_RED"CLuaBaseEntity::changeContainerSize: bad container id (%u)\n", LocationID);
        }
	}
	return 0;
}

/************************************************************************
*                                                                       *
*  Get Entity's id                                                      *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getID(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushinteger( L, m_PBaseEntity->id );
	return 1;
}

// TODO: should be renamed to targid
inline int32 CLuaBaseEntity::getShortID(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushinteger( L, m_PBaseEntity->targid );
	return 1;
}

/************************************************************************
*                                                                       *
*  Get Entity's name                                                    *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getName(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushstring( L, m_PBaseEntity->GetName() );
	return 1;
}

/************************************************************************
*                                                                       *
*  Gets the current weapon's base DMG; used for WS calcs                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getWeaponDmg(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weapondam = ((CBattleEntity*)m_PBaseEntity)->GetMainWeaponDmg();

	lua_pushinteger( L, weapondam );
	return 1;
}


/************************************************************************
*                                                                       *
*  Gets the current offhand weapon's base DMG; used for WS calcs        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getOffhandDmg(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weapondam = ((CBattleEntity*)m_PBaseEntity)->GetSubWeaponDmg();

	lua_pushinteger( L, weapondam );
	return 1;
}

/************************************************************************
*                                                                       *
*  Gets the current weapon's base DMG; used for WS calcs                *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getWeaponDmgRank(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weapondam = ((CBattleEntity*)m_PBaseEntity)->GetMainWeaponRank() * 9;

	lua_pushinteger( L, weapondam );
	return 1;
}


/************************************************************************
*                                                                       *
*  Gets the current offhand weapon's base DMG; used for WS calcs        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getOffhandDmgRank(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weapondam = ((CBattleEntity*)m_PBaseEntity)->GetSubWeaponRank() * 9;

	lua_pushinteger( L, weapondam );
	return 1;
}

/************************************************************************
*                                                                       *
*  Gets the skill type of weapon in slot								*
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::getWeaponSkillType(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
	{
		uint8 SLOT = (uint8)lua_tointeger(L,1);
		if (SLOT > 3)
		{
			lua_pushinteger(L,0);
			return 1;
		}
		CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT];
		if(weapon == NULL)
        {
            lua_pushinteger(L,0);
			return 1;
		}
		lua_pushinteger( L, weapon->getSkillType() );
		return 1;
	}
	ShowError(CL_RED"lua::getWeaponSkillType :: Invalid slot specified!" CL_RESET);
	return 0;
}

/************************************************************************
*                                                                       *
*  Gets the subskill type of weapon in slot								*
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::getWeaponSubSkillType(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	if( !lua_isnil(L,1) && lua_isstring(L,1) )
	{
		uint8 SLOT = (uint8)lua_tointeger(L,1);
		if (SLOT > 3)
		{
			ShowDebug(CL_CYAN"lua::getWeaponSubskillType slot not a weapon\n" CL_RESET);
			lua_pushinteger(L,0);
			return 1;
		}
		CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT];

		if(weapon == NULL)
		{
		    ShowDebug(CL_CYAN"lua::getWeaponSubskillType weapon in specified slot is null!\n" CL_RESET);
			return 0;
		}

		lua_pushinteger( L, weapon->getSubSkillType() );
		return 1;
	}
	ShowError(CL_RED"lua::getWeaponSubskillType :: Invalid slot specified!" CL_RESET);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getRangedDmg(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weapondam = ((CBattleEntity*)m_PBaseEntity)->GetRangedWeaponDmg();

	lua_pushinteger( L, weapondam);
	return 1;
}

inline int32 CLuaBaseEntity::getRangedDmgForRank(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint16 weaponrank = ((CBattleEntity*)m_PBaseEntity)->GetRangedWeaponRank() * 9;

	lua_pushinteger( L, weaponrank);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getAmmoDmg(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT_AMMO];

	if(weapon == NULL)
	{
	    ShowDebug(CL_CYAN"lua::getAmmoDmg weapon in ammo slot is null!\n" CL_RESET);
		return 0;
    }
	lua_pushinteger( L, weapon->getDamage());
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getRATT(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT_RANGED];

	if(weapon == NULL)
	{
	    ShowDebug(CL_CYAN"lua::getRATT weapon in ranged slot is null!\n" CL_RESET);
		return 0;
    }

	lua_pushinteger( L, ((CBattleEntity*)m_PBaseEntity)->RATT(weapon->getSkillType()));
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getRACC(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT_RANGED];

	if(weapon == NULL)
	{
	    ShowDebug(CL_CYAN"lua::getRACC weapon in ranged slot is null!\n" CL_RESET);
		return 0;
    }
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	int skill = PChar->GetSkill(weapon->getSkillType());
	int acc = skill;
	if(skill>200){ acc = 200 + (skill-200)*0.9;}
	acc += PChar->getMod(MOD_RACC);
	acc += PChar->AGI()/2;
	acc = ((100 +  PChar->getMod(MOD_RACCP)) * acc)/100 +
		dsp_min(((100 +  PChar->getMod(MOD_FOOD_RACCP)) * acc)/100,  PChar->getMod(MOD_FOOD_RACC_CAP));

	lua_pushinteger( L, acc);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getACC(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	uint8 slot = SLOT_MAIN;
	uint8 offsetAccuracy = 0;
	// if((L,1) == 1){
	//	slot = SLOT_SUB;
	//}
	if((L,2) > 0){
		offsetAccuracy = (L,2);
	}

	CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;
	uint16 ACC = PEntity->ACC(slot,offsetAccuracy);

	lua_pushinteger(L,ACC);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getEVA(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;
	uint16 EVA = PEntity->EVA();

	lua_pushinteger(L,EVA);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::isWeaponTwoHanded(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	CItemWeapon* weapon = ((CBattleEntity*)m_PBaseEntity)->m_Weapons[SLOT_MAIN];

	if(weapon == NULL)
	{
	    ShowDebug(CL_CYAN"lua::getWeaponDmg weapon in main slot is null!\n" CL_RESET);
		return 0;
    }
	lua_pushboolean( L, weapon->isTwoHanded() );
	return 1;
}

/************************************************************************
	Returns the value of a single hit against the target monster
	(can return -1 if it misses). Useful for calculating physical
	BP damage, but it isn't limited to just pets. Usage:
	damage = attacker:getMeleeHitDamage(target,acc);
	the acc value is OPTIONAL and it should be an int (e.g. 95 = 95% acc)
************************************************************************/

inline int32 CLuaBaseEntity::getMeleeHitDamage(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);

	CBattleEntity* PAttacker = (CBattleEntity*)m_PBaseEntity;
	CBattleEntity* PDefender = (CBattleEntity*)PLuaBaseEntity->GetBaseEntity();
	int hitrate = battleutils::GetHitRate(PAttacker, PDefender);
	if(!lua_isnil(L,2) && lua_isnumber(L,2)){
		hitrate = lua_tointeger(L,2);
	}

	if(rand()%100 < hitrate){
		float DamageRatio = battleutils::GetDamageRatio(PAttacker, PDefender, false, 0);
		int damage = (uint16)((PAttacker->GetMainWeaponDmg() + battleutils::GetFSTR(PAttacker,PDefender,SLOT_MAIN)) * DamageRatio);
		lua_pushinteger( L,damage );
		return 1;
	}
	lua_pushinteger( L,-1 );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::resetRecasts(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    // only reset for players
    if(m_PBaseEntity->objtype == TYPE_PC){
		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	    PChar->PRecastContainer->Del(RECAST_MAGIC);
	    PChar->PRecastContainer->Del(RECAST_ABILITY);

		PChar->pushPacket(new CCharSkillsPacket(PChar));
		return 0;
	}

	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::resetRecast(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

    // only reset for players
    if(m_PBaseEntity->objtype == TYPE_PC){
		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

        RECASTTYPE recastContainer = (RECASTTYPE)lua_tointeger(L,1);
        uint16 recastID = lua_tointeger(L,2);

        if (PChar->PRecastContainer->Has(recastContainer, recastID))
        {
	        PChar->PRecastContainer->Del(recastContainer, recastID);
            PChar->PRecastContainer->Add(recastContainer, recastID, 0);
        }

		PChar->pushPacket(new CCharSkillsPacket(PChar));
		return 0;
	}

	return 0;
}

/***************************************************************
  Attempts to register a BCNM or Dynamis instance.
  INPUT: The BCNM ID to register.
  OUTPUT: The instance number assigned, or -1 if it's all full.
  Call on: The Orb trader
****************************************************************/

inline int32 CLuaBaseEntity::bcnmRegister(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	int instance = 0;

	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);
	int Pzone = PChar->getZone();
	  if(Pzone == 37 || Pzone == 38){
		           if(PChar->loc.zone->m_InstanceHandler->hasFreeSpecialInstance(lua_tointeger(L,1))){
					   	   ShowDebug("Free Special Instance found for BCNMID %i \n",lua_tointeger(L,1));
			               instance = PChar->loc.zone->m_InstanceHandler->registerBcnm(lua_tointeger(L,1),PChar);

					            if(instance!=-1){
			                    ShowDebug("Registration successful!\n");
			                    lua_pushinteger( L,instance);
		                         }
		                         else{
			                     ShowDebug("Unable to register BCNM Special Instance.\n");
			                     lua_pushinteger( L,instance);
		                         }
				   }
				   else
				   {
				   		ShowDebug("BCNM Registration Failed : No free Special instances for BCNMID %i \n",lua_tointeger(L,1));
		                lua_pushinteger( L,-1);
				   }
	  }
	  else
	  if(PChar->loc.zone->m_InstanceHandler->hasFreeInstance()){

		    if(Pzone > 184 && Pzone < 189 || Pzone > 133 && Pzone < 136 || Pzone > 38  && Pzone < 43 ){
			   ShowDebug("Free Dynamis Instance found for BCNMID %i \n",lua_tointeger(L,1));
			   instance = PChar->loc.zone->m_InstanceHandler->registerDynamis(lua_tointeger(L,1),PChar);
		    }
		    else{
			   ShowDebug("Free BCNM Instance found for BCNMID %i \n",lua_tointeger(L,1));
			   instance = PChar->loc.zone->m_InstanceHandler->registerBcnm(lua_tointeger(L,1),PChar);
		    }

		if(instance!=-1){
			ShowDebug("Registration successful!\n");
			lua_pushinteger( L,instance);
		   }
		   else{
			ShowDebug("Unable to register BCNM Instance.\n");
			lua_pushinteger( L,instance);
		}
	 }
	 else{
		ShowDebug("BCNM Registration Failed : No free instances for BCNMID %i \n",lua_tointeger(L,1));
	 lua_pushinteger( L,-1);
	 }

	return 1;
}

/***************************************************************
  Attempts to enter a BCNM or Dynamis instance.
  OUTPUT: 1 if successful, 0 if not.
  Call on: Any player. (e.g. non-orb trader in same pt)
****************************************************************/

inline int32 CLuaBaseEntity::bcnmEnter(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	int Pzone = PChar->getZone();
	if(Pzone > 184 && Pzone < 189 || Pzone > 133 && Pzone < 136 || Pzone > 38  && Pzone < 43 ){
		if(PChar->StatusEffectContainer->HasStatusEffect(EFFECT_DYNAMIS, 0)){
			uint16 effect_bcnmid = PChar->StatusEffectContainer->GetStatusEffect(EFFECT_DYNAMIS,0)->GetPower();
			if(PChar->loc.zone->m_InstanceHandler->enterBcnm(effect_bcnmid,PChar)){
				lua_pushinteger( L,1);
				return 1;
			}
		}
	}
	else{
		if(PChar->StatusEffectContainer->HasStatusEffect(EFFECT_BATTLEFIELD)){
			uint16 effect_bcnmid = PChar->StatusEffectContainer->GetStatusEffect(EFFECT_BATTLEFIELD,0)->GetPower();
			if(PChar->loc.zone->m_InstanceHandler->enterBcnm(effect_bcnmid,PChar)){
				lua_pushinteger( L,1);
				return 1;
			}
		}
	}
	ShowDebug("%s is unable to enter.\n",PChar->GetName());
	lua_pushinteger( L,0);
	return 1;
}

/***************************************************************
  Attempts to leave a BCNM instance.
  INPUT: The type of leave (1=RUN AWAY, 2=TREASURE CHEST)
  OUTPUT: 1 if successful, 0 if not.
  Call on: Anyone who selects "Leave" or "Run Away"
****************************************************************/

inline int32 CLuaBaseEntity::bcnmLeave(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	if(PChar->StatusEffectContainer->HasStatusEffect(EFFECT_BATTLEFIELD)){
		uint16 effect_bcnmid = PChar->StatusEffectContainer->GetStatusEffect(EFFECT_BATTLEFIELD,0)->GetPower();
		uint8 typeOfExit = lua_tointeger(L,1);
		if(typeOfExit==1 && PChar->loc.zone->m_InstanceHandler->leaveBcnm(effect_bcnmid,PChar)){
			ShowDebug("BCNM Leave :: %s left BCNMID %i \n",PChar->GetName(),effect_bcnmid);
		}
		else if(typeOfExit==2 && PChar->loc.zone->m_InstanceHandler->winBcnm(effect_bcnmid,PChar)){
			ShowDebug("BCNM Leave :: Won BCNMID %i \n",effect_bcnmid);
		}
	}
	else{
		ShowDebug("BCNM Leave :: %s does not have EFFECT_BATTLEFIELD. \n",PChar->GetName());
	}

	lua_pushinteger( L,0);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::isInBcnm(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if(PChar->m_insideBCNM){
		lua_pushinteger( L,1);
		return 1;
	}
	lua_pushinteger( L,0);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::getInstanceID(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	uint8 inst = 255;

	if(PChar->loc.zone != NULL && PChar->loc.zone->m_InstanceHandler != NULL){
		inst = PChar->loc.zone->m_InstanceHandler->findInstanceIDFor(PChar);
	}

	lua_pushinteger( L,inst);
	return 1;
}


//==========================================================//

inline int32 CLuaBaseEntity::getBCNMloot(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	uint8 inst = 255;

	if(PChar->loc.zone != NULL && PChar->loc.zone->m_InstanceHandler != NULL){
		inst = PChar->loc.zone->m_InstanceHandler->findInstanceIDFor(PChar);
		PChar->loc.zone->m_InstanceHandler->openTreasureChest(PChar);
	}

	lua_pushinteger( L,inst);
	return 1;
}


//==========================================================//

//returns 1 if all 3 instances are full. Temp measure until event param struct is found out.
inline int32 CLuaBaseEntity::isBcnmsFull(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	uint8 full = 1;

	if(PChar->loc.zone!=NULL && PChar->loc.zone->m_InstanceHandler!=NULL &&
		PChar->loc.zone->m_InstanceHandler->hasFreeInstance()){
		full = 0;
	}
	lua_pushinteger( L,full);
	return 1;
}
inline int32 CLuaBaseEntity::isSpecialIntanceEmpty(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	uint8 full = 1;


	if(PChar->loc.zone!=NULL && PChar->loc.zone->m_InstanceHandler!=NULL &&

		PChar->loc.zone->m_InstanceHandler->hasSpecialInstanceEmpty(lua_tointeger(L,1))){

		full = 0;
	}
	lua_pushinteger( L,full);
	return 1;
}

inline int32 CLuaBaseEntity::getSpecialInstanceLeftTime(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	uint16 Leftime = 0;


	if(PChar->loc.zone!=NULL && PChar->loc.zone->m_InstanceHandler!=NULL){
		     Leftime = PChar->loc.zone->m_InstanceHandler->SpecialInstanceLeftTime(lua_tointeger(L,1),gettick());
	}

	lua_pushinteger( L,Leftime);
	return 1;
}
// Add time on your Special instance
inline int32 CLuaBaseEntity::addTimeToSpecialInstance(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	PChar->loc.zone->m_InstanceHandler->GiveTimeToInstance(lua_tointeger(L,1),lua_tointeger(L,2));

	return 1;
}
inline int32 CLuaBaseEntity::BCNMSetLoot(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);
		if(PChar->loc.zone!=NULL && PChar->loc.zone->m_InstanceHandler!=NULL){
PChar->loc.zone->m_InstanceHandler->SetLootToBCNM(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3));
		}
    return 0;
}

inline int32 CLuaBaseEntity::RestoreAndHealOnInstance(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    // only reset for players
    if(m_PBaseEntity->objtype == TYPE_PC){
		CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
         PChar->loc.zone->m_InstanceHandler->RestoreOnInstance(lua_tointeger(L,1));
		return 0;
	}
	return 0;
}
inline int32 CLuaBaseEntity::setSpawn(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
		PMob->m_SpawnPoint.x = (float) lua_tonumber(L,1);
	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
		PMob->m_SpawnPoint.y = (float) lua_tonumber(L,2);
	if( !lua_isnil(L,3) && lua_isnumber(L,3) )
		PMob->m_SpawnPoint.z = (float) lua_tonumber(L,3);

	return 0;
}

inline int32 CLuaBaseEntity::setRespawnTime(lua_State* L)
{

	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	if( !lua_isnil(L,1) && lua_isnumber(L,1) )
	{
		PMob->m_RespawnTime = lua_tointeger(L, 1) * 1000;

	    PMob->PBattleAI->SetLastActionTime(gettick());
        if (PMob->PBattleAI->GetCurrentAction() == ACTION_NONE)
        {
            PMob->PBattleAI->SetCurrentAction(ACTION_SPAWN);
        }
	}
	else
	{
		ShowWarning("CLuaBaseEntity::setRespawnTime (%d) Tried to set respawn without a time\n", PMob->id);
	}

	PMob->m_AllowRespawn = true;

	return 0;
}
inline int32 CLuaBaseEntity::addPlayerToSpecialInstance(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	int instance = PChar->loc.zone->m_InstanceHandler->SpecialInstanceAddPlayer(lua_tointeger(L,1),PChar);

	if(instance!=-1){
		ShowDebug("Registration successful!\n");
		lua_pushinteger( L,instance);
	}
	else{
		ShowDebug("Unable to register BCNM Instance.\n");
		lua_pushinteger( L,-1);
	}

	return 1;
}

// Return unique ID for Dynamis
inline int32 CLuaBaseEntity::getDynamisUniqueID(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	lua_pushinteger( L, PChar->loc.zone->m_InstanceHandler->getUniqueDynaID(lua_tointeger(L,1)));

	return 1;
}

// Add time on your dynamis instance
inline int32 CLuaBaseEntity::addTimeToDynamis(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	PChar->loc.zone->m_InstanceHandler->dynamisMessage(448,lua_tointeger(L,1));

	return 1;
}

//Launch dynamis mob part 2 (when mega boss is defeated)
inline int32 CLuaBaseEntity::launchDynamisSecondPart(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	PChar->loc.zone->m_InstanceHandler->launchDynamisSecondPart();

	return 1;
}

inline int32 CLuaBaseEntity::addPlayerToDynamis(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PChar->loc.zone->m_InstanceHandler == NULL);

	int instance = PChar->loc.zone->m_InstanceHandler->dynamisAddPlayer(lua_tointeger(L,1),PChar);

	if(instance!=-1){
		ShowDebug("Registration successful!\n");
		lua_pushinteger( L,instance);
	}
	else{
		ShowDebug("Unable to register BCNM Instance.\n");
		lua_pushinteger( L,-1);
	}

	return 1;
}

inline int32 CLuaBaseEntity::isInDynamis(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    lua_pushboolean(L, ((CBattleEntity*)m_PBaseEntity)->isInDynamis());
	return 1;
}

/************************************************************************
*                                                                       *
*  Check if mob is in battlefield list									*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::isInBattlefieldList(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	if(PMob->loc.zone->m_InstanceHandler->checkMonsterInList(PMob))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

/************************************************************************
*                                                                       *
*  Add mob in battlefield list											*
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::addInBattlefieldList(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	PMob->loc.zone->m_InstanceHandler->insertMonsterInList(PMob);

	return 0;
}

/************************************************************************
*                                                                       *
*  Открываем дверь и автоматически закрываем через 7 секунд             *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::openDoor(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_NPC);

    if (m_PBaseEntity->animation == ANIMATION_CLOSE_DOOR)
    {
        uint32 OpenTime = (!lua_isnil(L,1) && lua_isnumber(L,1)) ? (uint32)lua_tointeger(L,1) * 1000 : 7000;

        m_PBaseEntity->animation = ANIMATION_OPEN_DOOR;
        m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_UPDATE));

        CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("close_door", gettick()+OpenTime, m_PBaseEntity, CTaskMgr::TASK_ONCE, close_door));
    }
	return 0;
}

/************************************************
 * Just for debugging. Similar to @animatoin.   *
 * Injects an action packet with the specified  *
 * parameters. Used for quickly finding anims.  *
 ************************************************/
inline int32 CLuaBaseEntity::injectActionPacket(lua_State* L) {
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint16 action = (uint16)lua_tointeger(L,1);
	uint16 anim = (uint16)lua_tointeger(L,2);

	ACTIONTYPE actiontype = ACTION_MAGIC_FINISH;
	switch (action) {
	case 3: actiontype = ACTION_WEAPONSKILL_FINISH; break;
	case 4: actiontype = ACTION_MAGIC_FINISH; break;
	case 6: actiontype = ACTION_JOBABILITY_FINISH; break;
	case 11: actiontype = ACTION_MOBABILITY_FINISH; break;
    case 14: actiontype = ACTION_DANCE; break;
	}

	apAction_t Action;
    PChar->m_ActionList.clear();

	Action.ActionTarget = PChar;
	Action.reaction   = REACTION_NONE;
	Action.speceffect = SPECEFFECT_NONE;
	Action.animation  = anim;
	Action.param      = 10;
	Action.messageID  = 0;

	// If you use ACTION_MOBABILITY_FINISH, the first param = anim, the second param = skill id.
	if (actiontype == ACTION_MOBABILITY_FINISH) {
		CBattleEntity* PTarget = PChar->PBattleAI->GetBattleTarget();
		if (PTarget == NULL) {
			ShowError("Cannot use MOBABILITY_FINISH on a null battle target! Engage a mob! \n");
			return 0;
		}
		else if(PTarget->objtype != TYPE_MOB) {
			ShowError("Battle target must be a monster for MOBABILITY_FINISH \n");
			return 0;
		}
		CMobEntity* PMob = (CMobEntity*)PTarget;
		PMob->m_ActionList.clear();

		ACTIONTYPE oldAction = PMob->PBattleAI->GetCurrentAction();
		PMob->PBattleAI->SetCurrentAction(actiontype);
		// we have to make a fake mob skill for this to work.
		CMobSkill* skill = new CMobSkill(anim);
		skill->setAnimationID(anim);
		Action.animation = anim;
		skill->setMsg(185); // takes damage default msg
		Action.messageID = 185;
		PMob->PBattleAI->SetCurrentMobSkill(skill);
		PMob->m_ActionList.push_back(Action);
		PMob->loc.zone->PushPacket(PMob, CHAR_INRANGE, new CActionPacket(PMob));
		PMob->PBattleAI->SetCurrentAction(oldAction);
		PMob->PBattleAI->SetCurrentMobSkill(NULL);
		delete skill;
		skill = NULL;
		return 0;
	}

	ACTIONTYPE oldAction = PChar->PBattleAI->GetCurrentAction();
	PChar->PBattleAI->SetCurrentSpell(1);
	PChar->PBattleAI->SetCurrentAction(actiontype);
    PChar->m_ActionList.push_back(Action);
	PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE_SELF, new CActionPacket(PChar));
	PChar->PBattleAI->SetCurrentAction(oldAction);

	return 0;
}

/************************************************************************
* can be used by all npc for disappear a certain time					*
* npc:hideNPC() : disappear for 15sec									*
* you can add time in second : hideNPC(30) : disappear for 30sec		*
************************************************************************/

inline int32 CLuaBaseEntity::hideNPC(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_NPC);

    if (m_PBaseEntity->status == STATUS_NORMAL)
    {
        uint32 OpenTime = (!lua_isnil(L,1) && lua_isnumber(L,1)) ? (uint32)lua_tointeger(L,1) * 1000 : 15000;

        m_PBaseEntity->status = STATUS_DISAPPEAR;
        m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity, CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity, ENTITY_DESPAWN));

        CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("reappear_npc", gettick()+OpenTime, m_PBaseEntity, CTaskMgr::TASK_ONCE, reappear_npc));
    }
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::getCP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	lua_pushinteger( L, PChar->RegionPoints[PChar->profile.nation] );
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::addCP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	int32 cp = (int32)lua_tointeger(L,1);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->RegionPoints[PChar->profile.nation] += cp;
	charutils::SaveCharPoints(PChar);
	PChar->pushPacket(new CConquestPacket(PChar));

	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::delCP(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	int32 cp = (int32)lua_tointeger(L,1);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->RegionPoints[PChar->profile.nation] -= cp;
	charutils::SaveCharPoints(PChar);
	PChar->pushPacket(new CConquestPacket(PChar));

	return 0;
}

/************************************************************************
* player:getPoint(pointID) : return the player's number point			*
* pointID: conquest(0~2) imperial(3) assault(4~9) zeni(10)...			*
*  full list in charutils.cpp											*
************************************************************************/

inline int32 CLuaBaseEntity::getPoint(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	int32 point = (int32)lua_tointeger(L,1);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	lua_pushinteger( L, PChar->RegionPoints[point] );
	return 1;
}

/************************************************************************
* player:addPoint(pointID,number) : add number point to player			*
* pointID: conquest(0~2) imperial(3) assault(4~9) zeni(10)...			*
*  full list in charutils.cpp											*
************************************************************************/

inline int32 CLuaBaseEntity::addPoint(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	int32 type = (int32)lua_tointeger(L,1);
	int32 point = (int32)lua_tointeger(L,2);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->RegionPoints[type] += point;
	charutils::SaveCharPoints(PChar);

	return 0;
}

/************************************************************************
* player:delPoint(pointID,number) : del number point to player			*
* pointID: conquest(0~2) imperial(3) assault(4~9) zeni(10)...			*
*  full list in charutils.cpp											*
************************************************************************/

inline int32 CLuaBaseEntity::delPoint(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	int32 type = (int32)lua_tointeger(L,1);
	int32 point = (int32)lua_tointeger(L,2);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->RegionPoints[type] -= point;
	charutils::SaveCharPoints(PChar);

	return 0;
}

/************************************************************************
* player:getNationTeleport(tpID) : return the player's teleport			*
* tpID: supply(0~2) runic(3) maw(4) past(5~7)...						*
*  full list in charutils.cpp											*
************************************************************************/

inline int32 CLuaBaseEntity::getNationTeleport(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	int32 nation = (int32)lua_tointeger(L,1);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	switch(nation)
	{
		case 0: lua_pushinteger( L, PChar->nationtp.sandoria ); return 1; break;
		case 1: lua_pushinteger( L, PChar->nationtp.bastok ); return 1; break;
		case 2: lua_pushinteger( L, PChar->nationtp.windurst ); return 1; break;
		case 3: lua_pushinteger( L, PChar->nationtp.ahturhgan ); return 1; break;
		case 4: lua_pushinteger( L, PChar->nationtp.maw ); return 1; break;
		case 5: lua_pushinteger( L, PChar->nationtp.pastsandoria ); return 1; break;
		case 6: lua_pushinteger( L, PChar->nationtp.pastbastok ); return 1; break;
		case 7: lua_pushinteger( L, PChar->nationtp.pastwindurst ); return 1; break;
		default :
			ShowDebug(CL_CYAN"lua::getNationTeleport no region with this number!\n" CL_RESET);
			return 0;
	}
}

/************************************************************************
* player:addNationTeleport(tpID,number) : add tp to a player variable	*
* tpID: supply(0~2) runic(3) maw(4) past(5~7)...						*
* number: 1,2,4,8,16,32,64,...											*
*  full list in charutils.cpp											*
************************************************************************/

inline int32 CLuaBaseEntity::addNationTeleport(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));
    DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	uint16 nation = (uint16)lua_tointeger(L,1);
	uint32 newTP = (uint32)lua_tointeger(L,2);
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	switch(nation)
	{
		case 0: PChar->nationtp.sandoria += newTP; break;
		case 1: PChar->nationtp.bastok += newTP; break;
		case 2: PChar->nationtp.windurst += newTP; break;
		case 3: PChar->nationtp.ahturhgan += newTP; break;
		case 4: PChar->nationtp.maw += newTP; break;
		case 5: PChar->nationtp.pastsandoria += newTP; break;
		case 6: PChar->nationtp.pastbastok += newTP; break;
		case 7: PChar->nationtp.pastwindurst += newTP; break;
		default :
			ShowDebug(CL_CYAN"lua::addNationTeleport no region with this number!\n" CL_RESET);
			return 0;
	}

	charutils::SaveCharPoints(PChar);
	return 0;
}

//==========================================================//

inline int32 CLuaBaseEntity::isBehind(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);

	lua_pushboolean( L,(abs(PLuaBaseEntity->GetBaseEntity()->loc.p.rotation - m_PBaseEntity->loc.p.rotation) < 23));
	return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::isFacing(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);

    DSP_DEBUG_BREAK_IF(PLuaBaseEntity == NULL);

    lua_pushboolean( L, isFaceing(m_PBaseEntity->loc.p, PLuaBaseEntity->GetBaseEntity()->loc.p, 45));
    return 1;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::getStealItem(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	DropList_t* DropList = itemutils::GetDropList(((CMobEntity*)m_PBaseEntity)->m_DropID);

	if (DropList != NULL && DropList->size())
	{
		for(uint8 i = 0; i < DropList->size(); ++i)
		{
			if (DropList->at(i).DropType == 2)
			{
				lua_pushinteger(L, DropList->at(i).ItemID);
				return 1;
			}
		}
	}
    lua_pushinteger(L, 0);
	return 1;
}

//==========================================================//

inline int32 CLuaBaseEntity::checkDistance(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);

	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;
	CBattleEntity* PMob = (CBattleEntity*)PLuaBaseEntity->GetBaseEntity();

	float calcdistance = distance(PBattle->loc.p, PMob->loc.p);

	lua_pushnumber( L,calcdistance);
	return 1;
}

inline int32 CLuaBaseEntity::checkBaseExp(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	CMobEntity* PMob = (CMobEntity*)m_PBaseEntity;

	bool isbaseexp = false;
	uint32 baseexp = charutils::GetRealExp(PMob->m_HiPCLvl, PMob->GetMLevel());
	if (baseexp != 0) isbaseexp = true;

	lua_pushboolean( L,isbaseexp);
	return 1;
}

inline int32 CLuaBaseEntity::checkSoloPartyAlliance(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	uint8 SoloPartyAlliance = 0;
	if (PChar->PParty != NULL)
	{
		SoloPartyAlliance = 1;
		if (PChar->PParty->m_PAlliance != NULL) SoloPartyAlliance = 2;
	}

	lua_pushinteger( L,SoloPartyAlliance);
	return 1;
}

inline int32 CLuaBaseEntity::checkExpPoints(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));
	DSP_DEBUG_BREAK_IF(lua_isnil(L,2) || !lua_isnumber(L,2));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);
	float baseexp = (float)lua_tonumber(L,2);
	float exp = 0;

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	CMobEntity* PMob = (CMobEntity*)PLuaBaseEntity->GetBaseEntity();
	uint8 charlvl = PChar->GetMLevel();
	uint8 maxlevel = PMob->m_HiPCLvl;

	if (map_config.fov_party_gap_penalties == 1)
	{
		if (maxlevel > 50 || maxlevel > (charlvl+7))
		{
			exp = (float)baseexp*(float)((float)(charlvl)/(float)(maxlevel));
		}
		else
		{
			exp = (float)baseexp*(float)((float)(charutils::GetExpNEXTLevel(charlvl))/(float)(charutils::GetExpNEXTLevel(maxlevel)));
		}
	}
	else exp = baseexp;

	lua_pushnumber( L,exp);
	return 1;
}

inline int32 CLuaBaseEntity::checkFovAllianceAllowed(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	uint8 FovAlliance = map_config.fov_allow_alliance;

	lua_pushinteger( L,FovAlliance);
	return 1;
}



/************************************************************************
*																		*
*  Charm a monster														*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::charmPet(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	if ( m_PBaseEntity != NULL )
	{
		if ( m_PBaseEntity->objtype != TYPE_MOB )
		{
			CLuaBaseEntity* PEntity = Lunar<CLuaBaseEntity>::check(L,1);
			battleutils::tryToCharm((CBattleEntity*)m_PBaseEntity, (CBattleEntity*)PEntity->GetBaseEntity());
		}
	}
	return 0;
}


/************************************************************************
*																		*
*  Makes your pet stay put												*
*																		*
************************************************************************/

inline int32 CLuaBaseEntity::petStay(lua_State *L)
{
	if ( m_PBaseEntity != NULL )
	{
		if ( m_PBaseEntity->objtype != TYPE_MOB )
		{
			petutils::MakePetStay((CBattleEntity*)m_PBaseEntity);
		}
	}
	return 0;
}

inline int32 CLuaBaseEntity::getObjType(lua_State *L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushinteger(L, m_PBaseEntity->objtype);
	return 1;
}

inline int32 CLuaBaseEntity::isPC(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushboolean( L, m_PBaseEntity->objtype == TYPE_PC);
	return 1;
}

inline int32 CLuaBaseEntity::isNPC(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushboolean( L, m_PBaseEntity->objtype == TYPE_NPC);
	return 1;
}

inline int32 CLuaBaseEntity::isMob(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

    lua_pushboolean( L, m_PBaseEntity->objtype == TYPE_MOB);
	return 1;
}

inline int32 CLuaBaseEntity::isPet(lua_State *L){
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	lua_pushboolean( L, m_PBaseEntity->objtype == TYPE_PET);
	return 1;
}

inline int32 CLuaBaseEntity::hasTrait(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	lua_pushboolean( L, charutils::hasTrait((CCharEntity*)m_PBaseEntity, lua_tointeger(L, 1)));
	return 1;
}

inline int32 CLuaBaseEntity::isTrickAttackAvailable(lua_State *L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isuserdata(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);
	CBattleEntity* PMob = (CBattleEntity*)PLuaBaseEntity->GetBaseEntity();
	if (PMob != NULL)
	{
		CBattleEntity* taTarget = battleutils::getAvailableTrickAttackChar((CBattleEntity*)m_PBaseEntity, PMob);
		lua_pushboolean( L, (taTarget != NULL ? true : false));
		return 1;
	}
	return 0;
}


inline int32 CLuaBaseEntity::setDelay(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    ((CMobEntity*)m_PBaseEntity)->m_Weapons[SLOT_MAIN]->setBaseDelay(lua_tonumber(L, 1));
	return 0;
}
inline int32 CLuaBaseEntity::setDamage(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    ((CMobEntity*)m_PBaseEntity)->m_Weapons[SLOT_MAIN]->setDamage(lua_tonumber(L, 1));
	return 0;
}

//cast spell in parameter - if no parameter, cast any spell in the spell list
inline int32 CLuaBaseEntity::castSpell(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	if (lua_isnumber(L,1))
	{
		((CMobEntity*)m_PBaseEntity)->PBattleAI->SetCurrentSpell(lua_tointeger(L, 1));
		((CMobEntity*)m_PBaseEntity)->PBattleAI->SetCurrentAction(ACTION_MAGIC_START);
	} else {
		((CMobEntity*)m_PBaseEntity)->PBattleAI->SetLastMagicTime(0);
	}
	return 0;
}
inline int32 CLuaBaseEntity::useMobAbility(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	if (lua_isnumber(L,1))
	{
		if (((CMobEntity*)m_PBaseEntity)->PBattleAI->GetBattleTarget() != NULL)
		{
			uint16 mobskillId = lua_tointeger(L, 1);
			CMobSkill* mobskill = battleutils::GetMobSkill(mobskillId);

			if(mobskill != NULL)
			{
				((CMobEntity*)m_PBaseEntity)->PBattleAI->SetCurrentMobSkill(mobskill);
				if( mobskill->getActivationTime() != 0)
				{
					apAction_t Action;
					((CMobEntity*)m_PBaseEntity)->m_ActionList.clear();
					if(mobskill->getValidTargets() == TARGET_ENEMY){ //enemy
						Action.ActionTarget = ((CMobEntity*)m_PBaseEntity)->PBattleAI->GetBattleTarget();
					}
					else if(mobskill->getValidTargets() == TARGET_SELF){ //self
						Action.ActionTarget = ((CMobEntity*)m_PBaseEntity);
					}
					Action.reaction   = REACTION_HIT;
					Action.speceffect = SPECEFFECT_HIT;
					Action.animation  = 0;
					Action.param	  = mobskill->getMsgForAction();//m_PMobSkill->getAnimationID();
					Action.messageID  = 43; //readies message

					((CMobEntity*)m_PBaseEntity)->m_ActionList.push_back(Action);
					((CMobEntity*)m_PBaseEntity)->loc.zone->PushPacket(((CMobEntity*)m_PBaseEntity), CHAR_INRANGE, new CActionPacket((CMobEntity*)m_PBaseEntity));
				}
				((CMobEntity*)m_PBaseEntity)->PBattleAI->SetBattleSubTarget(((CMobEntity*)m_PBaseEntity)->PBattleAI->GetBattleTarget());
				((CMobEntity*)m_PBaseEntity)->PBattleAI->SetCurrentAction(ACTION_MOBABILITY_USING);
			}
			else
			{
				ShowWarning("lua_baseentity::useMobAbility NULL mobskill used %d", mobskillId);
			}
		}
	} else {
		((CMobEntity*)m_PBaseEntity)->PBattleAI->SetCurrentAction(ACTION_MOBABILITY_START);
	}
	return 0;
}

inline int32 CLuaBaseEntity::SetAutoAttackEnabled(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isboolean(L, 1));

	((CBattleEntity*)m_PBaseEntity)->PBattleAI->SetAutoAttackEnabled(lua_toboolean(L, 1));

	return 0;
}

inline int32 CLuaBaseEntity::SetMagicCastingEnabled(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isboolean(L, 1));

	((CBattleEntity*)m_PBaseEntity)->PBattleAI->SetMagicCastingEnabled(lua_toboolean(L, 1));

	return 0;
}

inline int32 CLuaBaseEntity::SetMobAbilityEnabled(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isboolean(L, 1));

	((CBattleEntity*)m_PBaseEntity)->PBattleAI->SetMobAbilityEnabled(lua_toboolean(L, 1));

	return 0;
}

inline int32 CLuaBaseEntity::updateTarget(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	((CMobEntity*)m_PBaseEntity)->PBattleAI->SetBattleTarget(((CMobEntity*)m_PBaseEntity)->PEnmityContainer->GetHighestEnmity());

	return 0;
}

/************************************************************************
*                                                                       *
*  Gets the extra var stored in the mob entity.  Number parameter       *
*  determines the number of results returned and their size:			*
*  1 - returns one uint32												*
*  2 - returns two uint16s												*
*  3 - returns a uint16, then two uint8s								*
*  4 - returns 4 uint8s													*
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::getExtraVar(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	int32 n = lua_tointeger(L,1);

	if (n == 1)
	{
		lua_pushinteger(L, ((CMobEntity*)m_PBaseEntity)->m_extraVar);
	} else if (n == 2) {
		uint32 var = ((CMobEntity*)m_PBaseEntity)->m_extraVar;
		uint16 var1 = var & 0x0000FFFF;
		uint16 var2 = var >>= 16;
		lua_pushinteger(L, var1);
		lua_pushinteger(L, var2);
	} else if (n == 3) {
		uint32 var = ((CMobEntity*)m_PBaseEntity)->m_extraVar;
		uint16 var1 = var >>= 16;
		uint8 var2 = var & 0x0000FF00;
		var2 >>= 8;
		uint8 var3 = var & 0x000000FF;
		lua_pushinteger(L, var1);
		lua_pushinteger(L, var2);
		lua_pushinteger(L, var3);
	} else {
		uint32 var = ((CMobEntity*)m_PBaseEntity)->m_extraVar;
	    uint8 var1 = (var & 0x000000FF);
	    uint8 var2 = (var & 0x0000FF00) >> 8;
	    uint8 var3 = (var & 0x00FF0000) >> 16;
	    uint8 var4 = (var & 0xFF000000) >> 24;
		lua_pushinteger(L, var4);
		lua_pushinteger(L, var3);
		lua_pushinteger(L, var2);
		lua_pushinteger(L, var1);
	}

	return n;
}

/************************************************************************
*                                                                       *
*  Gets the extra var stored in the mob entity.  Number of parameters   *
*  value sizes:															*
*  1 - stores one uint32												*
*  2 - stores two uint16s												*
*  3 - stores a uint16, then two uint8s									*
*  4 - stores 4 uint8s													*
*                                                                       *
************************************************************************/
inline int32 CLuaBaseEntity::setExtraVar(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	((CMobEntity*)m_PBaseEntity)->m_extraVar = lua_tonumber(L, 1);

	int32 n = lua_gettop(L);

	if (n == 1)
	{
		((CMobEntity*)m_PBaseEntity)->m_extraVar = lua_tointeger(L, 1);
	} else if (n == 2) {
		uint32 var1 = lua_tointeger(L, 1) & 0x0000FFFF;
		uint32 var2 = lua_tointeger(L, 2) << 16;
		((CMobEntity*)m_PBaseEntity)->m_extraVar = var1 + var2;
	} else if (n == 3) {
		uint32 var1 = lua_tointeger(L, 1) << 16;
		uint32 var2 = (lua_tointeger(L, 2) & 0x000000FF) << 8;
		uint32 var3 = (lua_tointeger(L, 3) & 0x000000FF);
		((CMobEntity*)m_PBaseEntity)->m_extraVar = var1 + var2 + var3;
	} else {
		uint32 var1 = lua_tointeger(L, 1) << 24;
		uint32 var2 = (lua_tointeger(L, 2) & 0x000000FF) << 16;
		uint32 var3 = (lua_tointeger(L, 3) & 0x000000FF) << 8;
		uint32 var4 = (lua_tointeger(L, 4) & 0x000000FF);
		((CMobEntity*)m_PBaseEntity)->m_extraVar = var1 + var2 + var3 + var4;
	}
	return 0;
}

inline int32 CLuaBaseEntity::setSpellList(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_MOB);

	DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

	((CMobEntity*)m_PBaseEntity)->m_SpellListContainer = mobSpellList::GetMobSpellList(lua_tonumber(L,1));

	return 0;
}

inline int32 CLuaBaseEntity::hasValidJugPetItem(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CItemWeapon* PItem = (CItemWeapon*)((CCharEntity*)m_PBaseEntity)->getStorage(LOC_INVENTORY)->GetItem(((CCharEntity*)m_PBaseEntity)->equip[SLOT_AMMO]);

	if (PItem != NULL && PItem->getSubSkillType() >= SUBSKILL_SHEEP && PItem->getSubSkillType() <= SUBSKILL_TOLOI)
	{
		lua_pushboolean(L, true);
		return 1;
	}
	else
	{
		lua_pushboolean(L, false);
		return 1;
	}
}

inline int32 CLuaBaseEntity::hasTarget(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

	lua_pushboolean(L,((CBattleEntity*)m_PBaseEntity)->PBattleAI->GetBattleTarget() != NULL);
	return 1;
}

inline int32 CLuaBaseEntity::setBattleSubTarget(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(lua_isnil(L,1));

	CLuaBaseEntity* PLuaBaseEntity = Lunar<CLuaBaseEntity>::check(L,1);
	CBattleEntity* PTarget = (CBattleEntity*)PLuaBaseEntity->GetBaseEntity();

	((CBattleEntity*)m_PBaseEntity)->PBattleAI->SetBattleSubTarget(PTarget);

	return 0;
}

inline int32 CLuaBaseEntity::hasTPMoves(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF((m_PBaseEntity->objtype == TYPE_NPC) || (m_PBaseEntity->objtype == TYPE_PC));

	uint16 familyID = 0;

	if (m_PBaseEntity->objtype & TYPE_PET)
	{
		familyID = ((CPetEntity*)m_PBaseEntity)->m_Family;
	}
	else if (m_PBaseEntity->objtype & TYPE_MOB)
	{
		familyID = ((CMobEntity*)m_PBaseEntity)->m_Family;
	}
	std::vector<CMobSkill*> MobSkills = battleutils::GetMobSkillsByFamily(familyID);
	lua_pushboolean(L,MobSkills.size() != 0);
	return 1;
}

inline int32 CLuaBaseEntity::getMaster(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC)

	if(((CBattleEntity*)m_PBaseEntity)->PMaster != NULL)
	{
		//uint32 petid = (uint32);

		CBaseEntity* PMaster = ((CBattleEntity*)m_PBaseEntity)->PMaster;

		lua_pushstring(L,CLuaBaseEntity::className);
		lua_gettable(L,LUA_GLOBALSINDEX);
		lua_pushstring(L,"new");
		lua_gettable(L,-2);
		lua_insert(L,-2);
		lua_pushlightuserdata(L,(void*)PMaster);
		lua_pcall(L,2,1,0);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

inline int32 CLuaBaseEntity::recalculateAbilitiesTable(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC)

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    return 0;
}

inline int32 CLuaBaseEntity::recalculateSkillsTable(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC)

    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

    charutils::BuildingCharSkillsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    return 0;
}

inline int32 CLuaBaseEntity::isSpellAoE(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(lua_isnil(L,1) || !lua_isnumber(L,1));

    CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;
    CSpell* PSpell = spell::GetSpell(lua_tonumber(L,1));

    if (PSpell != NULL)
    {
        lua_pushboolean(L,battleutils::GetSpellAoEType(PEntity, PSpell) > 0);
    }
    else
    {
        lua_pushboolean(L,false);
    }

    return 1;
}

inline int32 CLuaBaseEntity::getBaseHP(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;

    lua_pushnumber(L,PEntity->health.maxhp);
    return 1;
}

inline int32 CLuaBaseEntity::getBaseMP(lua_State* L)
{
    DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
    DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_NPC);

    CBattleEntity* PEntity = (CBattleEntity*)m_PBaseEntity;

    lua_pushnumber(L,PEntity->health.maxmp);
    return 1;
}

inline int32 CLuaBaseEntity::checkNameFlags(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if(PChar->nameflags.flags & (uint32)lua_tonumber(L,1))
		lua_pushboolean(L,true);
	else
		lua_pushboolean(L,false);
	return 1;
}

inline int32 CLuaBaseEntity::getGMLevel(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	lua_pushnumber(L,PChar->m_GMlevel);
	return 1;
}

inline int32 CLuaBaseEntity::setGMLevel(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	PChar->m_GMlevel = (uint8)lua_tonumber(L,1);
	charutils::SaveCharGMLevel(PChar);
	return 0;
}
inline int32 CLuaBaseEntity::PrintToPlayer(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);

	if (lua_isstring(L,1) && !lua_isnil(L,1))
	{
		zoneutils::GetZone(m_PBaseEntity->getZone())->PushPacket(
			m_PBaseEntity,
			CHAR_INRANGE_SELF,
			new CChatMessagePacket((CCharEntity*)m_PBaseEntity,MESSAGE_SYSTEM_1,(char*)lua_tostring(L,1)));
	}
	return 0;
}
/*
Walk through the given points. NPC only.

Usage:

	npc:pathThrough({
		-217, -57, 379, -- point 1
		-264, -55, 378 -- point 2
	}, PATHFLAG_RUN | PATHFLAG_WALLHACK);
*/
inline int32 CLuaBaseEntity::pathThrough(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	position_t points[50];

	uint8 length = lua_objlen(L, 1);
	uint8 pos = 0;

	DSP_DEBUG_BREAK_IF(length > 50*3);

	// grab points from array and store in points array
	for(uint8 i=1; i<length; i+=3)
	{
		lua_rawgeti(L, 1, i);
		points[pos].x = lua_tonumber(L, -1);
		lua_pop(L,1);

 		lua_rawgeti(L, 1, i+1);
		points[pos].y = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, i+2);
		points[pos].z = lua_tonumber(L, -1);
		lua_pop(L,1);

		pos++;
	}

	uint8 flags = 0;

	if(lua_isnumber(L, 2))
	{
		flags = lua_tointeger(L, 2);
	}

	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;

	if(PBattle->PBattleAI->m_PPathFind->PathThrough(points, pos, flags))
	{
		PBattle->PBattleAI->SetCurrentAction(ACTION_ROAMING);
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, false);
	}

	return 1;
}

inline int32 CLuaBaseEntity::knockback(lua_State* L)
{
	return 0; // KnockBack is bit in ActionPacket. Don't need to do something in scripts
}

/*
Usage:

	npc:atPoint(-200, 10, -300);
	npc:atPoint({-200, 10, -300});
*/
inline int32 CLuaBaseEntity::atPoint(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	float posX = 0;
	float posY = 0;
	float posZ = 0;

	if(lua_isnumber(L, 1))
	{
		posX = lua_tonumber(L,1);
		posY = lua_tonumber(L,2);
		posZ = lua_tonumber(L,3);
	}
	else
	{
		// its a table
		lua_rawgeti(L, 1, 1);
		posX = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 2);
		posY = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 3);
		posZ = lua_tonumber(L, -1);
		lua_pop(L,1);
	}

	lua_pushboolean(L, m_PBaseEntity->loc.p.x == posX && m_PBaseEntity->loc.p.y == posY && m_PBaseEntity->loc.p.z == posZ);

	return 1;
}

/*
Usage:

	npc:lookAt(-200, 10, -300);
	npc:lookAt({-200, 10, -300});
*/
inline int32 CLuaBaseEntity::lookAt(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	float posX = 0;
	float posY = 0;
	float posZ = 0;

	if(lua_isnumber(L, 1))
	{
		posX = lua_tonumber(L,1);
		posY = lua_tonumber(L,2);
		posZ = lua_tonumber(L,3);
	}
	else
	{
		// its a table
		lua_rawgeti(L, 1, 1);
		posX = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 2);
		posY = lua_tonumber(L, -1);
		lua_pop(L,1);

		lua_rawgeti(L, 1, 3);
		posZ = lua_tonumber(L, -1);
		lua_pop(L,1);
	}

	position_t point;

	point.x = posX;
	point.y = posY;
	point.z = posZ;

	m_PBaseEntity->loc.p.rotation = getangle(m_PBaseEntity->loc.p, point);

    m_PBaseEntity->loc.zone->PushPacket(m_PBaseEntity,CHAR_INRANGE, new CEntityUpdatePacket(m_PBaseEntity,ENTITY_UPDATE));

	return 0;
}

/*
Checks if the entity is following a path.
*/
inline int32 CLuaBaseEntity::isFollowingPath(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity == NULL);

	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;

	lua_pushboolean(L, PBattle->PBattleAI != NULL &&
		PBattle->PBattleAI->m_PPathFind != NULL &&
		PBattle->PBattleAI->m_PPathFind->IsFollowingPath());

	return 1;
}

/*
Clears the current path and stops moving.
*/
inline int32 CLuaBaseEntity::clearPath(lua_State* L)
{
	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;
	DSP_DEBUG_BREAK_IF(PBattle->PBattleAI == NULL);

	if(PBattle->PBattleAI->m_PPathFind != NULL)
	{
		PBattle->PBattleAI->m_PPathFind->Clear();
	}

	return 0;
}

/*
Wait for a given number of milliseconds.

Usage:

	npc:wait(1000); -- wait one second
*/
inline int32 CLuaBaseEntity::wait(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype == TYPE_PC);

	CBattleEntity* PBattle = (CBattleEntity*)m_PBaseEntity;

	DSP_DEBUG_BREAK_IF(PBattle->PBattleAI == NULL);

	int32 waitTime = 4000;

	if(lua_isnumber(L, 1)){
		waitTime = lua_tonumber(L, 1);
	}

	PBattle->PBattleAI->Wait(waitTime);

	return 1;
}

inline int32 CLuaBaseEntity::unlockAttachment(lua_State* L)
{
	DSP_DEBUG_BREAK_IF(m_PBaseEntity->objtype != TYPE_PC);
	DSP_DEBUG_BREAK_IF(lua_isnil(L, -1) || !lua_isnumber(L,-1));

	uint16 itemID = lua_tointeger(L, -1);

	CItem* PItem = itemutils::GetItem(itemID);
	lua_pushboolean(L,puppetutils::UnlockAttachment((CCharEntity*)m_PBaseEntity, PItem));
	return 1;
}
///////////////////////////////////////////////////////////////
//COMMAND SYSTEM
///////////////////////////////////////////////////////////////
inline int32 CLuaBaseEntity::Zone(lua_State *L)
{
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	char buf[110];
	if(PChar !=NULL)
	{
     
   if(lua_isnil(L,1) || !lua_isnumber(L,1) || !lua_tolstring(L,1,NULL))
	{
		sprintf(buf,"COMMAND Example: .zone ID CS", (uint8)lua_tointeger(L,1));
	               PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		sprintf(buf,"AS COMMAND Example: .zone 240 305", (uint8)lua_tointeger(L,1));
	               PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		sprintf(buf,"OR COMMAND Example: .zone 240", (uint8)lua_tointeger(L,1));
	               PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	}
   
  
   if(lua_isnil(L,2) || !lua_isnumber(L,2))
	{
		PChar->eventid =-1;
	}

	if( !lua_isnil(L,2) && lua_isnumber(L,2) )
	{
			
		uint8 eventid = (uint8)lua_tointeger(L,2);
		PChar->eventid =eventid;
	}
		float to_x = 0;
		float to_y = 0;
		float to_z = 0;
		uint8 to_rot = 0;
		uint8 zone =(uint8)lua_tointeger(L,1);
		const char * Query = "SELECT x,y,z,r FROM zonesystem WHERE zone= '%u';";
	          int32 ret3 = Sql_Query(SqlHandle,Query,(uint8)lua_tointeger(L,1));
			

	             if (ret3 != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	                {
						
				    to_x =  Sql_GetFloatData(SqlHandle,0);
					ShowMessage(CL_YELLOW"TO X %.3f \n"CL_RESET,to_x);
				    to_y =  Sql_GetFloatData(SqlHandle,1);
					ShowMessage(CL_YELLOW"TO X %.3f \n"CL_RESET,to_y);
				    to_z =  Sql_GetFloatData(SqlHandle,2);
					ShowMessage(CL_YELLOW"TO X %.3f \n"CL_RESET,to_z);
				    to_rot =  Sql_GetUIntData(SqlHandle,3);
					ShowMessage(CL_YELLOW"TO ROT %u \n"CL_RESET,to_rot);
					ShowMessage(CL_YELLOW"PCHAR %s ID %u \n"CL_RESET,PChar->GetName(),PChar->id);
                   
				   PChar->loc.p.x = to_x;
				   PChar->loc.p.y = to_y;
				   PChar->loc.p.z = to_z;
				   PChar->loc.p.rotation = to_rot;
				   
		           PChar->loc.boundary = 0;
				   const int8* Query = "UPDATE chars SET returning = '1', pos_zone='%u', pos_prevzone='%u', pos_x='%.3f', pos_y='%.3f', pos_z='%.3f', pos_rot='%u' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,zone,zone,to_x,to_y,to_z,to_rot,PChar->id);
					   
					   
             PChar->is_returning = 1;
				  
                    PChar->loc.destination = (uint8)lua_tointeger(L,1);
				   PChar->status = STATUS_DISAPPEAR;
			PChar->animation = ANIMATION_NONE;

			PChar->clearPacketList();
				 
	                PChar->pushPacket(new CServerIPPacket(PChar,2));
						return false;
		
				    }
				 else
				 {
                   ShowMessage(CL_YELLOW"NO ZONELINE FOUND IN DATABASE BY THE ID %u \n"CL_RESET,(uint8)lua_tointeger(L,1));
				   
                   sprintf(buf,"There is no zone found in the database by the ID: %d", (uint8)lua_tointeger(L,1));
	               PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
				   return false;
				 }
		
       
	
   
			
	return false;
	}
	return false;
}

inline int32 CLuaBaseEntity::show_Command_Menu(lua_State *L)
{
	
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	if(m_PBaseEntity == NULL)
	{
		
		
		return true;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		
		return true;
	}
	
	
	if(PChar->Account_Level==0)
	{
	char buf[110];
	sprintf(buf,"Player Commands." );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	char buf1[110];
	sprintf(buf1,"?hello .com .ah .setspeed .setexprates" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf1)));
		
	return true;
	}
	if(PChar->Account_Level==1)
	{
	char buf[110];
	sprintf(buf,"Gm Commands." );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	char buf1[110];
	sprintf(buf1,"?hello #server .com .ah .setspeed .setexprates .setmainjob .setmainlevel" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf1)));
	char buf2[110];
	sprintf(buf2,".setsubjob .setsublevel .zone" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf2)));
	return true;
	}
	if(PChar->Account_Level==2)
	{
	char buf[110];
	sprintf(buf,"Sgm Commands." );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	char buf1[110];
	sprintf(buf1,"?hello #server .com .ah .gm .setspeed .setexprates .setmainjob .setmainlevel" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf1)));
	char buf2[110];
	sprintf(buf2,".setsubjob .setsublevel .zone" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf2)));
	
	return true;
	}
	if(PChar->Account_Level==3)
	{
	char buf[110];
	sprintf(buf,"Lgm Commands." );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	char buf1[110];
	sprintf(buf1,"?hello #server .com .ah .gm .setspeed .setexprates .setmainjob .setmainlevel" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf1)));
	char buf2[110];
	sprintf(buf2,".setsubjob .setsublevel .zone" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf2)));
	
	return true;
	}
	if(PChar->Account_Level==4)
	{
	char buf[110];
	sprintf(buf,"Server Owner Commands." );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	char buf1[110];
	sprintf(buf1,"?hello #server .com .ah .gm .setspeed .setexprates .setmainjob .setmainlevel" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf1)));
	char buf2[110];
	sprintf(buf2,".setsubjob .setsublevel .zone" );
	PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf2)));
	
	return true;
	}
	
	
	
	
	
	
    
	return false;
}
inline int32 CLuaBaseEntity::Set_Exp_Rates(lua_State *L)
{
    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		char buf[110];
     sprintf(buf,"Command Example: .setexprates 1");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	 }
	 if(PChar->Account_Level==0)
	 {
	 if(lua_tointeger(L,1) > 5 || lua_tointeger(L,1) < 1)
	       {
		   
		   char buf[110];
     sprintf(buf,"Command Example: .setexprates 1-5");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	 if(PChar->Account_Level==1 || PChar->Account_Level==2)
	 {
	 if(lua_tointeger(L,1) > 10 || lua_tointeger(L,1) < 1)
	       {
		 
		   char buf[110];
     sprintf(buf,"Command Example: .setexprates 1-10");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	  if(PChar->Account_Level==3 ||PChar->Account_Level==4)
	 {
	 if(lua_tointeger(L,1) > 20 || lua_tointeger(L,1) < 1)
	       {
		   
		   char buf[110];
     sprintf(buf,"Command Example: .setexprates 1-20");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	  int32 exprates = (uint8)dsp_min(lua_tointeger(L,-1), 20);
	  PChar->SetExpRates = exprates;
	   char buf[110];
     sprintf(buf,"Setting Exp Rates: %u", exprates);
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	 const int8* Query = "UPDATE chars SET setexprates = '%u' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,exprates,PChar->id);
	 return false;
}
inline int32 CLuaBaseEntity::set_speed(lua_State *L)
{
    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		//ShowNotice(CL_RED"TRACER:COMMAND: setspeed Error\n" CL_RESET);
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		//ShowNotice(CL_RED"TRACER:COMMAND: setspeed Error1\n" CL_RESET);
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		char buf[110];
     sprintf(buf,"Command Example: .setspeed 50");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	 }
	 if(PChar->Account_Level==0)
	 {

		  if(lua_tointeger(L,1) > 75 || lua_tointeger(L,1) < 30)
	       {
		   
		   char buf[110];
     sprintf(buf,"Command Example: .setspeed 30-75");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	 if(PChar->Account_Level==1 || PChar->Account_Level==2)
	 {

		  if(lua_tointeger(L,1) > 100 || lua_tointeger(L,1) < 30)
	       {
		   
		   char buf[110];
     sprintf(buf,"Command Example: .setspeed 30-100");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	 if(PChar->Account_Level==3 ||PChar->Account_Level==4)
	 {

		  if(lua_tointeger(L,1) > 125 || lua_tointeger(L,1) < 30)
	       {
		   
		   char buf[110];
     sprintf(buf,"Command Example: .setspeed 30-125");
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	       }
	 }
	 uint8 speed = (uint8)dsp_min(lua_tointeger(L,-1), 255);
	 CBattleEntity* PPet = PChar->PPet;
	 const int8* Query = "UPDATE chars SET speed = '%u' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,speed,PChar->id);
	if(PPet == NULL)
	{
		PChar->speed = speed;
     PChar->pushPacket(new CCharUpdatePacket(PChar));
    
     char buf[110];
     sprintf(buf,"Setting Speed: %u", speed);
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	return false;
	}
	else
	{

		PChar->speed = speed;
		PPet->speed = speed;
		PChar->pushPacket(new CEntityUpdatePacket(PPet, ENTITY_UPDATE));
		PPet->loc.zone->PushPacket(PPet, CHAR_INRANGE, new CEntityUpdatePacket(PPet, ENTITY_UPDATE));
		PPet->loc.zone->PushPacket(PPet, CHAR_INRANGE_SELF, new CEntityUpdatePacket(PPet, ENTITY_UPDATE));
     PChar->pushPacket(new CCharUpdatePacket(PChar));
    
     char buf[110];
     sprintf(buf,"Setting Speed: %u", speed);
	 PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	return false;
	}
     
            
    return false;
}
inline int32 CLuaBaseEntity::change_Job(lua_State *L)
{
	 CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainJob 1");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	
		return false;
	 }
	  if(lua_tointeger(L,1) > 20 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainjob 1-20");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	 if(PChar->GetSJob() == PChar->GetMJob() ||PChar->GetMJob() == PChar->GetSJob())
	 {
		 char buf[110];
        sprintf(buf,"Unable To Change Your Main Job To Sub Job!");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		 return false;
	 }  
	      

	

	PChar->resetPetZoningInfo();

	PChar->jobs.unlocked |= (1 << (uint8)lua_tointeger(L,1));
	PChar->SetMJob((uint8)lua_tointeger(L,1));

    charutils::BuildingCharSkillsTable(PChar);
	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

    //charutils::SaveCharStats(PChar);
    //charutils::SaveCharJob(PChar, PChar->GetMJob());
    //charutils::SaveCharExp(PChar, PChar->GetMJob());
	//charutils::SaveCharPoints(PChar);
	charutils::UpdateHealth(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
    PChar->pushPacket(new CMenuMeritPacket(PChar));
    PChar->pushPacket(new CCharSyncPacket(PChar));
	
	int32 n = (uint8)lua_tointeger(L,1);
	string_t name= "none";
	if(n==1)
	{
		name ="Warrior";
	}
	if(n==2)
	{
		name ="Monk";
	}
	if(n==3)
	{
		name ="White Mage";
	}
	if(n==4)
	{
		name ="Black Mage";
	}
	if(n==5)
	{
		name ="Red Mage";
	}
	if(n==6)
	{
		name ="Thief";
	}
	if(n==7)
	{
		name ="Paladin";
	}
	if(n==8)
	{
		name ="Dark Knight";
	}
	if(n==9)
	{
		name ="Beastmaster";
	}
	if(n==10)
	{
		name ="Bard";
	}
	if(n==11)
	{
		name ="Ranger";
	}
	if(n==12)
	{
		name ="Samurai";
	}
	if(n==13)
	{
		name ="Ninja";
	}
	if(n==14)
	{
		name ="Dragoon";
	}
	if(n==15)
	{
		name ="Summoner";
	}
	if(n==16)
	{
		name ="Blue Mage";
	}
	if(n==17)
	{
		name ="Corsair";
	}
	if(n==18)
	{
		name ="Puppetmaster";
	}
	if(n==19)
	{
		name ="Dancer";
	}
	if(n==20)
	{
		name ="Scholar";
	}
	
        char buf[110];
        sprintf(buf,"Changeing Job: ID %u NAME: %s",(int)n,name.c_str());
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	return false;
}


/************************************************************************
*                                                                       *
*  GM command @changesJOB !!! FOR DEBUG ONLY !!!                        *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::change_sub_Job(lua_State *L)
{
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsubJob 1");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	 }
	 if(lua_tointeger(L,1) > 20 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsubjob 1-20");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}

	if(PChar->GetSJob() == PChar->GetMJob() ||PChar->GetMJob() == PChar->GetSJob())
	 {
		 char buf[110];
        sprintf(buf,"Unable To Change Your Sub Job To Main Job!");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		 return false;
	 }

	PChar->jobs.unlocked |= (1 << (uint8)lua_tointeger(L,1));
	PChar->SetSJob((uint8)lua_tointeger(L,1));


    charutils::BuildingCharSkillsTable(PChar);
	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
    charutils::BuildingCharAbilityTable(PChar);
    charutils::BuildingCharTraitsTable(PChar);
    charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

   //charutils::SaveCharStats(PChar);
  // charutils::SaveCharJob(PChar, PChar->GetMJob());
  // charutils::SaveCharExp(PChar, PChar->GetMJob());
	charutils::UpdateHealth(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
    PChar->pushPacket(new CCharSkillsPacket(PChar));
    PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
    PChar->pushPacket(new CMenuMeritPacket(PChar));
    PChar->pushPacket(new CCharSyncPacket(PChar));
	
	
	int32 n = (uint8)lua_tointeger(L,1);
	string_t name= "none";
	if(n==1)
	{
		name ="Warrior";
	}
	if(n==2)
	{
		name ="Monk";
	}
	if(n==3)
	{
		name ="White Mage";
	}
	if(n==4)
	{
		name ="Black Mage";
	}
	if(n==5)
	{
		name ="Red Mage";
	}
	if(n==6)
	{
		name ="Thief";
	}
	if(n==7)
	{
		name ="Paladin";
	}
	if(n==8)
	{
		name ="Dark Knight";
	}
	if(n==9)
	{
		name ="Beastmaster";
	}
	if(n==10)
	{
		name ="Bard";
	}
	if(n==11)
	{
		name ="Ranger";
	}
	if(n==12)
	{
		name ="Samurai";
	}
	if(n==13)
	{
		name ="Ninja";
	}
	if(n==14)
	{
		name ="Dragoon";
	}
	if(n==15)
	{
		name ="Summoner";
	}
	if(n==16)
	{
		name ="Blue Mage";
	}
	if(n==17)
	{
		name ="Corsair";
	}
	if(n==18)
	{
		name ="Puppetmaster";
	}
	if(n==19)
	{
		name ="Dancer";
	}
	if(n==20)
	{
		name ="Scholar";
	}
	
        char buf[110];
        sprintf(buf,"Changeing Sub Job: ID %u NAME: %s",(int)n,name.c_str());
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	return false;
}



/************************************************************************
*                                                                       *
*  GM command @setslevel !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::set_sub_Level(lua_State *L)
{
    CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsublevel 1");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	 }
	
	if(PChar->Account_Level == 1)
	 {
	if(lua_tointeger(L,1) > 99 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsublevel 1-99");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }
	 if(PChar->Account_Level == 2)
	 {
	if(lua_tointeger(L,1) > 101 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsublevel 1-101");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }
	 if(PChar->Account_Level == 3 ||PChar->Account_Level == 4)
	 {
	if(lua_tointeger(L,1) > 150 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setsublevel 1-150");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }

	 if(PChar->GetSJob() == NULL)
	 {
		 char buf[110];
        sprintf(buf,"Unable To Set Your Subjob Level!");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		 return false;
	 }
	

	PChar->SetSLevel((uint8)lua_tointeger(L,1));
	//PChar->jobs.exp[PChar->GetMJob()] = 0;
	PChar->jobs.job[PChar->GetSJob()] = (uint8)lua_tointeger(L,1);// THIS WAS SET FOR MAIN NOW SET FOR SUB
	//PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
   charutils::BuildingCharSkillsTable(PChar);
  charutils::BuildingCharAbilityTable(PChar);
  charutils::BuildingCharTraitsTable(PChar);
  charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

  // charutils::SaveCharStats(PChar);
  // charutils::SaveCharJob(PChar, PChar->GetMJob());
  // charutils::SaveCharExp(PChar, PChar->GetMJob());
	//charutils::SaveCharPoints(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
  PChar->pushPacket(new CCharSkillsPacket(PChar));
   PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
   PChar->pushPacket(new CMenuMeritPacket(PChar));
   PChar->pushPacket(new CCharSyncPacket(PChar));
	int32 n = (uint8)lua_tointeger(L,1);
        char buf[110];
        sprintf(buf,"Setting Level For Sub: %d", (int)n);
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	
	//ShowNotice(CL_RED"Player: Sets New Level\n" CL_RESET);
	return false;
}


/************************************************************************
*                                                                       *
*  GM command @setlevel !!! FOR DEBUG ONLY !!!                         *
*                                                                       *
************************************************************************/

inline int32 CLuaBaseEntity::set_Level(lua_State *L)
{
     CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		return false;
	}
	 if(lua_isnil(L,-1) || !lua_isnumber(L,-1))
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainlevel 1");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		return false;
	 }
	 if(PChar->Account_Level == 1)
	 {
	if(lua_tointeger(L,1) > 99 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainlevel 1-99");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }
	 if(PChar->Account_Level == 2)
	 {
	if(lua_tointeger(L,1) > 101 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainlevel 1-101");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }
	 if(PChar->Account_Level == 3||PChar->Account_Level == 4)
	 {
	if(lua_tointeger(L,1) > 150 || lua_tointeger(L,1) < 1)
	{
		
		char buf[110];
        sprintf(buf,"Command Example: .setmainlevel 1-150");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
			return false;
	}
	
	 }

    
	

	
	PChar->SetMLevel((uint8)lua_tointeger(L,1));
	//PChar->jobs.exp[PChar->GetMJob()] = 0;
	PChar->jobs.job[PChar->GetMJob()] = (uint8)lua_tointeger(L,1);
	PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

	charutils::CalculateStats(PChar);
    charutils::CheckValidEquipment(PChar);
   charutils::BuildingCharSkillsTable(PChar);
  charutils::BuildingCharAbilityTable(PChar);
  charutils::BuildingCharTraitsTable(PChar);
  charutils::BuildingCharWeaponSkills(PChar);

	PChar->UpdateHealth();
    PChar->health.hp = PChar->GetMaxHP();
    PChar->health.mp = PChar->GetMaxMP();

   //charutils::SaveCharStats(PChar);
  // charutils::SaveCharJob(PChar, PChar->GetMJob());
  // charutils::SaveCharExp(PChar, PChar->GetMJob());
	//charutils::SaveCharPoints(PChar);

    PChar->pushPacket(new CCharJobsPacket(PChar));
    PChar->pushPacket(new CCharStatsPacket(PChar));
  PChar->pushPacket(new CCharSkillsPacket(PChar));
   PChar->pushPacket(new CCharAbilitiesPacket(PChar));
    PChar->pushPacket(new CCharUpdatePacket(PChar));
   PChar->pushPacket(new CMenuMeritPacket(PChar));
   PChar->pushPacket(new CCharSyncPacket(PChar));
	int32 n = (uint8)lua_tointeger(L,1);
        char buf[110];
        sprintf(buf,"Setting Level For Main: %d", (int)n);
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
	
	//ShowNotice(CL_RED"Player: Sets New Level\n" CL_RESET);
    return 0;
}
inline int32 CLuaBaseEntity::god_mode(lua_State *L)
{
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;

	if(m_PBaseEntity == NULL)
	{
		
		//ShowNotice(CL_RED"TRACER:COMMAND: godmode Error\n" CL_RESET);
		return true;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		//ShowNotice(CL_RED"TRACER:COMMAND: godmode Error1\n" CL_RESET);
		return true;
	}
	/*
	FLAG_GM_SUPPORT     = 0x04000000,
    FLAG_GM_SENIOR      = 0x05000000,
    FLAG_GM_LEAD        = 0x06000000,
    FLAG_GM_PRODUCER    = 0x07000000,
	*/
	
	if(PChar->godmode==0)
	{
		PChar->godmode=1;
		if(PChar->Account_Level==1)
		{
		PChar->nameflags.flags =FLAG_GM_SUPPORT;
		
		PChar->pushPacket(new CCharUpdatePacket(PChar));
		}
		if(PChar->Account_Level==2)
		{
		PChar->nameflags.flags =FLAG_GM_SENIOR;
		
		PChar->pushPacket(new CCharUpdatePacket(PChar));
		}
		if(PChar->Account_Level==3)
		{
		PChar->nameflags.flags =FLAG_GM_LEAD;
		
		PChar->pushPacket(new CCharUpdatePacket(PChar));
		}
		if(PChar->Account_Level==4)
		{
		PChar->nameflags.flags =FLAG_GM_PRODUCER;
		
		PChar->pushPacket(new CCharUpdatePacket(PChar));
		}
		
		char buf[110];
        sprintf(buf,"Activating God Mode");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		const int8* Query = "UPDATE chars SET godmode = '1' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   const int8* Query = "UPDATE char_stats SET nameflags = '%u' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->nameflags.flags,PChar->id);
					   charutils::BuildingCharWeaponSkills(PChar);
		return false;
	}
	else
	{

		PChar->nameflags.flags =0;
		PChar->godmode=0;
		
		
		PChar->pushPacket(new CCharUpdatePacket(PChar));
		
		char buf[110];
        sprintf(buf,"God Mode Is Disabled");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));
		const int8* Query = "UPDATE chars SET godmode = '0' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   const int8* Query = "UPDATE char_stats SET nameflags = '0' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   charutils::BuildingCharWeaponSkills(PChar);
		return false;
	}
	
	
	
	
    
	return false;
}
inline int32 CLuaBaseEntity::Auction_House(lua_State *L)
{
	CCharEntity* PChar = (CCharEntity*)m_PBaseEntity;
	
	if(m_PBaseEntity == NULL)
	{
		
		return false;
	}
	if(m_PBaseEntity->objtype != TYPE_PC)
	{
		
		return false;
	}
				

				
						
						
                        PChar->pushPacket(new CAuctionHousePacket(2));
       char buf[110];
        sprintf(buf,"Auction House Counter");
	    PChar->pushPacket(new CChatMessageStringPacket(PChar, MESSAGE_STRING_SAY , ("%s",buf)));              
					
	return false;
}
//==========================================================//

const int8 CLuaBaseEntity::className[] = "CBaseEntity";

Lunar<CLuaBaseEntity>::Register_t CLuaBaseEntity::methods[] =
{
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,warp),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,leavegame),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getShortID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getName),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getHPP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,restoreHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,restoreMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getTP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addTP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delTP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setTP),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getStat),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMaxHP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMaxMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getFreeSlotsCount),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getXPos),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getYPos),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getZPos),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRotPos),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getZone),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getZoneName),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getCurrentRegion),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPreviousZone),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getContinentID),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,isZoneVisited),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getWeather),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setWeather),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,setPos),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPos),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRace),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getNation),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setNation),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addQuest),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delQuest),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getQuestStatus),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,completeQuest),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasCompleteQuest),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addMission),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delMission),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getCurrentMission),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasCompletedMission),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,completeMission),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRankPoints),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setRankPoints),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addRankPoints),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addKeyItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasKeyItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,seenKeyItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,unseenKeyItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delKeyItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSkillLevel),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMaxSkillLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSkillRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setSkillRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addSpell),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasSpell),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,canLearnSpell),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delSpell),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addLearnedAbility),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasLearnedAbility),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,canLearnAbility),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delLearnedAbility),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMainJob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMainLvl),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSubJob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSubLvl),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,unlockJob),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,levelCap),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,levelRestriction),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sjRestriction),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getVar),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setVar),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,addVar),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setPetName),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getAutomatonName),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setMaskBit),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMaskBit),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,countMaskBits),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isMaskFull),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,release),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,startEvent),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,startEventString),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,updateEvent),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getEventTarget),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,openSendBox),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,showText),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,messageBasic),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,messagePublic),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,messageSpecial),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,messageSystem),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,clearTargID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sendMenu),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sendGuild),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setHomePoint),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,tradeComplete),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,confirmTrade),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasTitle),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getTitle),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,addTitle),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,delTitle),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setTitle),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getGil),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addGil),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delGil),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setGil),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addExp),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,createShop),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addShopItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getFame),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setFame),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addFame),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getFameLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getAnimation),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setAnimation),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,AnimationSub),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,speed),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,resetPlayer),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,costume),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,canUseCostume),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,canUseChocobo),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setStatus),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,setPVPFlag),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sendRaise),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sendReraise),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,sendTractor),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,addStatusEffectEx),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,canGainStatusEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasStatusEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasBustEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getStatusEffectElement),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,delStatusEffectSilent),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,eraseStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,dispelStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,dispelAllStatusEffect),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,eraseAllStatusEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,stealStatusEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addCorsairRoll),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasPartyJob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addMod),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMod),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setMod),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delMod),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMobMod),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setMobMod),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,setFlag),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,moghouseFlag),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,injectPacket),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,showPosition),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,updateEnmity),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,resetEnmity),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,updateEnmityFromDamage),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getEquipID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getShieldSize),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,lockEquipSlot),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,unlockEquipSlot),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,canEquipItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,equipItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPetElement),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPetName),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,charmPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,spawnPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,despawnPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,petAttack),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,petRetreat),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,petStay),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,petAbility),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,familiar),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPetID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,needToZone),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getContainerSize),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,changeContainerSize),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPartyMember),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPartySize),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getAllianceSize),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addPartyEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,removePartyEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasPartyEffect),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setsLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,changeJob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,changesJob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setMerits),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMerit),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getWeaponDmg),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getOffhandDmg),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getWeaponDmgRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getOffhandDmgRank),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,openDoor),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,wakeUp),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,updateEnmityFromCure),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isWeaponTwoHanded),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getWeaponSkillType),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getWeaponSubSkillType),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRangedDmg),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRangedDmgForRank),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getAmmoDmg),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRATT),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getRACC),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getACC),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getEVA),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,capSkill),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,capAllSkills),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addAllSpells),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMeleeHitDamage),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,resetRecasts),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,resetRecast),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,bcnmRegister),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,bcnmEnter),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,bcnmLeave),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isInBcnm),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isBcnmsFull),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isSpecialIntanceEmpty),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSpecialInstanceLeftTime),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,RestoreAndHealOnInstance),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,BCNMSetLoot),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getInstanceID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addCP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delCP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getCP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addPoint),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,delPoint),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getPoint),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addNationTeleport),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getNationTeleport),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isBehind),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isFacing),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hideNPC),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getStealItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getBCNMloot),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getDynamisUniqueID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addPlayerToDynamis),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addTimeToDynamis),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,launchDynamisSecondPart),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isInDynamis),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isInBattlefieldList),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addInBattlefieldList),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,addPlayerToSpecialInstance),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,addTimeToSpecialInstance),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkDistance),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkBaseExp),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkSoloPartyAlliance),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkExpPoints),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkFovAllianceAllowed),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasImmunity),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,rageMode),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isUndead),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,isMobType),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getBattleTime),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,changeSkin),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSkinID),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getSystem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getFamily),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,createWornItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasWornItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getObjType),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isPC),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isNPC),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isMob),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isPet),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,injectActionPacket),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasTrait),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,isTrickAttackAvailable),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setDelay),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setDamage),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,castSpell),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,useMobAbility),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,SetAutoAttackEnabled),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,SetMagicCastingEnabled),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,SetMobAbilityEnabled),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,updateTarget),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getExtraVar),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setExtraVar),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setSpellList),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasValidJugPetItem),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasTarget),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setBattleSubTarget),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,hasTPMoves),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getMaster),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,recalculateAbilitiesTable),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,recalculateSkillsTable),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,isSpellAoE),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getBaseHP),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,getBaseMP),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,checkNameFlags),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getGMLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,setGMLevel),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,PrintToPlayer),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,getBaseMP),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,pathThrough),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,atPoint),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,lookAt),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,clearPath),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,isFollowingPath),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,wait),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,knockback),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,setSpawn),
    LUNAR_DECLARE_METHOD(CLuaBaseEntity,setRespawnTime),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,unlockAttachment),
	//////////////////////////////////////////////////////////////
	//COMMAND SYSTEM
	//////////////////////////////////////////////////////////////
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,show_Command_Menu),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,set_speed),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,Zone),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,Set_Exp_Rates),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,set_Level),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,set_sub_Level),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,change_Job),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,change_sub_Job),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,god_mode),
	LUNAR_DECLARE_METHOD(CLuaBaseEntity,Auction_House),
	{NULL,NULL}
};