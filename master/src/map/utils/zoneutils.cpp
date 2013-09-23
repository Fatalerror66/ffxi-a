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

#include <string.h>
#include "../../common/timer.h"

#include "../ai/ai_mob_dummy.h"
#include "../ai/ai_npc_dummy.h"

#include "../lua/luautils.h"

#include "../conquest_system.h"
#include "../map.h"
#include "../entities/mobentity.h"
#include "../entities/npcentity.h"
#include "zoneutils.h"
#include "mobutils.h"
#include "../mob_spell_list.h"
#include "../packets/entity_update.h"
#include "../packets/zone_in.h"

CZone* g_PZoneList[MAX_ZONEID];	// глобальный массив указателей на игровые зоны
CNpcEntity*  g_PTrigger;	// триггер для запуска событий


namespace zoneutils
{

/************************************************************************
*																		*
*  Реакция зон на смену времени суток									*
*																		*
************************************************************************/

void TOTDCharnge(TIMETYPE TOTD)
{
	for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
	{
		g_PZoneList[ZoneID]->TOTDChange(TOTD);
	}
}

void UpdateTreasureSpawnPoint(uint32 npcid, uint32 respawnTime)
{
	CBaseEntity* PNpc = zoneutils::GetEntity(npcid, TYPE_NPC);

	if (PNpc != NULL) {

		int32 ret = Sql_Query(SqlHandle, "SELECT pos, pos_rot, pos_x, pos_y, pos_z FROM `treasure_spawn_points` WHERE npcid=%u ORDER BY RAND() LIMIT 1", npcid);

		if ( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS) {
			PNpc->loc.p.rotation = Sql_GetIntData(SqlHandle,1);
			PNpc->loc.p.x = Sql_GetFloatData(SqlHandle,2);
			PNpc->loc.p.y = Sql_GetFloatData(SqlHandle,3);
			PNpc->loc.p.z = Sql_GetFloatData(SqlHandle,4);
			// ShowDebug(CL_YELLOW"zoneutils::UpdateTreasureSpawnPoint: After %i - %d (%f, %f, %f), %d\n" CL_RESET, Sql_GetIntData(SqlHandle,0), PNpc->id, PNpc->loc.p.x,PNpc->loc.p.y,PNpc->loc.p.z, PNpc->loc.zone->GetID());
		} else {
			ShowDebug(CL_RED"zonetuils::UpdateTreasureSpawnPoint: SQL error or treasure <%u> not found in treasurespawnpoints table.\n" CL_RESET, npcid);
		}

		CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("reappear_npc", gettick()+respawnTime, PNpc, CTaskMgr::TASK_ONCE, reappear_npc));
	} else {
		ShowDebug(CL_RED"zonetuils::UpdateTreasureSpawnPoint: treasure <%u> not found\n" CL_RESET, npcid);
	}
}

/************************************************************************
*																		*
*  determines what weather conditions each zone qualifies for			*
*  and what the current weather condition should be						*
*																		*
************************************************************************/

// TOTO: общая погода для нескольких зон

void UpdateWeather()
{
    uint8 WeatherChange = 0;
    uint8 WeatherFrequency = 0;

    for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
    {
        if (!g_PZoneList[ZoneID]->IsWeatherStatic())
        {
            WeatherFrequency = 0;
            WeatherChange = rand()%100;

            for (uint8 weather = 0; weather < MAX_WEATHER_ID; ++weather)
            {
                WeatherFrequency += g_PZoneList[ZoneID]->m_WeatherFrequency[weather];

                if (WeatherChange < WeatherFrequency)
                {

                    g_PZoneList[ZoneID]->SetWeather((WEATHER)weather);
          					luautils::OnZoneWeatherChange(ZoneID, weather);
          					break;
                }
            }
        }
    }
    ShowDebug(CL_CYAN"UpdateWeather Finished\n" CL_RESET);
}

/************************************************************************
*																		*
*  Возвращаем указатель на класс зоны с указанным ID.					*
*																		*
************************************************************************/
CZone* GetZoneByChar(uint16 ZoneID, CCharEntity* PChar)
{
	if(PChar == NULL)
	{
		return false;
	}
	//ShowDebug(CL_CYAN "GET ZONE %u BY CHAR %s \n"CL_RESET,ZoneID,PChar->GetName());
    if(ZoneID >= MAX_ZONEID)
		{
			return false;
	}
	
	//ShowDebug(CL_CYAN "GET IN EVENT ID %u BY CHAR %s\n"CL_RESET,PChar->is_inevent,PChar->GetName());
	//ShowDebug(CL_CYAN "GET EVENT ID %u BY CHAR %s \n"CL_RESET,PChar->eventid,PChar->GetName());
	           
				 if(PChar->first_login == 0)
				 {
				 
				uint8 zone = 0;
			float pos_x = 0.0;
			float pos_y = 0.0;
			float pos_z = 0.0;
			uint8 pos_rot = 0;
			const int8* Query = "SELECT home_zone,home_x,home_y,home_z,home_rot FROM chars WHERE charid = '%u';";
	             int32 ret = Sql_Query(SqlHandle,Query,PChar->id);

	          if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	             {
				   zone =  Sql_GetUIntData(SqlHandle,0);
				  // ShowDebug(CL_RED"PLAYER %s HOME ZONE %u\n"CL_RESET,PChar->GetName(),zone);
				   pos_x =  Sql_GetFloatData(SqlHandle,1);
				 //   ShowDebug(CL_RED"PLAYER %s HOME x %.3f\n"CL_RESET,PChar->GetName(),pos_x);
				   pos_y =  Sql_GetFloatData(SqlHandle,2);
				 //   ShowDebug(CL_RED"PLAYER %s HOME y %.3f\n"CL_RESET,PChar->GetName(),pos_y);
				   pos_z =  Sql_GetFloatData(SqlHandle,3);
				 //   ShowDebug(CL_RED"PLAYER %s HOME z %.3f\n"CL_RESET,PChar->GetName(),pos_z);
				   pos_rot =  Sql_GetUIntData(SqlHandle,4);
				 //   ShowDebug(CL_RED"PLAYER %s HOME r %u\n"CL_RESET,PChar->GetName(),pos_rot);
				   PChar->loc.p.x = pos_x;
			       PChar->loc.p.y = pos_y;
			       PChar->loc.p.z = pos_z;
			       PChar->loc.p.rotation =pos_rot;
                   PChar->loc.destination = zone;
				 
			PChar->loc.boundary = 0;

			
		
				  // const int8* Query = "UPDATE chars SET returning = '1', pos_zone='%u', pos_prevzone='%u', pos_x='%.3f', pos_y='%.3f', pos_z='%.3f', pos_rot='%u' WHERE charid = %u";
                  //     Sql_Query(SqlHandle,Query,zone,zone,pos_x,pos_y,pos_z,pos_rot,PChar->id);
					   
					   
             PChar->is_returning = 1;
			

			//PChar->status = STATUS_DISAPPEAR;
			//PChar->animation = ANIMATION_NONE;

			PChar->clearPacketList();
			
			PChar->pushPacket(new CZoneInPacket(PChar,PChar->eventid));
 
	
				 }
				}
	return g_PZoneList[ZoneID];
}
CZone* GetZone(uint16 ZoneID)
{
    if(ZoneID >= MAX_ZONEID)
	{
		ZoneID = MIN_ZONEID;
	}
	return g_PZoneList[ZoneID];
}

CNpcEntity* GetTrigger(uint16 TargID, uint16 ZoneID)
{
	g_PTrigger->targid = TargID;
	g_PTrigger->id = ((4096 + ZoneID) << 12) + TargID;

	uint32 npcid = 0;
	const char * Query = "SELECT npcid FROM npc_list WHERE npcid = '%u';";
	          int32 ret3 = Sql_Query(SqlHandle,Query,g_PTrigger->id);
			if (ret3 != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	             {
						
				   npcid =  Sql_GetUIntData(SqlHandle,0);
				   ShowWarning(CL_YELLOW"Server Has NPC NAME %s\n" CL_RESET,g_PTrigger->name.c_str());
				 ShowWarning(CL_YELLOW"Server Has NPC ID %u\n" CL_RESET,g_PTrigger->id);
				 ShowWarning(CL_YELLOW"Server Has NPC ZONE %u \n" CL_RESET,ZoneID);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK BODY %u \n" CL_RESET,g_PTrigger->look.body);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK FACE %u \n" CL_RESET,g_PTrigger->look.face);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK FEET %u \n" CL_RESET,g_PTrigger->look.feet);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK HANDS %u \n" CL_RESET,g_PTrigger->look.hands);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK HEAD %u \n" CL_RESET,g_PTrigger->look.head);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK LEGS %u \n" CL_RESET,g_PTrigger->look.legs);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK MAIN %u \n" CL_RESET,g_PTrigger->look.main);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK RACE %u \n" CL_RESET,g_PTrigger->look.race);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK RANGE %u \n" CL_RESET,g_PTrigger->look.ranged);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK SIZE %u \n" CL_RESET,g_PTrigger->look.size);
				 ShowWarning(CL_YELLOW"Server Has NPC LOOK SUB %u \n" CL_RESET,g_PTrigger->look.sub);
                 ShowWarning(CL_YELLOW"Server Has NPC <%u>\n" CL_RESET, npcid);
				 }
				 else
				 {
					 //INSERT INTO
					 Query = "INSERT INTO npc_list(npcid,name,zoneid) VALUES('%u','%s','%u');";

	if( Sql_Query(SqlHandle,Query,g_PTrigger->id,g_PTrigger->GetName(),ZoneID) == SQL_ERROR )
	{
		
		return g_PTrigger;
	}
	             ShowWarning(CL_YELLOW"Server needs NPC NAME %s\n" CL_RESET,g_PTrigger->name.c_str());
				 ShowWarning(CL_YELLOW"Server needs NPC ID %u\n" CL_RESET,g_PTrigger->id);
				 ShowWarning(CL_YELLOW"Server needs NPC ZONE %u \n" CL_RESET,ZoneID);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK BODY %u \n" CL_RESET,g_PTrigger->look.body);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK FACE %u \n" CL_RESET,g_PTrigger->look.face);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK FEET %u \n" CL_RESET,g_PTrigger->look.feet);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK HANDS %u \n" CL_RESET,g_PTrigger->look.hands);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK HEAD %u \n" CL_RESET,g_PTrigger->look.head);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK LEGS %u \n" CL_RESET,g_PTrigger->look.legs);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK MAIN %u \n" CL_RESET,g_PTrigger->look.main);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK RACE %u \n" CL_RESET,g_PTrigger->look.race);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK RANGE %u \n" CL_RESET,g_PTrigger->look.ranged);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK SIZE %u \n" CL_RESET,g_PTrigger->look.size);
				 ShowWarning(CL_YELLOW"Server needs NPC LOOK SUB %u \n" CL_RESET,g_PTrigger->look.sub);
                 ShowWarning(CL_YELLOW"Server needs NPC not found in database auto build npc success \n" CL_RESET);
				 }
	return g_PTrigger;
}

/************************************************************************
*                                                                       *
*  Получаем указатель на любую сущность по ID                           *
*                                                                       *
************************************************************************/

CBaseEntity* GetEntity(uint32 ID, uint8 filter)
{
    return g_PZoneList[(uint8)(ID >> 12)]->GetEntity((uint16)(ID & 0x0FFF), filter);
}

/************************************************************************
*                                                                       *
*  Получаем указатель на персонажа по имени                             *
*                                                                       *
************************************************************************/

CCharEntity* GetCharByName(int8* name)
{
    for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
    {
        CCharEntity* PChar = g_PZoneList[ZoneID]->GetCharByName(name);

        if (PChar != NULL)
        {
		    return PChar;
		}
    }
    return NULL;
}

/************************************************************************
*                                                                       *
*  Получаем указатель на CCharEntity по id и targid                     *
*                                                                       *
************************************************************************/

CCharEntity* GetCharFromRegion(uint32 charid, uint16 targid, uint8 RegionID)
{
    for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
	{
        if (g_PZoneList[ZoneID]->GetRegionID() == RegionID)
        {
            CBaseEntity* PEntity = g_PZoneList[ZoneID]->GetEntity(targid, TYPE_PC);

            if (PEntity != NULL && PEntity->id == charid)
            {
                return (CCharEntity*)PEntity;
            }
        }
    }
    return NULL;
}

/************************************************************************
*                                                                       *
*  Загружаем список NPC в указанную зону                                *
*                                                                       *
************************************************************************/

void LoadNPCList(CZone* PZone)
{
    const int8* Query =
        "SELECT \
          npcid,\
          name,\
          pos_rot,\
          pos_x,\
          pos_y,\
          pos_z,\
          flag,\
          speed,\
          speedsub,\
          animation,\
          animationsub,\
          namevis,\
          status,\
          unknown,\
          look,\
          name_prefix, \
		  zoneid \
        FROM npc_list \
        WHERE zoneid = %u AND npcid < 1024;";

    int32 ret = Sql_Query(SqlHandle, Query, PZone->GetID());

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			CNpcEntity* PNpc = new CNpcEntity;
            PNpc->targid = (uint16)Sql_GetUIntData(SqlHandle,0);
            PNpc->id = (uint32)PNpc->targid  + (PZone->GetID() << 12) + 0x1000000;

			PNpc->name.insert(0,Sql_GetData(SqlHandle,1));

			PNpc->loc.p.rotation = (uint8)Sql_GetIntData(SqlHandle,2);
			PNpc->loc.p.x = Sql_GetFloatData(SqlHandle,3);
			PNpc->loc.p.y = Sql_GetFloatData(SqlHandle,4);
			PNpc->loc.p.z = Sql_GetFloatData(SqlHandle,5);
			PNpc->loc.p.moving  = (uint16)Sql_GetUIntData(SqlHandle,6);

			PNpc->m_TargID = (uint32)Sql_GetUIntData(SqlHandle,6) >> 16; // вполне вероятно

			PNpc->speed = (uint8)Sql_GetIntData(SqlHandle,7);
			PNpc->speedsub = (uint8)Sql_GetIntData(SqlHandle,8);
			PNpc->animation = (uint8)Sql_GetIntData(SqlHandle,9);
			PNpc->animationsub = (uint8)Sql_GetIntData(SqlHandle,10);

			PNpc->namevis = (uint8)Sql_GetIntData(SqlHandle,11);
			PNpc->status  = (STATUSTYPE)Sql_GetIntData(SqlHandle,12);
			PNpc->unknown = (uint32)Sql_GetUIntData(SqlHandle,13);

			PNpc->name_prefix = (uint8)Sql_GetIntData(SqlHandle,15);
			PNpc->loc.destination = (uint16)Sql_GetIntData(SqlHandle,16);
			memcpy(&PNpc->look,Sql_GetData(SqlHandle,14),20);

			PZone->InsertNPC(PNpc);
		}
	}

	// Load npc patrol list. This will enable the npcs to move around and follow patrol points.

	const int8* QueryPatrol =
		"SELECT npc_dummies.npcid FROM npc_dummies \
		LEFT JOIN npc_list ON (npc_list.npcid + (4096 * zoneid) + 0x1000000) = npc_dummies.npcid \
		WHERE zoneid = %u;";


	ret = Sql_Query(SqlHandle, QueryPatrol, PZone->GetID());

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			uint32 npcid = (uint32)Sql_GetUIntData(SqlHandle,0);
			CNpcEntity* PNpc = (CNpcEntity*)PZone->GetEntity(npcid & 0x0FFF, TYPE_NPC);
			if(PNpc == NULL)
			{
				ShowError("zoneutils::LoadNPCList Npc could not be found (%d)\n", npcid);
			}
			else
			{
				PNpc->Check_Engagment = new CAINpcDummy(PNpc);
				PNpc->Check_Engagment->SetCurrentAction(ACTION_SPAWN);
			}
		}
	}
}

/************************************************************************
*                                                                       *
*  Загружаем список монстров в указанную зону                           *
*                                                                       *
************************************************************************/
void LoadMOBList(CZone* PZone)
{
    const int8* Query =
        "SELECT name, mobid, pos_rot, pos_x, pos_y, pos_z, \
			respawntime, spawntype, dropid, mob_groups.HP, mob_groups.MP, minLevel, maxLevel, \
			modelid, mJob, sJob, cmbSkill, cmbDelay, behavior, links, mobType, immunity, \
			systemid, mobsize, speed, \
			STR, DEX, VIT, AGI, `INT`, MND, CHR, EVA, DEF, \
			Slash, Pierce, H2H, Impact, \
			Fire, Ice, Wind, Earth, Lightning, Water, Light, Dark, Element, \
			mob_pools.familyid, name_prefix, unknown, animationsub, \
			(mob_family_system.HP / 100), (mob_family_system.MP / 100), hasSpellScript, spellList, ATT, ACC, mob_groups.poolid,mob_groups.zoneid \
			FROM mob_groups LEFT JOIN mob_pools ON mob_groups.poolid = mob_pools.poolid \
			LEFT JOIN mob_spawn_points ON mob_groups.groupid = mob_spawn_points.groupid \
			LEFT JOIN mob_family_system ON mob_pools.familyid = mob_family_system.familyid \
			WHERE (pos_x <> 0 AND pos_y <> 0 AND pos_z <> 0) \
			AND mob_groups.zoneid = %u;";

    int32 ret = Sql_Query(SqlHandle, Query, PZone->GetID());

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			CMobEntity* PMob = new CMobEntity;

			PMob->name.insert(0,Sql_GetData(SqlHandle,0));
			PMob->id = (uint32)Sql_GetUIntData(SqlHandle,1);
			PMob->targid = (uint16)PMob->id & 0x0FFF;

			PMob->m_SpawnPoint.rotation = (uint8)Sql_GetIntData(SqlHandle,2);
			PMob->m_SpawnPoint.x = Sql_GetFloatData(SqlHandle,3);
			PMob->m_SpawnPoint.y = Sql_GetFloatData(SqlHandle,4);
			PMob->m_SpawnPoint.z = Sql_GetFloatData(SqlHandle,5);

			PMob->m_RespawnTime = Sql_GetUIntData(SqlHandle,6) * 1000;
			PMob->m_SpawnType   = (SPAWNTYPE)Sql_GetUIntData(SqlHandle,7);
			PMob->m_DropID		= Sql_GetUIntData(SqlHandle,8);

			PMob->HPmodifier = (uint32)Sql_GetIntData(SqlHandle,9);
			PMob->MPmodifier = (uint32)Sql_GetIntData(SqlHandle,10);

			PMob->m_minLevel = (uint8)Sql_GetIntData(SqlHandle,11);
			PMob->m_maxLevel = (uint8)Sql_GetIntData(SqlHandle,12);

			memcpy(&PMob->look,Sql_GetData(SqlHandle,13),23);

			PMob->SetMJob(Sql_GetIntData(SqlHandle,14));
			PMob->SetSJob(Sql_GetIntData(SqlHandle,15));

			PMob->m_Weapons[SLOT_MAIN]->setMaxHit(1);
			PMob->m_Weapons[SLOT_MAIN]->setSkillType(Sql_GetIntData(SqlHandle,16));
			PMob->m_Weapons[SLOT_MAIN]->setDelay((Sql_GetIntData(SqlHandle,17) * 1000)/60);
			PMob->m_Weapons[SLOT_MAIN]->setBaseDelay((Sql_GetIntData(SqlHandle,17) * 1000)/60);

			PMob->m_Behaviour  = (uint16)Sql_GetIntData(SqlHandle,18);
			PMob->m_Link       = (uint8)Sql_GetIntData(SqlHandle,19);
			PMob->m_Type       = (uint8)Sql_GetIntData(SqlHandle,20);
			PMob->m_Immunity   = (IMMUNITY)Sql_GetIntData(SqlHandle,21);
			PMob->m_EcoSystem  = (ECOSYSTEM)Sql_GetIntData(SqlHandle,22);
			PMob->m_ModelSize += (uint8)Sql_GetIntData(SqlHandle,23);

			PMob->speed    = (uint8)Sql_GetIntData(SqlHandle,24);
			PMob->speedsub = (uint8)Sql_GetIntData(SqlHandle,24);

			/*if(PMob->speed != 0)
			{
				PMob->speed += map_config.speed_mod;
                // whats this for?
				PMob->speedsub += map_config.speed_mod;
			}*/

            PMob->strRank = (uint8)Sql_GetIntData(SqlHandle,25);
            PMob->dexRank = (uint8)Sql_GetIntData(SqlHandle,26);
            PMob->vitRank = (uint8)Sql_GetIntData(SqlHandle,27);
            PMob->agiRank = (uint8)Sql_GetIntData(SqlHandle,28);
            PMob->intRank = (uint8)Sql_GetIntData(SqlHandle,29);
            PMob->mndRank = (uint8)Sql_GetIntData(SqlHandle,30);
            PMob->chrRank = (uint8)Sql_GetIntData(SqlHandle,31);
            PMob->evaRank = (uint8)Sql_GetIntData(SqlHandle,32);
            PMob->defRank = (uint8)Sql_GetIntData(SqlHandle,33);
            PMob->attRank = (uint8)Sql_GetIntData(SqlHandle,55);
            PMob->accRank = (uint8)Sql_GetIntData(SqlHandle,56);
			PMob->loc.destination = (uint16)Sql_GetUIntData(SqlHandle,58);
			PMob->setModifier(MOD_SLASHRES, (uint16)(Sql_GetFloatData(SqlHandle,34) * 1000));
			PMob->setModifier(MOD_PIERCERES,(uint16)(Sql_GetFloatData(SqlHandle,35) * 1000));
			PMob->setModifier(MOD_HTHRES,   (uint16)(Sql_GetFloatData(SqlHandle,36) * 1000));
			PMob->setModifier(MOD_IMPACTRES,(uint16)(Sql_GetFloatData(SqlHandle,37) * 1000));

            PMob->setModifier(MOD_FIREDEF,    (int16)((Sql_GetFloatData(SqlHandle, 38) - 1) * -1000)); // These are stored as floating percentages
            PMob->setModifier(MOD_ICEDEF,     (int16)((Sql_GetFloatData(SqlHandle, 39) - 1) * -1000)); // and need to be adjusted into modifier units.
            PMob->setModifier(MOD_WINDDEF,    (int16)((Sql_GetFloatData(SqlHandle, 40) - 1) * -1000)); // Higher DEF = lower damage.
            PMob->setModifier(MOD_EARTHDEF,   (int16)((Sql_GetFloatData(SqlHandle, 41) - 1) * -1000)); // Negatives signify increased damage.
            PMob->setModifier(MOD_THUNDERDEF, (int16)((Sql_GetFloatData(SqlHandle, 42) - 1) * -1000)); // Positives signify reduced damage.
            PMob->setModifier(MOD_WATERDEF,   (int16)((Sql_GetFloatData(SqlHandle, 43) - 1) * -1000)); // Ex: 125% damage would be 1.25, 50% damage would be 0.50
            PMob->setModifier(MOD_LIGHTDEF,   (int16)((Sql_GetFloatData(SqlHandle, 44) - 1) * -1000)); // (1.25 - 1) * -1000 = -250 DEF
            PMob->setModifier(MOD_DARKDEF,    (int16)((Sql_GetFloatData(SqlHandle, 45) - 1) * -1000)); // (0.50 - 1) * -1000 = 500 DEF

            PMob->setModifier(MOD_FIRERES,    (int16)((Sql_GetFloatData(SqlHandle, 38) - 1) * -100)); // These are stored as floating percentages
            PMob->setModifier(MOD_ICERES,     (int16)((Sql_GetFloatData(SqlHandle, 39) - 1) * -100)); // and need to be adjusted into modifier units.
            PMob->setModifier(MOD_WINDRES,    (int16)((Sql_GetFloatData(SqlHandle, 40) - 1) * -100)); // Higher RES = lower damage.
            PMob->setModifier(MOD_EARTHRES,   (int16)((Sql_GetFloatData(SqlHandle, 41) - 1) * -100)); // Negatives signify lower resist chance.
            PMob->setModifier(MOD_THUNDERRES, (int16)((Sql_GetFloatData(SqlHandle, 42) - 1) * -100)); // Positives signify increased resist chance.
            PMob->setModifier(MOD_WATERRES,   (int16)((Sql_GetFloatData(SqlHandle, 43) - 1) * -100));
            PMob->setModifier(MOD_LIGHTRES,   (int16)((Sql_GetFloatData(SqlHandle, 44) - 1) * -100));
            PMob->setModifier(MOD_DARKRES,    (int16)((Sql_GetFloatData(SqlHandle, 45) - 1) * -100));

			PMob->m_Element = (uint8)Sql_GetIntData(SqlHandle,46);
			PMob->m_Family = (uint16)Sql_GetIntData(SqlHandle,47);
			PMob->m_name_prefix = (uint8)Sql_GetIntData(SqlHandle,48);
			PMob->m_unknown = (uint32)Sql_GetIntData(SqlHandle,49);

			//Special sub animation for Mob (yovra, jailer of love, phuabo)
			// yovra 1: en hauteur, 2: en bas, 3: en haut
			// phuabo 1: sous l'eau, 2: sort de l'eau, 3: rentre dans l'eau
			PMob->animationsub = (uint32)Sql_GetIntData(SqlHandle,50);

			// Setup HP / MP Stat Percentage Boost
			PMob->HPscale = Sql_GetFloatData(SqlHandle,51);
			PMob->MPscale = Sql_GetFloatData(SqlHandle,52);

			PMob->Check_Engagment = new CAIMobDummy(PMob);

			if (PMob->m_AllowRespawn = PMob->m_SpawnType == SPAWNTYPE_NORMAL)
			{
				PMob->Check_Engagment->SetCurrentAction(ACTION_SPAWN);
			}

			// Check if we should be looking up scripts for this mob
			PMob->m_HasSpellScript = (uint8)Sql_GetIntData(SqlHandle,53);

			PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(Sql_GetIntData(SqlHandle,54));

			PMob->m_Pool = Sql_GetUIntData(SqlHandle,57);

            // must be here first to define mobmods
			mobutils::InitializeMob(PMob, PZone);

            PZone->InsertMOB(PMob);

			luautils::OnMobInitialize(PMob);
		}
	}

	// attach pets to mobs
	const int8* PetQuery =
		"SELECT mob_mobid, pet_offset \
		FROM mob_pets \
		LEFT JOIN mob_spawn_points ON mob_pets.mob_mobid = mob_spawn_points.mobid \
		LEFT JOIN mob_groups ON mob_spawn_points.groupid = mob_groups.groupid \
		WHERE mob_groups.zoneid = %u;";

	ret = Sql_Query(SqlHandle, PetQuery, PZone->GetID());

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			uint32 masterid = (uint32)Sql_GetUIntData(SqlHandle,0);
			uint32 petid = masterid + (uint32)Sql_GetUIntData(SqlHandle,1);

			CMobEntity* PMaster = (CMobEntity*)PZone->GetEntity(masterid & 0x0FFF, TYPE_MOB);
			CMobEntity* PPet = (CMobEntity*)PZone->GetEntity(petid & 0x0FFF, TYPE_MOB);

			if(PMaster == NULL)
			{
				ShowError("zoneutils::loadMOBList PMaster is null. masterid: %d. Make sure x,y,z are not zeros!\n", masterid);
			}
			else if(PPet == NULL)
			{
				ShowError("zoneutils::loadMOBList PPet is null. petid: %d. Make sure x,y,z are not zeros!\n", petid);
			}
			else if(masterid == petid)
			{
				ShowError("zoneutils::loadMOBList Master and Pet are the same entity: %d\n", masterid);
			}
			else
			{
				// pet is always spawned by master
				PPet->m_AllowRespawn = false;
				PPet->m_SpawnType = SPAWNTYPE_SCRIPTED;
				PPet->Check_Engagment->SetCurrentAction(ACTION_NONE);
				PPet->SetDespawnTimer(0);

				PMaster->PPet = PPet;
				PPet->PMaster = PMaster;
			}
		}
	}
}

CZone* LoadPlayerMOBList(uint16 ZoneID,uint16 MobID)//function to reload moblist by player its not done yet i do not want to load a whole zone just one mob at a time.
{
	CZone* PZone = new CZone((ZONEID)ZoneID, GetCurrentRegion(ZoneID), GetCurrentContinent(ZoneID));
    const int8* Query =
        "SELECT name, mobid, pos_rot, pos_x, pos_y, pos_z, \
			respawntime, spawntype, dropid, mob_groups.HP, mob_groups.MP, minLevel, maxLevel, \
			modelid, mJob, sJob, cmbSkill, cmbDelay, behavior, links, mobType, immunity, \
			systemid, mobsize, speed, \
			STR, DEX, VIT, AGI, `INT`, MND, CHR, EVA, DEF, \
			Slash, Pierce, H2H, Impact, \
			Fire, Ice, Wind, Earth, Lightning, Water, Light, Dark, Element, \
			mob_pools.familyid, name_prefix, unknown, animationsub, \
			(mob_family_system.HP / 100), (mob_family_system.MP / 100), hasSpellScript, spellList, ATT, ACC, mob_groups.poolid,mob_groups.zoneid \
			FROM mob_groups LEFT JOIN mob_pools ON mob_groups.poolid = mob_pools.poolid \
			LEFT JOIN mob_spawn_points ON mob_groups.groupid = mob_spawn_points.groupid \
			LEFT JOIN mob_family_system ON mob_pools.familyid = mob_family_system.familyid \
			WHERE mobid = %u \
			AND mob_groups.zoneid = %u LIMIT 1;";

    int32 ret = Sql_Query(SqlHandle, Query, ZoneID);

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			CMobEntity* PMob = new CMobEntity;

			PMob->name.insert(0,Sql_GetData(SqlHandle,0));
			PMob->id = (uint32)Sql_GetUIntData(SqlHandle,1);
			PMob->targid = (uint16)PMob->id & 0x0FFF;

			PMob->m_SpawnPoint.rotation = (uint8)Sql_GetIntData(SqlHandle,2);
			PMob->m_SpawnPoint.x = Sql_GetFloatData(SqlHandle,3);
			PMob->m_SpawnPoint.y = Sql_GetFloatData(SqlHandle,4);
			PMob->m_SpawnPoint.z = Sql_GetFloatData(SqlHandle,5);

			PMob->m_RespawnTime = Sql_GetUIntData(SqlHandle,6) * 1000;
			PMob->m_SpawnType   = (SPAWNTYPE)Sql_GetUIntData(SqlHandle,7);
			PMob->m_DropID		= Sql_GetUIntData(SqlHandle,8);

			PMob->HPmodifier = (uint32)Sql_GetIntData(SqlHandle,9);
			PMob->MPmodifier = (uint32)Sql_GetIntData(SqlHandle,10);

			PMob->m_minLevel = (uint8)Sql_GetIntData(SqlHandle,11);
			PMob->m_maxLevel = (uint8)Sql_GetIntData(SqlHandle,12);

			memcpy(&PMob->look,Sql_GetData(SqlHandle,13),23);

			PMob->SetMJob(Sql_GetIntData(SqlHandle,14));
			PMob->SetSJob(Sql_GetIntData(SqlHandle,15));

			PMob->m_Weapons[SLOT_MAIN]->setMaxHit(1);
			PMob->m_Weapons[SLOT_MAIN]->setSkillType(Sql_GetIntData(SqlHandle,16));
			PMob->m_Weapons[SLOT_MAIN]->setDelay((Sql_GetIntData(SqlHandle,17) * 1000)/60);
			PMob->m_Weapons[SLOT_MAIN]->setBaseDelay((Sql_GetIntData(SqlHandle,17) * 1000)/60);

			PMob->m_Behaviour  = (uint16)Sql_GetIntData(SqlHandle,18);
			PMob->m_Link       = (uint8)Sql_GetIntData(SqlHandle,19);
			PMob->m_Type       = (uint8)Sql_GetIntData(SqlHandle,20);
			PMob->m_Immunity   = (IMMUNITY)Sql_GetIntData(SqlHandle,21);
			PMob->m_EcoSystem  = (ECOSYSTEM)Sql_GetIntData(SqlHandle,22);
			PMob->m_ModelSize += (uint8)Sql_GetIntData(SqlHandle,23);

			PMob->speed    = (uint8)Sql_GetIntData(SqlHandle,24);
			PMob->speedsub = (uint8)Sql_GetIntData(SqlHandle,24);

			/*if(PMob->speed != 0)
			{
				PMob->speed += map_config.speed_mod;
                // whats this for?
				PMob->speedsub += map_config.speed_mod;
			}*/

            PMob->strRank = (uint8)Sql_GetIntData(SqlHandle,25);
            PMob->dexRank = (uint8)Sql_GetIntData(SqlHandle,26);
            PMob->vitRank = (uint8)Sql_GetIntData(SqlHandle,27);
            PMob->agiRank = (uint8)Sql_GetIntData(SqlHandle,28);
            PMob->intRank = (uint8)Sql_GetIntData(SqlHandle,29);
            PMob->mndRank = (uint8)Sql_GetIntData(SqlHandle,30);
            PMob->chrRank = (uint8)Sql_GetIntData(SqlHandle,31);
            PMob->evaRank = (uint8)Sql_GetIntData(SqlHandle,32);
            PMob->defRank = (uint8)Sql_GetIntData(SqlHandle,33);
            PMob->attRank = (uint8)Sql_GetIntData(SqlHandle,55);
            PMob->accRank = (uint8)Sql_GetIntData(SqlHandle,56);

			PMob->setModifier(MOD_SLASHRES, (uint16)(Sql_GetFloatData(SqlHandle,34) * 1000));
			PMob->setModifier(MOD_PIERCERES,(uint16)(Sql_GetFloatData(SqlHandle,35) * 1000));
			PMob->setModifier(MOD_HTHRES,   (uint16)(Sql_GetFloatData(SqlHandle,36) * 1000));
			PMob->setModifier(MOD_IMPACTRES,(uint16)(Sql_GetFloatData(SqlHandle,37) * 1000));

            PMob->setModifier(MOD_FIREDEF,    (int16)((Sql_GetFloatData(SqlHandle, 38) - 1) * -1000)); // These are stored as floating percentages
            PMob->setModifier(MOD_ICEDEF,     (int16)((Sql_GetFloatData(SqlHandle, 39) - 1) * -1000)); // and need to be adjusted into modifier units.
            PMob->setModifier(MOD_WINDDEF,    (int16)((Sql_GetFloatData(SqlHandle, 40) - 1) * -1000)); // Higher DEF = lower damage.
            PMob->setModifier(MOD_EARTHDEF,   (int16)((Sql_GetFloatData(SqlHandle, 41) - 1) * -1000)); // Negatives signify increased damage.
            PMob->setModifier(MOD_THUNDERDEF, (int16)((Sql_GetFloatData(SqlHandle, 42) - 1) * -1000)); // Positives signify reduced damage.
            PMob->setModifier(MOD_WATERDEF,   (int16)((Sql_GetFloatData(SqlHandle, 43) - 1) * -1000)); // Ex: 125% damage would be 1.25, 50% damage would be 0.50
            PMob->setModifier(MOD_LIGHTDEF,   (int16)((Sql_GetFloatData(SqlHandle, 44) - 1) * -1000)); // (1.25 - 1) * -1000 = -250 DEF
            PMob->setModifier(MOD_DARKDEF,    (int16)((Sql_GetFloatData(SqlHandle, 45) - 1) * -1000)); // (0.50 - 1) * -1000 = 500 DEF

            PMob->setModifier(MOD_FIRERES,    (int16)((Sql_GetFloatData(SqlHandle, 38) - 1) * -100)); // These are stored as floating percentages
            PMob->setModifier(MOD_ICERES,     (int16)((Sql_GetFloatData(SqlHandle, 39) - 1) * -100)); // and need to be adjusted into modifier units.
            PMob->setModifier(MOD_WINDRES,    (int16)((Sql_GetFloatData(SqlHandle, 40) - 1) * -100)); // Higher RES = lower damage.
            PMob->setModifier(MOD_EARTHRES,   (int16)((Sql_GetFloatData(SqlHandle, 41) - 1) * -100)); // Negatives signify lower resist chance.
            PMob->setModifier(MOD_THUNDERRES, (int16)((Sql_GetFloatData(SqlHandle, 42) - 1) * -100)); // Positives signify increased resist chance.
            PMob->setModifier(MOD_WATERRES,   (int16)((Sql_GetFloatData(SqlHandle, 43) - 1) * -100));
            PMob->setModifier(MOD_LIGHTRES,   (int16)((Sql_GetFloatData(SqlHandle, 44) - 1) * -100));
            PMob->setModifier(MOD_DARKRES,    (int16)((Sql_GetFloatData(SqlHandle, 45) - 1) * -100));

			PMob->m_Element = (uint8)Sql_GetIntData(SqlHandle,46);
			PMob->m_Family = (uint16)Sql_GetIntData(SqlHandle,47);
			PMob->m_name_prefix = (uint8)Sql_GetIntData(SqlHandle,48);
			PMob->m_unknown = (uint32)Sql_GetIntData(SqlHandle,49);
			PMob->loc.destination = (uint16)Sql_GetUIntData(SqlHandle,58);
			//Special sub animation for Mob (yovra, jailer of love, phuabo)
			// yovra 1: en hauteur, 2: en bas, 3: en haut
			// phuabo 1: sous l'eau, 2: sort de l'eau, 3: rentre dans l'eau
			PMob->animationsub = (uint32)Sql_GetIntData(SqlHandle,50);

			// Setup HP / MP Stat Percentage Boost
			PMob->HPscale = Sql_GetFloatData(SqlHandle,51);
			PMob->MPscale = Sql_GetFloatData(SqlHandle,52);

			PMob->Check_Engagment = new CAIMobDummy(PMob);

			if (PMob->m_AllowRespawn = PMob->m_SpawnType == SPAWNTYPE_NORMAL)
			{
				PMob->Check_Engagment->SetCurrentAction(ACTION_SPAWN);
			}

			// Check if we should be looking up scripts for this mob
			PMob->m_HasSpellScript = (uint8)Sql_GetIntData(SqlHandle,53);

			PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(Sql_GetIntData(SqlHandle,54));

			PMob->m_Pool = Sql_GetUIntData(SqlHandle,57);

            // must be here first to define mobmods
			mobutils::InitializeMob(PMob, PZone);

            PZone->InsertMOB(PMob);

			luautils::OnMobInitialize(PMob);
		}
	}

	// attach pets to mobs
	const int8* PetQuery =
		"SELECT mob_mobid, pet_offset \
		FROM mob_pets \
		LEFT JOIN mob_spawn_points ON mob_pets.mob_mobid = mob_spawn_points.mobid \
		LEFT JOIN mob_groups ON mob_spawn_points.groupid = mob_groups.groupid \
		WHERE mob_groups.zoneid = %u LIMIT 50;";

	ret = Sql_Query(SqlHandle, PetQuery, ZoneID);

	if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			uint32 masterid = (uint32)Sql_GetUIntData(SqlHandle,0);
			uint32 petid = masterid + (uint32)Sql_GetUIntData(SqlHandle,1);

			CMobEntity* PMaster = (CMobEntity*)PZone->GetEntity(masterid & 0x0FFF, TYPE_MOB);
			CMobEntity* PPet = (CMobEntity*)PZone->GetEntity(petid & 0x0FFF, TYPE_MOB);

			if(PMaster == NULL)
			{
				ShowError("zoneutils::loadMOBList PMaster is null. masterid: %d. Make sure x,y,z are not zeros!\n", masterid);
			}
			else if(PPet == NULL)
			{
				ShowError("zoneutils::loadMOBList PPet is null. petid: %d. Make sure x,y,z are not zeros!\n", petid);
			}
			else if(masterid == petid)
			{
				ShowError("zoneutils::loadMOBList Master and Pet are the same entity: %d\n", masterid);
			}
			else
			{
				// pet is always spawned by master
				PPet->m_AllowRespawn = false;
				PPet->m_SpawnType = SPAWNTYPE_SCRIPTED;
				PPet->Check_Engagment->SetCurrentAction(ACTION_NONE);
				PPet->SetDespawnTimer(0);

				PMaster->PPet = PPet;
				PPet->PMaster = PMaster;
			}
		}
	}
	return false;
}

/************************************************************************
*																		*
*  Инициализация зон. Возрождаем всех монстров при старте сервера.		*
*																		*
************************************************************************/

void LoadZoneList()
{
	g_PTrigger = new CNpcEntity();	// нужно в конструкторе CNpcEntity задавать модель по умолчанию

	for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
	{
        CZone* PZone = new CZone((ZONEID)ZoneID, GetCurrentRegion(ZoneID), GetCurrentContinent(ZoneID));

		LoadNPCList(PZone);
		LoadMOBList(PZone);

		PZone->ZoneServer(-1);
		g_PZoneList[ZoneID] = PZone;

		if (PZone->GetLANIP() != 0 || PZone->GetWANIP() != 0 || PZone->GetPort() != 0)
		{
			luautils::OnZoneInitialize(PZone->GetID());
		}
	}
    UpdateWeather();
}

/************************************************************************
*                                                                       *
*  Узнаем текущий регион по номеру зоны                                 *
*                                                                       *
************************************************************************/

REGIONTYPE GetCurrentRegion(uint16 ZoneID)
{
	switch (ZoneID)
	{
		case ZONE_BOSTAUNIEUX_OUBLIETTE:
		case ZONE_EAST_RONFAURE:
		case ZONE_FORT_GHELSBA:
		case ZONE_GHELSBA_OUTPOST:
		case ZONE_HORLAIS_PEAK:
		case ZONE_KING_RANPERRES_TOMB:
		case ZONE_WEST_RONFAURE:
		case ZONE_YUGHOTT_GROTTO:
			return REGION_RONFAURE;
		case ZONE_GUSGEN_MINES:
		case ZONE_KONSCHTAT_HIGHLANDS:
		case ZONE_LA_THEINE_PLATEAU:
		case ZONE_ORDELLES_CAVES:
		case ZONE_SELBINA:
		case ZONE_VALKURM_DUNES:
			return REGION_ZULKHEIM;
		case ZONE_BATALLIA_DOWNS:
		case ZONE_CARPENTERS_LANDING:
		case ZONE_DAVOI:
		case ZONE_THE_ELDIEME_NECROPOLIS:
		case ZONE_JUGNER_FOREST:
		case ZONE_MONASTIC_CAVERN:
		case ZONE_PHANAUET_CHANNEL:
			return REGION_NORVALLEN;
		case ZONE_DANGRUF_WADI:
		case ZONE_KORROLOKA_TUNNEL:
		case ZONE_NORTH_GUSTABERG:
		case ZONE_PALBOROUGH_MINES:
		case ZONE_SOUTH_GUSTABERG:
		case ZONE_WAUGHROON_SHRINE:
		case ZONE_ZERUHN_MINES:
			return REGION_GUSTABERG;
		case ZONE_BEADEAUX:
		case ZONE_CRAWLERS_NEST:
		case ZONE_PASHHOW_MARSHLANDS:
		case ZONE_QULUN_DOME:
		case ZONE_ROLANBERRY_FIELDS:
			return REGION_DERFLAND;
		case ZONE_BALGAS_DAIS:
		case ZONE_EAST_SARUTABARUTA:
		case ZONE_FULL_MOON_FOUNTAIN:
		case ZONE_GIDDEUS:
		case ZONE_INNER_HORUTOTO_RUINS:
		case ZONE_OUTER_HORUTOTO_RUINS:
		case ZONE_TORAIMARAI_CANAL:
		case ZONE_WEST_SARUTABARUTA:
			return REGION_SARUTABARUTA;
		case ZONE_BIBIKI_BAY:
		case ZONE_BUBURIMU_PENINSULA:
		case ZONE_LABYRINTH_OF_ONZOZO:
		case ZONE_MANACLIPPER:
		case ZONE_MAZE_OF_SHAKHRAMI:
		case ZONE_MHAURA:
		case ZONE_TAHRONGI_CANYON:
			return REGION_KOLSHUSHU;
		case ZONE_ALTAR_ROOM:
		case ZONE_ATTOHWA_CHASM:
		case ZONE_BONEYARD_GULLY:
		case ZONE_CASTLE_OZTROJA:
		case ZONE_GARLAIGE_CITADEL:
		case ZONE_MERIPHATAUD_MOUNTAINS:
		case ZONE_SAUROMUGUE_CHAMPAIGN:
			return REGION_ARAGONEU;
		case ZONE_BEAUCEDINE_GLACIER:
		case ZONE_CLOISTER_OF_FROST:
		case ZONE_FEIYIN:
		case ZONE_PSOXJA:
		case ZONE_QUBIA_ARENA:
		case ZONE_RANGUEMONT_PASS:
		case ZONE_THE_SHROUDED_MAW:
			return REGION_FAUREGANDI;
		case ZONE_BEARCLAW_PINNACLE:
		case ZONE_CASTLE_ZVAHL_BAILEYS:
		case ZONE_CASTLE_ZVAHL_KEEP:
		case ZONE_THRONE_ROOM:
		case ZONE_ULEGUERAND_RANGE:
		case ZONE_XARCABARD:
			return REGION_VALDEAUNIA;
		case ZONE_BEHEMOTHS_DOMINION:
		case ZONE_LOWER_DELKFUTTS_TOWER:
		case ZONE_MIDDLE_DELKFUTTS_TOWER:
		case ZONE_QUFIM_ISLAND:
		case ZONE_STELLAR_FULCRUM:
		case ZONE_UPPER_DELKFUTTS_TOWER:
			return REGION_QUFIMISLAND;
		case ZONE_THE_BOYAHDA_TREE:
		case ZONE_CLOISTER_OF_STORMS:
		case ZONE_DRAGONS_AERY:
		case ZONE_HALL_OF_THE_GODS:
		case ZONE_ROMAEVE:
		case ZONE_THE_SANCTUARY_OF_ZITAH:
			return REGION_LITELOR;
		case ZONE_CLOISTER_OF_TREMORS:
		case ZONE_EASTERN_ALTEPA_DESERT:
		case ZONE_CHAMBER_OF_ORACLES:
		case ZONE_QUICKSAND_CAVES:
		case ZONE_RABAO:
		case ZONE_WESTERN_ALTEPA_DESERT:
			return REGION_KUZOTZ;
		case ZONE_CAPE_TERIGGAN:
		case ZONE_CLOISTER_OF_GALES:
		case ZONE_GUSTAV_TUNNEL:
		case ZONE_KUFTAL_TUNNEL:
		case ZONE_VALLEY_OF_SORROWS:
			return REGION_VOLLBOW;
		case ZONE_KAZHAM:
		case ZONE_NORG:
		case ZONE_SEA_SERPENT_GROTTO:
		case ZONE_YUHTUNGA_JUNGLE:
			return REGION_ELSHIMOLOWLANDS;
		case ZONE_CLOISTER_OF_FLAMES:
		case ZONE_CLOISTER_OF_TIDES:
		case ZONE_DEN_OF_RANCOR:
		case ZONE_IFRITS_CAULDRON:
		case ZONE_SACRIFICIAL_CHAMBER:
		case ZONE_TEMPLE_OF_UGGALEPIH:
		case ZONE_YHOATOR_JUNGLE:
			return REGION_ELSHIMOUPLANDS;
		case ZONE_THE_CELESTIAL_NEXUS:
		case ZONE_LALOFF_AMPHITHEATER:
		case ZONE_RUAUN_GARDENS:
		case ZONE_THE_SHRINE_OF_RUAVITAU:
		case ZONE_VELUGANNON_PALACE:
			return REGION_TULIA;
		case ZONE_MINE_SHAFT_2716:
		case ZONE_NEWTON_MOVALPOLOS:
		case ZONE_OLDTON_MOVALPOLOS:
			return REGION_MOVALPOLOS;
		case ZONE_LUFAISE_MEADOWS:
		case ZONE_MISAREAUX_COAST:
		case ZONE_MONARCH_LINN:
		case ZONE_PHOMIUNA_AQUEDUCTS:
		case ZONE_RIVERNE_SITE_A01:
		case ZONE_RIVERNE_SITE_B01:
		case ZONE_SACRARIUM:
		case ZONE_SEALIONS_DEN:
			return REGION_TAVNAZIA;
        case ZONE_TAVNAZIAN_SAFEHOLD:
            return REGION_TAVNAZIAN_MARQ;
        case ZONE_SOUTHERN_SANDORIA:
        case ZONE_NORTHERN_SANDORIA:
        case ZONE_PORT_SANDORIA:
        case ZONE_CHATEAU_DORAGUILLE:
            return REGION_SANDORIA;
        case ZONE_BASTOK_MINES:
        case ZONE_BASTOK_MARKETS:
        case ZONE_PORT_BASTOK:
        case ZONE_METALWORKS:
            return REGION_BASTOK;
        case ZONE_WINDURST_WATERS:
        case ZONE_WINDURST_WALLS:
        case ZONE_PORT_WINDURST:
        case ZONE_WINDURST_WOODS:
        case ZONE_HEAVENS_TOWER:
            return REGION_WINDURST;
        case ZONE_RULUDE_GARDENS:
        case ZONE_UPPER_JEUNO:
        case ZONE_LOWER_JEUNO:
        case ZONE_PORT_JEUNO:
            return REGION_JEUNO;
        case ZONE_DYNAMIS_BASTOK:
        case ZONE_DYNAMIS_BEAUCEDINE:
        case ZONE_DYNAMIS_BUBURIMU:
        case ZONE_DYNAMIS_JEUNO:
        case ZONE_DYNAMIS_QUFIM:
        case ZONE_DYNAMIS_SAN_DORIA:
        case ZONE_DYNAMIS_TAVNAZIA:
        case ZONE_DYNAMIS_VALKURM:
        case ZONE_DYNAMIS_WINDURST:
        case ZONE_DYNAMIS_XARCABARD:
            return REGION_DYNAMIS;
        case ZONE_PROMYVION_DEM:
        case ZONE_PROMYVION_HOLLA:
        case ZONE_PROMYVION_MEA:
        case ZONE_PROMYVION_VAHZL:
        case ZONE_SPIRE_OF_DEM:
        case ZONE_SPIRE_OF_HOLLA:
        case ZONE_SPIRE_OF_MEA:
        case ZONE_SPIRE_OF_VAHZL:
        case ZONE_HALL_OF_TRANSFERENCE:
            return REGION_PROMYVION;
        case ZONE_ALTAIEU:
        case ZONE_EMPYREAL_PARADOX:
        case ZONE_THE_GARDEN_OF_RUHMET:
        case ZONE_GRAND_PALACE_OF_HUXZOI:
            return REGION_LUMORIA;
        case ZONE_APOLLYON:
        case ZONE_TEMENOS:
            return REGION_LIMBUS;
        case ZONE_AL_ZAHBI:
        case ZONE_AHT_URHGAN_WHITEGATE:
        case ZONE_BHAFLAU_THICKETS:
        case ZONE_THE_COLOSSEUM:
            return REGION_WEST_AHT_URHGAN;
        case ZONE_MAMOOL_JA_TRAINING_GROUNDS:
        case ZONE_MAMOOK:
        case ZONE_WAJAOM_WOODLANDS:
        case ZONE_AYDEEWA_SUBTERRANE:
        case ZONE_JADE_SEPULCHER:
            return REGION_MAMOOL_JA_SAVAGE;
        case ZONE_HALVUNG:
        case ZONE_MOUNT_ZHAYOLM:
        case ZONE_LEBROS_CAVERN:
        case ZONE_NAVUKGO_EXECUTION_CHAMBER:
            return REGION_HALVUNG;
        case ZONE_ARRAPAGO_REEF:
        case ZONE_CAEDARVA_MIRE:
        case ZONE_LEUJAOAM_SANCTUM:
        case ZONE_NASHMAU:
        case ZONE_HAZHALM_TESTING_GROUNDS:
        case ZONE_TALACCA_COVE:
        case ZONE_PERIQIA:
            return REGION_ARRAPAGO;
        case ZONE_NYZUL_ISLE:
        case ZONE_ARRAPAGO_REMNANTS:
        case ZONE_ALZADAAL_UNDERSEA_RUINS:
        case ZONE_SILVER_SEA_REMNANTS:
            return REGION_ALZADAAL;
        case ZONE_SOUTHERN_SAN_DORIA_S:
        case ZONE_EAST_RONFAURE_S:
            return REGION_RONFAURE_FRONT;
        case ZONE_BASTOK_MARKETS_S:
        case ZONE_NORTH_GUSTABERG_S:
        case ZONE_RUHOTZ_SILVERMINES:
        case ZONE_GRAUBERG_S:
            return REGION_GUSTABERG_FRONT;
        case ZONE_WINDURST_WATERS_S:
        case ZONE_WEST_SARUTABARUTA_S:
        case ZONE_GHOYUS_REVERIE:
        case ZONE_FORT_KARUGO_NARUGO_S:
            return REGION_SARUTA_FRONT;
        case ZONE_BATALLIA_DOWNS_S:
        case ZONE_JUGNER_FOREST_S:
        case ZONE_LA_VAULE_S:
        case ZONE_EVERBLOOM_HOLLOW:
        case ZONE_THE_ELDIEME_NECROPOLIS_S:
            return REGION_NORVALLEN_FRONT;
        case ZONE_ROLANBERRY_FIELDS_S:
        case ZONE_PASHHOW_MARSHLANDS_S:
        case ZONE_CRAWLERS_NEST_S:
        case ZONE_BEADEAUX_S:
        case ZONE_VUNKERL_INLET_S:
            return REGION_DERFLAND_FRONT;
        case ZONE_SAUROMUGUE_CHAMPAIGN_S:
        case ZONE_MERIPHATAUD_MOUNTAINS_S:
        case ZONE_CASTLE_OZTROJA_S:
        case ZONE_GARLAIGE_CITADEL_S:
            return REGION_ARAGONEAU_FRONT;
        case ZONE_BEAUCEDINE_GLACIER_S:
            return REGION_FAUREGANDI_FRONT;
        case ZONE_XARCABARD_S:
        case ZONE_CASTLE_ZVAHL_BAILEYS_S:
        case ZONE_CASTLE_ZVAHL_KEEP_S:
        case ZONE_THRONE_ROOM_S:
            return REGION_VALDEAUNIA_FRONT;
        case ZONE_ABYSSEA_ALTEPA:
        case ZONE_ABYSSEA_ATTOHWA:
        case ZONE_ABYSSEA_EMPYREAL_PARADOX:
        case ZONE_ABYSSEA_GRAUBERG:
        case ZONE_ABYSSEA_KONSCHTAT:
        case ZONE_ABYSSEA_LA_THEINE:
        case ZONE_ABYSSEA_MISAREAUX:
        case ZONE_ABYSSEA_TAHRONGI:
        case ZONE_ABYSSEA_ULEGUERAND:
        case ZONE_ABYSSEA_VUNKERL:
            return REGION_ABYSSEA;
        case ZONE_WALK_OF_ECHOES:
            return REGION_THE_THRESHOLD;
        case ZONE_DIORAMA_ABDHALJS_GHELSBA:
        case ZONE_ABDHALJS_ISLE_PURGONORGO:
        case ZONE_MAQUETTE_ABDHALS_LEGION:
            return REGION_ABDHALJS;
        case ZONE_WESTERN_ADOULIN:
        case ZONE_EASTERN_ADOULIN:
        case ZONE_RALA_WATERWAYS:
        case ZONE_RALA_WATERWAYS_U:
            return REGION_ADOULIN_ISLANDS;
        case ZONE_CEIZAK_BATTLEGROUNDS:
        case ZONE_FORET_DE_HENNETIEL:
        case ZONE_SIH_GATES:
        case ZONE_MOH_GATES:
        case ZONE_CIRDAS_CAVERNS:
        case ZONE_CIRDAS_CAVERNS_U:
        case ZONE_YAHSE_HUNTING_GROUNDS:
        case ZONE_MORIMAR_BASALT_FIELDS:
            return REGION_EAST_ULBUKA;
	}
	return REGION_UNKNOWN;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

CONTINENTTYPE GetCurrentContinent(uint16 ZoneID)
{
    return GetCurrentRegion(ZoneID) != REGION_UNKNOWN ? THE_MIDDLE_LANDS : OTHER_AREAS;
}

/************************************************************************
*																		*
*  Освобождаем список зон												*
*																		*
************************************************************************/

void FreeZoneList()
{
	for (uint16 ZoneID = MIN_ZONEID; ZoneID < MAX_ZONEID; ZoneID++)
	{
		delete g_PZoneList[ZoneID];
	}
	delete g_PTrigger;
}

}; // namespace zoneutils