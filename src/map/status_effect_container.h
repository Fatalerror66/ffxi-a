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

#ifndef _STATUSEFFECTCONTAINER_H
#define _STATUSEFFECTCONTAINER_H

#include "../common/cbasetypes.h"
#include "../common/taskmgr.h"

#include "status_effect.h"

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

class CBattleEntity;

class CStatusEffectContainer
{
public:

	uint64	m_Flags;											// биты переполнения байтов m_StatusIcons (по два бита на каждый эффект)
  uint8 m_StatusIcons[32];                  // иконки статус-эффектов

  bool ApplyBardEffect(CStatusEffect* PStatusEffect, uint8 maxSongs);
  bool ApplyCorsairEffect(CStatusEffect* PStatusEffect, uint8 maxRolls, uint8 bustDuration);
    bool CanGainStatusEffect(EFFECT statusEffect, uint16 power); // returns true if the status effect will take effect
  bool AddStatusEffect(CStatusEffect* StatusEffect, bool silent = false);
  bool DelStatusEffect(EFFECT StatusID);
  bool DelStatusEffectSilent(EFFECT StatusID);
  bool DelStatusEffect(EFFECT StatusID, uint16 SubID);
    void DelStatusEffectsByFlag(uint16 flag, bool silent = false);                   // удаляем все эффекты с указанным типом
    void DelStatusEffectsByIcon(uint16 IconID);                 // удаляем все эффекты с указанной иконкой
    void DelStatusEffectsByType(uint16 Type);
  bool DelStatusEffectWithPower(EFFECT StatusID, uint16 power);
  void KillAllStatusEffect();

    bool HasStatusEffect(EFFECT StatusID);                      // проверяем наличие эффекта
  bool HasStatusEffect(EFFECT StatusID, uint16 SubID);        // проверяем наличие эффекта с уникальным subid
    bool HasStatusEffectByFlag(uint16 flag);

  EFFECT EraseStatusEffect();                                 // удаляем первый отрицательный эффект
    uint8 EraseAllStatusEffect();               // erases all status effects
  EFFECT DispelStatusEffect();                                // удаляем первый положительный эффект
    uint8 DispelAllStatusEffect();                // dispels all status effects
    CStatusEffect* StealStatusEffect();             // dispels one effect and returns it

  CStatusEffect* GetStatusEffect(EFFECT StatusID);
    CStatusEffect* GetStatusEffect(EFFECT StatusID, uint16 SubID);

    void UpdateStatusIcons();                                   // пересчитываем иконки эффектов
  void CheckEffects(uint32 tick);
  void CheckRegen(uint32 tick);

  void LoadStatusEffects();                 // загружаем эффекты персонажа
  void SaveStatusEffects();                 // сохраняем эффекты персонажа

    uint8 GetEffectsCount(uint16 SubID);                        // получаем количество эффектов с указанным subid

  void WakeUp(); // remove sleep effects
  bool CheckForElevenRoll();
  bool IsAsleep();
  bool HasBustEffect(uint16 id);
  bool HasPreventActionEffect(); // checks if owner has an effect that prevents actions, like stun, petrify, sleep etc

	 CStatusEffectContainer(CBattleEntity* PEntity);
	~CStatusEffectContainer();

private:

	CBattleEntity* m_POwner;

    // void ReplaceStatusEffect(EFFECT effect); //this needs to be implemented
	void RemoveStatusEffect(uint32 id, bool silent = false);	// удаляем эффект по его номеру в контейнере
	void SetEffectParams(CStatusEffect* StatusEffect);			// устанавливаем имя эффекта

    void OverwriteStatusEffect(CStatusEffect* StatusEffect);

	uint32 m_EffectCheckTime;
	uint32 m_RegenCheckTime;

	std::vector<CStatusEffect*>	m_StatusEffectList;
};

/************************************************************************
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

namespace effects
{
    void LoadEffectsParameters();
    uint16 GetEffectElement(uint16 effect);
};

#endif

