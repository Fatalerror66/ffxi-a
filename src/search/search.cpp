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

#include "../common/cbasetypes.h"
#include "../common/blowfish.h"
#include "../common/malloc.h"
#include "../common/md52.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/utils.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include "data_loader.h"
#include "search.h"
#include "tcp_request.h"

#include "packets/auction_history.h"
#include "packets/auction_list.h"
#include "packets/linkshell_list.h"
#include "packets/party_list.h"
#include "packets/search_list.h"

#define DEFAULT_BUFLEN	1024
#define CODE_LVL 17
#define CODE_JOB 13
#define CODE_ZONE 20
#define CODE_ZONE_ALL 16

struct SearchCommInfo
{
	SOCKET socket;
	uint32 ip;
	uint16 port;
};

const int8* SEARCH_CONF_FILENAME = "./conf/search_server.conf";
ppuint32 __stdcall TCPComm(void* lpParam);

extern void HandleSearchRequest(CTCPRequestPacket* PTCPRequest);
extern void HandleSearchComment(CTCPRequestPacket* PTCPRequest);
extern void HandleGroupListRequest(CTCPRequestPacket* PTCPRequest);
extern void HandleAuctionHouseHistoru(CTCPRequestPacket* PTCPRequest);
extern void HandleAuctionHouseRequest(CTCPRequestPacket* PTCPRequest);
extern search_req _HandleSearchRequest(CTCPRequestPacket* PTCPRequest, SOCKET socket);
extern std::string toStr(int number);

search_config_t search_config;

void search_config_read(const int8* file);


/************************************************************************
*																		*
*  Отображения содержимого входящего пакета в консоли					*
*																		*
************************************************************************/

void PrintPacket(char* data, int size)
{
	int8 message[50];
	memset(&message,0,50);

	printf("\n");

	for (int32 y = 0; y < size; y++) 
	{
		sprintf(message,"%s %02hx",message,(uint8)data[y]);
		if (((y+1)%16) == 0) 
		{
			message[48] = '\n';
			printf(message);
			memset(&message,0,50);
		}
	}
	if (strlen(message) > 0) 
	{
		message[strlen(message)] = '\n';
		printf(message);
	}
	printf("\n");
}

/************************************************************************
*																		*
*																		*
*																		*
************************************************************************/

int32 main (int32 argc, int8 **argv) 
{
    WSADATA wsaData;

    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo  hints;

    search_config_read(SEARCH_CONF_FILENAME);

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
	{
        ShowError("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, "54002", &hints, &result);
    if (iResult != 0)
	{
        ShowError("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) 
	{
        ShowError("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) 
	{
        ShowError("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) 
	{
		ShowError("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

   
    ShowMessage(CL_GREEN"FINAL FANTASY XI PRIVATE SERVER!\n"CL_RESET);
   ShowMessage(CL_GREEN"SEARCH SERVER INIT!\n"CL_RESET);
   ShowMessage(CL_GREEN"OK!\n"CL_RESET);
	while (true)
	{
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) 
		{
			ShowError("accept failed with error: %d\n", WSAGetLastError());
			continue;
		}
		SearchCommInfo CommInfo;

		CommInfo.socket = ClientSocket;
		CommInfo.ip = 0;
		CommInfo.port = 0;

		CreateThread(0,0,TCPComm,&CommInfo,0,0);
	}
    // TODO: сейчас мы никогда сюда не попадем

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) 
	{
        ShowError("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}

/************************************************************************
*                                                                       *
*  DSSearch-Server default config                                       *
*                                                                       *
************************************************************************/


/************************************************************************
*                                                                       *
*  DSSearch-Server config                                               *
*                                                                       *
************************************************************************/
void search_config_read(const int8* file)
{
	int8 line[1024], w1[1024], w2[1024];
	FILE* fp;

	fp = fopen(file,"r");
	if( fp == NULL )
	{
		ShowError("configuration file not found at: %s\n", file);
		return;
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
		
		if (strcmp(w1,"mysql_host") == 0)
		{
			search_config.mysql_host = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_login") == 0)
		{
			search_config.mysql_login = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_password") == 0)
		{
			search_config.mysql_password = aStrdup(w2);
		}
		else if (strcmp(w1,"mysql_port") == 0)
		{
			search_config.mysql_port = atoi(w2);
		}
		else if (strcmp(w1,"mysql_database") == 0)
		{
			search_config.mysql_database = aStrdup(w2);
		}
		else
		{
			ShowWarning(CL_YELLOW"Unknown setting '%s' in file %s\n" CL_RESET, w1, file);
		}
	}
	fclose(fp);
}

/************************************************************************
*																		*
*																		*
*																		*
************************************************************************/

ppuint32 __stdcall TCPComm(void* lpParam)
{
	SearchCommInfo CommInfo = *((SearchCommInfo*)lpParam);
    
	CTCPRequestPacket* PTCPRequest = new CTCPRequestPacket(&CommInfo.socket);

	if (PTCPRequest->ReceiveFromSocket() == 0)
	{
		delete PTCPRequest;
		return 0;
	}
	
	ShowMessage(CL_GREEN"TCPComm: CASE_CALL: %u Size: %u \n" CL_RESET,PTCPRequest->GetPacketType(),PTCPRequest->GetSize());
	
	switch(PTCPRequest->GetPacketType()) 
	{
		case TCP_SEARCH:
		{
			ShowMessage(CL_GREEN"TCP_SEARCH\n" CL_RESET);
            
        }
        break;
		case TCP_SEARCH_ALL:
        {
			//SEARCH AREA CURRENT REGION
			ShowMessage(CL_GREEN"TCP_SEARCH_ALL\n" CL_RESET);
            HandleSearchRequest(PTCPRequest);
        }
        break;
        case TCP_SEARCH_COMMENT:
		{
			ShowMessage("Search comment \n");
            HandleSearchComment(PTCPRequest);
		}
		break;
		case TCP_GROUP_LIST:
		{
			ShowMessage("Search group\n");
			HandleGroupListRequest(PTCPRequest);
		}
		break;
		case TCP_AH_REQUEST:
			{
			ShowMessage(CL_GREEN"TCP_AH_REQUEST\n" CL_RESET);
            
        }
        case TCP_AH_REQUEST_MORE:
        {
			ShowMessage(CL_GREEN"TCP_AH_REQUEST_MORE\n" CL_RESET);
            HandleAuctionHouseRequest(PTCPRequest);
		}
		break;
		case TCP_AH_HISTORY_SINGL:
			{
			ShowMessage(CL_GREEN"TCP_AH_HISTORY_SINGL\n" CL_RESET);
            
        }
        case TCP_AH_HISTORY_STACK:
		{
			ShowMessage(CL_GREEN"TCP_AH_HISTORY_STACK\n" CL_RESET);
            HandleAuctionHouseHistoru(PTCPRequest);
		}
		break;
	}
	delete PTCPRequest;
	return 1;
}

/************************************************************************
*                                                                       *
*  Запрос списка персонажей (party/linkshell)                           *
*                                                                       *
************************************************************************/

void HandleGroupListRequest(CTCPRequestPacket* PTCPRequest)
{
	uint8* data = (uint8*)PTCPRequest->GetData();

    uint32 partyid = RBUFL(data,(0x10));
    uint32 linkshellid = RBUFL(data,(0x18));

	ShowMessage("SEARCH::PartyID = %u\n", partyid);
    ShowMessage("SEARCH::LinkshlellID = %u\n", linkshellid);

    CDataLoader* PDataLoader = new CDataLoader();

    if (partyid != 0)
    {
        std::list<SearchEntity*> PartyList = PDataLoader->GetPartyList(partyid);

        CPartyListPacket* PPartyPacket = new CPartyListPacket(partyid,PartyList.size());

        for (std::list<SearchEntity*>::iterator it = PartyList.begin(); it != PartyList.end(); ++it)
        {
			PPartyPacket->AddPlayer(*it);
        }

        PrintPacket((int8*)PPartyPacket->GetData(), PPartyPacket->GetSize());
        PTCPRequest->SendToSocket(PPartyPacket->GetData(), PPartyPacket->GetSize());

        delete PPartyPacket;
    }
    else if (linkshellid != 0)
    {	
        std::list<SearchEntity*> LinkshellList = PDataLoader->GetLinkshellList(linkshellid);

		CLinkshellListPacket* PLinkshellPacket = new CLinkshellListPacket(linkshellid,LinkshellList.size());

        for (std::list<SearchEntity*>::iterator it = LinkshellList.begin(); it != LinkshellList.end(); ++it)
        {
            PLinkshellPacket->AddPlayer(*it);
        }

        PrintPacket((int8*)PLinkshellPacket->GetData(), PLinkshellPacket->GetSize());
        PTCPRequest->SendToSocket(PLinkshellPacket->GetData(), PLinkshellPacket->GetSize());

        delete PLinkshellPacket;
    }
    delete PDataLoader;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

void HandleSearchComment(CTCPRequestPacket* PTCPRequest)
{
    uint8 packet[] = 
    {
        0xCC, 0x00, 0x00, 0x00, 0x49, 0x58, 0x46, 0x46, 0x20, 0x9B, 0x16, 0xC8, 0x4C, 0x76, 0x07, 0x02, 
        0x17, 0x71, 0xB9, 0xA8, 0xF5, 0xB6, 0xCF, 0xED, 0xF1, 0xFF, 0x70, 0x52, 0xA9, 0xAE, 0x81, 0xB6, 
        0x1B, 0x2B, 0x7B, 0xA0, 0xC1, 0xD2, 0xD1, 0xFD, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 
        0x61, 0x43, 0x80, 0x37, 0x08, 0x74, 0xC5, 0x8D, 0x0C, 0x04, 0x13, 0xC0, 0x89, 0x17, 0x8F, 0x72, 
        0x93, 0xD6, 0x90, 0xF1, 0x21, 0x7A, 0xA5, 0xAC, 0x93, 0xD6, 0x90, 0xF1, 0x21, 0x7A, 0xA5, 0xAC, 
        0x93, 0xD6, 0x90, 0xF1, 0x21, 0x7A, 0xA5, 0xAC, 0x38, 0x25, 0x69, 0x79, 0x00, 0xC6, 0x7E, 0xDC, 
        0x80, 0x3D, 0x99, 0x85, 0xF4, 0xDF, 0xCF, 0xFC, 0x1A, 0x72, 0xE2, 0x0D 
    };
    PTCPRequest->SendRawToSocket(packet, sizeof(packet));
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

void HandleSearchRequest(CTCPRequestPacket* PTCPRequest)
{   
	search_req sr = _HandleSearchRequest(PTCPRequest,NULL);
	int totalCount = 0;

    CDataLoader* PDataLoader = new CDataLoader();
    std::list<SearchEntity*> SearchList = PDataLoader->GetPlayersList(sr,&totalCount);
	//PDataLoader->GetPlayersCount(sr)
    CSearchListPacket* PSearchPacket = new CSearchListPacket(totalCount);

    for (std::list<SearchEntity*>::iterator it = SearchList.begin(); it != SearchList.end(); ++it)
    {
        PSearchPacket->AddPlayer(*it);
    }

    //PrintPacket((int8*)PSearchPacket->GetData(), PSearchPacket->GetSize());
    PTCPRequest->SendToSocket(PSearchPacket->GetData(), PSearchPacket->GetSize());

    delete PSearchPacket;
    delete PDataLoader;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

void HandleAuctionHouseRequest(CTCPRequestPacket* PTCPRequest)
{
    uint8* data    = (uint8*)PTCPRequest->GetData();                            
	uint8  AHCatID = RBUFB(data,(0x16));

    //2 - уровень -- level
    //3 - раса -- race
    //4 - профессия -- job
    //5 - урон -- damage
    //6 - задержка -- delay
    //7 - защита -- defense
    //8 - сопротивление -- resistance
    //9 - название -- name
	string_t OrderByString = "ORDER BY";
	uint8 paramCount = RBUFB(data,0x12);
    for (uint8 i = 0; i < paramCount; ++i) // параметры сортировки предметов
    {
		uint8 param = RBUFL(data,(0x18)+8*i);
        ShowMessage(" Param%u: %u\n", i, param);
		switch (param) {
			case 2:
				OrderByString.append(" item_armor.level DESC,");
			case 5:
				OrderByString.append(" item_weapon.dmg DESC,");
			case 6:
				OrderByString.append(" item_weapon.delay DESC,");
			case 9:
				OrderByString.append(" item_basic.sortname,");
		}
    }

	OrderByString.append(" item_basic.itemid");
	int8* OrderByArray = (int8*)OrderByString.data();

	CDataLoader* PDataLoader = new CDataLoader();                        
    std::vector<ahItem*> ItemList = PDataLoader->GetAHItemsToCategory(AHCatID,OrderByArray);

    uint8 PacketsCount = (ItemList.size() / 20) + (ItemList.size() % 20 != 0) + (ItemList.size() == 0);

    for(uint8 i = 0; i < PacketsCount; ++i) 
    {
        CAHItemsListPacket* PAHPacket = new CAHItemsListPacket(20*i);

        PAHPacket->SetItemCount(ItemList.size());  

        for (uint16 y = 20*i; (y != 20*(i+1)) && (y < ItemList.size()); ++y)
        {
            PAHPacket->AddItem(ItemList.at(y));
        }

        PTCPRequest->SendToSocket(PAHPacket->GetData(), PAHPacket->GetSize());
        delete PAHPacket;
    }
    delete PDataLoader;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

void HandleAuctionHouseHistoru(CTCPRequestPacket* PTCPRequest)
{
    uint8* data   = (uint8*)PTCPRequest->GetData();                            
	uint16 ItemID = RBUFW(data,(0x12));
    uint8  stack  = RBUFB(data,(0x15));

	CAHHistoryPacket* PAHPacket = new CAHHistoryPacket(ItemID);

    CDataLoader* PDataLoader = new CDataLoader();
    std::vector<ahHistory*> HistoryList = PDataLoader->GetAHItemHystory(ItemID, stack != 0);

	for (uint8 i = 0; i < HistoryList.size(); ++i)
	{
		PAHPacket->AddItem(HistoryList.at(i));
	}

    PTCPRequest->SendToSocket(PAHPacket->GetData(), PAHPacket->GetSize());

    delete PDataLoader;
    delete PAHPacket;
}

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

search_req _HandleSearchRequest(CTCPRequestPacket* PTCPRequest, SOCKET socket)
{
	// суть в том, чтобы заполнить некоторую структуру, на основании которой будет создан запрос к базе
	// результат поиска в базе отправляется клиенту

	uint32 bitOffset = 0;

	unsigned char sortDescending = 0;
	unsigned char isPresent = 0;
	unsigned char areaCount = 0;

	uint8 name[16];
	uint8 nameLen = 0;
	uint8 minLvl = 0;
	uint8 maxLvl = 0;
	uint8 jobid = 0;
    uint16 areas[10];

	uint8* data = (uint8*)PTCPRequest->GetData();
	uint8  size = RBUFB(data,(0x10));

	uint16 workloadBits = size * 8;

    memset(areas, 0, sizeof(areas));
	//ShowMessage("Received a search packet with size %u byte\n", size);
	
	while(bitOffset < workloadBits)
	{
		if ((bitOffset+5) >= workloadBits)
		{
			bitOffset = workloadBits;
			break;
			
		}

		uint8 EntryType = (uint8)unpackBitsLE(&data[0x11], bitOffset, 5);
		bitOffset+=5;

		if ((EntryType != SEARCH_FRIEND) &&
			(EntryType != SEARCH_LINKSHELL) &&
			(EntryType != SEARCH_COMMENT) &&
			(EntryType != SEARCH_FLAGS2))		
		{
			if ((bitOffset+3) >= workloadBits) //so 0000000 at the end does not get interpretet as name entry ...
			{
				bitOffset=workloadBits;
				break;
			}
			sortDescending = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,1);
			bitOffset+=1;

			isPresent = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,1);
			bitOffset+=1;
		}

		switch(EntryType)
		{
			case SEARCH_NAME:
			{
				if (isPresent==0x1) //Name send
				{
					if ((bitOffset+5) >= workloadBits)
					{
						bitOffset=workloadBits;
						break;
					}
					nameLen = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,5);
					name[nameLen]='\0';
					
					bitOffset+=5;

					for (unsigned char i = 0; i < nameLen; i++)
					{
						name[i] = (char)unpackBitsLE(&data[0x11],bitOffset,7);
						bitOffset+=7;
					}	
					ShowStatus(CL_YELLOW"SEARCH::Name Entry Found. (%s).\n"CL_RESET,name);
					
				}
				
				break;
			}			
			case SEARCH_AREA: //Area Code Entry - 10 bit
			{
				if (isPresent == 0) //no more Area entries
				{
					//printf("SEARCH::Area List End found.\n");
				}
				else // 8 Bit = 1 Byte per Area Code
				{
					areas[areaCount] = (uint16)unpackBitsLE(&data[0x11],bitOffset,10);
					areaCount++;
					bitOffset+=10;
				//	printf("SEARCH::Area List Entry found(%2X)!\n",areas[areaCount-1]);
				}
				break;
			}
			case SEARCH_NATION: //Country - 2 bit
			{
				if (isPresent==0x1)
				{
					unsigned char country = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,2);
					bitOffset+=2;

					printf("SEARCH::Nationality Entry found. (%2X) Sorting: (%s).\n",country,(sortDescending==0x00)?"ascending":"descending");
				}
				break;
			}
			case SEARCH_JOB: //Job - 5 bit
			{
				if (isPresent==0x1)
				{
					unsigned char job = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,5);
					bitOffset+=5;
					jobid = job;
					//printf("SEARCH::Job Entry found. (%2X) Sorting: (%s).\n",job,(sortDescending==0x00)?"ascending":"descending");
				}
				//packetData.sortDescendingByJob=sortDescending;
				//printf("SEARCH::SortByJob: %s.\n",(sortDescending==0x00)?"ascending":"descending");
				break;
			}
			case SEARCH_LEVEL: //Level- 16 bit
			{
				if (isPresent==0x1)
				{
					unsigned char fromLvl = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,8);
					bitOffset+=8;
					unsigned char toLvl = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,8);
					bitOffset+=8;
					minLvl = fromLvl;
					maxLvl = toLvl;
					//printf("SEARCH::Level Entry found. (%d - %d) Sorting: (%s).\n",fromLvl,toLvl,(sortDescending==0x00)?"ascending":"descending");
				}
				//packetData.sortDescendingByLevel=sortDescending;
				//printf("SEARCH::SortByLevel: %s.\n",(sortDescending==0x00)?"ascending":"descending");
				break;
			}
			case SEARCH_RACE: //Race - 4 bit
			{
				if (isPresent==0x1)
				{
					unsigned char race = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,4);
					bitOffset+=4;
					printf("SEARCH::Race Entry found. (%2X) Sorting: (%s).\n",race,(sortDescending==0x00)?"ascending":"descending");
				}
				printf("SEARCH::SortByRace: %s.\n",(sortDescending==0x00)?"ascending":"descending");
				//packetData.sortDescendingByRace=sortDescending;
				break;
			}
			case SEARCH_RANK: //Rank - 2 byte
			{
				if (isPresent==0x1)
				{
					unsigned char fromRank = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,8);
					bitOffset+=8;
					unsigned char toRank = (unsigned char)unpackBitsLE(&data[0x11],bitOffset,8);
					bitOffset+=8;

					printf("SEARCH::Rank Entry found. (%d - %d) Sorting: (%s).\n",fromRank,toRank,(sortDescending==0x00)?"ascending":"descending");
				}
				printf("SEARCH::SortByRank: %s.\n",(sortDescending==0x00)?"ascending":"descending");
				//packetData.sortDescendingByRank=sortDescending;
				break;
			}
			case SEARCH_COMMENT: //4 Byte
			{
				unsigned int comment = (unsigned int)unpackBitsLE(&data[0x11],bitOffset,32); 
				bitOffset+=32;

				printf("SEARCH::Comment Entry found. (%8X).\n",comment);
				break;
			}
			//the following 4 Entries were generated with /sea (ballista|friend|linkshell|away|inv) 
			//so they may be off
			case SEARCH_LINKSHELL: // 4 Byte
			{
				unsigned int lsId= (unsigned int)unpackBitsLE(&data[0x11],bitOffset,32);
				bitOffset+=32;

				printf("SEARCH::Linkshell Entry found. Value: %.8X\n",lsId);
				break;
			}
			case SEARCH_FRIEND: // Friend Packet, 0 byte
			{
				printf("SEARCH::Friend Entry found.\n");
				break;
			}
			case SEARCH_FLAGS1: // Flag Entry #1, 2 byte, 
			{ 
				if (isPresent==0x1)
				{
					unsigned short flags1 = (unsigned short)unpackBitsLE(&data[0x11],bitOffset,16);
					bitOffset+=16;

					printf("SEARCH::Flag Entry #1 (%.4X) found. Sorting: (%s).\n",flags1,(sortDescending==0x00)?"ascending":"descending");
				}
				printf("SEARCH::SortByFlags: %s\n",(sortDescending == 0? "ascending" : "descending"));
				//packetData.sortDescendingByFlags=sortDescending;
				break;
			}
			case SEARCH_FLAGS2: // Flag Entry #2 - 4 byte
			{
				unsigned int flags=(unsigned int)unpackBitsLE(&data[0x11],bitOffset,32);

				bitOffset+=32;
				
				break;
			}
			default:
			{
				printf("SEARCH::Unknown Search Param %.2X!\n",EntryType);
				//outputPacket=true;
				break;
			}
		}
	}
	printf("\n");

	ShowMessage("Name: %s Job: %u Lvls: %u ~ %u \n",(nameLen>0 ? name : 0),jobid,minLvl,maxLvl);

	search_req sr;
	sr.jobid = jobid;
	sr.maxlvl = maxLvl;
	sr.minlvl = minLvl;
	sr.nameLen = nameLen;
	memcpy(sr.zoneid, areas, sizeof(sr.zoneid));
	if(nameLen>0){
		sr.name.insert(0,(int8*)name);
	}

	return sr;
	// не обрабатываем последние биты, что мешает в одну кучу например "/blacklist delete Name" и "/sea all Name"
}

