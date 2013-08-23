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

#include "../common/blowfish.h"
#include "../common/md52.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/taskmgr.h"
#include "../common/timer.h"
#include "../common/utils.h"

#include <string.h>
#include "alliance.h"
#include "utils/blueutils.h"
#include "party.h"
#include "packet_system.h"
#include "conquest_system.h"
#include "utils/battleutils.h"
#include "utils/charutils.h"
#include "utils/petutils.h"
#include "utils/fishingutils.h"
#include "utils/itemutils.h"
#include "utils/jailutils.h"
#include "linkshell.h"
#include "map.h"
#include "entities/mobentity.h"
#include "entities/npcentity.h"
#include "spell.h"
#include "utils/synthutils.h"
#include "trade_container.h"
#include "zone.h"
#include "utils/zoneutils.h"

#include "items/item_shop.h"

#include "lua/luautils.h"

#include "packets/auction_house.h"
#include "packets/bazaar_check.h"
#include "packets/bazaar_close.h"
#include "packets/bazaar_confirmation.h"
#include "packets/bazaar_item.h"
#include "packets/bazaar_message.h"
#include "packets/bazaar_purchase.h"
#include "packets/campaing_map.h"
#include "packets/char.h"
#include "packets/char_abilities.h"
#include "packets/char_appearance.h"
#include "packets/char_check.h"
#include "packets/char_equip.h"
#include "packets/char_emotion.h"
#include "packets/char_jobs.h"
#include "packets/char_job_extra.h"
#include "packets/char_health.h"
#include "packets/char_skills.h"
#include "packets/char_spells.h"
#include "packets/char_stats.h"
#include "packets/char_sync.h"
#include "packets/char_update.h"
#include "packets/chat_message.h"
#include "packets/chocobo_digging.h"
#include "packets/chocobo_music.h"
#include "packets/conquest_map.h"
#include "packets/cs_position.h"
#include "packets/delivery_box.h"
#include "packets/downloading_data.h"
#include "packets/entity_update.h"
#include "packets/guild_menu_buy.h"
#include "packets/guild_menu_sell.h"
#include "packets/guild_menu_buy_update.h"
#include "packets/guild_menu_sell_update.h"
#include "packets/inventory_assign.h"
#include "packets/inventory_finish.h"
#include "packets/inventory_item.h"
#include "packets/inventory_modify.h"
#include "packets/inventory_size.h"
#include "packets/lock_on.h"
#include "packets/linkshell_equip.h"
#include "packets/linkshell_message.h"
#include "packets/menu_config.h"
#include "packets/menu_merit.h"
#include "packets/merit_points_categories.h"
#include "packets/message_basic.h"
#include "packets/message_debug.h"
#include "packets/message_standard.h"
#include "packets/message_system.h"
#include "packets/party_invite.h"
#include "packets/party_map.h"
#include "packets/party_search.h"
#include "packets/position.h"
#include "packets/release.h"
#include "packets/send_box.h"
#include "packets/server_ip.h"
#include "packets/server_message.h"
#include "packets/shop_appraise.h"
#include "packets/shop_buy.h"
#include "packets/stop_downloading.h"
#include "packets/trade_action.h"
#include "packets/trade_request.h"
#include "packets/trade_item.h"
#include "packets/trade_update.h"
#include "packets/wide_scan_track.h"
#include "packets/world_pass.h"
#include "packets/zone_in.h"
#include "packets/zone_visited.h"
#include "packets/menu_raisetractor.h"

uint8 PacketSize[512];
void (*PacketParser[512])(map_session_data_t*, CCharEntity*, int8*);

/************************************************************************
*																		*
*  Отображения содержимого входящего пакета в консоли					*
*																		*
************************************************************************/

void PrintPacket(int8* data)
{
	int8 message[50];
	memset(&message,0,50);

	for(int y = 0; y < data[1]*2; y++)
	{
		sprintf(message,"%s %02hx",message,(uint8)data[y]);
		if(((y+1)%16) == 0)
		{
			message[48] = '\n';
			ShowDebug(message);
			memset(&message,0,50);
		}
	}
	if (strlen(message) > 0)
	{
		message[strlen(message)] = '\n';
		ShowDebug(message);
	}
}

/************************************************************************
*                                                                       *
*  Неизвестный пакет                                                    *
*                                                                       *
************************************************************************/

void SmallPacket0x000(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    ShowWarning(CL_YELLOW"parse: Unhandled game packet %03hX from user: %s\n" CL_RESET, (RBUFW(data,0) & 0x1FF), PChar->GetName());
    return;
}

/************************************************************************
*                                                                       *
*  Нереализованный пакет                                                *
*                                                                       *
************************************************************************/

void SmallPacket0xFFF(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    ShowDebug(CL_CYAN"parse: SmallPacket is not implemented Type<%03hX>\n" CL_RESET, (RBUFW(data,0) & 0x1FF));
    return;
}

/************************************************************************
*																		*
*  Вход в зону															*
*																		*
*  Обновляем sessionkey и порт клиента при каждом переходе между зонами *
*  Мы должны оставлять правильный ключ при переходе с сервера на сервер	*
*																		*
************************************************************************/

void SmallPacket0x00A(map_session_data_t* session, CCharEntity* PChar, int8* data)
{

	

		 uint8 login_status = 0; 
				 uint8 godmode_status = 0; 
				 uint8 setexprates_status = 1; 
				 uint8 ragemode_status = 0; 
				 uint8 Online_status = 1; 
				 uint8 Shutdown_status = 1; 
				 uint8 Account_Id = 0; 
				 uint8 zone = 0; 
				 uint8 lastzone = 0; 
				 uint8 inevent=0;
				 int16 eventid=0;
				 float pos_x=0;
				 float pos_y=0;
				 float pos_z=0;
				 uint8 pos_rot=0;
				 float death_pos_x=0;
				 float death_pos_y=0;
				 float death_pos_z=0;
				 uint8 death_pos_rot=0;
				 uint8 death_pos_zone=0;
				 uint8 nation=0;
				 uint8 returning=0;
				 uint8 deathstate =0;
                 const int8* Query = "SELECT first_login,godmode,setexprates,ragemode,online,accid,pos_zone,pos_prevzone,inevent,eventid,pos_x,pos_y,pos_z,pos_rot,shutdown,nation,returning,deathstate,death_x,death_y,death_z,death_rot,death_zone FROM chars WHERE charid = '%u';";
	             int32 ret = Sql_Query(SqlHandle,Query,PChar->id);

	             if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	                {
					
		           login_status =  Sql_GetUIntData(SqlHandle,0);
				   godmode_status =  Sql_GetUIntData(SqlHandle,1);
				   setexprates_status =  Sql_GetUIntData(SqlHandle,2);
				   ragemode_status =  Sql_GetUIntData(SqlHandle,3);
				   Online_status =  Sql_GetUIntData(SqlHandle,4);
				   Account_Id =  Sql_GetUIntData(SqlHandle,5);
				   zone =  Sql_GetUIntData(SqlHandle,6);
				   lastzone =  Sql_GetUIntData(SqlHandle,7);
				   inevent =  Sql_GetUIntData(SqlHandle,8);
				   eventid =  Sql_GetUIntData(SqlHandle,9);
				   pos_x =  Sql_GetFloatData(SqlHandle,10);
				   pos_y =  Sql_GetFloatData(SqlHandle,11);
				   pos_z =  Sql_GetFloatData(SqlHandle,12);
				   pos_rot =  Sql_GetUIntData(SqlHandle,13);
					Shutdown_status =  Sql_GetUIntData(SqlHandle,14);
					nation =  Sql_GetUIntData(SqlHandle,15);
					returning =  Sql_GetUIntData(SqlHandle,16);
					deathstate =Sql_GetUIntData(SqlHandle,17);
					death_pos_x =  Sql_GetFloatData(SqlHandle,18);
				   death_pos_y =  Sql_GetFloatData(SqlHandle,19);
				   death_pos_z =  Sql_GetFloatData(SqlHandle,20);
				   death_pos_rot =  Sql_GetUIntData(SqlHandle,21);
				   death_pos_zone =  Sql_GetUIntData(SqlHandle,22);
					
				   PChar->eventid = eventid;
				   PChar->first_login = login_status;
				   PChar->is_returning = returning;
				   PChar->godmode = godmode_status;
				   PChar->SetExpRates = setexprates_status;
				   PChar->Rage_Mode = ragemode_status;
				   PChar->online_status = Online_status;
				   PChar->shutdown_status = Shutdown_status;
				   PChar->accid = Account_Id;
				  PChar->profile.nation = nation;
				  PChar->is_inevent = inevent;
				  PChar->is_dead = deathstate;
				  if(PChar->first_login == 0)
				  {
					  ShowNotice(CL_RED"MY FIRST LOGIN SYAS I AM A NEW PLAYER ON CREATION\n"CL_RESET);
				  }
				  if(PChar->first_login == 1)
				  {
					  ShowNotice(CL_RED"MY FIRST LOGIN SYAS I AM A NEW PLAYER ON CREATION\n AND I AM AT FRIST LOGIN IN A EVENT\n"CL_RESET);
				  }
				  if(PChar->first_login == 2)
				  {
					  
					  if(PChar->is_returning == 1)
					  {
                      ShowNotice(CL_RED"MY FIRST LOGIN SYAS I AM A RETURNING PLAYER\n"CL_RESET);
					  }
					  else
					  {
						  ShowNotice(CL_RED"MY FIRST ZONING SYAS I AM AT ZONELINE\n"CL_RESET);
					  }
				  }
				  if(PChar->first_login == 3)
				  {
					  ShowNotice(CL_RED"MY ZONING SYAS I AM IN EXITING MOGHOUSE\n"CL_RESET);
				  }
				 

			if(	 data != NULL)
			{
		    WBUFL(data,(0x5C)) = 0;
			}
 

 
    bool firstlogin = false; // временное решение, до появления PlayTime
 

 
	PChar->clearPacketList();
 

 
	if (PChar->status == STATUS_DISAPPEAR)
 
	{
 
        if (PChar->loc.zone != NULL)
 
        {
 ShowNotice(CL_RED"MY ZONE IS NOT NULL %u\n"CL_RESET,PChar->loc.zone);
            PacketParser[0x00D](session, PChar, NULL);
 
        }
 

 
		session->blowfish.key[4] += 2;
 
        session->blowfish.status = BLOWFISH_SENT;
 

 
		md5((uint8*)(session->blowfish.key), session->blowfish.hash, 20);
 

 
		for(uint32 i = 0; i < 16; ++i)
 
		{
 
			if(session->blowfish.hash[i] == 0)
 
			{
 
				memset(session->blowfish.hash+i, 0, 16-i);
 
				break;
 
			}
 
		}
 
		blowfish_init((int8*)session->blowfish.hash, 16, session->blowfish.P, session->blowfish.S[0]);
 

 
		int8 session_key[20*2+1];
 
		bin2hex(session_key,(uint8*)session->blowfish.key,20);
 

 
        uint16 destination = PChar->loc.destination;
 

 
        if(destination >= MAX_ZONEID){
 
            ShowWarning("packet_system::SmallPacket0x00A player tried to enter zone out of range: %d\n", destination);
 
            // holy crap out of range, lets go to the safe place
 
            PChar->loc.destination = destination = ZONE_RESIDENTIAL_AREA;
 
        }
 

 
        if (destination != ZONE_RESIDENTIAL_AREA && destination != ZONE_214)
         {
              ShowNotice(CL_RED"MY DESTINATION %u\n"CL_RESET,destination);
			  if(deathstate == 1)
			  {
				  ShowNotice(CL_RED"MY DESTINATION IS DEATH ZONE %u\n"CL_RESET,destination);
               zoneutils::GetZone(death_pos_zone)->IncreaseZoneCounter(PChar);
			  }
			  else
			  {
				   ShowNotice(CL_RED"MY DESTINATION IS NORM ZONE %u\n"CL_RESET,destination);
              zoneutils::GetZone(destination)->IncreaseZoneCounter(PChar);
			  }
 
		} else {
 
			ShowNotice(CL_RED"ELSE MY DESTINATION %u\n"CL_RESET,destination);

			 if(login_status==0)
				   {
					   //bool firstlogin = true;
					  ShowNotice(CL_RED"TRACER: WE ARE NEW CHARATER FIRST TIME LOGIN CharId %u Status=%u\n" CL_RESET ,PChar->id,login_status);
					  PChar->PMeritPoints->SaveMeritPoints(PChar->id, true);
					  ShowNotice(CL_RED"TRACER: getNation %u\n" CL_RESET,PChar->profile.nation);
					  if(PChar->profile.nation==0)
					  {
                      ShowNotice(CL_RED"TRACER: SANDY %u\n" CL_RESET,PChar->profile.nation);
					  ShowNotice(CL_RED"TRACER: SANDY GETZONE %u\n" CL_RESET,PChar->getZone());
					   
						  //SANDY
						  PChar->loc.p.x = -96;
						  PChar->loc.p.y = -1;
						  PChar->loc.p.z = -40;
						  PChar->loc.p.rotation = 224;
						  PChar->loc.destination = 230;
						  PChar->profile.home_point.p.x = -96;
                          PChar->profile.home_point.p.y = -1;
                          PChar->profile.home_point.p.z = -40;
                          PChar->profile.home_point.p.rotation = 224;
                          PChar->profile.home_point.destination = 230;  

						 Query = "UPDATE chars \
                            SET home_zone = '230', home_rot = '224', home_x = '-96', home_y = '-1', home_z = '-40'  , pos_prevzone = '230',pos_zone ='230',first_login ='2',inevent='1',eventid='503' \
							WHERE charid = %u;";
                         Sql_Query(SqlHandle, Query, PChar->id);
						
                      // return;
			           
					  }
					  if(PChar->profile.nation==1)
					  {
					  ShowNotice(CL_RED"TRACER: BASTOCK %u\n" CL_RESET,PChar->profile.nation);
					  ShowNotice(CL_RED"TRACER: BASTOCK GETZONE %u\n" CL_RESET,PChar->getZone());
					  
						  //BASTOK
						PChar->loc.p.x = -45;
						  PChar->loc.p.y = -0;
						  PChar->loc.p.z = 26;
						  PChar->loc.p.rotation = 213;
						  PChar->loc.destination = 234;
						  PChar->profile.home_point.p.x = -45;
                          PChar->profile.home_point.p.y = -0;
                          PChar->profile.home_point.p.z = 26;
                          PChar->profile.home_point.p.rotation = 213;
                          PChar->profile.home_point.destination = 234; 
						 Query = "UPDATE chars \
                            SET home_zone = '234', home_rot = '213', home_x = '-45', home_y = '-0', home_z = '26' ,pos_prevzone = '234',pos_zone = '234',first_login ='2',inevent='1',eventid='1' \
							WHERE charid = %u;";
                       Sql_Query(SqlHandle, Query,PChar->id);
					
					 // return;
					  
					  }
					  if(PChar->profile.nation==2)
					  {//2
					  ShowNotice(CL_RED"TRACER: WINDY %u\n" CL_RESET,PChar->profile.nation);
					  ShowNotice(CL_RED"TRACER: WINDY GETZONE %u\n" CL_RESET,PChar->getZone());
					 
						  //WINDY
						  PChar->loc.p.x = -120;
						  PChar->loc.p.y = -6;
						  PChar->loc.p.z = 175;
						  PChar->loc.p.rotation = 48;
						  PChar->loc.destination = 240;
						  PChar->profile.home_point.p.x = -120;
                          PChar->profile.home_point.p.y = -6;
                          PChar->profile.home_point.p.z = 175;
                          PChar->profile.home_point.p.rotation = 48;
                          PChar->profile.home_point.destination = 240; 
						Query = "UPDATE chars \
                            SET home_zone = '240', home_rot = '48', home_x = '-120', home_y = '-6', home_z = '175'  ,pos_prevzone = '240',pos_zone = '240',first_login ='2',inevent='1',eventid='305'\
							WHERE charid = %u;";
                        Sql_Query(SqlHandle, Query, PChar->id);
						
					 // return;
					  }//2
		             // return;
					   ShowNotice(CL_RED"WE ARE AT FIRST LOGIN IN EVENT %u\n"CL_RESET,inevent);
					  
				      ShowNotice(CL_RED"EVENT == %u\n"CL_RESET,inevent);
					  ShowNotice(CL_RED"EVENT NUMBER == %u\n"CL_RESET,eventid);
					  //PChar->pushPacket(new CDownloadingDataPacket());
                      //PChar->pushPacket(new CZoneInPacket(PChar,eventid)); 
					  //PChar->pushPacket(new CZoneVisitedPacket(PChar));
					  
	                }
			  firstlogin = true;
 

 
        for(uint32 i = 0; i < sizeof(PChar->m_ZonesList); ++i)
 
        {
 
           if (PChar->m_ZonesList[i] != 0) firstlogin = false;
 
        }
 
		PChar->m_ZonesList[PChar->getZone() >> 3] |= (1 << (PChar->getZone()%8));
			
            PChar->loc.zone = zoneutils::GetZoneByChar(PChar->loc.destination,PChar);
			
 
        }
 

 
       
 

        if(PChar->is_returning == 1)
		{
		/*const int8* fmtQuery = "UPDATE accounts_sessions SET session_key = x'%s', server_addr = %u, client_port = %u WHERE charid = %u";
 

 
		Sql_Query(SqlHandle,fmtQuery,
 
			session_key,
 
            PChar->loc.zone->GetIP(),
 
			session->client_port,
 
			PChar->id);*/
 

 
		const int8* deathTsQuery = "SELECT death FROM char_stats where charid = %u;";
 
		int32 ret = Sql_Query(SqlHandle,deathTsQuery, PChar->id);
 
		if (Sql_NextRow(SqlHandle) == SQL_SUCCESS) {
 
			PChar->m_DeathTimestamp = (uint32)Sql_GetUIntData(SqlHandle,0);
 
		}
		}
 

 

 
		// set new characters merits to zero on char creation
 
		if (firstlogin)
 
		{
 
			PChar->PMeritPoints->SaveMeritPoints(PChar->id, true);
 
		}
 

 
		PChar->status = STATUS_NORMAL;
 
	}
 
	else
 
	{
 
        if (PChar->loc.zone != NULL)
 
        {
			//PChar->status = STATUS_SHUTDOWN;
            // session->shuttingDown = true;
    		//CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("clean_session", gettick()+1, session, CTaskMgr::TASK_ONCE, map_clean_session));
            ShowWarning(CL_YELLOW"Client cannot receive packet or key is invalid: %s\n" CL_RESET, PChar->GetName());
 
        }
 
	}
 
    if (PChar->loc.prevzone == 0 && !firstlogin)
     {
 ShowNotice(CL_RED"ELSE MY PREVZONE %u\n"CL_RESET,PChar->loc.prevzone);
		PChar->loc.prevzone = PChar->getZone();
 
	}

	              

	              /* if(login_status==1)
				   {
					   
					  ShowNotice(CL_RED"WE ARE AT FIRST LOGIN IN EVENT %u\n"CL_RESET,inevent);
					  
				      ShowNotice(CL_RED"EVENT == %u\n"CL_RESET,inevent);
					  ShowNotice(CL_RED"EVENT NUMBER == %u\n"CL_RESET,eventid);
					  PChar->pushPacket(new CDownloadingDataPacket());
                      PChar->pushPacket(new CZoneInPacket(PChar,eventid)); 
					  PChar->pushPacket(new CZoneVisitedPacket(PChar));
			        return;
					}*/

	             if(login_status==2)
	              {
	              
				 
	              ShowNotice(CL_RED"WE ARE A RETURNING PLAYER IN STATUS %u\n"CL_RESET,login_status);
				  if(returning == 1 && deathstate == 0)
				  {
					  PChar->loc.p.x = pos_x;
					  ShowNotice(CL_RED"RETURNING TO X %.3f\n"CL_RESET,pos_x);
					  PChar->loc.p.y = pos_y;
					  ShowNotice(CL_RED"RETURNING TO Y %.3f\n"CL_RESET,pos_y);
					  PChar->loc.p.z = pos_z;
					  ShowNotice(CL_RED"RETURNING TO Z %.3f\n"CL_RESET,pos_z);
		              PChar->loc.p.rotation = pos_rot;
				      ShowNotice(CL_RED"RETURNING TO R %u\n"CL_RESET,pos_rot);
					  
				  }
				  if( deathstate == 1)
				  {
					  PChar->loc.p.x = death_pos_x;
					  ShowNotice(CL_RED"RETURNING TO HOME POINT FROM DEATH POS X %.3f\n"CL_RESET,pos_x);
					  PChar->loc.p.y = death_pos_y;
					  ShowNotice(CL_RED"RETURNING TO HOME POINT FROM DEATH POS Y %.3f\n"CL_RESET,pos_y);
					  PChar->loc.p.z = death_pos_z;
					  ShowNotice(CL_RED"RETURNING TO HOME POINT FROM DEATH POS Z %.3f\n"CL_RESET,pos_z);
		              PChar->loc.p.rotation = death_pos_rot;
				      ShowNotice(CL_RED"RETURNING TO HOME POINT FROM DEATH POS R %u\n"CL_RESET,pos_rot);
					  PChar->loc.destination =death_pos_zone;
				  }
				  
				  if(PChar->loc.destination == 0)
				  {
					  ShowNotice(CL_RED"WE ARE IN MOG HOUSE LETS SET LAST POSTION \n"CL_RESET);
					      const int8* Query = "UPDATE chars SET first_login = '3' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
				  }
				  PChar->pushPacket(new CDownloadingDataPacket());
				  if(inevent != 0)
				  {
				  if(PChar->eventid == -1)
				  {
					  int16 EventID = luautils::OnZoneIn(PChar);
					  PChar->pushPacket(new CZoneInPacket(PChar,EventID));
				  }
				  else
				  {
					  PChar->pushPacket(new CZoneInPacket(PChar,PChar->eventid));
					  PChar->eventid = -1;
				  }
				  }
				  else
				  {
					  int16 EventID = luautils::OnZoneIn(PChar);
					  PChar->pushPacket(new CZoneInPacket(PChar,EventID));
				  }
				  
	            //  charutils::SaveCharPosition(PChar);
	              charutils::SaveZonesVisited(PChar);
                  charutils::RecoverFailedSendBox(PChar);
	              
	              PChar->pushPacket(new CZoneVisitedPacket(PChar));
                  return;
	              }
				 if(login_status==3)
	              {
	             

	              ShowNotice(CL_RED"WE ARE IN MOG HOUSE PLAYER IN STATUS %u\n"CL_RESET,login_status);
				  if(PChar->loc.destination == 0)
				  {
					  ShowNotice(CL_RED"WE ARE IN MOG HOUSE LETS SET LAST POSTION \n"CL_RESET);
				  }
				  else
				  {
					   ShowNotice(CL_RED"WE ARE IN EXITING MOG HOUSE LETS GET LAST POSTION \n"CL_RESET);
					 
					      PChar->loc.p.x = pos_x;
						  PChar->loc.p.y = pos_y;
						  PChar->loc.p.z = pos_z;
						  if(zone == 48)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 48\n"CL_RESET);
		                   PChar->loc.p.rotation = 189;
		                   }
						  if(zone == 50)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 48\n"CL_RESET);
		                   PChar->loc.p.rotation = 255;
		                   }
						 if(zone == 230)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 230\n"CL_RESET);
		                   PChar->loc.p.rotation = 89;
		                   }
	                     if(zone == 231)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 231\n"CL_RESET);
		                   PChar->loc.p.rotation = 152;
		                   }
	                     if(zone == 232)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 232\n"CL_RESET);
		                   PChar->loc.p.rotation = 159;
		                   }
						  if(zone == 234)//BASTOK MINE CS 0x7534
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 234\n"CL_RESET);
		                   PChar->loc.p.rotation = 132;
		                   }
						  if(zone == 235)//BASTOK MARKETS CS 0x7534
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 235\n"CL_RESET);
		                   PChar->loc.p.rotation = 129;
		                   }
						   if(zone == 236)//BASTOK PORT CS 0x7534
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 236\n"CL_RESET);
		                   PChar->loc.p.rotation = 188;
		                   }
						   if(zone == 238)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 238\n"CL_RESET);
		                   PChar->loc.p.rotation = 192;
		                   }
						   if(zone == 239)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 239\n"CL_RESET);
		                   PChar->loc.p.rotation = 253;
		                   }
						   if(zone == 240)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 240\n"CL_RESET);
		                   PChar->loc.p.rotation = 64;
		                   }
						   if(zone == 241)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 241\n"CL_RESET);
		                   PChar->loc.p.rotation = 3;
		                   }
						   if(zone == 243)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 243\n"CL_RESET);
						   PChar->loc.p.x = 51;
						   PChar->loc.p.y = 9;
						   PChar->loc.p.z = -73;
		                   PChar->loc.p.rotation = 191;
		                   }
						   if(zone == 244)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 244\n"CL_RESET);
		                   PChar->loc.p.rotation = 174;
		                   }
						   if(zone == 245)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 245\n"CL_RESET);
		                   PChar->loc.p.rotation = 88;
		                   }
						   if(zone == 246)
	                       {
		                   ShowMessage(CL_GREEN"EXITING MOG HOUSE 246\n"CL_RESET);
		                   PChar->loc.p.rotation = 2;
		                   }
						 
				  }
				  
	              PChar->pushPacket(new CDownloadingDataPacket());
				  
				 // 0= SANDY
                 // 1 = BASTOK
				 // 2 = WINDY
					  
				  if(nation == 0 && zone == 48 || nation == 0 && zone == 50 || nation == 0 && zone == 234
					  || nation == 0 && zone == 235 || nation == 0 && zone == 236 || nation == 0 && zone == 238 || nation == 0 && zone == 239 || nation == 0 && zone == 240 
					  || nation == 0 && zone == 241 || nation == 0 && zone == 243 || nation == 0 && zone == 244 || nation == 0 && zone == 245 || nation == 0 && zone == 246)
				  {
					  ShowMessage(CL_GREEN"PROFILE NATION SANDY\n"CL_RESET);
					  //AND IF THEY CHANGE JOBS 0x7534
	              PChar->pushPacket(new CZoneInPacket(PChar,-1));
				  }
				  
				  if(nation == 1 && zone == 48 || nation == 1 && zone == 50 || nation == 1 && zone == 230 
					  || nation == 1 && zone == 231 || nation == 1 && zone == 232 || nation == 1 && zone == 238 || nation == 1 && zone == 239 || nation == 1 && zone == 240 
					  || nation == 1 && zone == 241 || nation == 1 && zone == 243 || nation == 1 && zone == 244 || nation == 1 && zone == 245 || nation == 1 && zone == 246)
				  {
					  ShowMessage(CL_GREEN"PROFILE NATION BASTOK\n"CL_RESET);
					  //AND IF THEY CHANGE JOBS 0x7534
	              PChar->pushPacket(new CZoneInPacket(PChar,-1));
				  }
				  
				  if(nation == 2 && zone == 48 || nation == 2 && zone == 50 || nation == 2 && zone == 230 
					  || nation == 2 && zone == 231 || nation == 2 && zone == 232 || nation == 2 && zone == 234 || nation == 2 && zone == 235 || nation == 2 && zone == 246 
					  || nation == 2 && zone == 243 || nation == 2 && zone == 244 || nation == 2 && zone == 245 || nation == 2 && zone == 246)
				  {
					  ShowMessage(CL_GREEN"PROFILE NATION WINDY\n"CL_RESET);
					  //AND IF THEY CHANGE JOBS ??? NEED TO DO 0x7534
	              PChar->pushPacket(new CZoneInPacket(PChar,-1));
				  }
				  else
				  {
                  PChar->pushPacket(new CZoneInPacket(PChar,-1));
				  }
	              PChar->pushPacket(new CZoneVisitedPacket(PChar));
                  return;
	              }
 
	int16 EventID = luautils::OnZoneIn(PChar);
 

 
	//charutils::SaveCharPosition(PChar);
 
	charutils::SaveZonesVisited(PChar);
 

 
	PChar->pushPacket(new CDownloadingDataPacket());
 
	PChar->pushPacket(new CZoneInPacket(PChar,EventID));
 
	PChar->pushPacket(new CZoneVisitedPacket(PChar));
 

 
    charutils::RecoverFailedSendBox(PChar);
 

 
	return; 
	}

	return;
};

/************************************************************************
*																		*
*  Пакет-запрос информации о персонаже. Приходит только во время 		*
*  перехода между зонами / входа в игру (что почти одно и то же)		*
*																		*
************************************************************************/

void SmallPacket0x00C(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->pushPacket(new CInventorySizePacket(PChar));
	PChar->pushPacket(new CMenuConfigPacket(PChar));
	PChar->pushPacket(new CCharJobsPacket(PChar));
	PChar->pushPacket(new CChocoboMusicPacket());

	if (PChar->PParty != NULL)
	{
		PChar->PParty->ReloadParty();
	}


	// TODO: в MogHouse TreasurePool сейчас не создается, по этому необходима проверка
	if (PChar->PTreasurePool != NULL)
	{
		PChar->PTreasurePool->UpdatePool(PChar);
	}
    PChar->loc.zone->SpawnTransport(PChar);



	// respawn any pets from last zone
	if (PChar->petZoningInfo.respawnPet == true)
	{
		// only repawn pet in valid zones and al zahbi (for beseiged)
		if (PChar->loc.zone->CanUseMisc(MISC_PET) == true || PChar->loc.zone->GetID() == 48)
		{
			switch (PChar->petZoningInfo.petType)
			{
				case PETTYPE_AUTOMATON:
				case PETTYPE_JUG_PET:
				case PETTYPE_WYVERN:
					petutils::SpawnPet(PChar, PChar->petZoningInfo.petID, true);
					break;

				default:
					break;
			}
			// reset the petZoning info
			PChar->resetPetZoningInfo();
		}
	}


	return;
}

/************************************************************************
*																		*
*  Персонаж покидает зону каким-либо из возможных способов. В случае 	*
*  выхода из игры, текущая зона находится в переменной zone, во всех	*
*  остальных случаех в prevzone. Делаем соответствующую проверку для	*
*  правильного удаления персонажа их списка зоны.						*
*																		*
************************************************************************/

void SmallPacket0x00D(map_session_data_t* session, CCharEntity* PChar, int8* data)
{

    session->blowfish.status = BLOWFISH_WAITING;

    PChar->TradePending.clean();
    PChar->InvitePending.clean();
	PChar->PWideScanTarget = NULL;

	if (PChar->animation == ANIMATION_ATTACK)
	{
		PChar->animation = ANIMATION_NONE;
	}

    PChar->PRecastContainer->Del(RECAST_MAGIC);

    charutils::SaveCharStats(PChar);
	//charutils::SaveCharPosition(PChar);
	charutils::SaveCharExp(PChar, PChar->GetMJob());
	charutils::SaveCharPoints(PChar);

	if (PChar->status == STATUS_SHUTDOWN)
	{

		if (PChar->PParty != NULL)
		{
			if(PChar->PParty->m_PAlliance != NULL)
			{
				if(PChar->PParty->GetLeader() == PChar)
				{
					if(PChar->PParty->members.size() == 1)
					{
						if(PChar->PParty->m_PAlliance->partyList.size() == 2)
						{
							PChar->PParty->m_PAlliance->dissolveAlliance();
						}
						else if(PChar->PParty->m_PAlliance->partyList.size() == 3)
						{
							PChar->PParty->m_PAlliance->removeParty(PChar->PParty);
						}
					}
					else
					{	//party leader logged off - will pass party lead
						PChar->PParty->RemoveMember(PChar);
					}
				}
				else
				{	//not party leader - just drop from party
					PChar->PParty->RemoveMember(PChar);
				}
			}
			else
			{
            //normal party - just drop group
			PChar->PParty->RemoveMember(PChar);
			}
		}
        if (PChar->PLinkshell != NULL)
        {
            // удаляем персонажа из linkshell
            PChar->PLinkshell->DelMember(PChar);
        }

        if (!session->shuttingDown)
        {
            const int8* Query = "UPDATE chars SET shutdown = '1' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   charutils::SaveCharPosition(PChar);
            session->shuttingDown = true;
    		CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("close_session", gettick()+10, session, CTaskMgr::TASK_ONCE, map_close_session));
        }

	}
	else  // проверка именно при покидании зоны, чтобы не делать двойную проверку при входе в игру
	{
        charutils::CheckEquipLogic(PChar, SCRIPT_CHANGEZONE, PChar->getZone());
	}
    // персонаж может отвалиться во время перехода между зонами,
    // map_cleanup вызовет этот метод и zone персонажа будет NULL
    if (PChar->loc.zone != NULL)
    {
        PChar->loc.zone->DecreaseZoneCounter(PChar);
    }

	PChar->status = STATUS_DISAPPEAR;
    PChar->PBattleAI->Reset();
	return;
}

/************************************************************************
*																		*
*  Запрос на список квестов и миссий, предметов и ключевых предметов	*
*																		*
************************************************************************/

void SmallPacket0x00F(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	charutils::SendKeyItems(PChar);
	charutils::SendQuestMissionLog(PChar);

	PChar->pushPacket(new CCharSpellsPacket(PChar));
	PChar->pushPacket(new CCharAbilitiesPacket(PChar));
	PChar->pushPacket(new CCharSyncPacket(PChar));
	PChar->pushPacket(new CBazaarMessagePacket(PChar));
    PChar->pushPacket(new CMeritPointsCategoriesPacket(PChar));

	charutils::SendInventory(PChar);
	return;
}

/************************************************************************
*																		*
*  Первый пакет после входа в игру / перехода между зонами, является	*
*  подтверждением завершения перехода персонажа.						*
*  Привязываем экипированные предметы           	 					*
*																		*
************************************************************************/

void SmallPacket0x011(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    session->blowfish.status = BLOWFISH_ACCEPTED;

	PChar->health.tp = 0;

	for(uint8 i = 0; i < 16; ++i)
	{
		if (PChar->equip[i] != 0)
		{
			PChar->pushPacket(new CEquipPacket(PChar->equip[i], i));
		}
	}
	return;
}

/************************************************************************
*																		*
*  Перемещение персонажа (обновление позиции в зоне)					*
*  Обновление цели, выбранной персонажем, для правильного отображения	*
*  поворота головы. Из-за блуждания монстров необходимо проверять их 	*
*  видимость постоянно, так же как и видимость питомцев					*
*																		*
************************************************************************/

void SmallPacket0x015(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->status != STATUS_SHUTDOWN &&
        PChar->status != STATUS_DISAPPEAR)
	{
		//bool isUpdate = ( (PChar->status == STATUS_UPDATE) ||
						//  (PChar->loc.p.x  != RBUFF(data,(0x04))) ||
						 // (PChar->loc.p.z  != RBUFF(data,(0x0C))) ||
						//  (PChar->m_TargID != RBUFW(data,(0x16))) );

		PChar->loc.p.x = RBUFF(data,(0x04));
		PChar->loc.p.y = RBUFF(data,(0x08));
		PChar->loc.p.z = RBUFF(data,(0x0C));

		PChar->loc.p.moving   = RBUFW(data,(0x12));
		PChar->loc.p.rotation = RBUFB(data,(0x14));

		PChar->m_TargID = RBUFW(data,(0x16));

		//if (isUpdate)
		//{
			PChar->status = STATUS_NORMAL;
            PChar->loc.zone->SpawnPCs(PChar);
			PChar->loc.zone->SpawnNPCs(PChar);
		//}

		PChar->loc.zone->SpawnMOBs(PChar);
		PChar->loc.zone->SpawnPETs(PChar);

		if (PChar->PWideScanTarget != NULL)
		{
			PChar->pushPacket(new CWideScanTrackPacket(PChar->PWideScanTarget));

			if (PChar->PWideScanTarget->status == STATUS_DISAPPEAR)
			{
				PChar->PWideScanTarget = NULL;
			}
		}
	}
	return;
}

/************************************************************************
*																		*
*  Клиент запрашивает информацию об NPC, для отображения их в событиях	*
*																		*
************************************************************************/

void SmallPacket0x016(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 targid = RBUFW(data,(0x04));

	if (PChar->targid == targid)
	{
		PChar->pushPacket(new CCharPacket(PChar, ENTITY_SPAWN));
        PChar->pushPacket(new CCharUpdatePacket(PChar));
	}else{
		CBaseEntity* PNpc = PChar->loc.zone->GetEntity(targid, TYPE_NPC);

		if (PNpc == NULL)
		{
			PNpc = zoneutils::GetTrigger(targid, PChar->getZone());
		}
		PChar->pushPacket(new CEntityUpdatePacket(PNpc, ENTITY_SPAWN));
	}
	return;
}

/************************************************************************
*																		*
*  Kлиент сообщает серверу об ошибочном типе npc						*
*																		*
************************************************************************/

void SmallPacket0x017(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 targid = RBUFW(data,(0x04));
	uint32 npcid  = RBUFL(data,(0x08));
	uint8  type   = RBUFB(data,(0x12));

	ShowError(CL_RED"SmallPacket0x17: Incorrect NPC(%u,%u) type(%u)\n" CL_RESET, targid, npcid, type);
	return;
}

/************************************************************************
*																		*
*  Различные действия персонажа в игре: рыбалка, команды боя, общение	*
*  с npc и подобные														*
*																		*
************************************************************************/

void SmallPacket0x01A(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 ID     = RBUFL(data,(0x04));
	uint16 TargID =	RBUFW(data,(0x08));
	uint8  action = RBUFB(data,(0x0A));


	switch (action)
	{
		case 0x00: // trigger
		{
			if (PChar->m_Costum != 0 || PChar->animation == ANIMATION_SYNTH)
            {
                PChar->pushPacket(new CReleasePacket(PChar, RELEASE_STANDARD));
                return;
            }
            CBaseEntity* PNpc = zoneutils::GetZone(PChar->getZone() != 0 ? PChar->getZone() : PChar->loc.prevzone)->GetEntity(TargID, TYPE_NPC);

			if (PNpc != NULL)
			{
				if (luautils::OnTrigger(PChar, PNpc) == -1 && PNpc->animation == ANIMATION_CLOSE_DOOR)
				{
					PNpc->animation = ANIMATION_OPEN_DOOR;
					PChar->loc.zone->PushPacket(PNpc, CHAR_INRANGE, new CEntityUpdatePacket(PNpc,ENTITY_UPDATE));
					CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("close_door", gettick()+7000, PNpc, CTaskMgr::TASK_ONCE, close_door));
				}
			}
			if(PChar->m_event.EventID == -1)
			{
				PChar->m_event.reset();
				PChar->pushPacket(new CReleasePacket(PChar, RELEASE_STANDARD));
			}
		}
		break;
		case 0x02: // attack
		{
			PChar->PBattleAI->SetCurrentAction(ACTION_ENGAGE, TargID);

            if (PChar->PBattleAI->GetCurrentAction() == ACTION_ENGAGE)
            {
                if (PChar->animation == ANIMATION_CHOCOBO)
                {
                    PChar->StatusEffectContainer->DelStatusEffectSilent(EFFECT_CHOCOBO);
                }
                PChar->PBattleAI->CheckCurrentAction(gettick());
            }
		}
		break;
		case 0x03: // spellcast
		{
			uint16 SpellID = RBUFW(data,(0x0C));

			PChar->PBattleAI->SetCurrentSpell(SpellID);
			PChar->PBattleAI->SetCurrentAction(ACTION_MAGIC_START, TargID);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x04: // disengage
		{
			PChar->PBattleAI->SetCurrentAction(ACTION_DISENGAGE);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x05: // call for help
		{
			for (SpawnIDList_t::iterator it = PChar->SpawnMOBList.begin(); it != PChar->SpawnMOBList.end(); ++it)
			{
				CMobEntity* MOB = (CMobEntity*)it->second;

				if (MOB->animation == ANIMATION_ATTACK &&
					MOB->PBattleAI->GetBattleTarget() == PChar)
				{
					MOB->m_CallForHelp = 0x20;
					PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE_SELF, new CMessageBasicPacket(PChar,PChar,0,0,19));
					break;
				}
			}
			PChar->pushPacket(new CMessageBasicPacket(PChar,PChar,0,0,22));
		}
		break;
		case 0x07: // weaponskill
		{
			uint16 WSkillID = RBUFW(data,(0x0C));
			PChar->PBattleAI->SetCurrentWeaponSkill(WSkillID);
			PChar->PBattleAI->SetCurrentAction(ACTION_WEAPONSKILL_START, TargID);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x09: // jobability
		{
			uint16 JobAbilityID = RBUFW(data,(0x0C));
			PChar->PBattleAI->SetCurrentJobAbility(JobAbilityID-16);
			PChar->PBattleAI->SetCurrentAction(ACTION_JOBABILITY_START, TargID);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x0B: // homepoint
		{
            // remove weakness on homepoint
             ShowDebug(CL_RED"PLAYER %s HOME POINT %u\n"CL_RESET,PChar->GetName(),PChar->getZone());
            // remove weakness on homepoint
            PChar->StatusEffectContainer->DelStatusEffectSilent(EFFECT_WEAKNESS);

			PChar->health.hp = PChar->GetMaxHP();
            PChar->health.mp = PChar->GetMaxMP();

			PChar->setMijinGakure(false);
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
				   ShowDebug(CL_RED"PLAYER %s HOME ZONE %u\n"CL_RESET,PChar->GetName(),zone);
				   pos_x =  Sql_GetFloatData(SqlHandle,1);
				    ShowDebug(CL_RED"PLAYER %s HOME x %.3f\n"CL_RESET,PChar->GetName(),pos_x);
				   pos_y =  Sql_GetFloatData(SqlHandle,2);
				    ShowDebug(CL_RED"PLAYER %s HOME y %.3f\n"CL_RESET,PChar->GetName(),pos_y);
				   pos_z =  Sql_GetFloatData(SqlHandle,3);
				    ShowDebug(CL_RED"PLAYER %s HOME z %.3f\n"CL_RESET,PChar->GetName(),pos_z);
				   pos_rot =  Sql_GetUIntData(SqlHandle,4);
				    ShowDebug(CL_RED"PLAYER %s HOME r %u\n"CL_RESET,PChar->GetName(),pos_rot);
				   PChar->loc.p.x = pos_x;
			       PChar->loc.p.y = pos_y;
			       PChar->loc.p.z = pos_z;
			       PChar->loc.p.rotation =pos_rot;
                   PChar->loc.destination = zone;
				 
			PChar->loc.boundary = 0;

			
		
				   const int8* Query = "UPDATE chars SET returning = '1',deathstate = '0', pos_zone='%u', pos_prevzone='%u', pos_x='%.3f', pos_y='%.3f', pos_z='%.3f', pos_rot='%u' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,zone,zone,pos_x,pos_y,pos_z,pos_rot,PChar->id);
					  
					   
             PChar->is_returning = 1;
			 PChar->is_dead = 0;
			

			PChar->status = STATUS_DISAPPEAR;
			PChar->animation = ANIMATION_NONE;

			PChar->clearPacketList();
			PChar->pushPacket(new CServerIPPacket(PChar,2));
			  }
		}
		break;
		case 0x0C: 	// assist
		{
			battleutils::assistTarget(PChar, TargID);
		}
		break;
		case 0x0D: 	// raise menu
	    {
			if(RBUFB(data,(0x0C)) == 0) //ACCEPTED RAISE
            {
				PChar->PBattleAI->SetCurrentAction(ACTION_RAISE_MENU_SELECTION);
				PChar->PBattleAI->CheckCurrentAction(gettick());
			}
            PChar->m_hasRaise = 0;
	    }
        break;
		case 0x0E: // рыбалка
		{
			fishingutils::StartFishing(PChar);
		}
		break;
		case 0x0F: // смена цели во время боя
		{
			PChar->PBattleAI->SetCurrentAction(ACTION_CHANGE_TARGET, TargID);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x10: // rangedattack
		{
			PChar->PBattleAI->SetCurrentAction(ACTION_RANGED_START, TargID);
			PChar->PBattleAI->CheckCurrentAction(gettick());
		}
		break;
		case 0x11: // chocobo digging
		{
			// bunch of gysahl greens
			uint8 slotID = PChar->getStorage(LOC_INVENTORY)->SearchItem(4545);

			if (slotID != ERROR_SLOTID)
			{
				charutils::UpdateItem(PChar, LOC_INVENTORY, slotID, -1);

				PChar->pushPacket(new CInventoryFinishPacket());
				PChar->pushPacket(new CChocoboDiggingPacket(PChar));
			}else{
				// You don't have any gysahl greens
				PChar->pushPacket(new CMessageSystemPacket(4545,0,39));
			}
		}
		break;
		case 0x12: 	// dismount
        {
            PChar->status = STATUS_UPDATE;
			PChar->animation = ANIMATION_NONE;
			PChar->StatusEffectContainer->DelStatusEffectSilent(EFFECT_CHOCOBO);
			PChar->pushPacket(new CCharUpdatePacket(PChar));
        }
        break;
		case 0x13: // tractor menu
		{
                // по любому, это работает неправильно. проблемный код в комментарии

			    //PChar->PBattleAI->SetCurrentAction(ACTION_RAISE_MENU_SELECTION);
			    //PChar->PBattleAI->CheckCurrentAction(gettick());

			/*
			if(RBUFB(data,(0x0C)) == 0)
			{
				PChar->status = STATUS_DISAPPEAR;
				PChar->loc.p  = PChar->m_StartActionPos;

				PChar->loc.boundary = 0;
				PChar->clearPacketList();
				PChar->pushPacket(new CServerIPPacket(PChar,2));
			}else{
				PChar->m_hasTractor = 0;
			}
			*/
		}
		break;
		case 0x14: // окончание обновления данных персонажа
		{
			if (PChar->loc.destination == 0)
			{
				ShowDebug(CL_RED"PLAYER %s Spawn MOG %u\n"CL_RESET,PChar->GetName(),PChar->getZone());
				zoneutils::GetZone(PChar->loc.prevzone)->SpawnMoogle(PChar);
			}else{
				PChar->loc.zone->SpawnPCs(PChar);
				PChar->loc.zone->SpawnNPCs(PChar);
				PChar->loc.zone->SpawnMOBs(PChar);
			}
			charutils::SaveCharPosition(PChar);
			if(PChar->godmode == 0)
			{
				PChar->nameflags.flags =0;
				PChar->pushPacket(new CCharUpdatePacket(PChar));
			}
			if(PChar->is_inevent != 0)
	            {
				
				const int8* Query = "UPDATE chars SET  inevent = '0' WHERE charid = %u";
                Sql_Query(SqlHandle,Query,PChar->id);
			    PChar->is_inevent = 0;
			    }
			
			if(PChar->first_login == 0 )
			{
				
				const int8* Query = "UPDATE chars SET first_login = '1' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
                 
				  PChar->first_login = 2;
				  
					   
			}
			if(PChar->first_login == 2 )
			{
				
				const int8* Query = "UPDATE chars SET first_login = '2' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   
             PChar->first_login = 2;
			 
					   
			}
			if(PChar->first_login == 3 )
			{
				
				const int8* Query = "UPDATE chars SET first_login = '2' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   
             PChar->first_login = 2;
			 
					   
			}
			if(PChar->is_returning == 1 )
			{
				
				const int8* Query = "UPDATE chars SET returning = '0' WHERE charid = %u";
                       Sql_Query(SqlHandle,Query,PChar->id);
					   
					   
             PChar->is_returning = 0;
			 

			}
			if(PChar->online_status == 0 )
			{
				
                const int8* Query = "UPDATE chars SET online = '1',shutdown ='0' WHERE charid = %u";
                Sql_Query(SqlHandle,Query,PChar->id);
                PChar->online_status =1;
				
					   
			}
			
		}
		break;
        case 0x15: break; // ballista - quarry
        case 0x16: break; // ballista - sprint
		case 0x18: // /blockaid
		{
			// Blockaid is currently inactive
			PChar->pushPacket(new CMessageSystemPacket(0,0,224));
		}
		break;
		default:
		{
			ShowWarning(CL_YELLOW"CLIENT PERFORMING UNHANDLED ACTION %02hX\n" CL_RESET, action);
			return;
		}
		break;
	}
	ShowDebug(CL_CYAN"CLIENT %s PERFORMING ACTION %02hX\n" CL_RESET, PChar->GetName(),action);
	return;
}

/************************************************************************
*                                                                       *
*  Генерация World Pass                                                 *
*                                                                       *
************************************************************************/

void SmallPacket0x01B(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    // 0 - world pass, 2 - gold world pass; +1 - purchase

    PChar->pushPacket(new CWorldPassPacket(RBUFB(data,(0x04)) & 1 ? rand() % 9999999999 : 0));
    return;
}

/************************************************************************
*																		*
*  Назначение пакета неизвестно, но начинает появляться при				*
*  использовании способностей, если у персонажа есть питомец. Возможно	*
*  клиент требует недостающие пакеты.									*
*																		*
************************************************************************/

void SmallPacket0x01C(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PrintPacket(data);
	return;
}

/************************************************************************
*																		*
*  Удаление предметов из хранилищ										*
*																		*
************************************************************************/

void SmallPacket0x028(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	int32 quantity = RBUFB(data,(0x04));
	uint8  slotID  = RBUFB(data,(0x09));

    CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);

    if (PItem != NULL && !PItem->isSubType(ITEM_LOCKED))
    {
        uint16 ItemID = PItem->getID();

	    if (charutils::UpdateItem(PChar, LOC_INVENTORY, slotID, -quantity) != 0)
	    {
            // TODO: сломать linkshell, если раковина была выброшена

		    PChar->pushPacket(new CMessageStandardPacket(NULL, ItemID, quantity, 180));
            PChar->pushPacket(new CInventoryFinishPacket());
	    }
        return;
    }
    ShowWarning(CL_YELLOW"SmallPacket0x028: Attempt of removal NULL or LOCKED item from slot %u\n" CL_RESET, slotID);
	return;
}

/************************************************************************
*																		*
*  Перемещение предметов между хранилищами								*
*																		*
************************************************************************/

void SmallPacket0x029(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 quantity       = RBUFB(data,(0x04));
	uint8  FromLocationID = RBUFB(data,(0x08));
	uint8  ToLocationID   = RBUFB(data,(0x09));
	uint8  FromSlotID	  = RBUFB(data,(0x0A));
	uint8  ToSlotID		  = RBUFB(data,(0x0B));

    if (ToLocationID   >= MAX_CONTAINER_ID ||
        FromLocationID >= MAX_CONTAINER_ID)
        return;

	CItem* PItem = PChar->getStorage(FromLocationID)->GetItem(FromSlotID);

    if(PItem == NULL || PItem->isSubType(ITEM_LOCKED))
	{
		if(PItem==NULL){
			ShowWarning(CL_YELLOW"SmallPacket0x29: Trying to move NULL item from location %u slot %u to location %u slot %u of quan %u \n" CL_RESET, FromLocationID, FromSlotID, ToLocationID, ToSlotID,quantity);
		}
		else{
			ShowWarning(CL_YELLOW"SmallPacket0x29: Trying to move LOCKED item %i from location %u slot %u to location %u slot %u of quan %u \n" CL_RESET, PItem->getID(),FromLocationID, FromSlotID, ToLocationID, ToSlotID,quantity);
		}

		uint8 size = PChar->getStorage(FromLocationID)->GetSize();
		for(uint8 slotID = 0; slotID <= size; ++slotID)
		{
			CItem* PItem = PChar->getStorage(FromLocationID)->GetItem(slotID);
			if(PItem != NULL)
			{
				PChar->pushPacket(new CInventoryItemPacket(PItem, FromLocationID, slotID));
			}
		}
		PChar->pushPacket(new CInventoryFinishPacket());

		return;
	}
	if(PItem->getQuantity() < quantity)
	{
		ShowWarning(CL_YELLOW"SmallPacket0x29: Trying to move too much quantity from location %u slot %u\n" CL_RESET, FromLocationID, FromSlotID);
		return;
	}

	uint32 NewQuantity = PItem->getQuantity() - quantity;

	if(NewQuantity != 0) // делим пачку
	{
		if (charutils::AddItem(PChar, ToLocationID, PItem->getID(), quantity) != ERROR_SLOTID)
		{
			charutils::UpdateItem(PChar, FromLocationID, FromSlotID, -(int32)quantity);
		}
	}
    else // переносим всю пачку, или пытаемся объединить одинаковые предметы
    {
		if (ToSlotID < 82) // 80 + 1
		{
			// объединение еще не реализовано
			ShowDebug("SmallPacket0x29: Trying to unite items\n", FromLocationID, FromSlotID);
			return;
		}

		uint8 NewSlotID = PChar->getStorage(ToLocationID)->InsertItem(PItem);

		if(NewSlotID != ERROR_SLOTID)
		{
			const int8* Query = "UPDATE char_inventory \
								 SET location = %u, slot = %u \
								 WHERE charid = %u AND location = %u AND slot = %u;";

			if( Sql_Query(SqlHandle, Query, ToLocationID, NewSlotID, PChar->id, FromLocationID, FromSlotID) != SQL_ERROR &&
				Sql_AffectedRows(SqlHandle) != 0)
			{
				PChar->getStorage(FromLocationID)->InsertItem(NULL, FromSlotID);

				PChar->pushPacket(new CInventoryItemPacket(NULL, FromLocationID, FromSlotID));	// убираем предмет из FormLocationID
				PChar->pushPacket(new CInventoryItemPacket(PItem, ToLocationID, NewSlotID));		// добавляем предмет в ToLocationID
			}
            else // в случае ошибки отменяем перемещение предмета
            {
                PChar->getStorage(ToLocationID)->InsertItem(NULL, NewSlotID);       // убираем предмет
                PChar->getStorage(FromLocationID)->InsertItem(PItem, FromSlotID);   // возвращаем предмет (для обновления Location и Slot предмета)
            }
		}
        else
        {
            // клиент не позволяет перемещать предмет в полный контейнер.
            // если мы видим это сообщение, значит данные клиента и сервера различаются
			// Client thinks that ToLocationID is NOT full, so lets send those packets again and tell them it is!
			uint8 size = PChar->getStorage(ToLocationID)->GetSize();
			for(uint8 slotID = 0; slotID <= size; ++slotID)
			{
				CItem* PItem = PChar->getStorage(ToLocationID)->GetItem(slotID);
				if(PItem != NULL)
				{
					PChar->pushPacket(new CInventoryItemPacket(PItem, ToLocationID, slotID));
				}
			}
			PChar->pushPacket(new CInventoryFinishPacket());

			ShowError(CL_RED"SmallPacket0x29: Location %u Slot %u is full\n" CL_RESET, ToLocationID,ToSlotID);
			return;
		}
	}
	PChar->pushPacket(new CInventoryFinishPacket());
	return;
}

/************************************************************************
*                                                                       *
*  Запрос начала обмена между персонажами (trade)                       *
*                                                                       *
************************************************************************/

void SmallPacket0x032(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    uint32 charid = RBUFL(data,(0x04));
    uint16 targid = RBUFW(data,(0x08));

    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(targid, TYPE_PC);

    if ((PTarget != NULL) && (PTarget->id == charid))
    {
        if(jailutils::InPrison(PChar) || jailutils::InPrison(PTarget))
        {
            // If either player is in prison don't allow the trade.
            PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
            return;
        }

        if (PTarget->TradePending.id == PChar->id)
        {
            ShowDebug(CL_CYAN"You have already sent a trade request to %s\n" CL_RESET, PTarget->GetName());
            return;
        }
        if (!PTarget->UContainer->IsContainerEmpty())
        {
            ShowDebug(CL_CYAN"You cannot trade with %s at this time\n" CL_RESET, PTarget->GetName());
            return;
        }
        PChar->TradePending.id     = charid;
        PChar->TradePending.targid = targid;

        PTarget->TradePending.id     = PChar->id;
        PTarget->TradePending.targid = PChar->targid;
        PTarget->pushPacket(new CTradeRequestPacket(PChar));
    }
    return;
}

/************************************************************************
*                                                                       *
*  Запрос начала обмена между персонажами (trade)                       *
*                                                                       *
************************************************************************/

void SmallPacket0x033(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->TradePending.targid, TYPE_PC);

    if (PTarget != NULL && PChar->TradePending.id == PTarget->id)
    {
        uint16 action = RBUFB(data,(0x04));

        switch (action)
        {
            case 0x00: // request accepted
            {
                // цели обмена у персонажей соответствующие
                if (PChar->TradePending.id == PTarget->id && PTarget->TradePending.id == PChar->id)
                {
                    // контейнеры у персонажей свободны
                    if (PChar->UContainer->IsContainerEmpty() && PTarget->UContainer->IsContainerEmpty())
                    {
                        // между персонажами соответствующая дистанция
                        if (distance(PChar->loc.p, PTarget->loc.p) < 6)
                        {
                            PChar->UContainer->SetType(UCONTAINER_TRADE);
                            PChar->pushPacket(new CTradeActionPacket(PTarget, action));

                            PTarget->UContainer->SetType(UCONTAINER_TRADE);
                            PTarget->pushPacket(new CTradeActionPacket(PChar, action));
                            return;
                        }
                    }
                    PChar->TradePending.clean();
                    PTarget->TradePending.clean();

                    ShowDebug(CL_CYAN"Trade: UContainer is not empty\n" CL_RESET);
                }
            }
            break;
            case 0x01: // trade cancelled
            {
                // цели обмена у персонажей соответствующие
                if (PChar->TradePending.id == PTarget->id && PTarget->TradePending.id == PChar->id)
                {
                    // контейнер у цели зарезервирован для обмена
                    if (PTarget->UContainer->GetType() == UCONTAINER_TRADE)
                    {
                        PTarget->TradePending.clean();
                        PTarget->UContainer->Clean();

                        PTarget->pushPacket(new CTradeActionPacket(PChar, action));
                    }
                }
                if (PChar->UContainer->GetType() == UCONTAINER_TRADE)
                {
                    PChar->UContainer->Clean();
                }
                PChar->TradePending.clean();
            }
            break;
            case 0x02: // trade accepted
            {
                // цели обмена у персонажей соответствующие
                if (PChar->TradePending.id == PTarget->id && PTarget->TradePending.id == PChar->id)
                {
                    PChar->UContainer->SetLock();
                    PTarget->pushPacket(new CTradeActionPacket(PChar, action));

                    // совершаем обмен предметами в контейнерах персонажей
                    if (PTarget->UContainer->IsLocked())
                    {
                        if (charutils::CanTrade(PChar, PTarget) && charutils::CanTrade(PTarget, PChar))
                        {
                            charutils::DoTrade(PChar, PTarget);
                            PTarget->pushPacket(new CTradeActionPacket(PTarget, 9));

                            charutils::DoTrade(PTarget, PChar);
                            PChar->pushPacket(new CTradeActionPacket(PChar, 9));
                        }
                        else
                        {
							// обмен не состоялся:
                            // недостаточно места в контейнере одного или обоих персонажей
                            // or
                            // rare item in the exchange is owned by the recipient

                            PChar->pushPacket(new CTradeActionPacket(PTarget, 1));
                            PTarget->pushPacket(new CTradeActionPacket(PChar, 1));
                        }
                        PChar->TradePending.clean();
                        PChar->UContainer->Clean();

                        PTarget->TradePending.clean();
                        PTarget->UContainer->Clean();
                    }
                }
            }
            break;
        }
    }
    return;
}

/************************************************************************
*                                                                       *
*  Заполняем ячейки окошка trade (обмен между персонажами)              *
*                                                                       *
************************************************************************/

void SmallPacket0x034(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    uint32 quantity    = RBUFL(data,(0x04));
    uint16 itemID      = RBUFW(data,(0x08));
    uint8  invSlotID   = RBUFB(data,(0x0A));
    uint8  tradeSlotID = RBUFB(data,(0x0B));

    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->TradePending.targid, TYPE_PC);

    if (PTarget != NULL && PTarget->id == PChar->TradePending.id)
    {
        CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(invSlotID);

        // We used to disable Rare/Ex items being added to the container, but that is handled properly else where now
        if (PItem != NULL && PItem->getID() == itemID)
        {
            // если количество предметов равно нулю, то удаляем предмет из контейнера
            PItem->setReserve(quantity);
            PChar->UContainer->SetItem(tradeSlotID, quantity > 0 ? PItem : NULL);

            PChar->pushPacket(new CTradeItemPacket(PItem, tradeSlotID));
            PTarget->pushPacket(new CTradeUpdatePacket(PItem, tradeSlotID));
        }
    }
    return;
}

/************************************************************************
*																		*
*  Передача предметов NPC (команда обмен - "trade")						*
*																		*
************************************************************************/

void SmallPacket0x036(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 npcid  = RBUFL(data,(0x04));
	uint16 targid = RBUFW(data,(0x3A));

	CBaseEntity* PNpc = PChar->loc.zone->GetEntity(targid, TYPE_NPC);

	// Moogles are zone dependent, and zoneid = 0 is for all residential areas, so if a char trades to a moogle
	// then you won't find the right NPC if you used zoneid=0. Thankfully, the prevzone is the real zone we want
	// so this should return an NPC.
	if (PNpc == NULL && PChar->loc.prevzone != 0) {
		PNpc = zoneutils::GetZone(PChar->loc.prevzone)->GetEntity(targid, TYPE_NPC);
		if (strcmp(PNpc->GetName(),"Moogle") != 0) {
			// we must restrict this check only for Moogles else other NPCs could be interpreted
			// incorrectly as targetted.
			return;
		}
	}

	if ((PNpc != NULL) && (PNpc->id == npcid))
	{
        uint8 numItems = RBUFB(data,(0x3C));

	    PChar->Container->Clean();

	    for(int32 slotID = 0; slotID < numItems; ++slotID)
	    {
		    uint8  invSlotID = RBUFB(data,(0x30+slotID));
		    uint32 Quantity  = RBUFL(data,(0x08+slotID*4));

		    CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(invSlotID);

            if (PItem != NULL && PItem->getQuantity() >= Quantity)
		    {
			    PChar->Container->setItem(slotID, PItem->getID(), invSlotID, Quantity, PItem);
		    }
	    }

        PChar->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DETECTABLE);
		luautils::OnTrade(PChar, PNpc);
	}
	return;
}

/************************************************************************
*																		*
*  Использование предметов												*
*																		*
************************************************************************/

void SmallPacket0x037(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 EntityID = RBUFL(data,(0x04));
	uint16 TargetID = RBUFW(data,(0x0C));
	uint8  SlotID   = RBUFB(data,(0x0E));

	CItemUsable* PItem = (CItemUsable*)PChar->getStorage(LOC_INVENTORY)->GetItem(SlotID);

	if ((PItem != NULL) && PItem->isType(ITEM_USABLE))
	{
		if (PItem->isType(ITEM_ARMOR))
		{
			//TODO: если ITEM_LOCKED, то должна быть проверка на то, что предмет экипирован
		}
		else if (PItem->isSubType(ITEM_LOCKED))
		{
			return;
		}
		if (PChar->UContainer->GetType() == UCONTAINER_EMPTY)
		{
			PChar->UContainer->SetType(UCONTAINER_USEITEM);
			PChar->UContainer->SetItem(0, PItem);

			PChar->PBattleAI->SetCurrentAction(ACTION_ITEM_START, TargetID);

			if (PChar->PBattleAI->GetCurrentAction() != ACTION_ITEM_START)
			{
				PChar->UContainer->Clean();
                return;
			}
            PChar->PBattleAI->CheckCurrentAction(gettick());
		}
	}
	return;
}

/************************************************************************
*																		*
*  Сортировка инвентаря													*
*  Жестокий алгоритм, но лучшего для неупорядоченного массива не найдем	*
*  80 - ячеек. Минимальное количество проверок - 80, максимальное 3240	*
*																		*
************************************************************************/

void SmallPacket0x03A(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	CItemContainer* PItemContainer = PChar->getStorage(RBUFB(data,(0x04)));

	uint8 size = PItemContainer->GetSize();

    if (gettick() - PItemContainer->LastSortingTime < 1000)
    {
        if (map_config.lightluggage_block == ++PItemContainer->SortingPacket)
        {
            ShowWarning(CL_YELLOW"lightluggage detected: <%s> will be removed from server\n" CL_RESET, PChar->GetName());

            PChar->status = STATUS_SHUTDOWN;
            PChar->pushPacket(new CServerIPPacket(PChar,1));
        }
        return;
    }
    else
    {
        PItemContainer->SortingPacket = 0;
        PItemContainer->LastSortingTime = gettick();
    }
    for (uint8 slotID = 1; slotID <= size; ++slotID)
    {
        CItem* PItem = PItemContainer->GetItem(slotID);

        if ((PItem != NULL) &&
            (PItem->getQuantity() < PItem->getStackSize()) &&
            !PItem->isSubType(ITEM_LOCKED))
        {
            for (uint8 slotID2 = slotID+1; slotID2 <= size; ++slotID2)
            {
                CItem* PItem2 = PItemContainer->GetItem(slotID2);

                if ((PItem2 != NULL) &&
                    (PItem2->getID() == PItem->getID()) &&
                    (PItem2->getQuantity() < PItem2->getStackSize()) &&
                    !PItem2->isSubType(ITEM_LOCKED))
                {
                    uint32 totalQty = PItem->getQuantity() + PItem2->getQuantity();
                    uint32 moveQty  = 0;

                    if (totalQty >= PItem->getStackSize()) {
                        moveQty = PItem->getStackSize() - PItem->getQuantity();
                    } else {
                        moveQty = PItem2->getQuantity();
                    }
                    if(moveQty > 0)
                    {
						charutils::UpdateItem(PChar, PItemContainer->GetID(), slotID, moveQty);
                        charutils::UpdateItem(PChar, PItemContainer->GetID(), slotID2, -(int32)moveQty);
                    }
                }
            }
        }
    }
    PChar->pushPacket(new CInventoryFinishPacket());
	return;
}

/************************************************************************
*																		*
*  Вероятно сообщение о недополучении каких-либо инициализирующих		*
*  пакетов для монстров, npc или других игроков. всегда пустое.			*
*																		*
************************************************************************/

void SmallPacket0x03C(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	ShowWarning(CL_YELLOW"SmallPacket0x03C\n" CL_RESET);
	return;
}

/************************************************************************
*																		*
*  Персонаж голосует за предмет в TreasurePool							*
*																		*
************************************************************************/

void SmallPacket0x041(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PrintPacket(data);

    uint8 SlotID  = RBUFB(data,(0x04));

	PChar->PTreasurePool->LotItem(PChar, SlotID, 1+(rand()%999)); //1 ~ 998+1
	return;
}

/************************************************************************
*																		*
*  Персонаж отказывается от предмета в TreasurePool						*
*																		*
************************************************************************/

void SmallPacket0x042(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PrintPacket(data);

	uint8 SlotID  = RBUFB(data,(0x04));

	PChar->PTreasurePool->LotItem(PChar, SlotID, 0);
	return;
}

/************************************************************************
*																		*
*  Отправляем приветственное сообщение сервера.							*
*  По умолчанию появится сообщение <<< Welcome to "server name" >>> +	*
*  можно отправить дополнительные строки информации в размере 230 байт	*
*																		*
************************************************************************/

void SmallPacket0x04B(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    if(PChar->search.language == 205) // French
		PChar->pushPacket(new CServerMessagePacket(map_config.fr_server_message,PChar->search.language));
	//TODO: add another language
	else
		PChar->pushPacket(new CServerMessagePacket(map_config.server_message,PChar->search.language));

	return;
}

/************************************************************************
*																		*
*  Все действия с Delivery Box											*
*																		*
************************************************************************/

void SmallPacket0x04D(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8 action  = RBUFB(data,(0x04));
	uint8 boxtype = RBUFB(data,(0x05));
	uint8 slotID  = RBUFB(data,(0x06));

    ShowDebug(CL_CYAN"DeliveryBox Action (%02hx)\n" CL_RESET, RBUFB(data,(0x04)));
    PrintPacket(data);

	// 0x01 - отправка клиенту старых предметов
	// 0x02 - добавление предметов в список отправляемых (подготовка к отправке)
	// 0x03 - подтверждение отправки (отправляем предметы)
	// 0x04 - возврат неправильно отправленного предмета (отмена выполненной отправки)
	// 0x05 - отправка клиенту количества новых предметов
	// 0x06 - отправка клиенту новых предметов
	// 0x07 - removes a delivered item from sending box
	// 0x08 - обновление предмета в ячейке перед удалением
	// 0x09 - вернуть предмет отправителю
    // 0x0a - взять предмет из ячейки
	// 0x0b - удаление предмета из ячейки
	// 0x0c - подтверждение введенного имени в окне отправки
	// 0x0d - открытие окна отправки почты
	// 0x0e - открытие окна приема почты
	// 0x0f - закрытие окна почты

	switch (action)
	{
		case 0x01:
		{
			// отправляем персонажу старые предметы (предметы, которые персонаж уже видел в delivery box)
			// все старые предметы расположены в ячейках 0-7


            if (boxtype == 0x01)
            {
			    const int8* fmtQuery = "SELECT itemid, itemsubid, slot, quantity, sender \
                                        FROM delivery_box \
							            WHERE charid = %u \
                                        AND box = %u \
							            ORDER BY slot \
							            LIMIT 8";

	            int32 ret = Sql_Query(SqlHandle, fmtQuery, PChar->id, boxtype);

	            if (ret != SQL_ERROR)
	            {
                    uint8 slotid = 8;
                    uint8 old_items = 0;

                    if (Sql_NumRows(SqlHandle) != 0)
                    {
		                while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		                {
                            CItem* PItem = itemutils::GetItem(Sql_GetIntData(SqlHandle,0));

                            if(PItem != NULL) // Prevent an access violation in the event that an item doesn't exist for an ID
                            {
                                PItem->setSubID(Sql_GetIntData(SqlHandle,1));
                                PItem->setSlotID(Sql_GetIntData(SqlHandle,2));
                                PItem->setQuantity(Sql_GetUIntData(SqlHandle,3));
                                PItem->setSender(Sql_GetData(SqlHandle,4));

                                if (PItem->getSlotID() < 8)
                                {
                                    old_items++;
                                    PChar->UContainer->SetItem(PItem->getSlotID(), PItem);
                                    continue;
                                }
                                PChar->UContainer->SetItem(slotid++, PItem);
                            }
                        }
		            }
                    for (uint8 i = 0; i < 8; ++i)
                    {
                        if (!PChar->UContainer->IsSlotEmpty(i))
                        {
                            PChar->pushPacket(new CDeliveryBoxPacket(action, PChar->UContainer->GetItem(i), old_items));
                        }
                    }
                }
            }
            else if (boxtype == 0x02)
            {
                for (uint8 i = 0; i < 8; ++i)
                {
                    PChar->pushPacket(new CSendBoxPacket(action, PChar->UContainer->GetItem(i), i, PChar->UContainer->GetItemsCount()));
                }
            }
            return;
		}
        case 0x02: //add items to send box
        {
            uint8 invslot = RBUFB(data, (0x07));
            uint32 quantity = RBUFL(data, (0x08));
            CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(invslot);

            if (PItem && PItem->getQuantity() >= quantity && PChar->UContainer->IsSlotEmpty(slotID) && !(PItem->getFlag() & ITEM_FLAG_EX))
            {
                int32 ret = Sql_Query(SqlHandle, "SELECT charid FROM chars WHERE charname = '%s' LIMIT 1", data+0x10);
                if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) > 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
                {
                    uint32 charid = Sql_GetUIntData(SqlHandle, 0);

                    ret = Sql_Query(SqlHandle,
                        "INSERT INTO delivery_box(charid, charname, box, slot, itemid, itemsubid, quantity, senderid, sender) \
                        VALUES(%u, '%s', 2, %u, %u, %u, %u, %u, '%s'); ",
                        charid,
                        data+0x10,
                        slotID,
                        PItem->getID(),
                        PItem->getSubID(),
                        quantity,
                        PChar->id,
                        PChar->GetName());

                    if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                    {
                        CItem* PUBoxItem = itemutils::GetItem(PItem->getID());
                        PUBoxItem->setReceiver(data+0x10);
                        PUBoxItem->setSender((int8*)PChar->GetName());
                        PUBoxItem->setQuantity(quantity);
                        PUBoxItem->setSlotID(PItem->getSlotID());
                        PChar->UContainer->SetItem(slotID, PUBoxItem);
                        PChar->pushPacket(new CSendBoxPacket(action, PUBoxItem, slotID, PChar->UContainer->GetItemsCount()));
                        charutils::UpdateItem(PChar, LOC_INVENTORY, invslot, -(int32)quantity);
                        PChar->pushPacket(new CInventoryFinishPacket());
                    }
                }
            }
            return;
        }
        case 0x03: //send items
        {
            uint8 send_items = 0;
            for (int i = 0; i < 8; i++)
            {
                if (!PChar->UContainer->IsSlotEmpty(i) &&
                    !PChar->UContainer->GetItem(i)->isSent())
                {
                    send_items++;
                }
            }
            if (!PChar->UContainer->IsSlotEmpty(slotID))
            {
                CItem* PItem = PChar->UContainer->GetItem(slotID);

                if (PItem && !PItem->isSent())
                {
                    bool isAutoCommitOn = Sql_GetAutoCommit(SqlHandle);
                    bool commit = false;

                    if(Sql_SetAutoCommit(SqlHandle, false) && Sql_TransactionStart(SqlHandle))
                    {
                        int32 ret = Sql_Query(SqlHandle, "SELECT charid FROM chars WHERE charname = '%s' LIMIT 1", PItem->getReceiver());

                        if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) > 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
                        {
                            uint32 charid = Sql_GetUIntData(SqlHandle, 0);

                            ret = Sql_Query(SqlHandle, "UPDATE delivery_box SET sent = 1 WHERE charid = %u AND senderid = %u AND slot = %u AND box = 2;", charid, PChar->id, slotID);

                            if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                            {
                                ret = Sql_Query(SqlHandle,
                                    "INSERT INTO delivery_box(charid, charname, box, itemid, itemsubid, quantity, senderid, sender) \
                                    VALUES(%u, '%s', 1, %u, %u, %u, %u, '%s'); ",
                                    charid,
                                    PItem->getReceiver(),
                                    PItem->getID(),
                                    PItem->getSubID(),
                                    PItem->getQuantity(),
                                    PChar->id,
                                    PChar->GetName());
                                if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                                {
                                    PChar->pushPacket(new CSendBoxPacket(action, PItem, slotID, send_items, 0x02));
                                    PChar->pushPacket(new CSendBoxPacket(action, PItem, slotID, send_items, 0x01));
                                    commit = true;
                                }
                            }
                        }
                        if(!commit || !Sql_TransactionCommit(SqlHandle))
                        {
                            Sql_TransactionRollback(SqlHandle);
                            ShowError("Could not finalize send transaction. PlayerID: %d Target: %s slotID: %d", PChar->id, PItem->getReceiver(), slotID);
                        }

                        Sql_SetAutoCommit(SqlHandle, isAutoCommitOn);
                    }
                }
            }
            return;
        }
        case 0x04: //cancel send
        {
            if (PChar->UContainer->GetType() != UCONTAINER_DELIVERYBOX) return;

            if (!PChar->UContainer->IsSlotEmpty(slotID) )
            {
                bool isAutoCommitOn = Sql_GetAutoCommit(SqlHandle);
                bool commit = false;

                if(Sql_SetAutoCommit(SqlHandle, false) && Sql_TransactionStart(SqlHandle))
                {
                    int32 ret = Sql_Query(SqlHandle, "SELECT charid FROM chars WHERE charname = '%s' LIMIT 1", PChar->UContainer->GetItem(slotID)->getReceiver());

                    if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) > 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
                    {
                        uint32 charid = Sql_GetUIntData(SqlHandle, 0);
                        ret = Sql_Query(SqlHandle, "UPDATE delivery_box SET sent = 0 WHERE senderid = %u AND box = 2 AND slot = %u LIMIT 1;", PChar->id, slotID);

                        if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                        {
                            CItem* PItem = PChar->UContainer->GetItem(slotID);
                            int32 ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE senderid = %u AND box = 1 AND charid = %u AND itemid = %u AND quantity = %u LIMIT 1;",
                                PChar->id, charid, PItem->getID(), PItem->getQuantity());

                            if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                            {
                                PChar->UContainer->GetItem(slotID)->setSent(false);
                                commit = true;
                                PChar->pushPacket(new CSendBoxPacket(action, PChar->UContainer->GetItem(slotID), slotID, PChar->UContainer->GetItemsCount(), 0x02));
                                PChar->pushPacket(new CSendBoxPacket(action, PChar->UContainer->GetItem(slotID), slotID, PChar->UContainer->GetItemsCount(), 0x01));
                            }
                        }
                    }
                    if(!commit || !Sql_TransactionCommit(SqlHandle))
                    {
                        Sql_TransactionRollback(SqlHandle);
                        ShowError("Could not finalize cancel send transaction. PlayerID: %d slotID: %d", PChar->id, slotID);
                    }

                    Sql_SetAutoCommit(SqlHandle, isAutoCommitOn);
                }
            }
        }
		case 0x05:
		{
			// отправляем персонажу количество новых предметов (предметы, которые персонаж еще не видел в delivery box)
			// все новые предметы помещаются в контейнет начиная со значения 8
			// перемещаем новые предметы в свободные ячейки delivery box

            if (PChar->UContainer->GetType() != UCONTAINER_DELIVERYBOX) return;

            if (boxtype == 0x01)
            {
                uint8 new_items = 0;

                for (uint8 slotid = 8; slotid < 16; ++slotid)
		        {
                    if (PChar->UContainer->IsSlotEmpty(slotid)) break;

                    CItem* PItem = PChar->UContainer->GetItem(slotid);

                    for (uint8 i = 0; i < 8 ; ++i)
                    {
                        if (PChar->UContainer->IsSlotEmpty(i))
                        {
                            int32 ret = Sql_Query(SqlHandle, "UPDATE delivery_box SET slot = %u WHERE charid = %u AND slot = %u AND box = 1", i, PChar->id, PItem->getSlotID());

                            if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) != 0)
                            {
                                ret = Sql_Query(SqlHandle, "UPDATE delivery_box SET received = 1 WHERE charid = %u AND sender = '%s' AND box = 2 AND received = 0 \
                                                           AND quantity = %u LIMIT 1", PChar->id, PItem->getSender(), PItem->getQuantity());

                                if (ret != SQL_ERROR)
                                {
                                    PItem->setSlotID(i);
                                    PChar->UContainer->SetItem(i, PItem);
                                    PChar->UContainer->SetItem(slotid, NULL);
                                    new_items++;
                                }
                            }
                            break;
                        }
                    }
                }
                PChar->pushPacket(new CDeliveryBoxPacket(action, new_items));
			}
            else if (boxtype == 0x02)
            {
                uint8 received_items = 0;

                int32 ret = Sql_Query(SqlHandle, "SELECT charid FROM delivery_box WHERE senderid = %u AND received = 1 AND box = 2", PChar->id);

                if (ret != SQL_ERROR )
                {
                    received_items = Sql_NumRows(SqlHandle);
                }
                PChar->pushPacket(new CSendBoxPacket(action, 0xFF, 0x02));
                PChar->pushPacket(new CSendBoxPacket(action, received_items, 0x01));
            }
			return;
		}
		case 0x06:
		case 0x08:
		{
			// 0x08 и 0x06 идентичны
			//
			// 0х06 - добавляем предмет в определенную ячейку
			// 0х08 - обновляем предмет в определенной ячейке

			// отправляем персонажу все новые предметы (предметы, которые персонаж еще не видел в delivery box)
			// клиент отправляет запрос серверу для каждого нового предмета, при этом указывая, какая ячейка его интересует
			//
			// сервер должен отправлять два пакета с действием 0х06
			// у первого пакета data[0x0c] равняется 0x02, у второго 0x01, в остальном заголовок идентичен
			// возможно это связано с тем, что я сам отправлял себе предметы
			//
			// 0x4b 0x2c 0x00 0x00 0x06 0x01 0x01 0x01 0xff 0xff 0xff 0xff 0x02 0x01 0xff 0xff
			// 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
			// 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
			// 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
			// 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
			//
			// зачем нужен этот пустой пакет я не знаю, но и без него все отлично работает
			// предположительно он отчищает целевую ячейку от предметов, на всякий случай

            if (PChar->UContainer->GetType() == UCONTAINER_DELIVERYBOX &&
               !PChar->UContainer->IsSlotEmpty(slotID))
            {
                PChar->pushPacket(new CDeliveryBoxPacket(action, PChar->UContainer->GetItem(slotID), 1));
            }
			return;
		}
        case 0x07:
        {
            uint8 received_items = 0;
            uint8 first_received = 0xFF;
            for (int i = 0; i < 8; ++i)
            {
                if (!PChar->UContainer->IsSlotEmpty(i))
                {
                    CItem* PItem = PChar->UContainer->GetItem(i);
                    if (PItem->isSent())
                    {
                        if (first_received == 0xFF)
                        {
                            first_received = i;
                        }
                        received_items++;
                    }
                }
            }
            CItem* PItem = PChar->UContainer->GetItem(first_received);

            int32 ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE senderid = %u AND box = 2 AND slot = %u LIMIT 1;", PChar->id, first_received);

            if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
            {
                PChar->pushPacket(new CSendBoxPacket(action, 0, 0x02));
                PChar->pushPacket(new CSendBoxPacket(action, PItem, first_received, received_items, 0x01));
                PChar->UContainer->SetItem(first_received, NULL);
                delete PItem;
            }
        }
        case 0x09: // Option: Return
        {
            if (PChar->UContainer->GetType() == UCONTAINER_DELIVERYBOX &&
               !PChar->UContainer->IsSlotEmpty(slotID))
            {
                bool isAutoCommitOn = Sql_GetAutoCommit(SqlHandle);
                bool commit = false; // When in doubt back it out.

                CItem* PItem = PChar->UContainer->GetItem(slotID);
                uint32 senderID = 0;
                string_t senderName;

                if(Sql_SetAutoCommit(SqlHandle, false) && Sql_TransactionStart(SqlHandle))
                {
                    // Get sender of delivery record
                    int32 ret = Sql_Query(SqlHandle, "SELECT senderid, sender FROM delivery_box WHERE charid = %u AND slot = %u AND box = 1 LIMIT 1;", PChar->id, slotID);

                    if(ret != SQL_ERROR && Sql_NumRows(SqlHandle) > 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
                    {
                        senderID = Sql_GetUIntData(SqlHandle, 0);
                        senderName.insert(0,Sql_GetData(SqlHandle, 1));

                        if(senderID != 0)
                        {
                            // Insert a return record into delivery_box
                            ret = Sql_Query(SqlHandle,
                                "INSERT INTO delivery_box(charid, charname, box, itemid, itemsubid, quantity, senderid, sender) \
                                VALUES(%u, '%s', 1, %u, %u, %u, %u, '%s'); ",
                                senderID,
                                senderName.c_str(),
                                PItem->getID(),
                                PItem->getSubID(),
                                PItem->getQuantity(),
                                PChar->id,
                                PChar->GetName());

                            if(ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) > 0)
                            {
                                // Remove original delivery record
                                ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE charid = %u AND slot = %u AND box = 1 LIMIT 1;", PChar->id, slotID);

                                if(ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) > 0)
                                {
                                    ret = Sql_Query(SqlHandle, "UPDATE delivery_box SET received = 1 WHERE charid = %u AND senderid = %u AND box = 2 LIMIT 1;", PChar->id, senderID);

                                    if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) == 1)
                                    {
                                        PChar->UContainer->SetItem(slotID, NULL);
                                        PChar->pushPacket(new CDeliveryBoxPacket(action, PItem, PChar->UContainer->GetItemsCount()));
                                        delete PItem;
                                        commit = true;
                                    }
                                }
                            }
                        }
                    }

                    if(!commit || !Sql_TransactionCommit(SqlHandle))
                    {
                        Sql_TransactionRollback(SqlHandle);
                        ShowError("Could not finalize delivery return transaction. PlayerID: %d SenderID :%d ItemID: %d Quantity: %d", PChar->id, senderID, PItem->getID(), PItem->getQuantity());
                        PChar->pushPacket(new CDeliveryBoxPacket(action, PItem, PChar->UContainer->GetItemsCount(), 0xEB));
                    }

                    Sql_SetAutoCommit(SqlHandle, isAutoCommitOn);
                }
            }
			return;
        }
        case 0x0A: // Option: Take
        {
            if (PChar->UContainer->GetType() == UCONTAINER_DELIVERYBOX &&
               !PChar->UContainer->IsSlotEmpty(slotID))
            {
                bool isAutoCommitOn = Sql_GetAutoCommit(SqlHandle);
                bool commit = false;

                CItem* PItem = PChar->UContainer->GetItem(slotID);

                ShowMessage("FreeSlots %u\n", PChar->getStorage(LOC_INVENTORY)->GetFreeSlotsCount());
                ShowMessage("ItemId %u", PItem->getID());

                if (boxtype == 0x01 && !PItem->isType(ITEM_CURRENCY)  &&
                    PChar->getStorage(LOC_INVENTORY)->GetFreeSlotsCount() == 0)
                {
                    PChar->pushPacket(new CDeliveryBoxPacket(action, PItem, PChar->UContainer->GetItemsCount(), 0xB9));
                    return;
                }

                if(Sql_SetAutoCommit(SqlHandle, false) && Sql_TransactionStart(SqlHandle))
                {
                    if (boxtype == 0x01)
                    {
                        int32 ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE charid = %u AND slot = %u AND box = %u LIMIT 1", PChar->id, slotID, boxtype);

                        if (ret != SQL_ERROR &&  Sql_AffectedRows(SqlHandle) != 0)
                        {
                            charutils::AddItem(PChar, LOC_INVENTORY, PItem->getID(), PItem->getQuantity());
                            PChar->pushPacket(new CDeliveryBoxPacket(action, PItem, PChar->UContainer->GetItemsCount()));
                            commit = true;
                        }
                    }
                    else if (boxtype == 0x02)
                    {
                        int32 ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE senderid = %u AND sent = 0 AND slot = %u AND box = %u LIMIT 1", PChar->id, slotID, boxtype);

                        if (ret != SQL_ERROR &&  Sql_AffectedRows(SqlHandle) != 0)
                        {
                            uint8 loc = PChar->getStorage(LOC_INVENTORY)->SearchItemWithSpace(PItem->getID(), PItem->getQuantity());
                            if(loc != ERROR_SLOTID)
                            {
                                charutils::UpdateItem(PChar, LOC_INVENTORY, loc, PItem->getQuantity());
                            }
                            else
                            {
                                charutils::AddItem(PChar, LOC_INVENTORY, PItem->getID(), PItem->getQuantity());
                            }
                            PChar->pushPacket(new CSendBoxPacket(action, PItem, slotID, PChar->UContainer->GetItemsCount()));
                            commit = true;
                        } else {
                            PChar->pushPacket(new CSendBoxPacket(action, PItem, slotID, PChar->UContainer->GetItemsCount(), 0xEB));
                        }
                    }
                    PChar->pushPacket(new CInventoryFinishPacket());
                    PChar->UContainer->SetItem(slotID, NULL);
                    delete PItem;
                }
                if(!commit || !Sql_TransactionCommit(SqlHandle))
                {
                    Sql_TransactionRollback(SqlHandle);
                    ShowError("Could not finalize receive transaction. PlayerID: %d ItemID: %d Quantity: %d", PChar->id, PItem->getID(), PItem->getQuantity());
                }

                Sql_SetAutoCommit(SqlHandle, isAutoCommitOn);
            }
			return;
        }
		case 0x0B: // Option: Drop
		{
            // удаление предмета из ячейки

            if (PChar->UContainer->GetType() == UCONTAINER_DELIVERYBOX &&
               !PChar->UContainer->IsSlotEmpty(slotID))
            {
                int32 ret = Sql_Query(SqlHandle, "DELETE FROM delivery_box WHERE charid = %u AND slot = %u AND box = 1 LIMIT 1", PChar->id, slotID);

                if (ret != SQL_ERROR && Sql_AffectedRows(SqlHandle) != 0)
                {
                    CItem* PItem = PChar->UContainer->GetItem(slotID);
                    PChar->UContainer->SetItem(slotID, NULL);

                    PChar->pushPacket(new CDeliveryBoxPacket(action, PItem, PChar->UContainer->GetItemsCount()));
                    delete PItem;
                }
            }
			return;
		}
        case 0x0C: // Confirm name (send box)
        {
            int32 ret = Sql_Query(SqlHandle, "SELECT charid FROM chars WHERE charname = '%s' LIMIT 1", data+0x10);

            if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
            {
                PChar->pushPacket(new CSendBoxPacket(action, 0xFF, 0x02));
                PChar->pushPacket(new CSendBoxPacket(action, 0x00, 0x01));
            }
            else
            {
                PChar->pushPacket(new CSendBoxPacket(action, 0xFF, 0x02));
                PChar->pushPacket(new CSendBoxPacket(action, 0x00, 0xFB));
            }
            return;
        }
        case 0x0E: //open delivery box
        {
            PChar->UContainer->Clean();
            PChar->UContainer->SetType(UCONTAINER_DELIVERYBOX);

			const int8* fmtQuery = "SELECT itemid, itemsubid, slot, quantity, sender \
                                    FROM delivery_box \
							        WHERE charid = %u \
                                    AND box = %u \
                                    AND slot < 8 \
							        ORDER BY slot;";

	        int32 ret = Sql_Query(SqlHandle, fmtQuery, PChar->id, boxtype);

	        if (ret != SQL_ERROR)
	        {
                if (Sql_NumRows(SqlHandle) != 0)
                {
		            while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		            {
                        CItem* PItem = itemutils::GetItem(Sql_GetIntData(SqlHandle,0));

                        if(PItem != NULL) // Prevent an access violation in the event that an item doesn't exist for an ID
                        {
                            PItem->setSubID(Sql_GetIntData(SqlHandle,1));
                            PItem->setSlotID(Sql_GetIntData(SqlHandle,2));
                            PItem->setQuantity(Sql_GetUIntData(SqlHandle,3));
                            PItem->setSender(Sql_GetData(SqlHandle,4));
                            PChar->UContainer->SetItem(PItem->getSlotID(), PItem);
                        }
                    }
		        }
            }
	        PChar->pushPacket(new CDeliveryBoxPacket(action, 0));
            return;
        }
        case 0x0F:
        {
            if (PChar->UContainer->GetType() == UCONTAINER_DELIVERYBOX)
            {
                if (boxtype == 0x02)
                {
                    //recover items that failed to send
                    for (int i = 0; i < 8; ++i)
                    {
                        if (!PChar->UContainer->IsSlotEmpty(i) &&
                            !PChar->UContainer->GetItem(i)->isSent())
                        {
                            CItem* PItem = PChar->UContainer->GetItem(i);
                            uint8 loc = PChar->getStorage(LOC_INVENTORY)->SearchItemWithSpace(PItem->getID(), PItem->getQuantity());
                            if(loc != ERROR_SLOTID)
                            {
                                charutils::UpdateItem(PChar, LOC_INVENTORY, loc, PItem->getQuantity());
                            }
                            else
                            {
                                uint8 add = charutils::AddItem(PChar, LOC_INVENTORY, PItem->getID(), PItem->getQuantity(), true);
                            }
                        }
                    }
                    const int8* fmtQuery = "DELETE FROM delivery_box \
							            WHERE senderid = %u \
                                        AND box = 2 \
                                        AND slot < 8 \
                                        AND sent = 0 \
							            ORDER BY slot;";
                    int32 ret = Sql_Query(SqlHandle, fmtQuery, PChar->id);
                    DSP_DEBUG_BREAK_IF(ret == SQL_ERROR);
                }
                PChar->UContainer->Clean();
            }
        }
        break;
	}

	// отправка простых действий - открыть окно почты, закрыть окно почты

	PChar->pushPacket(new CDeliveryBoxPacket(action, 0));
	return;
}

/************************************************************************
*																		*
*  Все действия с Auction House                                         *
*																		*
************************************************************************/
void SmallPacket0x04E(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8  action   = RBUFB(data,(0x04));
    uint8  slotid   = RBUFB(data,(0x05));
    uint32 price    = RBUFL(data,(0x08));
    uint8  slot     = RBUFB(data,(0x0C));
    uint16 itemid   = RBUFW(data,(0x0E));
    uint8  quantity = RBUFB(data,(0x10));

    ShowDebug(CL_CYAN"AH Action (%02hx)\n" CL_RESET, RBUFB(data,(0x04)));

    // 0x04 - продажа предмета
    // 0x05 - похоже, что в ответ на этот пакет мы можем открыть список продаж или предложить персонажу подождать немного
    // 0x0A - получение списка продаваемых персонажем предметов
    // 0x0B - подтверждение покупки
    // 0x0E - покупка предмета
    // 0x0С - отмена продажи
    // 0x0D - обновление списка продаваемых персонажем предметов

    switch(action)
    {
        case 0x04:
        {
            CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slot);

            if ((PItem != NULL) &&
                (PItem->getID() == itemid) &&
               !(PItem->isSubType(ITEM_LOCKED)) &&
               !(PItem->getFlag() & ITEM_FLAG_NOAUCTION))
            {
                PItem->setCharPrice(price);
                PChar->pushPacket(new CAuctionHousePacket(action, PItem, quantity));
            }
		}
        break;
        case 0x05:
        {
			uint32 curTick = gettick();

			if(curTick - PChar->m_AHHistoryTimestamp > 5000)
            {
                PChar->m_ah_history.clear();
				PChar->m_AHHistoryTimestamp = curTick;
				PChar->pushPacket(new CAuctionHousePacket(action));

				// A single SQL query for the player's AH history which is stored in a Char Entity struct + vector.
				const int8* Query = "SELECT itemid, price, stack FROM auction_house WHERE seller = %u and sale=0 LIMIT 7;";

				int32 ret = Sql_Query(SqlHandle, Query, PChar->id);

				if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
				{
					while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
					{
						AuctionHistory_t ah;
						ah.itemid = (uint16)Sql_GetIntData(SqlHandle,0);
						ah.price  = (uint32)Sql_GetUIntData(SqlHandle,1);
						ah.stack  = (uint8)Sql_GetIntData(SqlHandle,2);
						ah.status = 0;
						PChar->m_ah_history.push_back(ah);
					}
				}
				ShowDebug("%s has %i items up on the AH. \n",PChar->GetName(),PChar->m_ah_history.size());
			}
			else
            {
				PChar->pushPacket(new CAuctionHousePacket(action, 246, 0, 0)); // try again in a little while msg
				break;
			}
		}
        case 0x0A:
        {
			uint8 totalItemsOnAh = PChar->m_ah_history.size();

            for (int8 slot = 0; slot < totalItemsOnAh; slot++)
            {
                PChar->pushPacket(new CAuctionHousePacket(0x0C, slot,PChar));
            }
		}
        break;
		case 0x0B:
        {
            CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slot);

            if ((PItem != NULL) &&
               !(PItem->isSubType(ITEM_LOCKED)) &&
               !(PItem->getFlag() & ITEM_FLAG_NOAUCTION))
            {
                if (quantity == 0 &&
                   (PItem->getStackSize() == 1 ||
                    PItem->getStackSize() != PItem->getQuantity()))
                {
                    ShowError(CL_RED"SmallPacket0x04E::AuctionHouse: Incorrect quantity of item\n" CL_RESET);
					PChar->pushPacket(new CAuctionHousePacket(action, 197, 0, 0)); //failed to place up
                    return;
                }
				if (PChar->m_ah_history.size() >= 7)
                {
					ShowError(CL_RED"SmallPacket0x04E::AuctionHouse: Unable to put up more than 7 items\n" CL_RESET);
					PChar->pushPacket(new CAuctionHousePacket(action, 197, 0, 0)); //failed to place up
				    return;
				}

                const int8* fmtQuery = "INSERT INTO auction_house(itemid, stack, seller, seller_name, date, price) \
									    VALUES(%u,%u,%u,'%s',%u,%u)";

			    if (Sql_Query(SqlHandle,
                              fmtQuery,
                              PItem->getID(),
                              quantity == 0,
                              PChar->id,
                              PChar->GetName(),
                              CVanaTime::getInstance()->getSysTime(),
                              price) == SQL_ERROR)
			    {
				    ShowError(CL_RED"SmallPacket0x04E::AuctionHouse: Cannot insert item to database\n" CL_RESET);
					PChar->pushPacket(new CAuctionHousePacket(action, 197, 0, 0)); //failed to place up
				    return;
			    }
                charutils::UpdateItem(PChar, LOC_INVENTORY, slot, -(int32)(quantity != 0 ? 1 : PItem->getStackSize()));

				PChar->pushPacket(new CAuctionHousePacket(action, 1, 0, 0)); //merchandise put up on auction msg
				PChar->pushPacket(new CAuctionHousePacket(0x0C, PChar->m_ah_history.size(),PChar)); //inform history of slot
            }
		}
        break;
		case 0x0E:
        {
            itemid = RBUFW(data,(0x0C));

            if (PChar->getStorage(LOC_INVENTORY)->GetFreeSlotsCount() == 0)
            {
                PChar->pushPacket(new CAuctionHousePacket(action, 0xE5, 0, 0));
            }
            else
            {
                CItem* PItem = itemutils::GetItemPointer(itemid);

                if (PItem != NULL)
                {
                    if (PItem->getFlag() & ITEM_FLAG_RARE)
                    {
                        for (uint8 LocID = 0; LocID < MAX_CONTAINER_ID; ++LocID)
			            {
				            if (PChar->getStorage(LocID)->SearchItem(itemid) != ERROR_SLOTID)
				            {
					            PChar->pushPacket(new CAuctionHousePacket(action, 0xE5, 0, 0));
					            return;
				            }
			            }
                    }
                    CItem* gil  = PChar->getStorage(LOC_INVENTORY)->GetItem(0);

	                if (gil != NULL &&
                        gil->isType(ITEM_CURRENCY) &&
                        gil->getQuantity() >= price)
	                {
                        const int8* fmtQuery = "UPDATE auction_house \
                                                SET buyer_name = '%s', sale = %u, sell_date = %u \
                                                WHERE itemid = %u AND buyer_name IS NULL AND stack = %u AND price <= %u \
                                                ORDER BY price \
                                                LIMIT 1";

                        if (Sql_Query(SqlHandle,
                                      fmtQuery,
                                      PChar->GetName(),
                                      price,
                                      CVanaTime::getInstance()->getSysTime(),
                                      itemid,
                                      quantity == 0,
                                      price) != SQL_ERROR &&
                            Sql_AffectedRows(SqlHandle) != 0)
                        {
                            uint8 SlotID = charutils::AddItem(PChar, LOC_INVENTORY, itemid, (quantity == 0 ? PItem->getStackSize() : 1));

			                if (SlotID != ERROR_SLOTID)
			                {
				                charutils::UpdateItem(PChar, LOC_INVENTORY, 0, -(int32)(price));

                                PChar->pushPacket(new CAuctionHousePacket(action, 0x01, itemid, price));
				                PChar->pushPacket(new CInventoryFinishPacket());
                            }
                            return;
                        }
                    }
                }
                PChar->pushPacket(new CAuctionHousePacket(action, 0xC5, itemid, price));
            }
        }
        break;
        case 0x0C: //removing item from ah
        {
			//check user has invent space
			if (PChar->getStorage(LOC_INVENTORY)->GetFreeSlotsCount() == 0)
            {
                PChar->pushPacket(new CAuctionHousePacket(action, 0xE5, PChar, slotid, true)); //invent full, unable to remove msg
				return;
            }
            else
            {
				const int8* delQuery = "DELETE FROM auction_house WHERE seller = %u AND id = %u AND sale = 0 LIMIT 1;";

				const int8* fmtQuery = "SELECT itemid, stack, id FROM auction_house WHERE seller = %u and sale=0;";
				int32 ret = Sql_Query(SqlHandle, fmtQuery, PChar->id);
				if( ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
				{
					uint8 count = 0;
					while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
					{
						if(count == slotid){
							uint16 delitemid = (uint16)Sql_GetUIntData(SqlHandle,0);
							uint16 delitemstack = (uint16)Sql_GetUIntData(SqlHandle,1);
							uint32 ahid = (uint32)Sql_GetUIntData(SqlHandle,2);
							int32 delret = Sql_Query(SqlHandle, delQuery, PChar->id, ahid);
							if(delret != SQL_ERROR){
								//add the item back to the users invent
								CItem* PDelItem = itemutils::GetItemPointer(delitemid);
								if(PDelItem != NULL){
									uint8 SlotID = charutils::AddItem(PChar, LOC_INVENTORY, delitemid,
										(delitemstack != 0 ? PDelItem->getStackSize() : 1));

									if (SlotID != ERROR_SLOTID)
									{
										PChar->pushPacket(new CAuctionHousePacket(action, 0, PChar, slotid, false));
						                PChar->pushPacket(new CInventoryFinishPacket());
			                        }
									else {
										ShowError("Failed to return item id %u stack %u to char... \n",delitemid,delitemstack);
									}
				                    return;
								}
							}
							break;
						}
						count++;
					}
				}
				//let client know something went wrong
				PChar->pushPacket(new CAuctionHousePacket(action, 0xE5, PChar, slotid, true)); //invent full, unable to remove msg
			}
		}
        break;
		case 0x0D:
        {
            PChar->pushPacket(new CAuctionHousePacket(action, slotid,PChar));
		}
        break;
	}
    return;
}

/************************************************************************
*																		*
*  Смена экипировки														*
*																		*
************************************************************************/

void SmallPacket0x050(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->status != STATUS_NORMAL &&
        PChar->status != STATUS_UPDATE)
		return;

	uint8 slotID      = RBUFB(data,(0x04));		// inventory slot
	uint8 equipSlotID = RBUFB(data,(0x05));		// charequip slot


	charutils::EquipItem(PChar, slotID, equipSlotID);
    charutils::SaveCharEquip(PChar);

	luautils::CheckForGearSet(PChar); // check for gear set on gear change
	return;
}

/************************************************************************
*																		*
*  Завершение синтеза													*
*																		*
************************************************************************/

void SmallPacket0x059(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	synthutils::sendSynthDone(PChar);
	return;
}

/************************************************************************
*																		*
*  Обновляем карты conquest, besieged, compaing							*
*																		*
************************************************************************/

void SmallPacket0x05A(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->pushPacket(new CConquestPacket(PChar));
	PChar->pushPacket(new CCampaingPacket(PChar,0));
	PChar->pushPacket(new CCampaingPacket(PChar,1));

	// пакет не на своем месте, возможно 0x0F
	PChar->pushPacket(new CStopDownloadingPacket(PChar));
//	luautils::CheckForGearSet(PChar); // also check for gear set
	return;
}

/************************************************************************
*																		*
*  Завершаем или обновляем текущее событие                              *
*																		*
************************************************************************/

void SmallPacket0x05B(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 EventID = RBUFW(data,(0x12));
	uint32 Result  = RBUFL(data,(0x08));

    if (PChar->m_event.Option != 0) Result = PChar->m_event.Option;

	if (RBUFB(data,(0x0E)) != 0){
		luautils::OnEventUpdate(PChar,EventID,Result);
	}else{
		luautils::OnEventFinish(PChar,EventID,Result);
		PChar->m_event.reset();
	}

	PChar->pushPacket(new CReleasePacket(PChar,RELEASE_EVENT));
	return;
}

/************************************************************************
*																		*
*  Завершаем или обновляем текущее событие                              *
*  Так же требуется обновить позицию персонажа                          *
*																		*
************************************************************************/

void SmallPacket0x05C(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 EventID = RBUFW(data,(0x1A));

	PChar->loc.p.x = RBUFF(data,(0x04));
	PChar->loc.p.y = RBUFF(data,(0x08));
	PChar->loc.p.z = RBUFF(data,(0x0C));
	PChar->loc.p.rotation = RBUFB(data,(0x1F));

	if (RBUFB(data,(0x1E)) != 0){
		luautils::OnEventUpdate(PChar,EventID,0);
	}else{
		luautils::OnEventFinish(PChar,EventID,0);
		PChar->m_event.reset();
	}

	PChar->pushPacket(new CCSPositionPacket(PChar));
	PChar->pushPacket(new CPositionPacket(PChar));
	PChar->pushPacket(new CReleasePacket(PChar,RELEASE_EVENT));
	return;
}

/************************************************************************
*																		*
*  Эмоции персонажей, включая специфические (/jobemote [job])			*
*																		*
************************************************************************/

void SmallPacket0x05D(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    if(jailutils::InPrison(PChar))
    {
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
        return;
    }

	PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE_SELF, new CCharEmotionPacket(PChar,data));
	return;
}

/************************************************************************
*																		*
*  Перемещение персонажа между зонами (zoneline)						*
*																		*
************************************************************************/

void SmallPacket0x05E(map_session_data_t* session, CCharEntity* PChar, int8* data)
{

	// handle pets on zone
	if (PChar->PPet != NULL)
	{
		CPetEntity* PPet = (CPetEntity*)PChar->PPet;

		if (PPet->objtype == TYPE_MOB)
		{
			// uncharm a charmed mob
			petutils::DespawnPet(PChar);
		}
		else if (PPet->objtype == TYPE_PET)
		{
			switch (PPet->getPetType())
			{
				case PETTYPE_JUG_PET:
				case PETTYPE_AUTOMATON:
				case PETTYPE_WYVERN:
					PChar->petZoningInfo.petHP = PPet->health.hp;
					PChar->petZoningInfo.petTP = PPet->health.tp;
					PChar->petZoningInfo.respawnPet = true;
					PChar->petZoningInfo.petType = PPet->getPetType();
					petutils::DespawnPet(PChar);
					break;

				case PETTYPE_AVATAR:
					petutils::DespawnPet(PChar);

				default:
					break;
			}
		}
	}


	uint32 zoneLineID = RBUFL(data,(0x04));
    //TODO: verify packet in adoulin expansion
	uint8  town		  = RBUFB(data,(0x16)); // используются при выходе из mog house
	uint8  zone		  = RBUFB(data,(0x17)); // используются при выходе из mog house

	// переход между зонами с использованием таблицы zoneline
	//
	// игнорируем все zoneline пакеты,
	// пока не завершен текущий переход

	if (PChar->status == STATUS_NORMAL ||
        PChar->status == STATUS_UPDATE)
	{
		PChar->status = STATUS_DISAPPEAR;
		PChar->loc.boundary = 0;

		zoneLine_t* PZoneLine = PChar->loc.zone->GetZoneLine(zoneLineID);

		if (PZoneLine == NULL) // разворачиваем персонажа на 180° и отправляем туда, откуда пришел
		{
            ShowError(CL_RED"SmallPacket0x5E: Zone line %u not found\n" CL_RESET, zoneLineID); // в идеале нужно добавить зону и координаты

			PChar->loc.p.rotation += 128;

            PChar->pushPacket(new CMessageSystemPacket(0,0,2));
            PChar->pushPacket(new CCSPositionPacket(PChar));

            PChar->status = STATUS_UPDATE;
            return;
		}else{
			if (zoneutils::GetZone(PZoneLine->m_toZone)->GetIP() == 0) 	// разворачиваем персонажа на 180° и отправляем туда, откуда пришел
			{
				ShowDebug(CL_CYAN"SmallPacket0x5E: Zone %u closed to chars\n" CL_RESET, PZoneLine->m_toZone);

				PChar->loc.p.rotation += 128;

                PChar->pushPacket(new CMessageSystemPacket(0,0,2));
                PChar->pushPacket(new CCSPositionPacket(PChar));

                PChar->status = STATUS_UPDATE;
                return;
			} else {
                // выход из MogHouse
				if(PZoneLine->m_zoneLineID == 1903324538)
				{
                    uint16 prevzone = PChar->loc.prevzone;

                    if (zone != 0)  // 0 - выход в предыдущую зону, остальные значения - выбор зоны по имени
                    {
					    switch (town)
					    {
						    case 1: prevzone = zone + 0xE5; break;
						    case 2: prevzone = zone + 0xE9; break;
						    case 3: prevzone = zone + 0xED; break;
						    case 4: prevzone = zone + 0xF2; break;
						    case 5: prevzone = zone + (zone == 1 ? 0x2F : 0x30); break;
					    }
                    }
                    PChar->loc.destination = prevzone;
				} else {
                    PChar->loc.destination = PZoneLine->m_toZone;
				}
				PChar->loc.p = PZoneLine->m_toPos;
			}
		}
        ShowInfo(CL_WHITE"Zoning from zone %u to zone %u: %s\n" CL_RESET, PChar->getZone(), PChar->loc.destination, PChar->GetName());
	}
	PChar->clearPacketList();
	PChar->pushPacket(new CServerIPPacket(PChar,2));
	return;
}

/************************************************************************
*                                                                       *
*  Персонаж присылает текстовую информацию для продолжения события      *
*                                                                       *
************************************************************************/

// zone 245 cs 0x00C7 Password

void SmallPacket0x060(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PrintPacket(data);

  //luautils::OnEventUpdate(PChar, 0, 0);
  //PChar->pushPacket(new CReleasePacket(PChar,RELEASE_EVENT));
	return;
}

/************************************************************************
*																		*
*																		*
*																		*
************************************************************************/

void SmallPacket0x061(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->pushPacket(new CCharUpdatePacket(PChar));
	PChar->pushPacket(new CCharHealthPacket(PChar));
	PChar->pushPacket(new CCharStatsPacket(PChar));
	PChar->pushPacket(new CCharSkillsPacket(PChar));
	PChar->pushPacket(new CMenuMeritPacket(PChar));
	PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
    PChar->pushPacket(new CCharJobExtraPacket(PChar, false));

	return;
}

/************************************************************************
*																		*
*  Завершение раскопок chocobo (chocobo gigging)						*
*																		*
************************************************************************/

void SmallPacket0x063(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	return;
}

/************************************************************************
*																		*
*  Помечаем KeyItem как "увиденный"										*
*																		*
************************************************************************/

void SmallPacket0x064(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8 KeyTable = RBUFB(data,(0x4A));
	memcpy(PChar->keys.seenList+(0x40*KeyTable),data+(0x08),0x40);

	charutils::SaveKeyItems(PChar);
	return;
}

/************************************************************************
*																		*
*  Окончание рыбалки													*
*																		*
************************************************************************/

void SmallPacket0x066(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PrintPacket(data);

	uint16 stamina = RBUFW(data,(0x08));
	uint8  action  = RBUFB(data,(0x0E));

	fishingutils::FishingAction(PChar, (FISHACTION)action, stamina);
	return;
}

/************************************************************************
*                                                                       *
*  Персонаж приглашает другого персонажа в группу                       *
*                                                                       *
************************************************************************/

void SmallPacket0x06E(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 charid = RBUFL(data,0x04);
	uint16 targid = RBUFW(data,0x08);

	// Персонаж не должен приглашать сам себя.
	if (PChar->id == charid)
		return;

    if(jailutils::InPrison(PChar))
    {
        // Initiator is in prison.  Send error message.
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
        return;
    }

    if (PChar->PParty == NULL || PChar->PParty->GetLeader() == PChar)
    {
        // если targid персонажа клиенту не известен, то получаем его из таблицы активных сессий
        if (targid == 0)
        {
	        int32 ret = Sql_Query(SqlHandle, "SELECT targid FROM accounts_sessions WHERE charid = %u LIMIT 1", charid);

	        if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	        {
		        targid = (uint16)Sql_GetIntData(SqlHandle,0);
	        }
        }

        CCharEntity* PInvitee = zoneutils::GetCharFromRegion(
			charid,
			targid,
            PChar->loc.zone->GetRegionID());

	    if (PInvitee != NULL && !jailutils::InPrison(PInvitee))
	    {
			//make sure intvitee isn't dead and that they dont already have an invite pending
			if (PInvitee->isDead() || PInvitee->InvitePending.id != 0)
		    {
			    PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 23));
			    return;
		    }


			//check to see if user is adding a party leader for alliance
			if (PInvitee->PParty != NULL)// && PChar->Party != NULL
		    {
				if (PInvitee->PParty->GetLeader() == PInvitee)
				{
					//make sure invitee does not already have alliance
					if (PInvitee->PParty->m_PAlliance == NULL)
					{
						//party is not already in alliance so add them
						PInvitee->InvitePending.id = PChar->id;
						PInvitee->InvitePending.targid = PChar->targid;
						PInvitee->pushPacket(new CPartyInvitePacket(PInvitee, PChar, INVITE_ALLIANCE));
						return;
					}
				}
			}

		    if (PInvitee->PParty != NULL)
		    {
			    PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 12));
			    return;
		    }

            PInvitee->InvitePending.id = PChar->id;
            PInvitee->InvitePending.targid = PChar->targid;
		    PInvitee->pushPacket(new CPartyInvitePacket(PInvitee, PChar, INVITE_PARTY));

		    if (PChar->PParty != NULL &&
			    PChar->PParty->GetSyncTarget() != NULL)
		    {
			    PInvitee->pushPacket(new CMessageStandardPacket(PInvitee, 0, 0, 235));
		    }
	    }
    }
	return;
}

/************************************************************************
*																		*
*  Персонаж покидает группу												*
*																		*
************************************************************************/

void SmallPacket0x06F(map_session_data_t* session, CCharEntity* PChar, int8* data)
{

	//alliance - party leader dispands dropping the party from the alliance
	if (PChar->PParty != NULL)
	{
		if (PChar->PParty->m_PAlliance != NULL)
		{
			if (PChar->PParty->m_PAlliance->getMainParty() != PChar->PParty)
			{
				if (PChar->PParty->GetLeader() == PChar)
				{
						//if there are only 2 parties then dissolve alliance
						if (PChar->PParty->m_PAlliance->partyCount() == 2)
						{
							PChar->PParty->m_PAlliance->dissolveAlliance();
							return;
						}
					//remove 1 party from alliance
					PChar->PParty->m_PAlliance->removeParty(PChar->PParty);
					return;
				}

			}
		}
	}

	//normal party member disband
	if (PChar->PParty != NULL){
	PChar->PParty->RemoveMember(PChar);
	}

	return;
}

/************************************************************************
*																		*
*  Лидер распускает группу												*
*																		*
************************************************************************/

void SmallPacket0x070(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	//this is where alliance leader can dissolve alliance completely
	if (PChar->PParty != NULL && PChar->PParty->GetLeader() == PChar)
	{
		if (PChar->PParty->m_PAlliance != NULL && PChar->PParty->m_PAlliance->getMainParty() == PChar->PParty)
		{
			//dissolve the entire alliance
			PChar->PParty->m_PAlliance->dissolveAlliance();
		}
		else
		{
			//just dissolve party
			PChar->PParty->DisbandParty();
		}
	}
	return;
}

/************************************************************************
*																		*
*  Удаляем члена группы или linckshell									*
*																		*
************************************************************************/

void SmallPacket0x071(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	switch(RBUFB(data,(0x0A)))
	{
		case 0: // party
		{
			if (PChar->PParty != NULL &&
				PChar->PParty->GetLeader() == PChar)
			{
				PChar->PParty->RemoveMemberByName(data+0x0C);
			}
		}
		break;
        case 1: // linkshell
        {
            if (PChar->PLinkshell != NULL)
            {
                CItemLinkshell* PItemLinkshell = (CItemLinkshell*)PChar->getStorage(LOC_INVENTORY)->GetItem(PChar->equip[SLOT_LINK]);

                if (PItemLinkshell != NULL && PItemLinkshell->isType(ITEM_LINKSHELL))
                {
					// TODO: can currently use command to kick sack holders
					if(PItemLinkshell->GetLSType() == LSTYPE_LINKSHELL || PItemLinkshell->GetLSType() == LSTYPE_PEARLSACK){
						PChar->PLinkshell->RemoveMemberByName(data+0x0C);
					}
                }
            }
        }
        break;
		default:
		{
			ShowError(CL_RED"SmallPacket0x071 : unknown byte <%.2X>\n" CL_RESET, RBUFB(data,(0x0A)));
		}
	}
	return;
}

/************************************************************************
*																		*
*  Персонаж отвечает на приглашение в группу							*
*																		*
************************************************************************/

void SmallPacket0x074(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    CCharEntity* PInviter = zoneutils::GetCharFromRegion(
        PChar->InvitePending.id,
        PChar->InvitePending.targid,
        PChar->loc.zone->GetRegionID());

	if (PInviter != NULL)
	{
		uint8 InviteAnswer = RBUFB(data,(0x04));

		if (InviteAnswer == 0)
		{
			//invitee declined invite
			PInviter->pushPacket(new CMessageStandardPacket(PInviter, 0, 0, 11));
			PChar->InvitePending.clean();
			return;
		}

		//check for alliance invite
		if (PChar->PParty != NULL && PInviter->PParty != NULL)
		{
			//both invitee and and inviter are party leaders
			if(PInviter->PParty->GetLeader() == PInviter && PChar->PParty->GetLeader() == PChar)
			{

				//the inviter already has an alliance and wants to add another party - only add if they have room for another party
				if(PInviter->PParty->GetLeader() == PInviter && PInviter->PParty->m_PAlliance != NULL)
				{
					if(PInviter->PParty->m_PAlliance->getMainParty()->GetLeader() == PInviter)
					{
						//break if alliance is full
						if(PInviter->PParty->m_PAlliance->partyCount() == 3)  return;


						//alliance is not full, add the new party
						PInviter->PParty->m_PAlliance->addParty(PChar->PParty);
						PChar->InvitePending.clean();
						return;
					}
				}


				//party leaders have no alliance - create a new one!
				CAlliance* PAlliance = new CAlliance(PInviter);
				PInviter->PParty->m_PAlliance->addParty(PChar->PParty);
				PChar->InvitePending.clean();
				return;
			}
		}


		//the rest is for a standard party invitation
        if (PChar->PParty == NULL)
		{
			if (PInviter->PParty == NULL)
			{
				CParty* PParty = new CParty(PInviter);
			}
            if (PInviter->PParty->GetLeader() == PInviter)
            {
				if(PInviter->PParty->members.size()==6){//someone else accepted invitation
					PInviter->pushPacket(new CMessageStandardPacket(PInviter, 0, 0, 14));
					PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 14));
				}
				else{
					PInviter->PParty->AddMember(PChar);
				}
            }
		}
	}
    PChar->InvitePending.clean();
	return;
}

/************************************************************************
*																		*
*  Персонаж запрашивает информацию о членах группы						*
*																		*
************************************************************************/

void SmallPacket0x076(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	//alliance
	if (PChar->PParty != NULL)
	{
		if (PChar->PParty->m_PAlliance != NULL)
		{
			PChar->PParty->ReloadPartyMembers(PChar);

			for (uint8 i = 0; i < PChar->PParty->m_PAlliance->partyList.size(); ++i)
			{
				if(PChar->PParty->m_PAlliance->partyList.at(i) != PChar->PParty)
				{
					for (uint8 a = 0; a < PChar->PParty->m_PAlliance->partyList.at(i)->members.size(); ++a)
					{
						PChar->PParty->m_PAlliance->partyList.at(i)->ReloadPartyMembers((CCharEntity*)PChar->PParty->m_PAlliance->partyList.at(i)->members.at(a));
					}
				}
			}
		return;

		}else{
			//normal party - no alliance
			PChar->PParty->ReloadPartyMembers(PChar);
			return;
		}
	}

}

/************************************************************************
*																		*
*																		*
*																		*
************************************************************************/

void SmallPacket0x077(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	switch(RBUFB(data,(0x14)))
	{
		case 0: // party
		{
			if (PChar->PParty != NULL &&
				PChar->PParty->GetLeader() == PChar)
			{
				PChar->PParty->AssignPartyRole(data+0x04, RBUFB(data,(0x15)));
			}
		}
		break;
        case 1: // linkshell
        {
            if (PChar->PLinkshell != NULL)
            {
				PChar->PLinkshell->ChangeMemberRank(data+0x04, RBUFB(data,(0x15)));
            }
        }
        break;
		default:
		{
			ShowError(CL_RED"SmallPacket0x077 : changing role packet with unknown byte <%.2X>\n" CL_RESET, RBUFB(data,(0x14)));
		}
	}
	return;
}

/************************************************************************
*																		*
*  Запрос перед поиском членов группы									*
*																		*
************************************************************************/

void SmallPacket0x078(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->pushPacket(new CPartySearchPacket(PChar));
	return;
}

/************************************************************************
*																		*
*  Покупка предмета в магазине											*
*																		*
************************************************************************/

void SmallPacket0x083(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8  quantity   = RBUFB(data,(0x04));
	uint8  shopSlotID = RBUFB(data,(0x0A));

	uint16 itemID = PChar->Container->getItemID(shopSlotID);
	uint32 price  = PChar->Container->getQuantity(shopSlotID); // здесь мы сохранили стоимость предмета

	CItem* gil  = PChar->getStorage(LOC_INVENTORY)->GetItem(0);

	if ((gil != NULL) && gil->isType(ITEM_CURRENCY))
	{
		if (gil->getQuantity() > (price * quantity))
		{
			uint8 SlotID = charutils::AddItem(PChar, LOC_INVENTORY, itemID, quantity);

			if (SlotID != ERROR_SLOTID)
			{
				charutils::UpdateItem(PChar, LOC_INVENTORY, 0, -(int32)(price * quantity));

				PChar->pushPacket(new CShopBuyPacket(shopSlotID, quantity));
				PChar->pushPacket(new CInventoryFinishPacket());
			}
		}
	}
	return;
}

/************************************************************************
*																		*
*  Оценка стоимости предмета перед продажей в магазин					*
*  Помещаем информацию о продаваемом предмете в последнюю ячейку		*
*  контейнера															*
*																		*
************************************************************************/

void SmallPacket0x084(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 quantity = RBUFL(data,(0x04));
	uint16 itemID   = RBUFW(data,(0x08));
	uint8  slotID   = RBUFB(data,(0x0A));

	CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);
	if( (PItem != NULL) &&
		(PItem->getID() == itemID) &&
	   !(PItem->getFlag() & ITEM_FLAG_NOSALE) )
	{
		// подготавливаем предмет для продажи
		PChar->Container->setItem(16, itemID, slotID, quantity);
		PChar->pushPacket(new CShopAppraisePacket(slotID, PItem->getBasePrice()));
	}
	return;
}

/************************************************************************
*																		*
*  Продажа предмета в магазин											*
*  Информация о продаваемом предмете была предварительно сохранена в	*
*  последней (16-ой) ячейке контейнера в 0x084-ом пакете 				*
*																		*
************************************************************************/

void SmallPacket0x085(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 quantity = PChar->Container->getQuantity(16);
	uint16 itemID   = PChar->Container->getItemID(16);
	uint8  slotID   = PChar->Container->getInvSlotID(16);

	CItem* gil   = PChar->getStorage(LOC_INVENTORY)->GetItem(0);
	CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);

	if ( (PItem != NULL) && ((gil != NULL) && gil->isType(ITEM_CURRENCY)) )
	{
		charutils::UpdateItem(PChar, LOC_INVENTORY, 0, quantity * PItem->getBasePrice());
		charutils::UpdateItem(PChar, LOC_INVENTORY, slotID, -(int32)quantity);

		PChar->pushPacket(new CMessageStandardPacket(0, itemID, quantity, 232));
		PChar->pushPacket(new CInventoryFinishPacket());
	}
	// очищаем ячейку для безопасности (защита от группы 0x085-ых пакетов)
	PChar->Container->setItem(16,0,-1,0);
	return;
}

/************************************************************************
*																		*
*  Начало синтеза														*
*  В нулевой ячейке будет храниться кристалл							*
*																		*
************************************************************************/

void SmallPacket0x096(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    if(jailutils::InPrison(PChar))
    {
        // Prevent crafting in prison
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
        return;
    }

	PChar->Container->Clean();

	uint32 ItemID    = RBUFL(data,(0x06));
	uint8  invSlotID = RBUFB(data,(0x08));

	uint8  numItems  = RBUFB(data,(0x09));

	PChar->Container->setItem(0, ItemID, invSlotID, 0);

	for(int32 SlotID = 0; SlotID < numItems; ++SlotID)
	{
		ItemID    = RBUFL(data,(0x0A+SlotID*2));
		invSlotID = RBUFB(data,(0x1A+SlotID));

		PChar->Container->setItem(SlotID+1, ItemID, invSlotID, 1);
	}

	synthutils::startSynth(PChar);
	return;
}

/************************************************************************
*																		*
*  Guild Purchase														*
*																		*
************************************************************************/

void SmallPacket0x0AA(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 itemID     = RBUFW(data,(0x04));
	uint8  quantity   = RBUFB(data,(0x07));
	uint8  shopSlotID = PChar->PGuildShop->SearchItem(itemID);
    CItemShop* item   = (CItemShop*)PChar->PGuildShop->GetItem(shopSlotID);
	CItem* gil        = PChar->getStorage(LOC_INVENTORY)->GetItem(0);

	if (((gil != NULL) && gil->isType(ITEM_CURRENCY)) && item != NULL && item->getQuantity() >= quantity)
	{
		if (gil->getQuantity() > (item->getBasePrice() * quantity))
		{
			uint8 SlotID = charutils::AddItem(PChar, LOC_INVENTORY, itemID, quantity);

			if (SlotID != ERROR_SLOTID)
			{
				charutils::UpdateItem(PChar, LOC_INVENTORY, 0, -(int32)(item->getBasePrice() * quantity));

				PChar->PGuildShop->GetItem(shopSlotID)->setQuantity(PChar->PGuildShop->GetItem(shopSlotID)->getQuantity()-quantity);
                PChar->pushPacket(new CGuildMenuBuyUpdatePacket(PChar, PChar->PGuildShop->GetItem(PChar->PGuildShop->SearchItem(itemID))->getQuantity(), itemID, quantity));
                PChar->pushPacket(new CInventoryFinishPacket());
			}
		}
	}
    //TODO: error messages!
	return;
}

/************************************************************************
*																		*
*  Генерация случайного числа (команда /diceroll)						*
*																		*
************************************************************************/

void SmallPacket0x0A2(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 diceroll = 1 + rand()%1000;

	PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE_SELF, new CMessageStandardPacket(PChar, diceroll, 88));
	return;
}

/************************************************************************
*																		*
*  Отправляем персонажу список продаваемых гильдией предметов			*
*																		*
************************************************************************/

void SmallPacket0x0AB(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->PGuildShop != NULL)
	{
		PChar->pushPacket(new CGuildMenuBuyPacket(PChar, PChar->PGuildShop));
	}
	return;
}

/************************************************************************
*																		*
*  Sell items to guild                                      			*
*																		*
************************************************************************/

void SmallPacket0x0AC(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->PGuildShop != NULL)
	{
	    uint16 itemID       = RBUFW(data,(0x04));
        uint8  slot         = RBUFB(data,(0x06));
	    uint8  quantity     = RBUFB(data,(0x07));
	    uint8  shopSlotID   = PChar->PGuildShop->SearchItem(itemID);
        CItemShop* shopItem = (CItemShop*)PChar->PGuildShop->GetItem(shopSlotID);
        CItem*     charItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slot);

        //TODO: add all sellable items to guild table
        if (shopItem && charItem && charItem->getQuantity() >= quantity)
	    {
			if (charutils::UpdateItem(PChar, LOC_INVENTORY, slot, -quantity) == itemID)
			{
				charutils::UpdateItem(PChar, LOC_INVENTORY, 0, (shopItem->getBasePrice() / 3) * quantity);

				PChar->PGuildShop->GetItem(shopSlotID)->setQuantity(PChar->PGuildShop->GetItem(shopSlotID)->getQuantity()+quantity);
                PChar->pushPacket(new CGuildMenuSellUpdatePacket(PChar, PChar->PGuildShop->GetItem(PChar->PGuildShop->SearchItem(itemID))->getQuantity(), itemID, quantity));
                PChar->pushPacket(new CInventoryFinishPacket());
			}
	    }
        //TODO: error messages!
    }
	return;
}

/************************************************************************
*																		*
*  Отправляем персонажу список покупаемых гильдией предметов			*
*																		*
************************************************************************/

void SmallPacket0x0AD(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->PGuildShop != NULL)
	{
		PChar->pushPacket(new CGuildMenuSellPacket(PChar, PChar->PGuildShop));
	}
	return;
}

/************************************************************************
*																		*
*  Текстовое общение между персонажами (chat)							*
*																		*
************************************************************************/

void SmallPacket0x0B5(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	/*if (RBUFB(data,(0x06)) == '@' && CmdHandler.call(PChar, (const int8*)data+7) == 0)
	{
		//this makes sure a command isn't sent to chat
	}
    else if (RBUFB(data,(0x06)) == '#' && PChar->nameflags.flags & FLAG_GM)
    {
        for (uint16 zone = 0; zone < MAX_ZONEID; ++zone)
        {
            zoneutils::GetZone(zone)->PushPacket(
                NULL,
                CHAR_INZONE,
                new CChatMessagePacket(PChar, MESSAGE_SYSTEM_1, data+7));
        }
    }
    else
    {
        if(jailutils::InPrison(PChar))
        {
            if(RBUFB(data,(0x04)) == MESSAGE_SAY)
            {
                PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_SAY, data+6));
            }
            else
            {
                PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
            }
        }
        else
        {
            switch(RBUFB(data,(0x04)))
            {
                case MESSAGE_SAY:
					{
						if (map_config.audit_chat == 1)
						{
							std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
							qStr +=PChar->GetName();
							qStr +="','SAY','";
							qStr += escape(data+6);
							qStr +="',current_timestamp());";
							const char * cC = qStr.c_str();
							Sql_QueryStr(SqlHandle, cC);
						}
						PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_SAY,     data+6));
					}
					break;
                case MESSAGE_EMOTION:	PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_EMOTION, data+6)); break;
                case MESSAGE_SHOUT:
					{
						if (map_config.audit_chat == 1)
						{
							std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
							qStr +=PChar->GetName();
							qStr +="','SHOUT','";
							qStr += escape(data+6);
							qStr +="',current_timestamp());";
							const char * cC = qStr.c_str();
							Sql_QueryStr(SqlHandle, cC);
						}
						PChar->loc.zone->PushPacket(PChar, CHAR_INSHOUT, new CChatMessagePacket(PChar, MESSAGE_SHOUT,   data+6));

					}
					break;
                case MESSAGE_LINKSHELL:
                {
                    if (PChar->PLinkshell != NULL)
                    {
						if (map_config.audit_chat == 1)
						{
							std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
							qStr +=PChar->GetName();
							qStr +="','LINKSHELL','";
							qStr += escape(data+6);
							qStr +="',current_timestamp());";
							const char * cC = qStr.c_str();
							Sql_QueryStr(SqlHandle, cC);
						}
                        PChar->PLinkshell->PushPacket(PChar, new CChatMessagePacket(PChar, MESSAGE_LINKSHELL, data+6));
                    }
                }
                break;
				case MESSAGE_PARTY:
                {
                    if (PChar->PParty != NULL)
                    {
						if (PChar->PParty->m_PAlliance == NULL)
						{
							if (map_config.audit_chat == 1)
							{
								std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
								qStr +=PChar->GetName();
								qStr +="','PARTY','";
								qStr += escape(data+6);
								qStr +="',current_timestamp());";
								const char * cC = qStr.c_str();
								Sql_QueryStr(SqlHandle, cC);
							}
							PChar->PParty->PushPacket(PChar, 0, new CChatMessagePacket(PChar, MESSAGE_PARTY, data+6));

						}else{ //alliance party chat
								for (uint8 i = 0; i < PChar->PParty->m_PAlliance->partyList.size(); ++i)
								{
									PChar->PParty->m_PAlliance->partyList.at(i)->PushPacket(PChar, 0, new CChatMessagePacket(PChar, MESSAGE_PARTY, data+6));
								}
								if (map_config.audit_chat == 1)
								{
									std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
									qStr +=PChar->GetName();
									qStr +="','ALLIANCE','";
									qStr += escape(data+6);
									qStr +="',current_timestamp());";
									const char * cC = qStr.c_str();
									Sql_QueryStr(SqlHandle, cC);
								}
							}
					}
                }
                break;
                case MESSAGE_YELL:
					{
						if (map_config.audit_chat == 1)
						{
							std::string qStr = ("INSERT into audit_chat (speaker,type,message,datetime) VALUES('");
							qStr +=PChar->GetName();
							qStr +="','YELL','";
							qStr += escape(data+6);
							qStr +="',current_timestamp());";
							const char * cC = qStr.c_str();
							Sql_QueryStr(SqlHandle, cC);
						}
						PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 256));
					}break;
            }
        }
	}

	return;*/
		ShowNotice("PCHAR CHAT SYSTEM CALLED\n");
	if(PChar != NULL)
	{
	if (RBUFB(data,(0x06)) == '.' )
	{ 
		
		int32 accid=0;
	    const int8* GetCharAccountID = "SELECT accid FROM chars WHERE charid = '%u';";
	    int32 ret = Sql_Query(SqlHandle,GetCharAccountID,PChar->id);

		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	       {
		    accid= (int32)Sql_GetIntData(SqlHandle,0);
	       // //ShowNotice(CL_RED"COMMAND TRACER: ACCOUNTID %u  \n" CL_RESET,accid);
			
			int32 secuitylevel=0;
			const int8* GetAccountSecuity = "SELECT security FROM accounts WHERE id = '%u';";
			int32 ret1 = Sql_Query(SqlHandle,GetAccountSecuity,accid);

	        if (ret1 != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	           {
		        secuitylevel= (int32)Sql_GetIntData(SqlHandle,0);
				PChar->Account_Level = secuitylevel;
				
                    if(PChar->Account_Level==0)
				     {
                 ////ShowNotice(CL_RED"COMMAND TRACER: Player called Command \n" CL_RESET);	
				       CmdHandler.pcall(PChar, (const int8*)data+7);
				     }
				     if(PChar->Account_Level==1)
				     {
               //  //ShowNotice(CL_RED"COMMAND TRACER: GM called Command \n" CL_RESET);	
				     CmdHandler.gcall(PChar, (const int8*)data+7);
				     }
				    if(PChar->Account_Level==2)
				     {
                // //ShowNotice(CL_RED"COMMAND TRACER: Mod GM called Command \n" CL_RESET);	
				     CmdHandler.mgcall(PChar, (const int8*)data+7);
				     }
				     if(PChar->Account_Level==3)
				     {
                // //ShowNotice(CL_RED"COMMAND TRACER: Admin GM called Command \n" CL_RESET);
				     CmdHandler.agcall(PChar, (const int8*)data+7);
				     }
					 if(PChar->Account_Level==4)
				     {
                // //ShowNotice(CL_RED"COMMAND TRACER: Admin GM called Command \n" CL_RESET);
				     CmdHandler.procall(PChar, (const int8*)data+7);
				     }
				
				return;
	            
	           }
			else
			{
				ShowNotice("SOME OTHER COMAMND WAS CALLED UNKNOWN\n");

            return;
			}
			return;
	        }
		else
		{
        ShowNotice("SOME OTHER COMAMND WAS CALLED UNKNOWN\n");
        return;
		}
		
		}
		
		
		
	
	else if (RBUFB(data,(0x06)) == '#')
	{
		if(PChar->Account_Level==1 || PChar->Account_Level==2 || PChar->Account_Level==3 || PChar->Account_Level==4)
		{
            for (uint16 zone = 0; zone < 256; ++zone)
            {
            zoneutils::GetZone(zone)->PushPacket(
                PChar,
                CHAR_INZONE,
                new CChatMessagePacket(PChar, MESSAGE_SYSTEM_1, data+7));
            }
		}
			
          
		
	}
	else if (RBUFB(data,(0x06)) == '?')
	{
		int32 accid=0;
	    const int8* GetCharAccountID = "SELECT accid FROM chars WHERE charid = '%u';";
	    int32 ret = Sql_Query(SqlHandle,GetCharAccountID,PChar->id);
		string_t message =data+7;
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	       {
		    accid= (int32)Sql_GetIntData(SqlHandle,0);
	      const int8* fmtQuery = "INSERT INTO chat SET user_id = %u, message = '%s', count = 1 ;";

		  Sql_Query(SqlHandle,fmtQuery,accid, message.c_str());
			
			
		}
            for (uint16 zone = 0; zone < 256; ++zone)
            {
            zoneutils::GetZone(zone)->PushPacket(
                PChar,
                CHAR_INZONE,
                new CChatMessagePacket(PChar, MESSAGE_SHOUT, data+7));
            }
		
			PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_SAY, data+7));
          
		
	}
    else
    {
        if(jailutils::InPrison(PChar))
        {
            if(RBUFB(data,(0x04)) == MESSAGE_SAY)
            {
                PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_SAY, data+6));
				PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE_SELF, new CChatMessagePacket(PChar, MESSAGE_SAY, data+6));
            }
            else
            {
                PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
            }
        }
        else
        {
            switch(RBUFB(data,(0x04)))
            {
                case MESSAGE_SAY:
					{
						
						PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_SAY,     data+6)); 
					}
					break;
                case MESSAGE_EMOTION:	PChar->loc.zone->PushPacket(PChar, CHAR_INRANGE, new CChatMessagePacket(PChar, MESSAGE_EMOTION, data+6)); break;
                case MESSAGE_SHOUT:	
					{
						
						PChar->loc.zone->PushPacket(PChar, CHAR_INSHOUT, new CChatMessagePacket(PChar, MESSAGE_SHOUT,   data+6)); 
					}
					break;
                case MESSAGE_LINKSHELL:
                {
                    if (PChar->PLinkshell != NULL)
                    {
						
                        PChar->PLinkshell->PushPacket(PChar, new CChatMessagePacket(PChar, MESSAGE_LINKSHELL, data+6));
                    }
                }
                break;
				case MESSAGE_PARTY:
                {
                    if (PChar->PParty != NULL)
                    {
						if (PChar->PParty->m_PAlliance == NULL)
						{
							
							PChar->PParty->PushPacket(PChar, 0, new CChatMessagePacket(PChar, MESSAGE_PARTY, data+6));

						}else{ //alliance party chat
								for (uint8 i = 0; i < PChar->PParty->m_PAlliance->partyList.size(); ++i)
								{
									PChar->PParty->m_PAlliance->partyList.at(i)->PushPacket(PChar, 0, new CChatMessagePacket(PChar, MESSAGE_PARTY, data+6));
								}
								
								
							}
					}
                }
                break;
                case MESSAGE_YELL:
					{
						
						PChar->loc.zone->PushPacket(PChar, CHAR_INSHOUT, new CChatMessagePacket(PChar, MESSAGE_YELL,   data+6)); 
					}
					break;
            }
        }
	}
	}

	return;
}

/************************************************************************
*																		*
*  Текстовое общение между персонажами /tell							*
*																		*
************************************************************************/

void SmallPacket0x0B6(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
   ShowNotice("PCHAR SEND TELL SYSTEM CALLED\n");
	string_t RecipientName = data+5;
	string_t message = data+20;
	//ShowNotice(CL_RED"SENDING MESSAGE %s TO PLAYER %s FROM PLAYER %s\n" CL_RESET,message.c_str(),RecipientName.c_str(),PChar->GetName());
	if(jailutils::InPrison(PChar))
    {
		//ShowNotice(CL_GREEN"SENDING MESSAGE: TO SENDER JAIL\n" CL_RESET);
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
        return;
    }
	CCharEntity* PTargetChar = zoneutils::GetCharByName(data+5);
	if(PTargetChar != NULL )
	{
		//ShowNotice(CL_GREEN"SENDING MESSAGE IS NOT NULL OK TO SEND\n" CL_RESET);
		if (PTargetChar->nameflags.flags == FLAG_AWAY)
			{
				//ShowNotice(CL_GREEN"SENDING MESSAGE: RECEIVER HAS AWAY FLAG SET TELL SENDER\n" CL_RESET);
				PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 181));
				return;
			}
		if( PTargetChar->status != STATUS_DISAPPEAR)
		{
			//ShowNotice(CL_GREEN"SENDING MESSAGE: RECEIVER IS NOT ZONING SEND MESSAGE OK\n" CL_RESET);
			if(PTargetChar == PChar)
			{
				PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 125));
				return;
			}
			
		PTargetChar->pushPacket(new CChatMessagePacket(PChar, MESSAGE_TELL,data+20 ));
		return;
		}
		else
		{
			//ShowNotice(CL_GREEN"SENDING MESSAGE: RECEIVER IS ZONEING TELL SENDER\n" CL_RESET);
			PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 125));
			return;
		}
		return;
	}
	else
	{

		//ShowNotice(CL_GREEN"SENDING MESSAGE: DO A FULL SWEEP\n" CL_RESET);
			map_session_list_t::iterator it = map_session_list.begin();
			while(it != map_session_list.end())
			{
				map_session_data_t* map_session_data = it->second;
				CCharEntity* PCharInMog = map_session_data->PChar;
				if(PCharInMog!=NULL)
				{
					//ShowNotice(CL_GREEN"SENDING MESSAGE: PLAYER IS NOT NULL\n" CL_RESET);
					PTargetChar = PCharInMog;
					if(PTargetChar == PChar)
			          {
				      PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 125));
				      break;
			          }
                    PCharInMog->pushPacket(new CChatMessagePacket(PChar, MESSAGE_TELL,data+20 ));
					break;
				}
				else
				{
                //ShowNotice(CL_GREEN"SENDING MESSAGE: RECEIVER IS NOT ONLINE TELL SENDER\n" CL_RESET);
		        PChar->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 125));
				break;
				}
				++it;
			}
		
		
		return;
	}
		 

	return;
}

/************************************************************************
*                                                                       *
*  Меняем режим	получения опыта и усиливаем/ослабляем merits            *
*                                                                       *
************************************************************************/

void SmallPacket0x0BE(map_session_data_t* session, CCharEntity* PChar, int8* data)
{

    uint8 operation = RBUFB(data,0x05);

    switch(RBUFB(data,(0x04)))
    {
        case 2: // изменение mode
        {
			// TODO: you can switch mode anywhere except in besieged & under level restriction
		    if (Sql_Query(SqlHandle, "UPDATE char_exp SET mode = %u WHERE charid = %u", operation, PChar->id) != SQL_ERROR)
			{
			    PChar->MeritMode = operation;
			    PChar->pushPacket(new CMenuMeritPacket(PChar));
			}
        }
        break;
        case 3: // изменение merit
        {
			if (PChar->getZone() == 0) // все операции обрабатываются только в moghouse
			{
				MERIT_TYPE merit = (MERIT_TYPE)(RBUFW(data,(0x06)) << 1);

				if (PChar->PMeritPoints->IsMeritExist(merit)) // проверяем присланный персонажем id
				{
					switch (operation)
					{
						case 0: PChar->PMeritPoints->LowerMerit(merit); break;
						case 1: PChar->PMeritPoints->RaiseMerit(merit); break;
					}
					PChar->pushPacket(new CMenuMeritPacket(PChar));
					PChar->pushPacket(new CMeritPointsCategoriesPacket(PChar, merit));

					charutils::SaveCharExp(PChar, PChar->GetMJob());
					PChar->PMeritPoints->SaveMeritPoints(PChar->id, false);



					charutils::BuildingCharSkillsTable(PChar);
					charutils::CalculateStats(PChar);
					charutils::CheckValidEquipment(PChar);
					charutils::BuildingCharAbilityTable(PChar);
					charutils::BuildingCharTraitsTable(PChar);
					charutils::BuildingCharWeaponSkills(PChar);

					PChar->UpdateHealth();
					PChar->addHP(PChar->GetMaxHP());
					PChar->addMP(PChar->GetMaxMP());
					PChar->pushPacket(new CCharUpdatePacket(PChar));
					PChar->pushPacket(new CCharHealthPacket(PChar));
					PChar->pushPacket(new CCharStatsPacket(PChar));
					PChar->pushPacket(new CCharSkillsPacket(PChar));
					PChar->pushPacket(new CCharAbilitiesPacket(PChar));
                    PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
                    PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
					PChar->pushPacket(new CCharSyncPacket(PChar));
				}
            }
        }
        break;
    }
	return;
}

/************************************************************************
*                                                                       *
*  Создание жемчужины (Pearl) экипированной LinkShell                   *
*                                                                       *
************************************************************************/

void SmallPacket0x0C3(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    CItemLinkshell* PItemLinkshell = (CItemLinkshell*)PChar->getStorage(LOC_INVENTORY)->GetItem(PChar->equip[SLOT_LINK]);

    if (PItemLinkshell != NULL && PItemLinkshell->isType(ITEM_LINKSHELL))
    {
        CItemLinkshell* PItemLinkPearl = new CItemLinkshell(*PItemLinkshell);

		if (PItemLinkPearl->GetLSType() == LSTYPE_PEARLSACK ||
            PItemLinkPearl->GetLSType() == LSTYPE_LINKSHELL)
        {
			PItemLinkPearl->setID(515);
			PItemLinkPearl->setSubType(ITEM_UNLOCKED);

			charutils::AddItem(PChar, LOC_INVENTORY, PItemLinkPearl);
		}
    }
    return;
}

/************************************************************************
*                                                                       *
*  Создание и экипировка LinkShell                                      *
*                                                                       *
************************************************************************/

void SmallPacket0x0C4(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    uint8 SlotID = RBUFB(data,(0x06));
    uint8 action = RBUFB(data,(0x07));

    CItemLinkshell* PItemLinkshell = (CItemLinkshell*)PChar->getStorage(LOC_INVENTORY)->GetItem(SlotID);

    if (PItemLinkshell != NULL && PItemLinkshell->isType(ITEM_LINKSHELL))
    {
        if (PItemLinkshell->getID() == 512) // создание новой linkshell
        {
            uint32   LinkshellID    = 0;
            uint16   LinkshellColor = RBUFW(data,(0x04));
            string_t LinkshellName  = data+8;
            int8     DecodedName[21];
            int8     EncodedName[16];

            DecodeStringLinkshell(data+8, DecodedName);
            EncodeStringLinkshell(DecodedName, EncodedName);
            // TODO: проверить имя на необходимость добавления окончания строки

            if (LinkshellID = linkshell::RegisterNewLinkshell(DecodedName, LinkshellColor)) // здесь дейтсвительно присваивание
            {
	            const int8* Query = "UPDATE char_inventory SET signature = '%s', itemId = 513 WHERE charid = %u AND location = 0 AND slot = %u LIMIT 1";

		        if (Sql_Query(SqlHandle, Query, DecodedName, PChar->id, SlotID) != SQL_ERROR &&
                    Sql_AffectedRows(SqlHandle) != 0)
                {
                    PItemLinkshell->setID(513);
                    PItemLinkshell->SetLSID(LinkshellID);
                    PItemLinkshell->setSignature(EncodedName); //because apparently the format from the packet isn't right, and is missing terminators
                    PItemLinkshell->SetLSColor(LinkshellColor);

                    PChar->pushPacket(new CInventoryItemPacket(PItemLinkshell, LOC_INVENTORY, SlotID));
                }
            }
            else
            {
                PChar->pushPacket(new CMessageStandardPacket(112));
                //DE
                //20
                //1D
                return;
            }
        }
        else
        {
            switch (action)
            {
                case 0: // unequip linkshell
                {
                    linkshell::DelOnlineMember(PChar, PItemLinkshell);

                    PItemLinkshell->setSubType(ITEM_UNLOCKED);

                    PChar->equip[SLOT_LINK] = 0;
                    PChar->nameflags.flags &= ~FLAG_LINKSHELL;

                    PChar->pushPacket(new CInventoryAssignPacket(PItemLinkshell, INV_NORMAL));
                }
                break;
                case 1: // equip linkshell
                {
                    if (PItemLinkshell->GetLSID() == 0) // linkshell no exists, item is unusable
                    {
                        PChar->pushPacket(new CMessageSystemPacket(0,0,110));
                        return;
                    }
                    if (PChar->PLinkshell != NULL) // switching linkshell group
                    {
                        CItemLinkshell* POldItemLinkshell = (CItemLinkshell*)PChar->getStorage(LOC_INVENTORY)->GetItem(PChar->equip[SLOT_LINK]);

                        if (POldItemLinkshell != NULL && POldItemLinkshell->isType(ITEM_LINKSHELL))
                        {
                            linkshell::DelOnlineMember(PChar, POldItemLinkshell);

                            POldItemLinkshell->setSubType(ITEM_UNLOCKED);
                            PChar->pushPacket(new CInventoryAssignPacket(POldItemLinkshell, INV_NORMAL));
                        }
                    }
                    linkshell::AddOnlineMember(PChar, PItemLinkshell);

                    PItemLinkshell->setSubType(ITEM_LOCKED);

                    PChar->equip[SLOT_LINK] = SlotID;
                    PChar->nameflags.flags |= FLAG_LINKSHELL;

                    PChar->pushPacket(new CInventoryAssignPacket(PItemLinkshell, INV_LINKSHELL));
                }
                break;
            }
            charutils::SaveCharStats(PChar);
            charutils::SaveCharEquip(PChar);

            if (PChar->status == STATUS_NORMAL) PChar->status = STATUS_UPDATE;

            PChar->pushPacket(new CLinkshellEquipPacket(PChar));
            PChar->pushPacket(new CInventoryItemPacket(PItemLinkshell, LOC_INVENTORY, SlotID));
        }
        PChar->pushPacket(new CInventoryFinishPacket());
        PChar->pushPacket(new CCharUpdatePacket(PChar));
    }
	return;
}

/************************************************************************
*																		*
*  Отображаем членов группы на карте									*
*																		*
************************************************************************/

void SmallPacket0x0D2(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	//alliance
	if (PChar->PParty != NULL)
	{
		if (PChar->PParty->m_PAlliance != NULL)
		{
			for (uint8 a = 0; a < PChar->PParty->m_PAlliance->partyList.size(); ++a)
			{
				for (uint8 i = 0; i < PChar->PParty->m_PAlliance->partyList.at(a)->members.size(); ++i)
				{
					CCharEntity* PPartyMember = (CCharEntity*)PChar->PParty->m_PAlliance->partyList.at(a)->members.at(i);

					if (PPartyMember->getZone() == PChar->getZone())
					{
						PChar->pushPacket(new CPartyMapPacket(PPartyMember));
					}
				}
			}
		return;

		}else{  //normal party - no alliance
				for (int32 i = 0; i < PChar->PParty->members.size(); ++i)
				{
					CCharEntity* PPartyMember = (CCharEntity*)PChar->PParty->members.at(i);

					if (PPartyMember->getZone() == PChar->getZone())
					{
						PChar->pushPacket(new CPartyMapPacket(PPartyMember));
					}
				}
				return;
			 }
	}
	PChar->pushPacket(new CPartyMapPacket(PChar));
	return;
}

/************************************************************************
*																		*
*  Отправляем жалобу GM'y												*
*  help desk -> i want to report -> yes -> yes -> execute				*
*																		*
************************************************************************/

void SmallPacket0x0D3(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	return;
}

/************************************************************************
*																		*
*  Установка разных nameFlags: party, away, autogroup,					*
*  флаги в help desk и т.д.												*
*																		*
************************************************************************/

void SmallPacket0x0DC(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	switch(RBUFW(data,(0x04)))
	{
		case 0x0001:
			PChar->nameflags.flags ^= FLAG_INVITE;
			break;
		case 0x0002:
			if(RBUFB(data,(0x10)) == 1)
				PChar->nameflags.flags |=  FLAG_AWAY;
			if(RBUFB(data,(0x10)) == 2)
				PChar->nameflags.flags &= ~FLAG_AWAY;
			break;
		case 0x0004:
			PChar->nameflags.flags ^= FLAG_ANON;
			break;
		case 0x4000:
			if(RBUFB(data,(0x10)) == 1)
                PChar->m_hasAutoTarget = false;
			if(RBUFB(data,(0x10)) == 2)
                PChar->m_hasAutoTarget = true;
			break;
		case 0x8000:
			//if(RBUFB(data,(0x10)) == 1)	// autogroup on
			//if(RBUFB(data,(0x10)) == 2)	// autogroup off
			break;
	}
    charutils::SaveCharStats(PChar);

	PChar->status = STATUS_UPDATE;
	PChar->pushPacket(new CMenuConfigPacket(PChar));
	PChar->pushPacket(new CCharUpdatePacket(PChar));
	return;
}

/************************************************************************
*																		*
*  Устанавливаем предпочтительный язык общения							*
*																		*
************************************************************************/

void SmallPacket0x0DB(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->search.language = RBUFB(data,(0x24));
	return;
}

/************************************************************************
*																		*
*  Проверяем монстров или персонажей									*
*																		*
*  170 - <target> seems It seems to have high evasion and defense.		*
*  171 - <target> seems It seems to have high evasion.					*
*  172 - <target> seems It seems to have high evasion but low defense.	*
*  173 - <target> seems It seems to have high defense.					*
*  174 - <target> seems													*
*  175 - <target> seems It seems to have low defense.					*
*  176 - <target> seems It seems to have low evasion but high defense.	*
*  177 - <target> seems It seems to have low evasion.					*
*  178 - <target> seems It seems to have low evasion and defense.		*
*																		*
*																		*
************************************************************************/

void SmallPacket0x0DD(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint32 id     = RBUFL(data,(0x04));
	uint16 targid = RBUFW(data,(0x08));

    if(jailutils::InPrison(PChar))
    {
        PChar->pushPacket(new CMessageBasicPacket(PChar, PChar, 0, 0, 316));
        return;
    }

	CBaseEntity* PEntity = PChar->loc.zone->GetEntity(targid, TYPE_MOB | TYPE_PC);

	if (PEntity == NULL || PEntity->id != id)
		return;

	switch (PEntity->objtype)
	{
		case TYPE_MOB:
		{
			CMobEntity* PTarget = (CMobEntity*)PEntity;

            if (PTarget->m_Type & MOBTYPE_NOTORIOUS)
			{
			    PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, 0, 249));
			}
            else
            {
				uint32 baseExp  = charutils::GetRealExp(PChar->GetMLevel(),PTarget->GetMLevel());
				uint16 charAcc  = PChar->ACC(SLOT_MAIN, (uint8)0);
				uint16 charAtt  = PChar->ATT();
				uint16 mobEva   = PTarget->EVA();
				uint16 mobDef   = PTarget->DEF();

                uint8 MessageValue = 0;

                // TODO: Логическая ошибка использовать exp для определения сложности боя
                // необходимы условия, основанные на разницах уровня
                // не стоит забывать, что эта разница увеличивается с ростом уровня персонажа
                // NOTE: message 0x41: Incredibly Easy Prey

				if      (baseExp >= 400) MessageValue = 0x47;
				else if (baseExp >= 240) MessageValue = 0x46;
 				else if (baseExp >= 120) MessageValue = 0x45;
 				else if (baseExp == 100) MessageValue = 0x44;
 				else if (baseExp >=  75) MessageValue = 0x43;
 				else if (baseExp >=  15) MessageValue = 0x42;
 				else if (baseExp ==   0) MessageValue = 0x40;
                if      (mobDef > charAtt && (mobEva - 30) > charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 170));//high eva high def
                else if ((mobDef * 1.25) > charAtt && mobDef <= charAtt && (mobEva - 30) > charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 171));//high eva
				else if ((mobDef * 1.25) <= charAtt && (mobEva - 30) > charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 172));//high eva low def
				else if (mobDef > charAtt && (mobEva - 30) <= charAcc && (mobEva + 10) > charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 173));//high def
				else if ((mobDef * 1.25) <= charAtt && (mobEva - 30) <= charAcc && (mobEva + 10) > charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 175));//low def
				else if (mobDef > charAtt && (mobEva + 10) <= charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 176));//low eva high def
				else if ((mobDef * 1.25) > charAtt && mobDef <= charAtt && (mobEva + 10) <= charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 177));//low eva
				else if ((mobDef * 1.25) <= charAtt && (mobEva + 10) <= charAcc)
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 178));//low eva low def
				else
					PChar->pushPacket(new CMessageBasicPacket(PChar, PTarget, 0, MessageValue, 174));//broke even
            }
		}
		break;
		case TYPE_PC:
		{
			CCharEntity* PTarget = (CCharEntity*)PEntity;

			PTarget->pushPacket(new CMessageStandardPacket(PChar, 0, 0, 89));

			PChar->pushPacket(new CBazaarMessagePacket(PTarget));
            PChar->pushPacket(new CCheckPacket(PChar, PTarget));
		}
		break;
	}
	return;
}

/************************************************************************
*																		*
*  Устанавливаем сообщене bazaar										*
*  Буфера в 256 байт вполне достаточно для преобразования 120 байт		*
*																		*
************************************************************************/

void SmallPacket0x0DE(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->bazaar.message.clear();
	PChar->bazaar.message.insert(0,data+4);

	int8 message[256];
	Sql_EscapeString(SqlHandle,message,data+4);

	Sql_Query(SqlHandle, "UPDATE char_stats SET bazaar_message = '%s' WHERE charid = %u;", message, PChar->id);
	return;
}

/************************************************************************
*																		*
*  Комментарий, отображаемый в списке поиска							*
*  Не уверен насчет необходимости удаления пробелов в конце строки,		*
*  об их назначении мы узнаем лишь после реализации поиска				*
*																		*
************************************************************************/

void SmallPacket0x0E0(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->search.message.clear();
	PChar->search.message.insert(0,data+4);

	PChar->search.messagetype = RBUFB(data,(0xA4));

    // в ответ на этот пакет не нужно посылать что-либо клиенту
	// эта информация используется лишь при поиске персонажа

	//                s   a   l   u   t
	//e0  4c  c2  00  73  61  6c  75  74  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  20
	//20  20  20  20  20  20  20  20  20  20  20  20  20  20  20  00
	//00  00  00  00  2f  15  4c  4b  57  49  4e  08  3f  00  00  00
	//ff  00  00  00  11  00  00  00

	// сообщение максимум 120, 3 строки по 40 символов, идущие подряд, начиная с 5-го байта
	// тип сообщения - 4й байт с конца

	//EXP party
	    //0x11 - seek party
		//0x12 - find member
		//0x13 - other
	//Mission
		//0x21 - seek party
		//0x22 - find member
		//0x23 - other
	//Quest
		//0x31 - seek party
		//0x32 - find member
		//0x33 - other
	//Battlefield
		//0x41 - seek party
		//0x42 - find member
		//0x43 - other
	//Item
		//0x51 - Want to Sell
		//0x52 - Want to Buy
		//0x53 - Other
	//Synthesis
		//0x61 - Need Made
		//0x62 - Can Make
		//0x63 - Other
    //Others
		//0x73 - others
	return;
}

/************************************************************************
*                                                                       *
*  Отправляем персонажу приветственное сообщение LinkShell (/lsmes)     *
*                                                                       *
************************************************************************/

void SmallPacket0x0E1(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    if (PChar->PLinkshell != NULL)
    {
        PChar->pushPacket(new CLinkshellMessagePacket(PChar->PLinkshell));
    }
    return;
}

/************************************************************************
*                                                                       *
*  Обновление преветственного cообщения LinkShell                       *
*                                                                       *
************************************************************************/

void SmallPacket0x0E2(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    CItemLinkshell* PItemLinkshell = (CItemLinkshell*)PChar->getStorage(LOC_INVENTORY)->GetItem(PChar->equip[SLOT_LINK]);

    if (PChar->PLinkshell != NULL && (PItemLinkshell != NULL && PItemLinkshell->isType(ITEM_LINKSHELL)))
    {
        switch (RBUFB(data,(0x04)) & 0xF0) // назначение первых бит пока неизвестно
        {
            case 0x20: // устанавливаем права на изменение сообщения
            {
                // TODO: ....
            }
            break;
            case 0x40: // изменяем сообщение
            {
                if (PItemLinkshell->GetLSType() == LSTYPE_LINKSHELL ||
                    PItemLinkshell->GetLSType() == LSTYPE_PEARLSACK)
                {
                    string_t Message = data+12;
                    uint32   MessageTime = time(NULL);
                    int8 sqlMessage[256];
                    Sql_EscapeString(SqlHandle,sqlMessage,Message.c_str());

                    const int8* Query = "UPDATE linkshells SET poster = '%s', message = '%s', messagetime = %u WHERE linkshellid = %u LIMIT 1";

                    if (Sql_Query(SqlHandle, Query, PChar->GetName(), sqlMessage, MessageTime, PChar->PLinkshell->getID()) != SQL_ERROR &&
                        Sql_AffectedRows(SqlHandle) != 0)
                    {
                        PChar->PLinkshell->setPoster((int8*)PChar->GetName());
                        PChar->PLinkshell->setMessage((int8*)Message.c_str());
                        PChar->PLinkshell->setMessageTime(MessageTime);

                        PChar->PLinkshell->PushPacket(NULL, new CLinkshellMessagePacket(PChar->PLinkshell));
                        return;
                    }
                }
            }
            break;
        }
    }
    PChar->pushPacket(new CLinkshellMessagePacket(NULL)); // you are not authorized to this action
    return;
}

/************************************************************************
*																		*
*  Запрос на выход из игры. Shutdown и Logout отправляют одинаковые		*
*  запросы. В MogHouse персонаж выходит сразу, без задержки в 30 секунд	*
*																		*
*  ExitType: 1 - logout; 3 - shutdown									*
*																		*
************************************************************************/

void SmallPacket0x0E7(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->status != STATUS_NORMAL)
		return;

	if (PChar->getZone() == 0 ||
		PChar->nameflags.flags & FLAG_GM)
	{
		PChar->status = STATUS_SHUTDOWN;
		PChar->pushPacket(new CServerIPPacket(PChar,1));
	}
	else if (PChar->animation == ANIMATION_NONE)
	{
		uint8 ExitType = (RBUFB(data,(0x06)) == 1 ? 7 : 35);

		if (PChar->PPet == NULL ||
		   (PChar->PPet->m_EcoSystem != SYSTEM_AVATAR &&
			PChar->PPet->m_EcoSystem != SYSTEM_ELEMENTAL))
		{
			PChar->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_HEALING,0,0,10,0));
		}
		PChar->status = STATUS_UPDATE;
		PChar->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_LEAVEGAME,0,ExitType,5,0));
	}
	else if (PChar->animation == ANIMATION_HEALING)
	{
		if (PChar->StatusEffectContainer->HasStatusEffect(EFFECT_LEAVEGAME))
		{
			PChar->status = STATUS_UPDATE;
			PChar->StatusEffectContainer->DelStatusEffect(EFFECT_HEALING);
		} else {
			uint8 ExitType = (RBUFB(data,(0x06)) == 1 ? 7 : 35);

			PChar->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_LEAVEGAME,0,ExitType,5,0));
		}
	}
	return;
}

/************************************************************************
*																		*
*  Отдых персонажа (восстановление жизней, выход из игры, прерывание	*
*  чтения заклинаний, повышенная агрессия монстров и т.д.)				*
*																		*
************************************************************************/

void SmallPacket0x0E8(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->status != STATUS_NORMAL)
		return;

	switch (PChar->animation)
	{
		case ANIMATION_NONE:
		{
			// cannot rest while stunned, slept etc
			if(PChar->StatusEffectContainer->HasPreventActionEffect())
			{
				return;
			}

			if (PChar->PPet == NULL ||
			   (PChar->PPet->m_EcoSystem != SYSTEM_AVATAR &&
				PChar->PPet->m_EcoSystem != SYSTEM_ELEMENTAL))
			{
				switch (PChar->PBattleAI->GetCurrentAction())
				{
					case ACTION_ITEM_USING:		PChar->PBattleAI->SetCurrentAction(ACTION_ITEM_INTERRUPT);	break;
					case ACTION_MAGIC_CASTING:	PChar->PBattleAI->SetCurrentAction(ACTION_MAGIC_INTERRUPT);	break;
				}
				PChar->status = STATUS_UPDATE;
				PChar->PBattleAI->CheckCurrentAction(gettick());
				PChar->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_HEALING,0,0,10,0));
				return;
			}
			PChar->pushPacket(new CMessageBasicPacket(PChar,PChar,0,0,345));
		}
		break;
		case ANIMATION_HEALING:
		{
			PChar->status = STATUS_UPDATE;
			PChar->StatusEffectContainer->DelStatusEffect(EFFECT_HEALING);
		}
		break;
	}
	return;
}

/************************************************************************
*																		*
*  Команда /sit (персонаж садится и наслаждается окружением)		 	*
*																		*
************************************************************************/

void SmallPacket0x0EA(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->status != STATUS_NORMAL)
		return;

	PChar->status = STATUS_UPDATE;
	PChar->animation = (PChar->animation == ANIMATION_SIT ? ANIMATION_NONE : ANIMATION_SIT);
	PChar->pushPacket(new CCharUpdatePacket(PChar));
	return;
}

/************************************************************************
*																		*
*  Удаление статус эффекта щелчком по его иконке						*
*																		*
************************************************************************/

void SmallPacket0x0F1(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 IconID = RBUFW(data,(0x04));

    if (IconID != 0) // удаляем только видимые эффекты
    {
	    PChar->StatusEffectContainer->DelStatusEffectsByIcon(IconID);
    }
	return;
}

/************************************************************************
*																		*
*  Клиент сообщает серверу, что входит в подобласть зоны				*
*  (в простонародье - boundary). Во избежание всяких казусов мы должны	*
*  сразу сохранять эту переменную.										*
*																		*
************************************************************************/

void SmallPacket0x0F2(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->loc.boundary = RBUFW(data,(0x06));

	charutils::SaveCharPosition(PChar);
	return;
}

/************************************************************************
*																		*
*  Сканирование местности (Wide Scan)							 		*
*																		*
************************************************************************/

void SmallPacket0x0F4(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->loc.zone->WideScan(PChar,PChar->getMod(MOD_WIDESCAN));
	return;
}

/************************************************************************
*																		*
*  Следим за выбранной целью в Wide Scan						 		*
*																		*
************************************************************************/

void SmallPacket0x0F5(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 TargID = RBUFW(data,(0x04));

	PChar->PWideScanTarget = PChar->loc.zone->GetEntity(TargID, TYPE_MOB | TYPE_NPC);
	return;
}

/************************************************************************
*																		*
*  Отмена слежения за целью, выбранной в Wide Scan				 		*
*																		*
************************************************************************/

void SmallPacket0x0F6(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	PChar->PWideScanTarget = NULL;
	return;
}

/************************************************************************
*																		*
*  Установка мебели в MogHouse									 		*
*																		*
************************************************************************/

void SmallPacket0x0FA(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 ItemID = RBUFW(data,(0x04));

	if (ItemID == 0)
	{
		// выход из режима установки мебели
		// здесь мы считаем ауру и добавляем необходимый keyitem
		return;
	}

	uint8  slotID	= RBUFB(data,(0x06));
	uint8  col		= RBUFB(data,(0x07));
	uint8  level	= RBUFB(data,(0x08));
	uint8  row		= RBUFB(data,(0x09));
	uint8  rotation	= RBUFB(data,(0x0A));

	CItemFurnishing* PItem = (CItemFurnishing*)PChar->getStorage(LOC_MOGSAFE)->GetItem(slotID);

	if (PItem != NULL &&
		PItem->getID() == ItemID &&
		PItem->isType(ITEM_FURNISHING))
	{
		if (PItem->getFlag() & ITEM_FLAG_WALLHANDING)
		{
			rotation = (col >= 2 ? 3 : 1);
		}
        const int8* Query =
            "UPDATE char_inventory "
            "SET "
              "locked = 1,"
              "col = %u,"
              "row = %u,"
              "level = %u,"
              "rotation = %u "
            "WHERE location = 1 AND slot = %u AND charid = %u";

        if (Sql_Query(SqlHandle, Query, col, row, level, rotation, slotID, PChar->id) != SQL_ERROR && Sql_AffectedRows(SqlHandle) != 0)
		{
		    PItem->setCol(col);
		    PItem->setRow(row);
		    PItem->setLevel(level);
		    PItem->setRotation(rotation);

		    PItem->setSubType(ITEM_LOCKED);

            PChar->getStorage(LOC_STORAGE)->AddBuff(PItem->getStorage());

            PChar->pushPacket(new CInventorySizePacket(PChar));
		}
		PChar->pushPacket(new CInventoryItemPacket(PItem, LOC_MOGSAFE, slotID));
		PChar->pushPacket(new CInventoryFinishPacket());
	}
	return;
}

/************************************************************************
*																		*
*  Удаление мебели в MogHouse									 		*
*																		*
************************************************************************/

void SmallPacket0x0FB(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint16 ItemID = RBUFW(data,(0x04));

	if (ItemID == 0)
	{
		return;
	}

    uint8  slotID = RBUFB(data,(0x06));

    CItemContainer* PItemContainer = PChar->getStorage(LOC_MOGSAFE);

	CItemFurnishing* PItem = (CItemFurnishing*)PItemContainer->GetItem(slotID);

	if (PItem != NULL &&
		PItem->getID() == ItemID &&
		PItem->isType(ITEM_FURNISHING))
	{
        // TODO: удаление мебели может никак не повлиять на размер хранилища, если сумма Storage превышала 80 ячеек

        PItemContainer = PChar->getStorage(LOC_STORAGE);

        uint8 RemovedSize = PItemContainer->GetSize() - dsp_min(PItemContainer->GetSize(), PItemContainer->GetBuff() - PItem->getStorage());

        if (PItemContainer->GetFreeSlotsCount() >= RemovedSize)
        {
            const int8* Query =
                "UPDATE char_inventory "
                "SET "
                  "locked = 0,"
                  "col = 0,"
                  "row = 0,"
                  "level = 0,"
                  "rotation = 0 "
                "WHERE location = 1 AND slot = %u AND charid = %u";

            if (Sql_Query(SqlHandle, Query, slotID, PChar->id) != SQL_ERROR && Sql_AffectedRows(SqlHandle) != 0)
		    {
		        PItem->setCol(0);
		        PItem->setRow(0);
		        PItem->setLevel(0);
		        PItem->setRotation(0);

                PItem->setSubType(ITEM_UNLOCKED);

                // пробегаться по предметам нужно лишь в случае, если новый размер контейнера изменится

                uint8 NewSize = PItemContainer->GetSize() - RemovedSize;

                for (uint8 SlotID = PItemContainer->GetSize(); SlotID > NewSize; --SlotID)
                {
                    if (PItemContainer->GetItem(SlotID) != NULL)
                    {
                        charutils::MoveItem(PChar, LOC_STORAGE, SlotID, ERROR_SLOTID);
                    }
                }
                PChar->getStorage(LOC_STORAGE)->AddBuff(-(int8)PItem->getStorage());

                PChar->pushPacket(new CInventorySizePacket(PChar));
            }
            PChar->pushPacket(new CInventoryItemPacket(PItem, LOC_MOGSAFE, PItem->getSlotID()));
		    PChar->pushPacket(new CInventoryFinishPacket());
        }
        else
        {
            ShowError(CL_RED"SmallPacket0x0FB: furnishing can't be removed\n" CL_RESET);
        }
	}
	return;
}

/************************************************************************
*																		*
*  Смена профессии. Разрешена только в зоне с флагом MISC_MOGMENU.		*
*  При смене профессии с персонажа снимаются все положительные эффекты	*
*																		*
************************************************************************/

void SmallPacket0x100(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->loc.zone->CanUseMisc(MISC_MOGMENU))
	{
		uint8 mjob = RBUFB(data,(0x04));
		uint8 sjob = RBUFB(data,(0x05));

		if ((mjob > 0x00) && (mjob < MAX_JOBTYPE))
		{
            JOBTYPE prevjob = PChar->GetMJob();
			PChar->resetPetZoningInfo();

			PChar->SetMJob(mjob);
			PChar->SetMLevel(PChar->jobs.job[PChar->GetMJob()]);
			PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

		    // If removing RemoveAllEquipment, please add a charutils::CheckUnarmedItem(PChar) if main hand is empty.
		    charutils::RemoveAllEquipment(PChar); // TODO: разобраться, зачем

            if (mjob == JOB_BLU)
                blueutils::LoadSetSpells(PChar);
            else if (prevjob == JOB_BLU)
                blueutils::UnequipAllBlueSpells(PChar);
		}
		if ((sjob > 0x00) && (sjob < MAX_JOBTYPE))
		{
			JOBTYPE prevsjob = PChar->GetSJob();

			PChar->SetSJob(sjob);
			PChar->SetSLevel(PChar->jobs.job[PChar->GetSJob()]);

			charutils::CheckEquipLogic(PChar, SCRIPT_CHANGESJOB, prevsjob);
            if (sjob == JOB_BLU)
                blueutils::LoadSetSpells(PChar);
            else if (prevsjob == JOB_BLU)
                blueutils::UnequipAllBlueSpells(PChar);

            uint16 subType = PChar->m_Weapons[SLOT_SUB]->getDmgType();

			if (subType > 0 && subType < 4)
            {
                charutils::UnequipItem(PChar, SLOT_SUB);
            }

		}

		charutils::BuildingCharSkillsTable(PChar);
		charutils::CalculateStats(PChar);
        charutils::BuildingCharTraitsTable(PChar);
        PChar->PRecastContainer->ResetAbilities();
		charutils::BuildingCharAbilityTable(PChar);
        charutils::BuildingCharWeaponSkills(PChar);

        PChar->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DISPELABLE);

        PChar->UpdateHealth();

        PChar->health.hp = PChar->GetMaxHP();
        PChar->health.mp = PChar->GetMaxMP();

        charutils::SaveCharStats(PChar);

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
	}
	return;
}

/************************************************************************
*																		*
*  Устанавливаем активные заклинания синего мага, модифицируем автомат	*
*																		*
************************************************************************/

void SmallPacket0x102(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	if (PChar->GetMJob() == JOB_BLU || PChar->GetSJob() == JOB_BLU) {
		// This may be a request to add or remove set spells, so lets check.
		uint8 job = RBUFB(data,(0x08));
		if (job == JOB_BLU) {
			uint8 spellToAdd = RBUFB(data,(0x04)); // this is non-zero if client wants to add.
			uint8 spellInQuestion = 0;
			uint8 spellIndex = -1;

			if (spellToAdd == 0x00) {
				for (uint8 i = 0x0C; i <= 0x1F; i++) {
					if ( RBUFB(data,i) > 0 ) {
						spellInQuestion = RBUFB(data,i);
						spellIndex = i - 0x0C;
						CBlueSpell* spell = (CBlueSpell*)spell::GetSpell(spellInQuestion + 0x200); // the spells in this packet are offsetted by 0x200 from their spell IDs.

						if (spell != NULL) {
							blueutils::SetBlueSpell(PChar, spell, spellIndex, (spellToAdd > 0));
						}
						else {
							ShowDebug("Cannot resolve spell id \n");
						}
					}
				}
                charutils::BuildingCharTraitsTable(PChar);
			    PChar->status = STATUS_UPDATE;
			    PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
			    PChar->pushPacket(new CCharJobExtraPacket(PChar, false));
			    PChar->pushPacket(new CCharStatsPacket(PChar));
			    charutils::CalculateStats(PChar);
			    PChar->UpdateHealth();
			    PChar->pushPacket(new CCharHealthPacket(PChar));
			}
			else {
				// loop all 20 slots and find which index they are playing with
				for (uint8 i = 0x0C; i <= 0x1F; i++) {
					if ( RBUFB(data,i) > 0 ) {
						spellInQuestion = RBUFB(data,i);
						spellIndex = i - 0x0C;
						break;
					}
				}

				if (spellIndex != -1 && spellInQuestion != 0) {
					CBlueSpell* spell = (CBlueSpell*)spell::GetSpell(spellInQuestion + 0x200); // the spells in this packet are offsetted by 0x200 from their spell IDs.

					if (spell != NULL) {
						blueutils::SetBlueSpell(PChar, spell, spellIndex, (spellToAdd > 0));
                        charutils::BuildingCharTraitsTable(PChar);
			            PChar->status = STATUS_UPDATE;
			            PChar->pushPacket(new CCharJobExtraPacket(PChar, true));
			            PChar->pushPacket(new CCharJobExtraPacket(PChar, false));
			            PChar->pushPacket(new CCharStatsPacket(PChar));
			            charutils::CalculateStats(PChar);
			            PChar->UpdateHealth();
			            PChar->pushPacket(new CCharHealthPacket(PChar));
					}
					else {
						ShowDebug("Cannot resolve spell id \n");
					}
				}
				else {
					ShowDebug("No match found. \n");
				}
			}
		}
		else {
			ShowDebug("Got 0x102 but it's not for JOB_BLU.");
		}
	}

	/* Unsurprisingly, this packet is also used for automaton attachments
	* 0x04 is non-zero if the client is adding an attachment
	* 0x08 is JOB_PUP
	* 0x0C is the head slot
	* 0x0D is the frame slot
	* 0x0E is the first attachment slot
	* 0x19 is the last attachment slot
	* multiple attachment slots may be set with Remove All
	*/

	return;
}

/************************************************************************
*																		*
*  Closing the "View wares", sending a message to the bazaar			*
*  that you have left the shop	                                        *
*																		*
************************************************************************/

void SmallPacket0x104(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->BazaarID.targid, TYPE_PC);

    if (PTarget != NULL && PTarget->id == PChar->BazaarID.id)
	{
        // надеюсь, что персонаж прописался в массиве однажны, но на всякий случай пробегаем по всему
        for (uint32 i = 0; i < PTarget->BazaarCustomers.size(); ++i)
		{
            if (PTarget->BazaarCustomers[i].id == PChar->targid)
            {
                PTarget->BazaarCustomers.erase(PTarget->BazaarCustomers.begin() + i--);
			}
		}
	    PTarget->pushPacket(new CBazaarCheckPacket(PChar, BAZAAR_LEAVE));
	}
    PChar->BazaarID.clean();
	return;
}

/************************************************************************
*                                                                       *
*  Clicking "View wares", opening the bazaar for browsing               *
*                                                                       *
************************************************************************/

void SmallPacket0x105(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    DSP_DEBUG_BREAK_IF(PChar->BazaarID.id != 0);
    DSP_DEBUG_BREAK_IF(PChar->BazaarID.targid != 0);

	uint32 charid = RBUFL(data,(0x04));

    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->m_TargID, TYPE_PC);

    if (PTarget != NULL && PTarget->id == charid && (PTarget->nameflags.flags & FLAG_BAZAAR))
	{
        PChar->BazaarID.id = PTarget->id;
        PChar->BazaarID.targid = PTarget->targid;

        EntityID_t EntityID = { PChar->id, PChar->targid };

		PTarget->pushPacket(new CBazaarCheckPacket(PChar, BAZAAR_ENTER));
        PTarget->BazaarCustomers.push_back(EntityID);

		CItemContainer* PBazaar = PTarget->getStorage(LOC_INVENTORY);

		for(uint8 SlotID = 1; SlotID <= PBazaar->GetSize(); ++SlotID)
		{
			CItem* PItem = PBazaar->GetItem(SlotID);

			if ((PItem != NULL) && (PItem->getCharPrice() != 0))
			{
                PChar->pushPacket(new CBazaarItemPacket(PItem, SlotID, PChar->loc.zone->GetTax()));
			}
		}
	}
	return;
}

/************************************************************************
*                                                                       *
*  Purchasing an item from a bazaar                                     *
*                                                                       *
************************************************************************/

void SmallPacket0x106(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8 Quantity = RBUFB(data,0x08);
	uint8 SlotID   = RBUFB(data,0x04);

    CCharEntity* PTarget = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->BazaarID.targid, TYPE_PC);

    if (PTarget == NULL || PTarget->id != PChar->BazaarID.id)
		return;

    CItemContainer* PBazaar = PTarget->getStorage(LOC_INVENTORY);
	CItemContainer* PBuyerInventory = PChar->getStorage(LOC_INVENTORY);

    if (PChar->id == PTarget->id || PBuyerInventory->GetFreeSlotsCount() == 0)
    {
        PChar->pushPacket(new CBazaarPurchasePacket(PTarget, false));
        return;
    }

	CItem* PBazaarItem = PBazaar->GetItem(SlotID);

    if ((PBazaarItem != NULL) && (PBazaarItem->getCharPrice() != 0) && (PBazaarItem->getQuantity() >= Quantity))
    {
        CItem* PItem = itemutils::GetItem(PBazaarItem);

        PItem->setCharPrice(0);
        PItem->setQuantity(Quantity);
        PItem->setSubType(ITEM_UNLOCKED);

        if (charutils::AddItem(PChar, LOC_INVENTORY, PItem) == ERROR_SLOTID)
            return;

        // TODO: мне так лень делать проверки на текущее количество gil, на первое время понадеемся на клиента

        uint32 Price1 = (PBazaarItem->getCharPrice() * Quantity);               // цена
        uint32 Price2 = (PChar->loc.zone->GetTax() * Price1) / 10000 + Price1;  // цена + налог

        charutils::UpdateItem(PChar,   LOC_INVENTORY, 0, -Price2);
        charutils::UpdateItem(PTarget, LOC_INVENTORY, 0,  Price1);

        PChar->pushPacket(new CBazaarPurchasePacket(PTarget, true));

        PTarget->pushPacket(new CBazaarConfirmationPacket(PChar, SlotID, Quantity));

        charutils::UpdateItem(PTarget, LOC_INVENTORY, SlotID, -Quantity);

        PTarget->pushPacket(new CInventoryItemPacket(PBazaar->GetItem(SlotID), LOC_INVENTORY, SlotID));
		PTarget->pushPacket(new CInventoryFinishPacket());

        bool BazaarIsEmpty = true;

        for(uint8 BazaarSlotID = 1; BazaarSlotID <= PBazaar->GetSize(); ++BazaarSlotID)
	    {
		    PItem = PBazaar->GetItem(BazaarSlotID);

		    if ((PItem != NULL) && (PItem->getCharPrice() != 0))
		    {
			    BazaarIsEmpty = false;
                break;
		    }
	    }
        for (uint32 i = 0; i < PTarget->BazaarCustomers.size(); ++i)
        {
            CCharEntity* PCustomer = (CCharEntity*)PTarget->loc.zone->GetEntity(PTarget->BazaarCustomers[i].targid, TYPE_PC);

            if (PCustomer != NULL && PCustomer->id == PTarget->BazaarCustomers[i].id)
            {
                if (PCustomer->id != PChar->id)
                {
                    PCustomer->pushPacket(new CBazaarConfirmationPacket(PChar, SlotID, Quantity));
                }
                PCustomer->pushPacket(new CBazaarItemPacket(PBazaar->GetItem(SlotID), SlotID, PChar->loc.zone->GetTax()));

                if (BazaarIsEmpty)
                {
                    PCustomer->pushPacket(new CBazaarClosePacket(PTarget));
                }
            }
        }
        if (BazaarIsEmpty)
        {
            PTarget->status = STATUS_UPDATE;
		    PTarget->nameflags.flags &= ~FLAG_BAZAAR;
		    PTarget->pushPacket(new CCharUpdatePacket(PTarget));
	    }
        return;
    }
    PChar->pushPacket(new CBazaarPurchasePacket(PTarget, false));
    return;
}

/************************************************************************
*																		*
*  Выходим из режима установки цен bazaar						 		*
*																		*
************************************************************************/

void SmallPacket0x109(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	CItemContainer* PStorage = PChar->getStorage(LOC_INVENTORY);

	for (uint8 slotID = 1; slotID <= PStorage->GetSize(); ++slotID)
	{
		CItem* PItem = PStorage->GetItem(slotID);

		if ((PItem != NULL) && (PItem->getCharPrice() != 0))
		{
            PChar->status = STATUS_UPDATE;
			PChar->nameflags.flags |= FLAG_BAZAAR;
			PChar->pushPacket(new CCharUpdatePacket(PChar));
			return;
		}
	}
	return;
}

/************************************************************************
*																		*
*  Устанавливаем цены bazaar на предметы							 	*
*																		*
************************************************************************/

void SmallPacket0x10A(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
	uint8  slotID = RBUFB(data,(0x04));
	uint32 price  = RBUFL(data,(0x08));

	CItem* PItem = PChar->getStorage(LOC_INVENTORY)->GetItem(slotID);

	if ((PItem != NULL) && !(PItem->getFlag() & ITEM_FLAG_EX))
	{
		Sql_Query(SqlHandle,"UPDATE char_inventory SET bazaar = %u WHERE charid = %u AND location = 0 AND slot = %u;", price, PChar->id, slotID);

		PItem->setCharPrice(price);
		PItem->setSubType((price == 0 ? ITEM_UNLOCKED : ITEM_LOCKED));

		PChar->pushPacket(new CInventoryItemPacket(PItem, LOC_INVENTORY, slotID));
		PChar->pushPacket(new CInventoryFinishPacket());
	}
	return;
}

/************************************************************************
*																		*
*  Opening "Set Prices" in bazaar-menu, closing the bazaar		 		*
*																		*
************************************************************************/

void SmallPacket0x10B(map_session_data_t* session, CCharEntity* PChar, int8* data)
{
    for (uint32 i = 0; i < PChar->BazaarCustomers.size(); ++i)
    {
        CCharEntity* PCustomer = (CCharEntity*)PChar->loc.zone->GetEntity(PChar->BazaarCustomers[i].targid, TYPE_PC);

        if (PCustomer != NULL && PCustomer->id == PChar->BazaarCustomers[i].id)
        {
			PCustomer->pushPacket(new CBazaarClosePacket(PChar));
		}
	}
    PChar->BazaarCustomers.clear();

    PChar->status = STATUS_UPDATE;
	PChar->nameflags.flags &= ~FLAG_BAZAAR;
	PChar->pushPacket(new CCharUpdatePacket(PChar));
	return;
}

/************************************************************************
*																		*
*  Инициализация массива процедур                   					*
*																		*
************************************************************************/

void PacketParserInitialize()
{
    for (uint16 i = 0; i < 512; ++i)
    {
        PacketSize[i]   = 0;
        PacketParser[i] = &SmallPacket0x000;
    }
    PacketSize[0x00A] = 0x2E; PacketParser[0x00A] = &SmallPacket0x00A;
    PacketSize[0x00C] = 0x00; PacketParser[0x00C] = &SmallPacket0x00C;
    PacketSize[0x00D] = 0x04; PacketParser[0x00D] = &SmallPacket0x00D;
    PacketSize[0x00F] = 0x00; PacketParser[0x00F] = &SmallPacket0x00F;
    PacketSize[0x011] = 0x00; PacketParser[0x011] = &SmallPacket0x011;
    PacketSize[0x015] = 0x10; PacketParser[0x015] = &SmallPacket0x015;
    PacketSize[0x016] = 0x04; PacketParser[0x016] = &SmallPacket0x016;
    PacketSize[0x017] = 0x00; PacketParser[0x017] = &SmallPacket0x017;
    PacketSize[0x01A] = 0x08; PacketParser[0x01A] = &SmallPacket0x01A;
    PacketSize[0x01B] = 0x00; PacketParser[0x01B] = &SmallPacket0x01B;
    PacketSize[0x01C] = 0x00; PacketParser[0x01C] = &SmallPacket0x01C;
    PacketSize[0x028] = 0x06; PacketParser[0x028] = &SmallPacket0x028;
    PacketSize[0x029] = 0x06; PacketParser[0x029] = &SmallPacket0x029;
    PacketSize[0x032] = 0x06; PacketParser[0x032] = &SmallPacket0x032;
    PacketSize[0x033] = 0x06; PacketParser[0x033] = &SmallPacket0x033;
    PacketSize[0x034] = 0x06; PacketParser[0x034] = &SmallPacket0x034;
    PacketSize[0x036] = 0x20; PacketParser[0x036] = &SmallPacket0x036;
    PacketSize[0x037] = 0x0A; PacketParser[0x037] = &SmallPacket0x037;
    PacketSize[0x03A] = 0x04; PacketParser[0x03A] = &SmallPacket0x03A;
    PacketSize[0x03C] = 0x00; PacketParser[0x03C] = &SmallPacket0x03C;
    PacketSize[0x041] = 0x00; PacketParser[0x041] = &SmallPacket0x041;
    PacketSize[0x042] = 0x00; PacketParser[0x042] = &SmallPacket0x042;
    PacketSize[0x04B] = 0x0C; PacketParser[0x04B] = &SmallPacket0x04B;
    PacketSize[0x04D] = 0x00; PacketParser[0x04D] = &SmallPacket0x04D;
    PacketSize[0x04E] = 0x1E; PacketParser[0x04E] = &SmallPacket0x04E;
    PacketSize[0x050] = 0x04; PacketParser[0x050] = &SmallPacket0x050;
    PacketSize[0x059] = 0x00; PacketParser[0x059] = &SmallPacket0x059;
    PacketSize[0x05A] = 0x02; PacketParser[0x05A] = &SmallPacket0x05A;
    PacketSize[0x05B] = 0x0A; PacketParser[0x05B] = &SmallPacket0x05B;
    PacketSize[0x05C] = 0x00; PacketParser[0x05C] = &SmallPacket0x05C;
    PacketSize[0x05D] = 0x08; PacketParser[0x05D] = &SmallPacket0x05D;
    PacketSize[0x05E] = 0x0C; PacketParser[0x05E] = &SmallPacket0x05E;
    PacketSize[0x060] = 0x00; PacketParser[0x060] = &SmallPacket0x060;
    PacketSize[0x061] = 0x02; PacketParser[0x061] = &SmallPacket0x061;
    PacketSize[0x063] = 0x00; PacketParser[0x063] = &SmallPacket0x063;
    PacketSize[0x064] = 0x26; PacketParser[0x064] = &SmallPacket0x064;
    PacketSize[0x066] = 0x0A; PacketParser[0x066] = &SmallPacket0x066;
    PacketSize[0x06E] = 0x06; PacketParser[0x06E] = &SmallPacket0x06E;
    PacketSize[0x06F] = 0x00; PacketParser[0x06F] = &SmallPacket0x06F;
    PacketSize[0x070] = 0x00; PacketParser[0x070] = &SmallPacket0x070;
    PacketSize[0x071] = 0x00; PacketParser[0x071] = &SmallPacket0x071;
    PacketSize[0x074] = 0x00; PacketParser[0x074] = &SmallPacket0x074;
    PacketSize[0x076] = 0x00; PacketParser[0x076] = &SmallPacket0x076;
    PacketSize[0x077] = 0x00; PacketParser[0x077] = &SmallPacket0x077;
    PacketSize[0x078] = 0x00; PacketParser[0x078] = &SmallPacket0x078;
    PacketSize[0x083] = 0x08; PacketParser[0x083] = &SmallPacket0x083;
    PacketSize[0x084] = 0x06; PacketParser[0x084] = &SmallPacket0x084;
    PacketSize[0x085] = 0x04; PacketParser[0x085] = &SmallPacket0x085;
    PacketSize[0x096] = 0x12; PacketParser[0x096] = &SmallPacket0x096;
    PacketSize[0x0A0] = 0x00; PacketParser[0x0A0] = &SmallPacket0xFFF;	// not implemented
    PacketSize[0x0A1] = 0x00; PacketParser[0x0A1] = &SmallPacket0xFFF;	// not implemented
    PacketSize[0x0A2] = 0x00; PacketParser[0x0A2] = &SmallPacket0x0A2;
    PacketSize[0x0AA] = 0x00; PacketParser[0x0AA] = &SmallPacket0x0AA;
    PacketSize[0x0AB] = 0x00; PacketParser[0x0AB] = &SmallPacket0x0AB;
    PacketSize[0x0AC] = 0x00; PacketParser[0x0AC] = &SmallPacket0x0AC;
    PacketSize[0x0AD] = 0x00; PacketParser[0x0AD] = &SmallPacket0x0AD;
    PacketSize[0x0B5] = 0x00; PacketParser[0x0B5] = &SmallPacket0x0B5;
    PacketSize[0x0B6] = 0x00; PacketParser[0x0B6] = &SmallPacket0x0B6;
    PacketSize[0x0BE] = 0x00; PacketParser[0x0BE] = &SmallPacket0x0BE;	//  merit packet
    PacketSize[0x0C3] = 0x00; PacketParser[0x0C3] = &SmallPacket0x0C3;
    PacketSize[0x0C4] = 0x0C; PacketParser[0x0C4] = &SmallPacket0x0C4;
    PacketSize[0x0CB] = 0x00; PacketParser[0x0CB] = &SmallPacket0xFFF;	// not implemented
    PacketSize[0x0D2] = 0x00; PacketParser[0x0D2] = &SmallPacket0x0D2;
    PacketSize[0x0D3] = 0x00; PacketParser[0x0D3] = &SmallPacket0x0D3;
    PacketSize[0x0D4] = 0x00; PacketParser[0x0D4] = &SmallPacket0xFFF;	// not implemented
    PacketSize[0x0DB] = 0x00; PacketParser[0x0DB] = &SmallPacket0x0DB;
    PacketSize[0x0DC] = 0x0A; PacketParser[0x0DC] = &SmallPacket0x0DC;
    PacketSize[0x0DD] = 0x08; PacketParser[0x0DD] = &SmallPacket0x0DD;
    PacketSize[0x0DE] = 0x40; PacketParser[0x0DE] = &SmallPacket0x0DE;
    PacketSize[0x0E0] = 0x4C; PacketParser[0x0E0] = &SmallPacket0x0E0;
    PacketSize[0x0E1] = 0x00; PacketParser[0x0E1] = &SmallPacket0x0E1;
    PacketSize[0x0E2] = 0x00; PacketParser[0x0E2] = &SmallPacket0x0E2;
    PacketSize[0x0E7] = 0x04; PacketParser[0x0E7] = &SmallPacket0x0E7;
    PacketSize[0x0E8] = 0x04; PacketParser[0x0E8] = &SmallPacket0x0E8;
    PacketSize[0x0EA] = 0x00; PacketParser[0x0EA] = &SmallPacket0x0EA;
    PacketSize[0x0F1] = 0x00; PacketParser[0x0F1] = &SmallPacket0x0F1;
    PacketSize[0x0F2] = 0x00; PacketParser[0x0F2] = &SmallPacket0x0F2;
    PacketSize[0x0F4] = 0x04; PacketParser[0x0F4] = &SmallPacket0x0F4;
    PacketSize[0x0F5] = 0x00; PacketParser[0x0F5] = &SmallPacket0x0F5;
    PacketSize[0x0F6] = 0x00; PacketParser[0x0F6] = &SmallPacket0x0F6;
    PacketSize[0x0FA] = 0x00; PacketParser[0x0FA] = &SmallPacket0x0FA;
    PacketSize[0x0FB] = 0x00; PacketParser[0x0FB] = &SmallPacket0x0FB;
    PacketSize[0x100] = 0x04; PacketParser[0x100] = &SmallPacket0x100;
    PacketSize[0x102] = 0x50; PacketParser[0x102] = &SmallPacket0x102;
    PacketSize[0x104] = 0x02; PacketParser[0x104] = &SmallPacket0x104;
    PacketSize[0x105] = 0x06; PacketParser[0x105] = &SmallPacket0x105;
    PacketSize[0x106] = 0x06; PacketParser[0x106] = &SmallPacket0x106;
    PacketSize[0x109] = 0x00; PacketParser[0x109] = &SmallPacket0x109;
    PacketSize[0x10A] = 0x06; PacketParser[0x10A] = &SmallPacket0x10A;
    PacketSize[0x10B] = 0x00; PacketParser[0x10B] = &SmallPacket0x10B;
}

/************************************************************************
*																		*
*																 		*
*																		*
************************************************************************/