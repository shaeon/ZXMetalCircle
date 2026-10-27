//
// spectranet.h - Tarjeta de red Spectranet (Dylan Smith, alioth.net)
//
// Basado en la emulación de Fuse (peripherals/spectranet.c, nic/w5100*.c,
// flash/am29f010.c; Philip Kendall y otros, GPL v2 o posterior).
//
// Hardware:
//   - 128 KB de flash AM29F010 (páginas 0x00-0x1F de 4 KB): firmware y ajustes
//   - 128 KB de RAM (páginas 0xC0-0xDF)
//   - Chip de red W5100 (páginas 0x40-0x47: sus 32 KB de registros y búferes)
// Con la tarjeta paginada, 0x0000-0x3FFF del Spectrum son:
//   0x0000 página 0 (flash 0, fija)   0x1000 página A (puerto 0x003B)
//   0x2000 página B (puerto 0x013B)   0x3000 página 3 (RAM 0xC0, fija)
// Se pagina al ejecutar 0x0008 o 0x3FF8-0x3FFF (antes de leer la
// instrucción), con la NMI o por el puerto 0x033B, y se despagina al ejecutar
// 0x007C (después de leerla). 0x023B: trampa programable (dirección que lanza
// una NMI); al leerlo, versión del CPLD.
//
// El W5100 se emula a nivel de socket (como Fuse): cada socket del chip es un
// socket TCP o UDP de la Pi (ver CSpectranetNet). El DHCP se responde aquí
// mismo con la configuración de red de la Pi.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_spectranet_h
#define _zx_spectranet_h

#include <circle/types.h>

#define SNET_FLASH_SIZE		0x20000
#define SNET_RAM_SIZE		0x20000

// Red de la Pi para los sockets del W5100 (nada bloquea: la conexión TCP se
// empieza con ConnectStart y se consulta con ConnectStatus)
class CSpectranetNet
{
public:
	virtual ~CSpectranetNet (void) {}

	virtual boolean IsAvailable (void) = 0;
	virtual int Open (boolean bTCP) = 0;				// devuelve un número >= 0
	virtual boolean Bind (int h, u16 nPort) = 0;
	virtual boolean ConnectStart (int h, const u8 *pIP, u16 nPort) = 0;	// TCP; FALSE = falla ya
	virtual int ConnectStatus (int h) = 0;		// 1 conectado, 0 en curso, -1 falló
	virtual boolean Listen (int h) = 0;
	virtual int Accept (int h) = 0;					// nueva conexión o -1
	virtual int Send (int h, const u8 *pData, unsigned nLength) = 0;
	virtual int SendTo (int h, const u8 *pData, unsigned nLength, const u8 *pIP, u16 nPort) = 0;
	// > 0 bytes; 0 = nada todavía; < 0 = conexión cerrada
	virtual int Recv (int h, u8 *pBuffer, unsigned nSize) = 0;
	virtual int RecvFrom (int h, u8 *pBuffer, unsigned nSize, u8 *pIP, u16 *pPort) = 0;
	virtual void Close (int h) = 0;
	// configuración de red de la Pi (para responder al DHCP)
	virtual boolean GetConfig (u8 *pIP, u8 *pMask, u8 *pGateway, u8 *pDNS) = 0;
};

class CSpectranet
{
public:
	CSpectranet (void);
	~CSpectranet (void);

	void SetNet (CSpectranetNet *pNet)	{ m_pNet = pNet; }

	// Flash: el contenido inicial (128 KB) y si se ha modificado desde la
	// última vez que se preguntó (para guardarla en la SD)
	void LoadFlash (const u8 *pData, unsigned nSize);
	const u8 *GetFlash (void) const		{ return m_pFlash; }
	boolean TakeFlashModified (void)	{ boolean b = m_bFlashModified; m_bFlashModified = FALSE; return b; }

	void Reset (void);

	// Sin trampas ni paginación al arrancar (para grabar el firmware en una
	// flash vacía con el instalador, como la opción "Spectranet disable" de Fuse)
	void SetTrapsDisabled (boolean bOn)	{ m_bTrapsDisabled = bOn; }

	// Memoria con la tarjeta paginada (0x0000-0x3FFF)
	boolean IsPaged (void) const		{ return m_bPaged; }
	u8 GetPage (unsigned nSlot) const	{ return m_Page[nSlot & 3]; }
	u8 Read (u16 nAddress);
	void Write (u16 nAddress, u8 nValue);

	// Ciclo M1: antes de leer (puede paginar o pedir una NMI) y después
	boolean M1Early (u16 nAddress);		// TRUE = lanzar NMI
	void M1Late (u16 nAddress);
	void NMI (void);			// NMI aceptada: se pagina
	void RETN (void);

	// Puertos (dirección completa de 16 bits)
	boolean IsPort (u16 nPort) const;
	u8 ReadPort (u16 nPort, u8 nULALast, boolean b128Bank);
	void WritePort (u16 nPort, u8 nValue);

	// Red: atender los sockets (una vez por cuadro)
	void Poll (void);
	boolean IsActive (void)	{ boolean b = m_bActivity; m_bActivity = FALSE; return b; }

private:
	void MapPage (unsigned nSlot, u8 nPage)	{ m_Page[nSlot] = nPage; }
	u8 ReadPage (u8 nPage, u16 nOffset);
	void WritePage (unsigned nSlot, u8 nPage, u16 nOffset, u8 nValue);

	// Flash AM29F010
	u8 FlashRead (u32 nOffset);
	void FlashWrite (u8 nSector, u16 nAddress, u8 nValue);

	// W5100
	u8 W5100Read (u16 nReg);
	void W5100Write (u16 nReg, u8 nValue);
	void W5100Reset (void);

	struct TSocket
	{
		u8 nMode;		// 0 cerrado, 1 TCP, 2 UDP
		u8 nState;		// Sn_SR
		u8 nIR;
		u8 Port[2], DIP[4], DPort[2], MSS[2], nProto;
		u16 nTxRR, nTxWR, nRxRSR, nRxRD, nOldRxRD;
		u8 TxBuf[0x800];
		u8 RxBuf[0x800];
		int nHandle;		// socket de la Pi (-1 = ninguno)
		boolean bBound;
		unsigned nBindCount;
		boolean bWritePending;
		u16 nLastSend;
		u16 DatagramLen[32];
		unsigned nDatagrams;
		unsigned nConnectPolls;		// llamadas a Poll esperando la conexión
	};

	void SocketCommand (TSocket *pS, u8 nCommand);
	void SocketClean (TSocket *pS);
	boolean SocketBind (TSocket *pS);
	void SocketRxAppend (TSocket *pS, const u8 *pData, unsigned nLength);
	boolean DHCPReply (TSocket *pS, const u8 *pReq, unsigned nLength);

	u8 *m_pFlash;
	u8 *m_pRAM;
	boolean m_bFlashModified;
	enum { FlashReset, FlashC2, FlashC3, FlashC4, FlashC5, FlashC6, FlashProgram, FlashAutoselect } m_FlashState;

	boolean m_bTrapsDisabled;
	boolean m_bPaged;
	boolean m_bPagedViaIO;
	boolean m_bNMIFlipFlop;
	u8 m_Page[4];
	u16 m_nTrap;
	boolean m_bTrapActive;
	boolean m_bTrapWriteMSB;
	u8 m_nControl;

	// W5100: registros comunes
	u8 m_GW[4], m_Sub[4], m_SHA[6], m_SIP[4];
	TSocket m_Socket[4];

	CSpectranetNet *m_pNet;
	boolean m_bActivity;

	// petición DHCP pendiente (la red de la Pi aún no está lista al arrancar)
	u8 m_DHCPRequest[548];
	unsigned m_nDHCPLength;
	int m_nDHCPSocket;		// -1 = ninguna
};

#endif
