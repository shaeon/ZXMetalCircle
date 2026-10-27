//
// earinput.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "earinput.h"

CEarInput::CEarInput (void)
:	m_bEnabled (FALSE),
	m_bStartLevel (TRUE),
	m_bLastLevel (TRUE),
	m_nEdges (0),
	m_nCursor (0),
	m_nQuietFrames (1000)
{
}

void CEarInput::BeginFrame (void)
{
	if (m_nEdges == 0)
	{
		if (m_nQuietFrames < 1000)
		{
			m_nQuietFrames++;
		}
	}
	else
	{
		m_nQuietFrames = 0;
	}

	m_bStartLevel = m_bLastLevel;
	m_nEdges = 0;
	m_nCursor = 0;
}

void CEarInput::AddEdge (unsigned nTstate, boolean bLevel)
{
	if (bLevel == m_bLastLevel || m_nEdges >= EAR_MAX_EDGES)
	{
		return;			// no es un cambio (o no cabe)
	}

	m_Time[m_nEdges] = nTstate;
	m_Level[m_nEdges] = bLevel ? 1 : 0;
	m_nEdges++;
	m_bLastLevel = bLevel;
}

boolean CEarInput::GetLevel (unsigned nTstate)
{
	// si se pregunta por un instante anterior, se vuelve a buscar desde el principio
	if (m_nCursor > 0 && m_Time[m_nCursor - 1] > nTstate)
	{
		m_nCursor = 0;
	}

	while (m_nCursor < m_nEdges && m_Time[m_nCursor] <= nTstate)
	{
		m_nCursor++;
	}

	return m_nCursor == 0 ? m_bStartLevel : m_Level[m_nCursor - 1] != 0;
}
