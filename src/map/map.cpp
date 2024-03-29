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
#include "../common/malloc.h"
#include "../common/md52.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/utils.h"
#include "../common/version.h"
#include "../common/zlib.h"
#include "../common/mmo.h"
#include "../login/lobby.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alliance.h"
#include "ability.h"
#include "utils/battleutils.h"
#include "utils/charutils.h"
#include "utils/fishingutils.h"
#include "utils/guildutils.h"
#include "utils/itemutils.h"
#include "linkshell.h"
#include "map.h"
#include "mob_spell_list.h"
#include "packet_system.h"
#include "party.h"
#include "utils/petutils.h"
#include "spell.h"

#include "transport.h"
#include "vana_time.h"
#include "utils/zoneutils.h"
#include "conquest_system.h"
#include "utils/mobutils.h"

#include "ai/ai_char_gm.h"
#include "ai/ai_char_normal.h"

#include "lua/luautils.h"

#include "packets/basic.h"
#include "packets/char.h"
#include "packets/server_ip.h"
#include "packets/char_update.h"
#include "packets/chat_message_string.h"

const int8* MAP_CONF_FILENAME = NULL;

int8*  g_PBuff   = NULL;                // глобальный буфер обмена пакетами
int8*  PTempBuff = NULL;                // временный  буфер обмена пакетами
Sql_t* SqlHandle = NULL;				// SQL descriptor

int32  map_fd = 0;						// main socket
uint32 map_amntplayers = 0;				// map amnt unique players

map_config_t map_config;				// map server settings
map_session_list_t map_session_list;
CCommandHandler CmdHandler;

/************************************************************************
*																		*
*  mapsession_getbyipp													*
*																		*
************************************************************************/

map_session_data_t* mapsession_getbyipp(uint64 ipp)
{
	map_session_list_t::iterator i = map_session_list.find(ipp);
	if( i == map_session_list.end() )
		return NULL;

	return (*i).second;
}

/************************************************************************
*																		*
*  mapsession_createsession												*
*																		*
************************************************************************/

map_session_data_t* mapsession_createsession(uint32 ip, uint16 port)
{
	map_session_data_t* map_session_data = new map_session_data_t;
	memset(map_session_data, 0, sizeof(map_session_data_t));

	CREATE(map_session_data->server_packet_data, int8, 1800 + 20);

	map_session_data->last_update = time(NULL);
	map_session_data->client_addr = ip;
	map_session_data->client_port = port;

	uint64 port64 = port;
	uint64 ipp = ip;
	ipp |= port64<<32;
	map_session_list[ipp] = map_session_data;

	ShowInfo(CL_WHITE"mapsession" CL_RESET":" CL_WHITE"%s" CL_RESET":" CL_WHITE"%u" CL_RESET" is coming to world...\n",ip2str(map_session_data->client_addr,NULL),map_session_data->client_port);
	return map_session_data;
}

/************************************************************************
*																		*
*  do_init																*
*																		*
************************************************************************/

int32 do_init(int32 argc, int8** argv)
{
	ShowStatus("do_init: begin server initialization...\n");

	MAP_CONF_FILENAME = "./conf/map_darkstar.conf";

	srand((uint32)time(NULL));

	
	map_config_read(MAP_CONF_FILENAME);
	ShowMessage("\t\t\t - " CL_GREEN"[OK]" CL_RESET"\n");
 	ShowStatus("do_init: map_config is reading");
	ShowMessage("\t\t - " CL_GREEN"[OK]" CL_RESET"\n");

	luautils::init();
	
    PacketParserInitialize();
	SqlHandle = Sql_Malloc();

	ShowStatus("do_init: sqlhandle is allocating");
	if( Sql_Connect(SqlHandle,map_config.mysql_login,
							  map_config.mysql_password,
							  map_config.mysql_host,
							  map_config.mysql_port,
							  map_config.mysql_database) == SQL_ERROR )
	{
		exit(EXIT_FAILURE);
	}
    Sql_Keepalive(SqlHandle);

    // отчищаем таблицу сессий при старте сервера (временное решение, т.к. в кластере это не будет работать)
    Sql_Query(SqlHandle, "TRUNCATE TABLE accounts_sessions");
	const char *Query = "UPDATE accounts SET online ='0', sessions ='0' WHERE online = '1'";
        Sql_Query(SqlHandle,Query);
		Query = "UPDATE accounts SET on_map ='0' WHERE on_map = '1'";
        Sql_Query(SqlHandle,Query);
		Query = "UPDATE chars SET online ='0' WHERE online = '1'";
        Sql_Query(SqlHandle,Query);
		Query = "UPDATE chars SET shutdown ='1' WHERE shutdown = '0'";
        Sql_Query(SqlHandle,Query);

	ShowMessage("\t\t - " CL_GREEN"[OK]" CL_RESET"\n");
	ShowStatus("do_init: zlib is reading");
	zlib_init();
	ShowMessage("\t\t\t - " CL_GREEN"[OK]" CL_RESET"\n");

	ShowStatus("do_init: loading items");
    itemutils::Initialize();
	ShowMessage("\t\t\t - " CL_GREEN"[OK]" CL_RESET"\n");

	// нужно будет написать один метод для инициализации всех данных в battleutils
	// и один метод для освобождения этих данных

	ShowStatus("do_init: loading spells");
	spell::LoadSpellList();
	mobSpellList::LoadMobSpellList();
	ShowMessage("\t\t\t - " CL_GREEN"[OK]" CL_RESET"\n");

    charutils::ResetAllTwoHours();
	guildutils::Initialize();
	charutils::LoadExpTable();
    linkshell::LoadLinkshellList();
    traits::LoadTraitsList();
    effects::LoadEffectsParameters();
	battleutils::LoadSkillTable();
	meritNameSpace::LoadMeritsList();
	nameSpaceUnlockableWeapons::LoadUnlockableWeaponList();
	ability::LoadAbilitiesList();
	battleutils::LoadWeaponSkillsList();
	battleutils::LoadMobSkillsList();
	battleutils::LoadEnmityTable();
    battleutils::LoadSkillChainDamageModifiers();
	petutils::LoadPetList();
	conquest::LoadConquestSystem();
	mobutils::LoadCustomMods();

	ShowStatus("do_init: loading zones");
	zoneutils::LoadZoneList();
	ShowMessage("\t\t\t - " CL_GREEN"[OK]" CL_RESET"\n");

	luautils::OnServerStart();
    fishingutils::LoadFishingMessages();

	ShowStatus("do_init: server is binding with port %u",map_config.usMapPort);
	map_fd = makeBind_udp(map_config.uiMapIp,map_config.usMapPort);
	ShowMessage("\t - " CL_GREEN"[OK]" CL_RESET"\n");

    CVanaTime::getInstance()->setCustomOffset(map_config.vanadiel_time_offset);

	CTaskMgr::getInstance()->AddTask("Check_Map_For_Player_Cleanup", gettick(), NULL, CTaskMgr::TASK_INTERVAL, Check_Map_For_Player_Cleanup, 700);
	

	CREATE(g_PBuff,   int8, 1800 + 20);
    CREATE(PTempBuff, int8, 1800 + 20);
	aFree((void*)map_config.mysql_login);
	aFree((void*)map_config.mysql_password);
	ShowStatus("The map-server is " CL_GREEN"ready" CL_RESET" to work...\n");
    ShowMessage("=======================================================================\n");
	return 0;
}

/************************************************************************
*																		*
*  do_final																*
*																		*
************************************************************************/

void do_final(void)
{
	aFree(g_PBuff);
    aFree(PTempBuff);

	aFree((void*)map_config.mysql_host);
	aFree((void*)map_config.mysql_database);

	itemutils::FreeItemList();
	battleutils::FreeWeaponSkillsList();
    battleutils::FreeSkillChainDamageModifiers();

	petutils::FreePetList();
	zoneutils::FreeZoneList();
	luautils::free();

	delete CTaskMgr::getInstance();
	delete CVanaTime::getInstance();

	Sql_Free(SqlHandle);
}

/************************************************************************
*																		*
*  do_abort																*
*																		*
************************************************************************/

void do_abort(void)
{
	do_final();
}

/************************************************************************
*																		*
*  set_server_type														*
*																		*
************************************************************************/

void set_server_type()
{
	SERVER_TYPE = DARKSTAR_SERVER_MAP;
}

/************************************************************************
*																		*
*  do_sockets															*
*																		*
************************************************************************/

int32 do_sockets(int32 next)
{
	fd_set rfd;

	struct timeval timeout;
	int32 ret;
	memcpy(&rfd, &readfds, sizeof(rfd));

	timeout.tv_sec  = next/1000;
	timeout.tv_usec = next%1000*1000;

	ret = sSelect(fd_max, &rfd, NULL, NULL, &timeout);

	if( ret == SOCKET_ERROR )
	{
		if( sErrno != S_EINTR )
		{
			ShowFatalError("do_sockets: select() failed, error code %d!\n", sErrno);
			exit(EXIT_FAILURE);
		}
		return 0; // interrupted by a signal, just loop and try again
	}

	last_tick = time(NULL);

	if( sFD_ISSET(map_fd,&rfd) )
	{
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		int32 ret = recvudp(map_fd,g_PBuff,1800,0,(struct sockaddr*)&from,&fromlen);
		if( ret != -1)
		{
			// find player char
		#	ifdef WIN32
			uint32 ip   = ntohl(from.sin_addr.S_un.S_addr);
		#	else
			uint32 ip   = ntohl(from.sin_addr.s_addr);
		#	endif

			uint64 port = ntohs(from.sin_port);
			uint64 ipp = ip;
			ipp |= port<<32;
			map_session_data_t* map_session_data = mapsession_getbyipp(ipp);

			if( map_session_data == NULL )
			{
				map_session_data = mapsession_createsession(ip,ntohs(from.sin_port));
				if( map_session_data == NULL )
				{
					return -1;
				}
			}

			map_session_data->last_update = time(NULL);
			size_t size = ret;

			if( recv_parse(g_PBuff,&size,&from,map_session_data) != -1 )
			{
				//ShowStatus("SOCKET IS OK\n");
				// если предыдущий пакет был потерян, то мы не собираем новый,
				// а отправляем предыдущий пакет повторно
				if (!parse(g_PBuff,&size,&from,map_session_data))
				{
					send_parse(g_PBuff,&size,&from,map_session_data);
				}

				ret = sendudp(map_fd,g_PBuff,size,0,(const struct sockaddr*)&from,fromlen);

				int8* data = g_PBuff;
				g_PBuff = map_session_data->server_packet_data;

				map_session_data->server_packet_data = data;
				map_session_data->server_packet_size = size;
			}
			else
			{
                ShowStatus("SOCKET IS NOT OK\n");
			}
		}
	}
	return 0;
}

/************************************************************************
*																		*
*  parse_console														*
*																		*
************************************************************************/

int32 parse_console(int8* buf)
{
	ShowStatus("int32 parse_console(int8* buf)\n");
	return 0;
}

/************************************************************************
*																		*
*  map_decipher_packet													*
*																		*
************************************************************************/

int32 map_decipher_packet(int8* buff, size_t size, sockaddr_in* from, map_session_data_t* map_session_data)
{
	uint16 tmp, i;

	// counting blocks whose size = 4 byte
	tmp = (size-FFXI_HEADER_SIZE)/4;
	tmp -= tmp%2;

#	ifdef WIN32
	uint32 ip   = ntohl(from->sin_addr.S_un.S_addr);
#	else
	uint32 ip   = ntohl(from->sin_addr.s_addr);
#	endif

	blowfish_t *pbfkey = &map_session_data->blowfish;

	for(i = 0; i < tmp; i += 2)
	{
		blowfish_decipher((uint32*)buff+i+7,(uint32*)buff+i+8, pbfkey->P, pbfkey->S[0]);
	}

	if( checksum((uint8*)(buff+FFXI_HEADER_SIZE),size-(FFXI_HEADER_SIZE+16),buff+size-16) == 0)
	{
		map_session_data->shuttingDown =false;
		uint32 map_time = CVanaTime::getInstance()->getSysSecond();
		//ShowMessage(CL_BG_RED"CHECK MAP TIME NOW %u \n"CL_RESET,map_time);
	const char *Query = "UPDATE accounts SET  map_time = '%u', on_map='1' WHERE sessions = %u";
                Sql_Query(SqlHandle,Query,map_time,map_session_data);
		
		return 0;
	}

	int8 ip_str[16];
	map_session_data->shuttingDown =true;
	ShowMessage(CL_BG_RED"CHECK MAP NOW AS I AM STILL ON MAP TRUE I HAVE LEFT\n"CL_RESET);
	//ShowError("map_encipher_packet: bad packet from <%s>\n",ip2str(ip,ip_str));
	return -1;
}

/************************************************************************
*																		*
*  main function to parse recv packets									*
*																		*
************************************************************************/

int32 recv_parse(int8* buff, size_t* buffsize, sockaddr_in* from, map_session_data_t* map_session_data)
{
	size_t size = *buffsize;
	int32 checksumResult = -1;

	checksumResult = checksum((uint8*)(buff+FFXI_HEADER_SIZE),size-(FFXI_HEADER_SIZE+16),buff+size-16);

	if(checksumResult == 0)
	{
		if (map_session_data->PChar == NULL)
		{
			uint32 CharID = RBUFL(buff,FFXI_HEADER_SIZE+0x0C);

			const int8* fmtQuery = "SELECT session_key FROM accounts_sessions WHERE charid = %u LIMIT 1;";

			int32 ret = Sql_Query(SqlHandle,fmtQuery,CharID);

			if (ret == SQL_ERROR ||
				Sql_NumRows(SqlHandle) == 0 ||
				Sql_NextRow(SqlHandle) != SQL_SUCCESS)
			{
				ShowError(CL_RED"recv_parse: Cannot load session_key for charid %u" CL_RESET, CharID);
			}
			else
			{
				int8* strSessionKey = NULL;
				Sql_GetData(SqlHandle,0,&strSessionKey,NULL);

				memcpy(map_session_data->blowfish.key,strSessionKey,20);
			}

			// наверное создание персонажа лучше вынести в метод charutils::LoadChar() и загрузку инвентаря туда же сунуть

			CCharEntity* PChar = new CCharEntity();

			PChar->id = CharID;
			PChar->Check_Engagment = new CAICharNormal(PChar);

			charutils::LoadChar(PChar);
			charutils::LoadInventory(PChar);

            luautils::OnGameIn(PChar);
			luautils::CheckForGearSet(PChar); // check for gear set on login

            PChar->status = STATUS_DISAPPEAR;

			map_session_data->PChar = PChar;
			uint8 accid = 0;
			const int8* Query = "SELECT accid FROM chars WHERE charid = '%u';";
	              ret = Sql_Query(SqlHandle,Query,PChar->id);

	          if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)
	             {
				   accid =  Sql_GetUIntData(SqlHandle,0);
				   PChar->accid =accid;
				   ShowDebug(CL_RED"PLAYERS ACCOUNT ID %u \n"CL_RESET,PChar->accid);
			Query = "UPDATE chars SET sessions ='%u' WHERE charid = %u";
        Sql_Query(SqlHandle,Query,map_session_data,PChar->id);
		Query = "UPDATE accounts SET sessions ='%u' WHERE id = %u";
        Sql_Query(SqlHandle,Query,map_session_data,PChar->accid);
		Query = "UPDATE accounts_sessions SET sessions ='%u' WHERE accid = %u";
        Sql_Query(SqlHandle,Query,map_session_data,PChar->accid);
		Query = "UPDATE accounts SET online ='1' WHERE id = %u";
        Sql_Query(SqlHandle,Query,PChar->accid);
			  }
		}
		map_session_data->client_packet_id = 0;
		map_session_data->server_packet_id = 0;
		return 0;
	}else{
		//char packets

		if( map_decipher_packet(buff,*buffsize,from,map_session_data) == -1)
		{
			*buffsize = 0;
			return -1;
		}
		// reading data size
		uint32 PacketDataSize = RBUFL(buff,*buffsize-sizeof(int32)-16);
		// creating buffer for decompress data
		int8* PacketDataBuff = NULL;
		CREATE(PacketDataBuff,int8,1800);
		// it's decompressing data and getting new size
		PacketDataSize = zlib_decompress(buff+FFXI_HEADER_SIZE,
										 PacketDataSize,
										 PacketDataBuff,
										 1800,
										 zlib_decompress_table);

		// it's making result buff
		// don't need memcpy header
		memcpy(buff+FFXI_HEADER_SIZE,PacketDataBuff,PacketDataSize);
		*buffsize = FFXI_HEADER_SIZE+PacketDataSize;
		
		//ShowStatus("int32 recv_parse(ELSE)\n");
		aFree(PacketDataBuff);
		return 0;
	}
	return -1;
}

/************************************************************************
*																		*
*  main function parsing the packets									*
*																		*
************************************************************************/

int32 parse(int8* buff, size_t* buffsize, sockaddr_in* from, map_session_data_t* map_session_data)
{
	

	
	//ShowDebug(" BUFF SIZE IS  %u\n",*buffsize);
	// начало обработки входящего пакета
	if (*buffsize > 1800)
	{
		ShowDebug("BUFF SIZE IS TO LARGER WILL DC THE PLAYER SKIP SENDING %u\n",*buffsize);
     return false;
	}
	
	
	
	int8* PacketData_Begin = &buff[FFXI_HEADER_SIZE];
	int8* PacketData_End   = &buff[*buffsize];

	CCharEntity *PChar = map_session_data->PChar;

	uint16 SmallPD_Size = 0;
	uint16 SmallPD_Type = 0;
	uint16 SmallPD_Code = RBUFW(buff,0);

	for(int8* SmallPD_ptr = PacketData_Begin;SmallPD_ptr + (RBUFB(SmallPD_ptr,1) & 0xFE)*2 <= PacketData_End && (RBUFB(SmallPD_ptr,1) & 0xFE);SmallPD_ptr = SmallPD_ptr + SmallPD_Size*2)
	{
		
		
		SmallPD_Size = (RBUFB(SmallPD_ptr,1) & 0x0FE);
		SmallPD_Type = (RBUFW(SmallPD_ptr,0) & 0x1FF);
		
		PacketParser[SmallPD_Type](map_session_data, PChar, SmallPD_ptr);
		if(SmallPD_Type != 21)//21 is player update packet that is called none stop so not want to spam
		{
		ShowMessage(CL_YELLOW"PACKET TYPE: %u\n" CL_RESET,SmallPD_Type);
		}
		
	     
		 
		      
	      
		
	}
    map_session_data->client_packet_id = SmallPD_Code;
	
//ShowDebug(CL_GREEN"PACKET CALL: CLIENT CODE %u\n"CL_RESET,map_session_data->client_packet_id);

	// здесь мы проверяем, получил ли клиент предыдущий пакет
	// если не получил, то мы не создаем новый, а отправляем предыдущий

	if (RBUFW(buff,2) != map_session_data->server_packet_id)
	{
		WBUFW(map_session_data->server_packet_data,2) = SmallPD_Code;
		WBUFW(map_session_data->server_packet_data,8) = (uint32)time(NULL);

		g_PBuff	 = map_session_data->server_packet_data;
	   *buffsize = map_session_data->server_packet_size;

		map_session_data->server_packet_data = buff;
		return -1;
	}

	// увеличиваем номер отправленного пакета только в случае отправки новых данных

	map_session_data->server_packet_id += 1;

	
	
	

	CBasicPacket* PSmallPacket;

	*buffsize = FFXI_HEADER_SIZE;

	while(!PChar->isPacketListEmpty() && *buffsize + PChar->firstPacketSize()*2 < 1800 )
	{
		PSmallPacket = PChar->popPacket();

		PSmallPacket->setCode(map_session_data->server_packet_id);
		memcpy(buff+*buffsize, PSmallPacket, PSmallPacket->getSize()*2);

		*buffsize += PSmallPacket->getSize()*2;

//		delete PSmallPacket;
	
	}
	return 0;
}

/************************************************************************
*																		*
*  main function is building big packet									*
*																		*
************************************************************************/

int32 send_parse(int8 *buff, size_t* buffsize, sockaddr_in* from, map_session_data_t* map_session_data)
{
	
	  if (*buffsize > 1800)
	{
		ShowWarning(CL_BG_RED"BUFF SIZE IS TO LARGE SKIP <%u>\n" CL_RESET,*buffsize);
		
		return 0;
	}
	
		
	//  - записать текущее время отправки пакета
    WBUFL(buff,8) = (uint32)time(NULL);
	WBUFW(buff,0) = map_session_data->server_packet_id;
	WBUFW(buff,2) = map_session_data->client_packet_id; 

	// сохранение текущего времени (32 BIT!)
	

	//Сжимаем данные без учета заголовка
	//Возвращаемый размер в 8 раз больше реальных данных
	uint32 PacketSize = zlib_compress(buff+FFXI_HEADER_SIZE, *buffsize-FFXI_HEADER_SIZE, PTempBuff, *buffsize, zlib_compress_table);

	//Запись размера данных без учета заголовка
	WBUFL(PTempBuff,(PacketSize+7)/8) = PacketSize;

	//Расчет hash'a также без учета заголовка, но с учетом записанного выше размера данных
	PacketSize = (PacketSize+7)/8+4;
	uint8 hash[16];
	md5((uint8*)PTempBuff, hash, PacketSize);
	memcpy(PTempBuff+PacketSize, hash, 16);
	PacketSize += 16;

    if (PacketSize > 1800 + 20)
    {
        ShowFatalError(CL_RED"%Memory manager: PTempBuff is overflowed (%u)\n" CL_RESET, PacketSize);
		return 0;
    }

	//making total packet
	memcpy(buff+FFXI_HEADER_SIZE, PTempBuff, PacketSize);

	uint32 CypherSize = (PacketSize/4)&-2;

	blowfish_t* pbfkey = &map_session_data->blowfish;

	for(uint32 j = 0; j < CypherSize; j += 2)
	{
		blowfish_encipher((uint32*)(buff)+j+7, (uint32*)(buff)+j+8, pbfkey->P, pbfkey->S[0]);
	}

	
	if(PacketSize+FFXI_HEADER_SIZE > 1800)
	{
		ShowWarning(CL_BG_RED"PACKET SIZE IS TO LARGE SKIP <%u>\n" CL_RESET,PacketSize+FFXI_HEADER_SIZE);
		
		return 0;
	}
	*buffsize = PacketSize+FFXI_HEADER_SIZE;
	//ShowWarning(CL_BG_YELLOW"AFTER PACKET SIZE <%u>\n" CL_RESET,PacketSize+FFXI_HEADER_SIZE);
	if (*buffsize > 1800)
	{
		//I NOTICED WHEN A PLAYER LOGS IN THERE ARE SOME CASES WHERE THEY ARE GETTING TO BIG OF A PACKET TO GOAL IS TO REDUCE THE PACKETS
		//BEING SENT ON LOGIN AND ON ZONING MAYBE SLOW IT DOWN A BIT, ONCE THIS REDUCTION IS DONE TEHN WE CAN START UPDATING OTHER PACKETS IT LOOKED LIKE THIS
		//WAS CALLED BECASUE OF ALL THE SPELL CASTING FROM THE MOBS. SO THAT IS TURNED OFF RIGHT NOW WHEN I HAD THE SPELLS TURN OFF FOR THE MOBS
		//I NEVER ONCE GOT THIS PACKET SIZE PROBLEM FOR DCING PLAYERS NOW THAT I HAVE IT BAD ENABLED IM GETTTING IT ONCE AGAIN 
		//AND I HAVENT BEEN GETTING THIS AT ALL
		ShowWarning(CL_YELLOW"send_parse: packet is very big <%u>\n" CL_RESET,*buffsize);
		ShowWarning(CL_YELLOW"GET SESSION<%u>\n" CL_RESET,map_session_data);
	
		//AS THIS HAPPESN WE NEED TO GET THEM OFF THE MAP 
		const char *Query = "UPDATE chars SET  online = '0', shutdown = '1', zoning = '-1', returning = '0' WHERE sessions = %u";
                Sql_Query(SqlHandle,Query,map_session_data);
		        Query = "UPDATE accounts SET  online = '0', sessions ='0', on_map ='0', map_time = '0', launcher = '0' WHERE sessions = %u";
                Sql_Query(SqlHandle,Query,map_session_data);
		 
	}
	return 0;
	
}

/************************************************************************
*																		*
*  Таймер для завершения сессии (без таймера мы этого сделать не можем,	*
*  т.к. сессия продолжает использоваться в do_sockets)					*
*																		*
************************************************************************/

int32 Close_Session_Clean_Map(uint32 tick, CTaskMgr::CTask* PTask)
{
	map_session_data_t* map_session_data = (map_session_data_t*)PTask->m_data;

	if (map_session_data != NULL &&
		map_session_data->server_packet_data != NULL &&		// bad pointer crashed here, might need dia to look at this one
		map_session_data->PChar != NULL)					// crash occured when both server_packet_data & PChar were NULL
	{
		Sql_Query(SqlHandle,"DELETE FROM accounts_sessions WHERE charid = %u",map_session_data->PChar->id);
        
		//ShowMessage(CL_YELLOW"FINISHED MAP CLEAN FOR PLAYERS ACCOUNT ID = %u GET\n"CL_RESET,map_session_data->PChar->accid);
		const char *Query = "UPDATE accounts SET online = '0', sessions ='0', on_map ='0', map_time = '0', launcher = '0' WHERE id = %u";
                Sql_Query(SqlHandle,Query,map_session_data->PChar->id);
					Query = "UPDATE chars SET  online = '0', shutdown = '1', zoning = '-1', returning = '0' WHERE charid = %u";
                Sql_Query(SqlHandle,Query,map_session_data->PChar->id);
		uint64 port64 = map_session_data->client_port;
		uint64 ipp	  = map_session_data->client_addr;
		ipp |= port64<<32;
		 
		map_session_data->PChar->StatusEffectContainer->SaveStatusEffects();

		aFree(map_session_data->server_packet_data);
		delete map_session_data->PChar;
		delete map_session_data;
		map_session_data = NULL;

		map_session_list.erase(ipp);
		ShowMessage(CL_YELLOW"MAP CLEANED 100% OK\n" CL_RESET);
		
		return 0;
	}

	ShowError(CL_RED"map_close_session: cannot close session, session not found\n" CL_RESET);
	return 1;
}

/************************************************************************
*																		*
*  Timer function that clenup all timed out players						*
*																		*
************************************************************************/

int32 Check_Map_For_Player_Cleanup(uint32 tick, CTaskMgr::CTask* PTask)
{
	
	uint32 checktime = CVanaTime::getInstance()->getSysSecond();
	
	map_session_list_t::iterator it = map_session_list.begin();

	while(it != map_session_list.end())
	{
		map_session_data_t* map_session_data = it->second;

        CCharEntity* PChar = map_session_data->PChar;
		//CZone* PZone = zoneutils::GetZone(PChar->loc.destination);
		
		
		
        if ( checktime == 0 || checktime == 5 || 
			checktime == 10 || checktime == 15 ||
			checktime == 20 || checktime == 25 || 
			checktime == 30 || checktime == 35 || 
			checktime == 40 || checktime == 45 ||
			checktime == 50 || checktime == 55)
        {
			if (PChar != NULL)
            {
					if( PChar->is_zoning == -1)//-1 PLAYER IS NOT ZONING RIGHT NOW
					   {
						charutils::SaveCharSystem(PChar);//SAVE CHARACTERS SYSTEM EVERY 5 SECONDS
						
						
					   }
					
							   
			}     
                            
			if(map_session_data->shuttingDown == false)//START SESSION SHUTDOWN STATUS
			  {
			   int8 shutdown = 0;
			   const char * Query = "SELECT shutdown FROM chars WHERE charid= '%u';";
	           int32 ret3 = Sql_Query(SqlHandle,Query,PChar->id);
			   if (ret3 != SQL_ERROR && Sql_NumRows(SqlHandle) != 0 && Sql_NextRow(SqlHandle) == SQL_SUCCESS)//START DATABASE SELECTION
	            {
				shutdown =  Sql_GetUIntData(SqlHandle,0);
				if (shutdown == 1)//START IF SHUTDOWN FROM DATABASE
		         {
				   if (PChar != NULL)
			        {
					  if(PChar->shutdown_status ==1)//THIS IS /logout normal command if this is not called they killed the boot.
						{
                        // ShowMessage(CL_YELLOW"I AM CLEANING THE MAP NOW: %s timed %u\n" CL_RESET, PChar->GetName(),time(NULL));
						  if(PChar->PParty != NULL && PChar->PParty->m_PAlliance != NULL && PChar->PParty->GetLeader() == PChar)
							{
						     if(PChar->PParty->members.size() == 1)
							  {
							   if(PChar->PParty->m_PAlliance->partyList.size() == 2)
								 {
								  PChar->PParty->m_PAlliance->dissolveAlliance();
							     }
								 else 
									 {
										   if(PChar->PParty->m_PAlliance->partyList.size() == 3)
											{
								              PChar->PParty->m_PAlliance->removeParty(PChar->PParty);
								            }
						             }
					           }
						    }
						if (PChar->PParty != NULL)
		                 {
			             PChar->PParty->RemoveMember(PChar);
		                 }
						if (PChar->PLinkshell != NULL)
                         {
                        PChar->PLinkshell->DelMember(PChar);
                         }
		                if (PChar->PPet != NULL && PChar->PPet->objtype == TYPE_MOB)
		                 {
		                 petutils::DespawnPet(PChar);
		                 }
						SpawnIDList_t::iterator PC = PChar->SpawnPCList.find(PChar->id);
                        if( !PChar->SpawnPCList.empty() )
			              {
							//ShowMessage(CL_YELLOW"I CLEANED: MY CHAR SPAWN PC\n" CL_RESET);
				          PChar->SpawnPCList.erase(PChar->id);
						  PChar->pushPacket(new CCharPacket(PChar,ENTITY_DESPAWN));
				          }
						CTaskMgr::getInstance()->AddTask(new CTaskMgr::CTask("close_session", gettick()+10, map_session_data, CTaskMgr::TASK_ONCE, Close_Session_Clean_Map));
						}
					  else// ELSE THE PLAYER KILLED THE BOOT AND IS STILL ON THE MAP AND THE LOGIN SERVER CALLED THIS METHOD
					  {
						  //SHOULD BE TWO FUNCTION CALLED LEAVEGAME AND PACKET PARSER
						 // ShowMessage(CL_YELLOW"PLAYER KILLED THE BOOT DIFFERENT MAP CLEAN\n" CL_RESET);
						  PChar->leavegame();
						  PacketParser[0x00D](map_session_data, PChar, 0);
					  }
		              } 
			        }//END IF SHUTDOWN FROM DATABASE
				   }//END DATABASE SELECTION
				}//END SESSION SHUTDOWN STATUS
				if(map_session_data->shuttingDown == true)//THIS FUNCTION IS ONLY CALLED WHEN A PLAYER IS ON MAP AND THE SERVER RESTARTS
				{
				//THIS SHOULD NEVER HAPPEN BUT IF IT DOSE REASON BEING THE SERVER CRASHS AND IT WAS RESTARTED
				//OR THE CLIENT LOST CONNECTION TO THE PHRAS FUNCTION
               // ShowWarning(CL_YELLOW"map_cleanup: WHITHOUT CHAR timed out, session closed\n" CL_RESET);
				const char *Query = "UPDATE chars SET  online = '0', shutdown = '1', zoning = '-1', returning = '0' WHERE sessions = %u";
                Sql_Query(SqlHandle,Query,map_session_data);
		        Query = "UPDATE accounts SET  online = '0', sessions ='0', on_map ='0', map_time = '0', launcher = '0' WHERE sessions = %u";
                Sql_Query(SqlHandle,Query,map_session_data);
		        Sql_Query(SqlHandle,"DELETE FROM accounts_sessions WHERE sessions = %u",map_session_data);
                aFree(map_session_data->server_packet_data);
				map_session_list.erase(it++);
                delete map_session_data;
				continue;
			    }
		}
        
		++it;
	}
	return 0;
}



int32 map_config_read(const int8* cfgName)
{
	int8 line[1024], w1[1024], w2[1024];
	FILE* fp;

	fp = fopen(cfgName,"r");
	if( fp == NULL )
	{
		ShowError("Map configuration file not found at: %s\n", cfgName);
		return 1;
	}

	while( fgets(line, sizeof(line), fp) )
	{
		int8* ptr;

        if( line[0] == '#' )
			continue;
		if( sscanf(line, "%[^:]: %[^\t\r\n]", w1, w2) < 2 )
			continue;

		//Strip trailing spaces
		ptr = w2 + strlen(w2);
		while (--ptr >= w2 && *ptr == ' ');
		ptr++;
		*ptr = '\0';

		if(strcmpi(w1,"timestamp_format") == 0)
		{
			strncpy(timestamp_format, w2, 20);
		}
		else if(strcmpi(w1,"stdout_with_ansisequence") == 0)
		{
			stdout_with_ansisequence = config_switch(w2);
		}
		else if(strcmpi(w1,"console_silent") == 0)
		{
			ShowInfo("Console Silent Setting: %d", atoi(w2));
			msg_silent = atoi(w2);
		}
		else if (strcmpi(w1,"map_port") == 0)
		{
			map_config.usMapPort = (atoi(w2));
		}
		else if (strcmp(w1,"buff_maxsize") == 0)
		{
			map_config.buffer_size = atoi(w2);
		}
		else if (strcmp(w1,"engage_dist_mod") == 0)
		{
			map_config.engage_dist_mod = atoi(w2);
		}
		else if (strcmp(w1,"magic_dist_mod") == 0)
		{
			map_config.magic_dist_mod = atoi(w2);
		}
		else if (strcmp(w1,"ranged_dist_mod") == 0)
		{
			map_config.ranged_dist_mod = atoi(w2);
		}
		else if (strcmp(w1,"remove_engage_delay") == 0)
		{
			map_config.remove_engage_delay = atoi(w2);
		}
		else if (strcmp(w1,"magic_movement") == 0)
		{
			map_config.magic_movement = atoi(w2);
		}	
		else if (strcmp(w1,"seal_crest_bonus") == 0)
		{
			map_config.seal_crest_bonus = atoi(w2);
		}
		else if (strcmp(w1,"always_seal_crest") == 0)
		{
			map_config.always_seal_crest = atoi(w2);
		}
		else if (strcmp(w1,"drop_rate_bonus") == 0)
		{
			map_config.drop_rate_bonus = atoi(w2);
		}
		else if (strcmp(w1,"all_mobs_drop_gil") == 0)
		{
			map_config.all_mobs_drop_gil = atoi(w2);
		}
		else if (strcmp(w1,"gil_bonus") == 0)
		{
			map_config.gil_bonus = atoi(w2);
		}
		else if (strcmp(w1,"chain_on_easy") == 0)
		{
			map_config.chain_on_easy = atoi(w2);
		}
		else if (strcmp(w1,"chain_bonus") == 0)
		{
			map_config.chain_bonus = atoi(w2);
		}		
		else if (strcmp(w1,"craft_skill_onfail") == 0)
		{
			map_config.craft_skill_onfail = atoi(w2);
		}
		else if (strcmp(w1,"craft_skillamt") == 0)
		{
			map_config.craft_skillamt = atoi(w2);
		}
		else if (strcmp(w1,"conquest_bonus") == 0)
		{
			map_config.conquest_bonus = atoi(w2);
		}
		else if (strcmp(w1,"craft_success") == 0)
		{
			map_config.craft_success = atoi(w2);
		}
		else if (strcmp(w1,"notorious_hp_mod") == 0)
		{
			map_config.notorious_hp_mod = atoi(w2);
		}
		else if (strcmp(w1,"notorious_mp_mod") == 0)
		{
			map_config.notorious_mp_mod = atoi(w2);
		}
		else if (strcmp(w1,"max_time_lastupdate") == 0)
		{
			map_config.max_time_lastupdate = atoi(w2);
		}
        else if (strcmp(w1,"vanadiel_time_offset") == 0)
        {
            map_config.vanadiel_time_offset = atoi(w2);
        }
        else if (strcmp(w1,"lightluggage_block") == 0)
        {
            map_config.lightluggage_block = atoi(w2);
        }
        else if (strcmp(w1,"exp_rate") == 0)
        {
            map_config.exp_rate = atof(w2);
        }
        else if (strcmp(w1,"exp_loss_rate") == 0)
        {
            map_config.exp_loss_rate = atof(w2);
        }
		else if (strcmp(w1,"thf_in_party_for_drops") == 0)
        {
            map_config.thf_in_party_for_drops = atof(w2);
        }
		else if (strcmp(w1,"exp_party_gap_penalties") == 0)
        {
            map_config.exp_party_gap_penalties = atof(w2);
        }
		else if (strcmp(w1,"fov_party_gap_penalties") == 0)
        {
            map_config.fov_party_gap_penalties = atof(w2);
        }
		else if (strcmp(w1,"fov_allow_alliance") == 0)
        {
            map_config.fov_allow_alliance = atof(w2);
        }
		else if (strcmp(w1,"mob_tp_multiplier") == 0)
        {
            map_config.mob_tp_multiplier = atof(w2);
        }
		else if (strcmp(w1,"player_tp_multiplier") == 0)
        {
            map_config.player_tp_multiplier = atof(w2);
        }
		else if (strcmp(w1,"exp_retain") == 0)
        {
            map_config.exp_retain = dsp_cap(atof(w2), 0.0f, 1.0f);
        }
		else if (strcmp(w1,"exp_loss_level") == 0)
		{
			map_config.exp_loss_level = atoi(w2);
		}
		else if (strcmp(w1,"speed_mod") == 0)
		{
			map_config.speed_mod = atoi(w2);
		}
		else if (strcmp(w1,"skillup_multiplier") == 0)
        {
            map_config.skillup_multiplier = atof(w2);
        }
		else if (strcmp(w1,"craft_multiplier") == 0)
        {
            map_config.craft_multiplier = atof(w2);
        }
		else if (strcmp(w1,"DNS_Servers_Address") == 0)
		{
			map_config.DNS_Servers_Address = aStrdup(w2);
		}
		else if (strcmp(w1,"NETWORK_Servers_Address") == 0)
		{
			map_config.NETWORK_Servers_Address = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_host") == 0)
		{
			map_config.mysql_host = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_host") == 0)
		{
			map_config.mysql_host = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_login") == 0)
		{
			map_config.mysql_login = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_password") == 0)
		{
			map_config.mysql_password = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_port") == 0)
		{
			map_config.mysql_port = atoi(w2);
		}
		else if (strcmp(w1,"mysql_database") == 0)
		{
			map_config.mysql_database = aStrdup(w2);
		}
        else if (strcmpi(w1,"server_message") == 0)
        {
            map_config.server_message = aStrdup(w2);

            uint32 length = (uint32)strlen(map_config.server_message);

            for(uint32 count = 0; count < length; ++count)
            {
                if (RBUFW(map_config.server_message, count) == 0x6E5C) //  \n = 0x6E5C in hex
                {
                    WBUFW(map_config.server_message, count) =  0x0A0D;
                }
	        }
        }
		else if (strcmpi(w1,"fr_server_message") == 0)
        {
            map_config.fr_server_message = aStrdup(w2);

            uint32 length = (uint32)strlen(map_config.fr_server_message);

            for(uint32 count = 0; count < length; ++count)
            {
                if (RBUFW(map_config.fr_server_message, count) == 0x6E5C) //  \n = 0x6E5C in hex
                {
                    WBUFW(map_config.fr_server_message, count) =  0x0A0D;
                }
	        }
        }
		else if (strcmpi(w1,"import") == 0)
		{
			map_config_read(w2);
		}
        else if(strcmpi(w1,"newstyle_skillups") == 0)
        {
            map_config.newstyle_skillups = atoi(w2);
        }
		else if (strcmp(w1,"max_merit_points") == 0)
		{
			map_config.max_merit_points = atoi(w2);
		}
		else if (strcmp(w1,"audit_chat") == 0)
		{
			map_config.audit_chat = atoi(w2);
		}
		else
		{
			ShowWarning(CL_YELLOW"Unknown setting '%s' in file %s\n" CL_RESET, w1, cfgName);
		}
	}

	fclose(fp);
	return 0;
}


