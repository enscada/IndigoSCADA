/*
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */
// Modified by Enscada limited http://www.enscada.com

#include "iec61850_simple_server_api.h"
#include "iso_server.h"
#include "acse.h"
#include "thread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

/* import IEC 61850 device model created from SCL-File */
extern IedModel staticIedModel;

static int running = 0;

void sigint_handler(int signalId)
{
	running = 0;
}

int main(int argc, char** argv) {

	IsoServer isoServer;
	AcseAuthenticationParameter auth;
	IedServer iedServer = IedServer_create(&staticIedModel);

	IedServer_setAllModelDefaultValues(iedServer);

	/* Activate authentication */
	auth = calloc(1, sizeof(struct sAcseAuthenticationParameter));
	auth->mechanism = AUTH_PASSWORD;
	auth->value.password.string = "indigoscada";
	isoServer = IedServer_getIsoServer(iedServer);
	IsoServer_setAuthenticationParameter(isoServer, auth);

	/* MMS server will be instructed to start listening to client connections. */
	IedServer_start(iedServer);

	if (!IedServer_isRunning(iedServer)) {
		printf("Staring server failed! Exit.\n");
		IedServer_destroy(iedServer);
		exit(-1);
	}

	running = 1;

	signal(SIGINT, sigint_handler);

	while (running) {
		Thread_sleep(1);
	}

	/* stop MMS server - close TCP server socket and all client sockets */
	IedServer_stop(iedServer);

	/* Cleanup - free all resources */
	IedServer_destroy(iedServer);
} /* main() */
