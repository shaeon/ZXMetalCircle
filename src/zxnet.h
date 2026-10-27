//
// zxnet.h - Red ZX Net del Interface 1
//
// Traducción a C++ de la emulación de SpectRusty (spectrusty-peripherals,
// network/zxnet.rs y zxnet_udp.rs; (C) 2020-2022 Rafal Michalski, LGPL 3 o
// posterior). La ROM del IF1 mueve el cable de red bit a bit con tiempos fijos;
// esta máquina de estados reconoce, por los tiempos (en T-states) de los OUT e
// IN al puerto 0xF7, los bytes de cada paquete (cabecera de 8 bytes y hasta
// 255 de datos) y los manda enteros por UDP; los que llegan se entregan igual.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_zxnet_h
#define _zx_zxnet_h

#include <circle/types.h>

#define ZXNET_HEAD_SIZE		8	// destino, origen, número (2), fin, tamaño, dchk, hchk
#define ZXNET_TAG_SIZE		6	// "ZXNET" + tipo
#define ZXNET_MAX_PACKET	(ZXNET_TAG_SIZE + ZXNET_HEAD_SIZE + 255)

// Transporte de los paquetes (UDP en la Pi; en memoria en las pruebas)
class CZXNetTransport
{
public:
	virtual ~CZXNetTransport (void) {}
	virtual void Send (const u8 *pData, unsigned nLength) = 0;
	// Devuelve la longitud recibida (0 = nada). Si bWait, espera un poco
	// (hasta ~50 ms) a la confirmación del otro lado.
	virtual unsigned Receive (u8 *pBuffer, unsigned nSize, boolean bWait) = 0;
};

// Paquetes en el formato de SpectRusty: "ZXNET", tipo (0 datos, 1 aceptado),
// cabecera y datos
class CZXNetSocket
{
public:
	CZXNetSocket (void);

	void SetTransport (CZXNetTransport *pTransport)	{ m_pTransport = pTransport; }

	const u8 *PacketData (void) const	{ return m_Packet + ZXNET_TAG_SIZE; }
	void BeginPacket (void);
	unsigned PushByte (u8 nByte);		// devuelve los bytes que lleva
	unsigned OutboundIndex (void) const	{ return m_nLength - ZXNET_TAG_SIZE; }
	void SendPacket (void);
	boolean RecvAccept (void);
	boolean RecvPacket (void);
	boolean PullByte (u8 *pByte);
	unsigned InboundIndex (void) const	{ return m_nPosition - ZXNET_TAG_SIZE; }
	void SendAccept (void);

private:
	void SendLastAccepted (void);

	CZXNetTransport *m_pTransport;
	u8 m_Packet[ZXNET_MAX_PACKET];
	unsigned m_nLength;
	unsigned m_nPosition;
	u8 m_LastAccepted[ZXNET_TAG_SIZE + ZXNET_HEAD_SIZE];
	boolean m_bHaveAccepted;
};

class CZXNet
{
public:
	CZXNet (void);

	CZXNetSocket *GetSocket (void)		{ return &m_Socket; }
	void Reset (void);

	// OUT al puerto 0xF7 en modo red (net = bit 0 a 0)
	void SendState (boolean bNet, u64 nTime);
	// IN del puerto 0xF7: estado del cable
	boolean PollState (u64 nTime);
	// OUT al puerto 0xEF con WAIT (bit 5 a 0): la ROM espera un byte
	void WaitData (u64 nTime);

private:
	enum TState
	{
		Idle, InputScout, InputStart, InputData,
		OutputScout, OutputStart, OutputData, OutputStop, OutputEnd
	};

	CZXNetSocket m_Socket;
	u64 m_nEventTime;
	boolean m_bOutbound;
	TState m_State;
	u8 m_nArg;			// contador (Idle) o bits/byte en curso
	boolean m_bNetState;
};

#endif
