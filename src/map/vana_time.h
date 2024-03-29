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

#ifndef _CVANATIME_H
#define _CVANATIME_H

#include "../common/cbasetypes.h"

enum DAYTYPE
{
	FIRESDAY		= 0,
	EARTHSDAY		= 1,
	WATERSDAY		= 2,
	WINDSDAY		= 3,
	ICEDAY			= 4,
	LIGHTNINGDAY	= 5,
	LIGHTSDAY		= 6,
	DARKSDAY		= 7
};

enum TIMETYPE
{
    TIME_NONE       = 0,
    TIME_MIDNIGHT   = 1,
    TIME_FOG        = 2,
    TIME_NEWDAY     = 3,
    TIME_DAWN       = 4,
    TIME_DAY        = 5,
    TIME_DUSK       = 6,
    TIME_EVENING    = 7,
    TIME_NIGHT      = 8
};

class CVanaTime
{
public:

	static	CVanaTime * getInstance();
	static	CVanaTime *getElevatorInstance();

	TIMETYPE SyncTime();
	TIMETYPE GetCurrentTOTD();

	uint32	 getDate();
	uint32	 getYear();
	uint32	 getMonth();
	uint32	 getDayOfTheMonth();
	uint32	 getHour();
	uint32	 getMinute();
	uint32	 getWeekday();
	uint32	 getMoonPhase();
	uint8	 getMoonDirection();
	uint32	 getSysTime();							// Timestamp of the system time
	uint32	 getSysHour();
	uint32	 getSysMinute();
	uint32	 getSysSecond();
	uint32	 getSysWeekDay();						// Number of day since sunday
	uint32	 getSysYearDay();						// Number of day since 1st january
    uint32   getVanaTime();
	uint32   getVanaElevatorTime();
	int32	 getCustomOffset();

	void	 setCustomOffset(int32 offset);

//private:

	static CVanaTime * _instance;

	CVanaTime();

	uint32	 m_vYear;								// Vanadiel Year
	uint32	 m_vMon;								// Vanadiel Month
	uint32	 m_vDate;								// Vanadiel Date (day of the month)
	uint32	 m_vHour;								// Vanadiel Hour
	uint32	 m_vMin;								// Vanadiel Minute
	uint32	 m_vDay;								// Vanadiel Day (Day of the week, fire,earth,wind)
	uint32	 m_vanaDate;							// Vanadiel time in integer format

	TIMETYPE m_TimeType;							// текущий тип времени

	int32	 m_customOffset;						// Смещение игрового времени в игровых минутах
};

#endif