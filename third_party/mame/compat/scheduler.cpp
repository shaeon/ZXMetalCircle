//
// scheduler.cpp - Temporizadores de la capa de compatibilidad con MAME
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "emu.h"

const attotime attotime::never (attotime::NEVER);
const attotime attotime::zero (0);

attotime mame_scheduler::s_now;
attotime mame_scheduler::s_next (attotime::NEVER);
bool mame_scheduler::s_bDirty = true;
emu_timer *mame_scheduler::s_list = 0;

void mame_scheduler::add (emu_timer *timer)
{
	timer->m_next = s_list;
	s_list = timer;
}

void mame_scheduler::remove (emu_timer *timer)
{
	for (emu_timer **pp = &s_list; *pp != 0; pp = &(*pp)->m_next)
	{
		if (*pp == timer)
		{
			*pp = timer->m_next;
			return;
		}
	}
}

// Ejecuta, en orden de vencimiento, los temporizadores que venzan hasta t
void mame_scheduler::advance_to (const attotime &t)
{
	// camino rápido: ningún temporizador vence antes de t
	if (!s_bDirty && t < s_next)
	{
		if (t > s_now)
		{
			s_now = t;
		}
		return;
	}

	for (unsigned nGuard = 0; nGuard < 100000; nGuard++)
	{
		emu_timer *pFirst = 0;
		for (emu_timer *p = s_list; p != 0; p = p->m_next)
		{
			if (p->m_enabled && p->m_expire <= t && (pFirst == 0 || p->m_expire < pFirst->m_expire))
			{
				pFirst = p;
			}
		}

		if (pFirst == 0)
		{
			break;
		}

		if (pFirst->m_expire > s_now)
		{
			s_now = pFirst->m_expire;
		}

		s_bDirty = true;
		if (pFirst->m_period.is_never ())
		{
			pFirst->m_enabled = false;
		}
		else
		{
			pFirst->m_expire = pFirst->m_expire + pFirst->m_period;
		}

		pFirst->m_thunk (pFirst->m_obj, pFirst->m_param);
	}

	if (t > s_now)
	{
		s_now = t;
	}

	// recalcula el próximo vencimiento
	s_next = attotime::never;
	for (emu_timer *p = s_list; p != 0; p = p->m_next)
	{
		if (p->m_enabled && p->m_expire < s_next)
		{
			s_next = p->m_expire;
		}
	}
	s_bDirty = false;
}
