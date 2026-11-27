//
// tapedeck.cpp - El casete del Spectrum: control de la cinta, carga y
// grabación instantáneas, aceleración de cargadores, arranque y parada
// automáticos y grabación por pulsos
//
// La carga instantánea y la aceleración de cargadores están adaptadas de
// Fuse (tape_traps.c, loader.c y loader_detector.c, © 2006-2018 Philip
// Kendall y otros, GPL 2 o posterior)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "tapedeck.h"
#include <circle/util.h>

// Detección de carga desde la cinta
#define AUTOSTOP_FRAMES		100	// 2 s sin cargar -> se para la cinta
#define AUTOPLAY_FRAMES		10
#define LOADER_READS_PLAY	500	// lecturas de cargador por cuadro para arrancar la cinta
#define LOADER_READS_ACTIVE	20	// por debajo, durante 2 s, se para

CTapeDeck::CTapeDeck (CTape *pTape, Z80 &rCPU, const CMemory &rMem, const CDivMMC &rDiv,
		      TWriteByte *pWrite, void *pWriteParam)
:	m_pTape (pTape),
	m_rCPU (rCPU),
	m_rMem (rMem),
	m_rDiv (rDiv),
	m_pWrite (pWrite),
	m_pWriteParam (pWriteParam),
	m_pEarInput (0),
	m_pSaveHandler (0),
	m_Mode (TapeModeFast),
	m_bLoaderAccel (TRUE),
	m_bRecording (FALSE),
	m_bRecordEar (FALSE),
	m_pRecBuffer (0),
	m_nRecLength (0),
	m_nRecCapacity (0),
	m_nRecPulses (0),
	m_nRecLastEdge (0),
	m_nRecRemainder (0),
	m_nIdleFrames (0),
	m_nLoadingFrames (0),
	m_bAutoStopped (TRUE),
	m_AccelMode (AccelNone),
	m_nAccelPC (0),
	m_nAccelEdges (0),
	m_nSeenEdge (0),
	m_nFrameEdges (0),
	m_nObservedEdges (0),
	m_nLoaderReads (0),
	m_nLastFEPC (0),
	m_nLastFEPort (0)
{
	memset (m_LastFERegs, 0, sizeof m_LastFERegs);
}

CTapeDeck::~CTapeDeck (void)
{
	FreeRecording ();
	m_pTape = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Mandos

void CTapeDeck::Play (u64 nNow)
{
	m_AccelMode = AccelNone;
	m_pTape->Play (nNow);
	m_bAutoStopped = FALSE;
	m_nIdleFrames = 0;
	m_nLoadingFrames = 0;
}

void CTapeDeck::Stop (void)
{
	m_pTape->Stop ();
	m_bAutoStopped = FALSE;		// parada manual: no arrancar sola
}

void CTapeDeck::Inserted (void)
{
	m_pTape->Stop ();
	m_bAutoStopped = TRUE;
	m_nIdleFrames = 0;
	m_nLoadingFrames = 0;
}

boolean CTapeDeck::IsAccelerating (void) const
{
	return m_Mode != TapeModeNormal && m_pTape->IsPlaying ();
}

////////////////////////////////////////////////////////////////////////////////
// Puerto 0xFE y trampas de la ROM de BASIC

// Lectura del EAR con la cinta en marcha (el audio ya ha avanzado)
boolean CTapeDeck::ReadEar (u64 nNow)
{
	boolean bEar = m_pTape->GetEarBit (nNow);

	// cargadores reconocidos: se salta su bucle de espera del flanco
	if (m_bLoaderAccel && m_Mode != TapeModeNormal)
	{
		CheckLoaderAcceleration (nNow);
		bEar = m_pTape->GetEarBit (nNow);
	}

	// ¿Ha cambiado la señal desde la lectura anterior? Un cargador lee al
	// menos una vez en cada pulso, así que ve casi todos los flancos
	u32 nEdges = m_pTape->GetEdgeCount ();
	if (nEdges != m_nSeenEdge)
	{
		m_nSeenEdge = nEdges;
		m_nObservedEdges++;
	}

	return bEar;
}

// SA-BYTES (0x04D0): con un CTapeSaveHandler y sin grabar por pulsos, el
// bloque se graba ya
boolean CTapeDeck::SaveTrap (void)
{
	return m_pSaveHandler != 0 && !m_bRecording && FlashSave ();
}

// LD-BYTES en 0x0562: carga instantánea (ver FlashLoad)
boolean CTapeDeck::LoadTrap (void)
{
	return    m_Mode == TapeModeFlash
	       && m_pTape->IsInserted ()
	       && !m_pTape->IsPlaying ()
	       && FlashLoad ();
}

// LD-BYTES (0x0556): arranca la cinta si hace falta
void CTapeDeck::LoadBytes (u64 nNow)
{
	if (   !m_pTape->IsInserted ()
	    || m_pTape->IsPlaying ()
	    || m_pTape->IsAtEnd ())
	{
		return;
	}

	// Con carga instantánea y un bloque estándar, la ROM sigue hasta 0x0562
	// y allí carga la trampa
	unsigned nLength;
	if (   m_Mode == TapeModeFlash
	    && m_pTape->GetStandardBlock (&nLength) != 0
	    && nLength > 0)
	{
		return;
	}

	// Bloques no estándar (turbo, TZX especiales) o modos por pulsos
	Play (nNow);
}

////////////////////////////////////////////////////////////////////////////////
// Cada cuadro

// Grabación de la entrada EAR externa: sus cambios de este cuadro
void CTapeDeck::RecordEarInput (u64 nFrameStart)
{
	if (m_bRecording && m_bRecordEar && m_pEarInput != 0)
	{
		for (unsigned i = 0; i < m_pEarInput->GetEdgeCount (); i++)
		{
			RecordEdge (nFrameStart + m_pEarInput->GetEdgeTime (i));
		}
	}
}

// Arranque y parada automáticos
void CTapeDeck::EndFrame (u64 nNow)
{
	// Parada pedida por la propia cinta (bloque de parada entre niveles de una
	// multicarga, como Basil the Great Mouse Detective): el cargador puede
	// volver a arrancarla. Antes contaba como parada manual y la cinta ya no
	// arrancaba sola, con el cargador esperando para siempre.
	if (!m_pTape->IsPlaying () && m_pTape->TakeStoppedByBlock ())
	{
		m_bAutoStopped = TRUE;
		m_nIdleFrames = 0;
		m_nLoadingFrames = 0;
	}

	if (m_pTape->IsPlaying ())
	{
		// Si el programa deja de leer la cinta durante un rato, se para. Pero
		// hay cargadores que leen poco: los acelerados (se saltan sus bucles
		// de espera) y los que pasan casi todo el tiempo en retardos, como el
		// de La Abadía del Crimen (unas 150 lecturas por cuadro). Lo que los
		// delata es que ven casi todos los flancos de la cinta: una lectura en
		// cada pulso. Un juego que solo mira el teclado ve muy pocos.
		//
		// Tampoco basta con leer mucho: los menús de algunos juegos miran el
		// teclado miles de veces por cuadro (Teenage Mutant Hero Turtles - The
		// Coin-Op) y la cinta seguía sonando por los bloques de los niveles
		// mientras se elegía en el menú; al pedir el nivel, la cinta ya estaba
		// en otro sitio. Por eso cuentan las lecturas con pinta de cargador
		// (ver CountLoaderRead), no todas.
		u32 nFrameEdges = m_pTape->GetEdgeCount () - m_nFrameEdges;
		boolean bFollowing = nFrameEdges > 0 && m_nObservedEdges * 2 >= nFrameEdges;
		boolean bLoading =    m_nLoaderReads >= LOADER_READS_ACTIVE
				   || m_nAccelEdges > 0
				   || (bFollowing && m_nLoaderReads > 0);
		if (!bLoading)
		{
			if (++m_nIdleFrames >= AUTOSTOP_FRAMES)
			{
				m_pTape->Stop ();
				m_bAutoStopped = TRUE;
				m_nIdleFrames = 0;
			}
		}
		else
		{
			m_nIdleFrames = 0;
		}
	}
	else if (   m_bAutoStopped
		 && m_pTape->IsInserted ()
		 && !m_pTape->IsAtEnd ())
	{
		// Un cargador propio está esperando la señal: se arranca la cinta.
		// Antes bastaba con muchas lecturas, y un menú que mira el teclado sin
		// parar la arrancaba sola (The Coin-Op, en carga instantánea).
		if (m_nLoaderReads > LOADER_READS_PLAY)
		{
			if (++m_nLoadingFrames >= AUTOPLAY_FRAMES)
			{
				Play (nNow);
			}
		}
		else
		{
			m_nLoadingFrames = 0;
		}
	}

	m_nAccelEdges = 0;
	m_nObservedEdges = 0;
	m_nLoaderReads = 0;
	m_nFrameEdges = m_pTape->GetEdgeCount ();
}

////////////////////////////////////////////////////////////////////////////////
// Grabación por pulsos y carga y grabación instantáneas

#define REC_TSTATES_PER_SAMPLE	16
#define REC_MAX_SIZE		(TAPE_MAX_SIZE - 64)

boolean CTapeDeck::IsEarInputActive (void) const
{
	return m_pEarInput != 0 && m_pEarInput->IsEnabled () && m_pEarInput->IsActive ();
}

boolean CTapeDeck::StartRecording (boolean bEarInput, u64 nNow)
{
	m_bRecordEar = bEarInput;
	FreeRecording ();

	m_nRecCapacity = 256 * 1024;
	m_pRecBuffer = new u8[m_nRecCapacity];
	if (m_pRecBuffer == 0)
	{
		m_nRecCapacity = 0;
		return FALSE;
	}

	m_nRecLength = 0;
	m_nRecPulses = 0;
	m_nRecRemainder = 0;
	m_nRecLastEdge = nNow;
	m_bRecording = TRUE;

	return TRUE;
}

void CTapeDeck::RecordEdge (u64 nTime)
{
	u64 nTotal = nTime - m_nRecLastEdge + m_nRecRemainder;
	m_nRecLastEdge = nTime;

	u64 nSamples = nTotal / REC_TSTATES_PER_SAMPLE;
	m_nRecRemainder = nTotal % REC_TSTATES_PER_SAMPLE;
	if (nSamples == 0)
	{
		nSamples = 1;
	}
	if (nSamples > 0xFFFFFFFF)
	{
		nSamples = 0xFFFFFFFF;
	}

	// crece el búfer si hace falta
	if (m_nRecLength + 5 > m_nRecCapacity)
	{
		unsigned nNew = m_nRecCapacity * 2;
		if (nNew > REC_MAX_SIZE)
		{
			nNew = REC_MAX_SIZE;
		}
		if (m_nRecLength + 5 > nNew)
		{
			return;		// grabación demasiado larga: se descarta el resto
		}

		u8 *pNew = new u8[nNew];
		if (pNew == 0)
		{
			return;
		}
		memcpy (pNew, m_pRecBuffer, m_nRecLength);
		delete [] m_pRecBuffer;
		m_pRecBuffer = pNew;
		m_nRecCapacity = nNew;
	}

	if (nSamples < 256)
	{
		m_pRecBuffer[m_nRecLength++] = (u8) nSamples;
	}
	else
	{
		m_pRecBuffer[m_nRecLength++] = 0;
		m_pRecBuffer[m_nRecLength++] = nSamples & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 8) & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 16) & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 24) & 0xFF;
	}

	m_nRecPulses++;
}

void CTapeDeck::StopRecording (u64 nNow)
{
	if (m_bRecording)
	{
		RecordEdge (nNow);	// cierra el último pulso
		m_bRecording = FALSE;
	}
}

const u8 *CTapeDeck::GetRecording (unsigned *pLength, unsigned *pPulses, unsigned *pSampleRate,
				    unsigned nClockFreq) const
{
	*pLength = m_nRecLength;
	*pPulses = m_nRecPulses;
	// muestras de 16 t-states del reloj del modelo (tiempo real, como una cinta)
	*pSampleRate = nClockFreq / REC_TSTATES_PER_SAMPLE;
	return m_pRecBuffer;
}

void CTapeDeck::FreeRecording (void)
{
	m_bRecording = FALSE;
	delete [] m_pRecBuffer;
	m_pRecBuffer = 0;
	m_nRecLength = m_nRecCapacity = m_nRecPulses = 0;
}

boolean CTapeDeck::FlashSave (void)
{
	static u8 Block[0x10000 + 2];

	unsigned nBytes = m_rCPU.getRegDE ();
	u16 nAddr = m_rCPU.getRegIX ();
	if (nBytes > 0xFFFF - 2)
	{
		return FALSE;
	}

	u8 nParity = m_rCPU.getRegA ();
	Block[0] = nParity;
	for (unsigned i = 0; i < nBytes; i++, nAddr++)
	{
		u8 nByte = m_rMem.ReadPage (nAddr >> 14)[nAddr & 0x3FFF];
		Block[i + 1] = nByte;
		nParity ^= nByte;
	}
	Block[nBytes + 1] = nParity;

	return m_pSaveHandler->SaveTapeBlock (Block, nBytes + 2);
}

static u8 Parity (u8 nValue)
{
	nValue ^= nValue >> 4;
	nValue ^= nValue >> 2;
	nValue ^= nValue >> 1;
	return (nValue & 1) ? 0 : 0x04;		// P/V = 1 si paridad par
}

// Flags de CP n (A - n), con los bits 3 y 5 del operando (como el Z80)
static u8 FlagsCP (u8 nA, u8 nN)
{
	u8 nRes = nA - nN;
	u8 nF = (nRes & 0x80) | (nRes == 0 ? 0x40 : 0) | (nN & 0x28) | 0x02;
	if ((nA & 0x0F) < (nN & 0x0F))			nF |= 0x10;	// H
	if ((nA ^ nN) & (nA ^ nRes) & 0x80)		nF |= 0x04;	// V
	if (nA < nN)					nF |= 0x01;	// C
	return nF;
}

// Flags de XOR / AND-OR lógicos: S, Z, 5, 3, P; H = N = C = 0
static u8 FlagsLogic (u8 nRes)
{
	return (nRes & 0xA8) | (nRes == 0 ? 0x40 : 0) | Parity (nRes);
}

// Carga instantánea: hace lo que haría LD-BYTES de la ROM desde 0x0562 (con el
// flag pedido en A' y LOAD/VERIFY en el acarreo de F') y deja los registros
// como tape_trap_load_block de Fuse (tape_traps.c, GPL 2 o posterior):
//
//	A  = paridad calculada (o la diferencia con el flag pedido)
//	F  = como tras CP 1 / XOR; acarreo = cargado bien
//	B  = 0xB0 (bien) o 0x00 (fallo al final del bloque)
//	C  = 0x01; H = paridad; L = último byte leído (o 1)
//	DE = DE - bytes leídos; IX = IX + bytes leídos
//	AF'= 0x0145
//
// La vuelta la hace el RET que devuelve breakpoint (): si se entró por 0x0556,
// pasa por SA/LD-RET (0x053F), que restaura el borde y hace EI, como al cargar
// de verdad; si se entró directamente en 0x0562 (Cabal), vuelve a quien llamó.
// Antes la trampa estaba en 0x0556 y volvía sin pasar por ahí, sin tocar B, C,
// H, L ni AF': el cargador de Basil the Great Mouse Detective descifraba mal
// sus bloques turbo con lo que quedaba en esos registros.
boolean CTapeDeck::FlashLoad (void)
{
	unsigned nLength;
	const u8 *pBlock = m_pTape->GetStandardBlock (&nLength);
	if (pBlock == 0)
	{
		return FALSE;
	}

	if (nLength == 0)
	{
		m_pTape->NextBlock ();
		return FALSE;
	}

	u8 nRequested = m_rCPU.getRegAx ();
	boolean bVerify = (m_rCPU.getRegFx () & 0x01) == 0;	// sin acarreo = VERIFY
	u16 nDE = m_rCPU.getRegDE ();
	u16 nIX = m_rCPU.getRegIX ();
	const u8 *pData = pBlock + 1;

	u8 nParity = pBlock[0];
	u8 nL = nParity;
	u8 nA = 0, nF = m_rCPU.getFlags (), nB = m_rCPU.getRegB ();
	unsigned nRead = nLength - 1;
	if (nRead > nDE)
	{
		nRead = nDE;
	}
	unsigned nProcessed = 0;
	boolean bFail = FALSE;

	if (nDE == 0)
	{
		// el fallo de la ROM con bloques de longitud 0: se lee un byte
		nB = 0xB0;
		nA = nParity;
		nF = FlagsCP (nA, 1);
	}
	else
	{
		m_rCPU.setRegAFx (0x0145);

		nA = nRequested ^ nParity;
		nF = FlagsLogic (nA);
		if (nA != 0)
		{
			bFail = TRUE;			// el flag no coincide
		}
		else
		{
			if (nRead > 0)
			{
				nL = pData[nRead - 1];
			}

			for (nProcessed = 0; nProcessed < nRead; nProcessed++)
			{
				u8 nByte = pData[nProcessed];
				nParity ^= nByte;
				if (bVerify)
				{
					if (nByte != ReadByte ((u16) (nIX + nProcessed)))
					{
						nL = nByte;
						bFail = TRUE;
						break;
					}
				}
				else
				{
					(*m_pWrite) (m_pWriteParam, (u16) (nIX + nProcessed), nByte);
				}
			}

			if (!bFail)
			{
				if (nDE == nProcessed && nRead + 1 < nLength)
				{
					nParity ^= pData[nRead];	// byte de paridad
					nA = nParity;
					nF = FlagsCP (nA, 1);	// acarreo si la paridad da 0
					nB = 0xB0;
				}
				else
				{
					// faltan bytes: falla al leer el siguiente (ROM, 0x05EC)
					nB = 0x00;			// INC B desde 0xFF
					nL = 1;
					nF = (nF & 0x01) | 0x50;	// Z y H de INC B
					bFail = TRUE;
				}
			}
		}
	}

	if (bFail)
	{
		nF &= ~0x01;
	}

	m_rCPU.setRegA (nA);
	m_rCPU.setFlags (nF);
	m_rCPU.setRegB (nB);
	m_rCPU.setRegC (0x01);
	m_rCPU.setRegH (nParity);
	m_rCPU.setRegL (nL);
	m_rCPU.setRegDE ((u16) (nDE - nProcessed));
	m_rCPU.setRegIX ((u16) (nIX + nProcessed));
	m_pTape->NextBlock ();

	return TRUE;
}

//
// Aceleración de cargadores, adaptada de Fuse (loader.c y loader_detector.c,
// © 2006-2018 Philip Kendall y otros, GPL 2 o posterior).
//
// Casi todos los cargadores esperan cada flanco de la cinta en un bucle como
// el de la ROM (LD-SAMPLE, 0x05ED):
//
//	INC B / RET Z / LD A,7F / IN A,(FE) / RRA / [RET NC] / XOR C / AND 20h / JR Z,bucle
//
// B cuenta las vueltas, y al volver el cargador mira B para saber si el pulso
// era corto (bit 0) o largo (bit 1). Cuando el IN está en un bucle reconocido
// y la cinta sabe qué pulso está sonando (solo en los datos), en vez de dar
// todas las vueltas: el flanco llega ya, B queda con un valor de pulso corto o
// largo, el bit 5 de C con el nuevo nivel, el acarreo a 1 (flanco encontrado)
// y se simula el RET del bucle. El tono guía y los sincronismos se emulan
// normalmente, así que el cargador ve una cinta normal, solo que sin esperas.
//

struct TLoaderPatternByte
{
	u8 nAlternatives;		// 0 = cualquier byte
	u8 Values[4];
};

#define LP(v)			{1, {(v)}}
#define LP2(a, b)		{2, {(a), (b)}}
#define LP3(a, b, c)		{3, {(a), (b), (c)}}
#define LP4(a, b, c, d)		{4, {(a), (b), (c), (d)}}
#define LP_ANY			{0, {0}}

#define ROM_LOADER_PREFIX	LP (0x04), LP (0xC8), LP (0x3E), LP3 (0x00, 0x7F, 0xFF)	// INC B; RET Z; LD A,n
#define ROM_LOADER_INPUT	LP (0xDB), LP (0xFE)					// IN A,(FE)
#define ROM_LOADER_EDGE_TEST	LP (0x1F)						// RRA
#define ROM_LOADER_LEVEL_TEST	LP (0xA9), LP (0xE6), LP (0x20), LP (0x28)		// XOR C; AND 20h; JR Z
#define ROM_LOADER_EDGE_ACTION	LP4 (0x00, 0xA7, 0xC8, 0xD0)				// NOP/AND A/RET Z/RET NC

static const TLoaderPatternByte RomLoader[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, ROM_LOADER_LEVEL_TEST, LP (0xF4)};
static const TLoaderPatternByte RomLoaderWithEdgeAction[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, ROM_LOADER_EDGE_ACTION,
	 ROM_LOADER_LEVEL_TEST, LP (0xF3)};

#define MICROPROSE_PREFIX	ROM_LOADER_PREFIX, LP (0x3E), LP (0x7F), ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST
static const TLoaderPatternByte MicroproseLoader[] =
	{MICROPROSE_PREFIX, ROM_LOADER_LEVEL_TEST, LP (0xF2)};
static const TLoaderPatternByte MicroproseLoaderWithEdgeAction[] =
	{MICROPROSE_PREFIX, ROM_LOADER_EDGE_ACTION, ROM_LOADER_LEVEL_TEST, LP (0xF1)};

static const TLoaderPatternByte SearchLoader[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40), LP (0x28), LP (0xF5)};
static const TLoaderPatternByte SearchLoaderWithRet[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40), LP (0xD8), LP (0x00),
	 LP (0x28), LP (0xF3)};

#define ALKATRAZ_SUFFIX		ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, LP (0xC8), ROM_LOADER_LEVEL_TEST, LP2 (0xF1, 0xF3)
static const TLoaderPatternByte AlkatrazLoader[] =
	{LP (0x03), LP (0xC3), LP_ANY, LP_ANY, ALKATRAZ_SUFFIX};
static const TLoaderPatternByte AlkatrazVariantLoader[] =
	{LP (0x04), LP (0x20), LP (0x01), LP (0xC9), ALKATRAZ_SUFFIX};

static const TLoaderPatternByte DigitalIntegrationLoader[] =
	{LP_ANY, LP_ANY, LP (0x05), LP (0xC8), ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40),
	 LP (0xCA), LP_ANY, LP_ANY};

static const TLoaderPatternByte ModifiedROMPrefix[] =
	{LP (0xD9), LP (0x11), LP_ANY, LP_ANY, LP (0x21), LP_ANY, LP_ANY, LP (0x06), LP (0x05),
	 LP (0xCD), LP_ANY, LP_ANY, LP (0xD0), LP (0xD9)};
static const TLoaderPatternByte ModifiedROMEdge2Prefix[] = {LP (0xD9), LP (0xA7)};

#define PATTERN(p)	p, sizeof p / sizeof p[0]

struct TAccelPattern
{
	const TLoaderPatternByte *pBytes;
	unsigned nLength;
};

static const TAccelPattern IncreasingPatterns[] =
{
	{PATTERN (RomLoader)},
	{PATTERN (RomLoaderWithEdgeAction)},
	{PATTERN (MicroproseLoader)},
	{PATTERN (MicroproseLoaderWithEdgeAction)},
	{PATTERN (SearchLoader)},
	{PATTERN (SearchLoaderWithRet)},
	{PATTERN (AlkatrazLoader)},
	{PATTERN (AlkatrazVariantLoader)},
};

// Lectura de memoria sin contención ni efectos (para reconocer el código)
u8 CTapeDeck::PeekLoader (u16 nAddress) const
{
	if (nAddress < 0x4000 && m_rDiv.IsPaged ())
	{
		return m_rDiv.Read (nAddress);
	}
	return m_rMem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF];
}

boolean CTapeDeck::LoaderPatternMatches (u16 nAddress, const TLoaderPatternByte *pPattern,
					 unsigned nLength) const
{
	for (unsigned i = 0; i < nLength; i++)
	{
		const TLoaderPatternByte &P = pPattern[i];
		if (P.nAlternatives == 0)
		{
			continue;
		}

		u8 nByte = PeekLoader ((u16) (nAddress + i));
		boolean bMatch = FALSE;
		for (unsigned j = 0; j < P.nAlternatives; j++)
		{
			if (nByte == P.Values[j])
			{
				bMatch = TRUE;
				break;
			}
		}
		if (!bMatch)
		{
			return FALSE;
		}
	}

	return TRUE;
}

boolean CTapeDeck::LoaderWordMatches (u16 nAddress, u16 nValue) const
{
	return    PeekLoader (nAddress) == (nValue & 0xFF)
	       && PeekLoader ((u16) (nAddress + 1)) == (nValue >> 8);
}

// nPC: dirección en la que empezaría el patrón (6 u 8 bytes antes del final del IN)
CTapeDeck::TAccelMode CTapeDeck::DetectLoader (u16 nPC) const
{
	// Digital Integration cuenta hacia abajo. Esos primeros bytes eligen otras
	// ramas en el detector original de Fuse, así que no pueden empezarlo.
	u8 nFirst = PeekLoader (nPC);
	if (   nFirst != 0x03 && nFirst != 0x04 && nFirst != 0x47
	    && LoaderPatternMatches (nPC, PATTERN (DigitalIntegrationLoader))
	    && LoaderWordMatches ((u16) (nPC + 10), (u16) (m_rCPU.getRegPC () - 4)))
	{
		return AccelDecreasing;
	}

	for (unsigned i = 0; i < sizeof IncreasingPatterns / sizeof IncreasingPatterns[0]; i++)
	{
		if (LoaderPatternMatches (nPC, IncreasingPatterns[i].pBytes, IncreasingPatterns[i].nLength))
		{
			return AccelIncreasing;
		}
	}

	return AccelNone;
}

// Technician Ted cambia la espera de LD-EDGE-1 por trabajo útil que forma una
// sola unidad de tiempo con el muestreo: acelerarlo estropea la carga (Fuse)
boolean CTapeDeck::ModifiedROMLoader (u16 nPC) const
{
	return    LoaderPatternMatches ((u16) (nPC - 40), PATTERN (ModifiedROMPrefix))
	       && LoaderWordMatches ((u16) (nPC - 30), (u16) (nPC - 24))
	       && LoaderPatternMatches ((u16) (nPC - 8), PATTERN (ModifiedROMEdge2Prefix))
	       && DetectLoader ((u16) (nPC - 6)) == AccelIncreasing;
}

// nPC: dirección siguiente al IN A,(FE)
CTapeDeck::TAccelMode CTapeDeck::DetectLoaderAtIn (u16 nPC) const
{
	if (ModifiedROMLoader (nPC))
	{
		return AccelNone;
	}

	TAccelMode Mode = DetectLoader ((u16) (nPC - 6));
	if (Mode == AccelNone)
	{
		Mode = DetectLoader ((u16) (nPC - 8));	// Microprose: otro LD A,7F antes del IN
	}

	return Mode;
}

// Se llama en cada lectura del puerto FE con la cinta en marcha, con PC ya
// detrás de la instrucción IN (como en Fuse)
void CTapeDeck::CheckLoaderAcceleration (u64 nNow)
{
	u16 nPC = m_rCPU.getRegPC ();

	// un IN en otro sitio: se deja de acelerar y se mira si es otro cargador
	if (m_AccelMode != AccelNone && nPC != m_nAccelPC)
	{
		m_AccelMode = AccelNone;
	}
	if (m_AccelMode == AccelNone)
	{
		m_AccelMode = DetectLoaderAtIn (nPC);
		m_nAccelPC = nPC;
		if (m_AccelMode == AccelNone)
		{
			return;
		}
	}

	// Si el último flanco llegó a su hora, el cargador todavía no lo ha visto:
	// este IN lo verá. Adelantar ahora el siguiente haría perder un flanco.
	if (m_pTape->HadNaturalEdge ())
	{
		m_pTape->ClearNaturalEdge ();
		return;
	}

	CTape::TPulseClass Class = m_pTape->GetPulseClass ();
	if (Class == CTape::PulseUnknown)
	{
		return;				// tono guía, sincronismos, pausas...
	}

	// El pulso en curso termina ya
	m_pTape->ForceEdge (nNow);
	m_nAccelEdges++;
	boolean bEarNow = m_pTape->GetEarBit (nNow);

	// B: valor de un pulso largo o corto, según el sentido de la cuenta
	boolean bHigh = (Class == CTape::PulseLong) != (m_AccelMode == AccelDecreasing);
	m_rCPU.setRegB (bHigh ? 0xFE : 0x00);

	// Tras el IN viene el final de la rutina de la ROM (o de una copia exacta):
	// RRA / RET NC / XOR C / AND 20h / JR Z,bucle y, al encontrar el flanco,
	// LD A,C / CPL / LD C,A / AND 7 / OR 8 / OUT (FE),A / SCF / RET. Entonces se
	// hace aquí lo mismo que haría (es lo más rápido).
	static const u8 RomEdgeTail[] = {0x1F, 0xD0, 0xA9, 0xE6, 0x20, 0x28, 0xF3, 0x79, 0x2F, 0x4F,
					 0xE6, 0x07, 0xF6, 0x08, 0xD3, 0xFE, 0x37, 0xC9};
	boolean bRomTail = TRUE;
	for (unsigned i = 0; i < sizeof RomEdgeTail; i++)
	{
		if (PeekLoader ((u16) (nPC + i)) != RomEdgeTail[i])
		{
			bRomTail = FALSE;
			break;
		}
	}
	if (bRomTail)
	{
		// C: nivel tras el flanco, invertido como hace CPL (el borde no importa
		// al cargador); acarreo = flanco encontrado; y el RET
		m_rCPU.setRegC ((m_rCPU.getRegC () & ~0x20) | (bEarNow ? 0x20 : 0x00));
		m_rCPU.setFlags (m_rCPU.getFlags () | 0x01);
		u16 nSP = m_rCPU.getRegSP ();
		u16 nRet = PeekLoader (nSP) | (PeekLoader ((u16) (nSP + 1)) << 8);
		m_rCPU.setRegSP ((u16) (nSP + 2));
		m_rCPU.setRegPC (nRet);
		return;
	}

	// Cualquier otro bucle: el IN que acaba de hacer ve ya el nivel tras el
	// flanco (bit 6, EAR), así que el propio bucle encuentra el flanco y sigue
	// por su camino. Antes se hacía siempre el RET como en la ROM; pero hay
	// cargadores cuyo bucle no termina en RET al encontrar el flanco (Hollywood
	// Poker, TZX: espera los dos flancos del bit seguidos, con EXX y DJNZ), y el
	// RET los devolvía a quien los llamó a mitad de bit.
	m_rCPU.setRegA ((m_rCPU.getRegA () & ~0x40) | (bEarNow ? 0x40 : 0x00));
}

// Una lectura del puerto 0xFE "de cargador": en el mismo IN que la anterior
// (mismo PC y mismo puerto) y con un contador que ha avanzado una unidad entre
// las dos (B en la ROM y casi todos los cargadores, E en el de La Abadía del
// Crimen...). Es el bucle que espera un flanco de la cinta y cuenta lo que
// tarda; Fuse los detecta igual (loader.c, mirando B). Un menú que espera una
// tecla repite el IN sin contar nada, y el teclado se lee fila a fila (cambia
// el puerto), así que no cuentan.
void CTapeDeck::CountLoaderRead (u16 nPort)
{
	u16 nPC = m_rCPU.getRegPC ();
	u16 nBC = m_rCPU.getRegBC (), nDE = m_rCPU.getRegDE (), nHL = m_rCPU.getRegHL ();
	u8 Regs[6] = {(u8) (nBC >> 8), (u8) nBC, (u8) (nDE >> 8), (u8) nDE, (u8) (nHL >> 8), (u8) nHL};

	if (nPC == m_nLastFEPC && nPort == m_nLastFEPort)
	{
		for (unsigned i = 0; i < 6; i++)
		{
			u8 nDiff = (u8) (Regs[i] - m_LastFERegs[i]);
			if (nDiff == 1 || nDiff == 0xFF)
			{
				m_nLoaderReads++;
				break;
			}
		}
	}

	m_nLastFEPC = nPC;
	m_nLastFEPort = nPort;
	memcpy (m_LastFERegs, Regs, sizeof Regs);
}
