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

#ifndef _CHARENTITY_H
#define _CHARENTITY_H

#include "../../common/cbasetypes.h"
#include "../../common/mmo.h"

#include <map>
#include <list>
#include <deque>

#include "battleentity.h"
#include "../item_container.h"
#include "../linkshell.h"
#include "petentity.h"
#include "automatonentity.h"

#include "../recast_container.h"
#include "../latent_effect_container.h"
#include "../trade_container.h"
#include "../treasure_pool.h"
#include "../merit.h"
#include "../universal_container.h"
#include "../utils/itemutils.h"

// Quest Areas

enum QUESTAREA
{
	QUESTS_SANDORIA		= 0,
	QUESTS_BASTOK		= 1,
	QUESTS_WINDURST		= 2,
	QUESTS_JEUNO		= 3,
	QUESTS_OTHER		= 4,
	QUESTS_OUTLANDS		= 5,
	QUESTS_AHTURHGAN	= 6,
	QUESTS_CRYSTALWAR	= 7,
	QUESTS_ABYSSEA		= 8,
	QUESTS_ADOULIN		= 9,
	QUESTS_COALITION	= 10
};

#define MAX_QUESTAREA	 11
#define MAX_QUESTID     256
#define MAX_MISSIONAREA	 10
#define MAX_MISSIONID    95

struct jobs_t
{
	uint32 unlocked;				// битовая маска профессий, доступных персонажу (первый бит - дополнительная профессия)
	uint8  job[MAX_JOBTYPE];		// текущий уровень для каждой из профессий
	uint16 exp[MAX_JOBTYPE];		// текущее количество опыта для каждой из профессий
	uint8  genkai;					// максимальный уровень профессий персонажа
};


struct event_t
{
	int32 EventID;                  // номер события
    int32 Option;                   // фиктивный возвращаемый результат

    CBaseEntity* Target;            // инициатор события

	string_t Script;                // путь к файлу, отвечающему за обработку события
	string_t Function;              // не используется

	void reset()
    {
		EventID = -1;
        Option  =  0;
        Target  =  0;
		Script.clear();
		Function.clear();
	}
};

struct profile_t
{
	uint8	   nation;			// принадлежность к государству
	uint8	   mhflag;			// флаг выхода из MogHouse
	uint16	   title;			// звание
	uint16     fame[4];			// известность
	uint8 	   rank[3];			// рагн в трех государствах
	uint32	   rankpoints;	    // очки ранга в трех государствах
	location_t home_point;		// точка возрождения персонажа
};


struct expChain_t
{
	uint16 chainNumber;
	uint32 chainTime;
};

struct NationTP_t
{
	uint32		sandoria;
	uint32		bastok;
	uint32		windurst;
	uint32		ahturhgan;
	uint32		maw;
	uint32		pastsandoria;
	uint32		pastbastok;
	uint32		pastwindurst;
};


struct PetInfo_t
{
	bool		respawnPet;		// used for spawning pet on zone
	uint8		petID;			// id as in wyvern(48) , carbuncle(8) ect..
	PETTYPE		petType;		// type of pet being transfered
	int32		petHP;			// pets hp
	float		petTP;			// pets tp
};

struct AuctionHistory_t
{
	uint16		itemid;
	uint8		stack;
	uint32		price;
	uint8		status; //e.g. if sold/not sold/on market
};

struct UnlockedAttachments_t
{
	uint8 heads;
	uint8 frames;
	uint32 attachments[8];
};

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

class CBasicPacket;

typedef std::deque<CBasicPacket*> PacketList_t;
typedef std::map<uint32,CBaseEntity*> SpawnIDList_t;
typedef std::vector<EntityID_t> BazaarList_t;

class CCharEntity : public CBattleEntity
{
public:

	jobs_t					jobs;							// доступрые профессии персонажа
	keyitems_t				keys;							// таблица ключевых предметов
	event_t					m_event;						// структура для запуска событый
	skills_t				RealSkills;						// структура всех реальных умений персонажа, с точностью до 0.1 и не ограниченных уровнем
	nameflags_t				nameflags;						// флаги перед именем персонажа
	profile_t				profile;						// профиль персонажа (все, что связывает города и персонажа)
	expChain_t				expChain;						// Exp Chains
	search_t				search;							// данные и комментарий, отображаемые в окне поиска
	bazaar_t				bazaar;							// все данные, необходимые для таботы bazaar
	uint16					m_EquipFlag;					// текущие события, обрабатываемые экипировкой (потом упакую в структуру, вместе с equip[])
    uint16					m_EquipBlock;					// заблокированные ячейки экипировки
	bool					m_EquipSwap;					// true if equipment was recently changed
	uint8					equip[17];						// экипировка персонажа

	uint8					m_ZonesList[36];				// список посещенных персонажем зон
	uint8					m_SpellList[128];				// список изученных заклинаний
    uint8					m_TitleList[94];				// список заслуженных завний
	uint8					m_Abilities[46];				// список текущих способностей
	uint8					m_LearnedAbilities[46];			// learnable abilities (corsair rolls)
	uint8					m_TraitList[16];				// список постянно активных способностей в виде битовой маски
    uint8					m_PetCommands[32];				// список доступных команд питомцу
	uint8					m_WeaponSkills[32];
	questlog_t				m_questLog[MAX_QUESTAREA];		// список всех квестов
	missionlog_t			m_missionLog[MAX_MISSIONAREA];	// список миссий
	assaultlog_t			m_assaultLog;					// список assault миссий
	campaignlog_t			m_campaignLog;					// список campaing миссий
    uint32					m_rangedDelay;					// ranged attack delay (with timestamp for repeat attacks, hence 32bit)for items, abilities and magic
	bool					m_insideBCNM;					// true if user is inside a bcnm
	uint32					m_lastBcnmTimePrompt;			// the last message prompt in seconds
	PetInfo_t				petZoningInfo;					// used to repawn dragoons pets ect on zone
	void					resetPetZoningInfo();			// reset pet zoning info (when changing job ect)
	uint8					m_SetBlueSpells[20];			// The 0x200 offsetted blue magic spell IDs which the user has set. (1 byte per spell)
	UnlockedAttachments_t	m_unlockedAttachments;			// Unlocked Automaton Attachments (1 bit per attachment)
    string_t                m_AutomatonName;                // TEMP AUTOMATON NAME

	// Эти миссии не нуждаются в списке пройденных, т.к. клиент автоматически
	// отображает более ранние миссии выплненными

	uint16			  m_copCurrent;					// текущая миссия Chains of Promathia
	uint16			  m_acpCurrent;					// текущая миссия A Crystalline Prophecy
	uint16			  m_mkeCurrent;					// текущая миссия A Moogle Kupo d'Etat
	uint16			  m_asaCurrent;					// текущая миссия A Shantotto Ascension

    // TODO: половина этого массива должна храниться в char_vars, а не здесь, т.к. эта информация не отображается в интерфейсе клиента и сервер не проводит с ними никаких операций

    uint32            RegionPoints[15];             // conquest points, imperial standing points etc
	NationTP_t		  nationtp;						// supply tp, runic portal, campaign tp,...

    uint8             GetGender();                  // узнаем пол персонажа

	int32			  firstPacketSize();            // размер первого пакета в PacketList
    void              clearPacketList();            // отчистка PacketList
    void              pushPacket(CBasicPacket*);    // добавление копии пакета в PacketList
	bool			  isPacketListEmpty();          // проверка размера PacketList
	CBasicPacket*	  popPacket();                  // получение первого пакета из PacketList

    CLinkshell*       PLinkshell;                   // linkshell, в которой общается персонаж
	CTreasurePool*	  PTreasurePool;                // сокровища, добытые с монстров
    CMeritPoints*     PMeritPoints;                 //
	bool			  MeritMode;					//If true then player is meriting

    CRecastContainer* PRecastContainer;             //

	CLatentEffectContainer* PLatentEffectContainer;

	CItemContainer*   PGuildShop;					// текущий магазин гильдии, в котором персонаж производит закупки
	CItemContainer*	  getStorage(uint8 LocationID);	// получение указателя на соответствующее хранилище

	CTradeContainer*  Container;                    // универсальный контейнер для обмена, синтеза, магазина и т.д.
	CUContainer*	  UContainer;					// новый универсальный контейнер для обмена, синтеза, магазина и т.д.

	CBaseEntity*	  PWideScanTarget;				// wide scane цель

	SpawnIDList_t	  SpawnPCList;					// список видимых персонажей
	SpawnIDList_t	  SpawnMOBList;					// список видимых монстров
	SpawnIDList_t	  SpawnPETList;					// список видимых питомцев
	SpawnIDList_t	  SpawnNPCList;					// список видимых npc

	void			  SetName(int8* name);			// устанавливаем имя персонажа (имя ограничивается 15-ю символами)

    EntityID_t        TradePending;                 // ID персонажа, предлагающего обмен
	EntityID_t        InvitePending;                // ID персонажа, отправившего приглашение в группу
    EntityID_t        BazaarID;                     // Pointer to the bazaar we are browsing.
	BazaarList_t	  BazaarCustomers;              // Array holding the IDs of the current customers

	uint32			  m_InsideRegionID;				// номер региона, в котором сейчас находится персонаж (??? может засунуть в m_event ???)
	uint8			  m_LevelRestriction;			// ограничение уровня персонажа
    uint16            m_Costum;                     // карнавальный костюм персонажа (модель)
	uint32			  m_AHHistoryTimestamp;			// Timestamp when last asked to view history
	uint32			  m_DeathTimestamp;				// Timestamp when you last died. This is set when you first login.

    uint8             m_PVPFlag;                    // pvp
	uint8			  m_hasTractor;					// checks if player has tractor already
	uint8			  m_hasRaise;					// checks if player has raise already
    uint8             m_hasAutoTarget;              // возможность использования AutoTarget функции
	position_t		  m_StartActionPos;				// позиция начала действия (использование предмета, начало стрельбы, позиция tractor)

	uint8			  m_GMlevel;                    // Level of the GM flag assigned to this character
	int32             Account_Level;
	int32             SetExpRates;
	int32             first_login;
	int32             is_zoning;
	int32             is_returning;
	int32             is_inevent;
	bool              is_being_chased;
	int32             is_dead;
	int32             online_status;
	int32             shutdown_status;
	int32             accid;
	int32			  godmode;
	int32			  eventid;
	int32             Rage_Mode;
	int32             Is_Public_0_Or_Private_1;

	int8			  getShieldSize();

	bool			getWeaponSkillKill();
	void			setWeaponSkillKill(bool isWeaponSkillKill);
	bool			getMijinGakure();
	void			setMijinGakure(bool isMijinGakure);

	int32           leavegame();

	bool			  isWeaponUnlocked(uint16 indexid);					// return if weapon is broken
	bool			  addWsPoints(uint8 points, uint16 WeaponIndex);	// return if weapon is broken
	UnlockedWeapons_t unlockedWeapons[MAX_UNLOCKABLE_WEAPONS];			// chars unlocked weapon status
	CAutomatonEntity*       PAutomaton;                     // Automaton statistics

	uint16 addTP(float tp);
	int32 addHP(int32 hp);
	int32 addMP(int32 mp);

    std::vector<AuctionHistory_t> m_ah_history;		// AH history list (в будущем нужно использовать UContainer)

	void SetPlayTime(uint32);						// Set playtime from database
	void AddPlayTime();								// Add playtime every second
	uint32 __inline const&	GetPlayTime() { return m_PlayTime; };

	 CCharEntity();									// конструктор
	~CCharEntity();									// деструктор

private:

	CItemContainer*   m_Inventory;
	CItemContainer*   m_Mogsafe;
	CItemContainer*   m_Storage;
	CItemContainer*	  m_Tempitems;
	CItemContainer*   m_Moglocker;
	CItemContainer*	  m_Mogsatchel;
	CItemContainer*	  m_Mogsack;

	bool			m_isWeaponSkillKill;
	bool			m_isMijinGakure;

	PacketList_t      PacketList;					// в этом списке хранятся все пакеты, предназначенные для отправки персонажу
	uint32 m_ZoneTimer;
	uint32 m_PlayTime;
};

#endif
