//
// main.cpp - ZXCircle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include <circle/startup.h>

int main (void)
{
	// CKernel es grande (decenas de KB): va en el heap, no en la pila.
	// No se puede volver de aquí: algunos destructores no están implementados.
	CKernel *pKernel = new CKernel;
	if (pKernel == 0 || !pKernel->Initialize ())
	{
		halt ();
		return EXIT_HALT;
	}

	TShutdownMode ShutdownMode = pKernel->Run ();

	switch (ShutdownMode)
	{
	case ShutdownReboot:
		reboot ();
		return EXIT_REBOOT;

	case ShutdownHalt:
	default:
		halt ();
		return EXIT_HALT;
	}
}
