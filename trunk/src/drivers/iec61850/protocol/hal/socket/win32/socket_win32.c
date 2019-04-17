/*
 *  socket_win32.c
 *
 *  Copyright 2013-2018 Michael Zillgith
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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#include "lib_memory.h"
#include "hal_socket.h"
#include "stack_config.h"


#ifndef __MINGW64_VERSION_MAJOR
struct tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};
#endif

#define SIO_KEEPALIVE_VALS    _WSAIOW(IOC_VENDOR,4)

struct sSocket {
	SOCKET fd;
	uint32_t connectTimeout;
};

struct sServerSocket {
	SOCKET fd;
	int backLog;
};

struct sHandleSet {
   fd_set handles;
   SOCKET maxHandle;
};

HandleSet
Handleset_new(void)
{
   HandleSet result = (HandleSet) GLOBAL_MALLOC(sizeof(struct sHandleSet));

   if (result != NULL) {
       FD_ZERO(&result->handles);
       result->maxHandle = INVALID_SOCKET;
   }
   return result;
}

void
Handleset_reset(HandleSet self)
{
    FD_ZERO(&self->handles);
    self->maxHandle = INVALID_SOCKET;
}

void
Handleset_addSocket(HandleSet self, const Socket sock)
{
   if (self != NULL && sock != NULL && sock->fd != INVALID_SOCKET) {
       FD_SET(sock->fd, &self->handles);

       if ((sock->fd > self->maxHandle) || (self->maxHandle == INVALID_SOCKET))
           self->maxHandle = sock->fd;
   }
}

int
Handleset_waitReady(HandleSet self, unsigned int timeoutMs)
{
   int result;

   if (self != NULL && self->maxHandle >= 0) {
       struct timeval timeout;

       timeout.tv_sec = timeoutMs / 1000;
       timeout.tv_usec = (timeoutMs % 1000) * 1000;
       result = select(self->maxHandle + 1, &self->handles, NULL, NULL, &timeout);
   } else {
       result = -1;
   }

   return result;
}

void
Handleset_destroy(HandleSet self)
{
   GLOBAL_FREEMEM(self);
}

static bool wsaStartupCalled = false;
static int socketCount = 0;

void
Socket_activateTcpKeepAlive(Socket self, int idleTime, int interval, int count)
{
    struct tcp_keepalive keepalive;
    DWORD retVal=0;

    keepalive.onoff = 1;
    keepalive.keepalivetime = CONFIG_TCP_KEEPALIVE_IDLE * 1000;
    keepalive.keepaliveinterval = CONFIG_TCP_KEEPALIVE_INTERVAL * 1000;

     if (WSAIoctl(self->fd, SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive),
                NULL, 0, &retVal, NULL, NULL) == SOCKET_ERROR)
     {
         if (DEBUG_SOCKET)
                printf("WIN32_SOCKET: WSAIotcl(SIO_KEEPALIVE_VALS) failed: %d\n",
                    WSAGetLastError());
     }
}


static void
setSocketNonBlocking(Socket self)
{
    unsigned long mode = 1;
	int tcpNoDelay = 1;
    if (ioctlsocket(self->fd, FIONBIO, &mode) != 0) {
        if (DEBUG_SOCKET)
            printf("WIN32_SOCKET: failed to set socket non-blocking!\n");
    }

    /* activate TCP_NODELAY */

    setsockopt(self->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcpNoDelay, sizeof(int));
}

static bool
prepareServerAddress(const char* address, int port, struct sockaddr_in* sockaddr)
{

	memset((char *) sockaddr , 0, sizeof(struct sockaddr_in));

	if (address != NULL) {
		struct hostent *server;
		server = gethostbyname(address);

		if (server == NULL) return false;

		memcpy((char *) &sockaddr->sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	}
	else
		sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);

    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons(port);

    return true;
}

static bool wsaStartUp()
{
	if (wsaStartupCalled == false) {
		int ec;
		WSADATA wsa;

		if ((ec = WSAStartup(MAKEWORD(2, 0), &wsa)) != 0) {
			if (DEBUG_SOCKET)
				printf("WIN32_SOCKET: winsock error: code %i\n", ec);
			return false;
		}
		else {
			wsaStartupCalled = true;
			return true;
		}
			
	}
	else
		return true;
}

static void wsaShutdown()
{
	if (wsaStartupCalled) {
		if (socketCount == 0) {
			WSACleanup();
			wsaStartupCalled = false;
		}

	}
}

ServerSocket
TcpServerSocket_create(const char* address, int port)
{
	ServerSocket serverSocket = NULL;
	int ec;
	SOCKET listen_socket = INVALID_SOCKET;
	struct sockaddr_in server_addr;
	int optionReuseAddr = 1;

	if (wsaStartUp() == false)
		return NULL;

	if (!prepareServerAddress(address, port, &server_addr))
	    return NULL;

	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listen_socket == INVALID_SOCKET) {
	    if (DEBUG_SOCKET)
	        printf("WIN32_SOCKET: socket failed with error: %i\n", WSAGetLastError());

		wsaShutdown();

		return NULL;
	}

	
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optionReuseAddr, sizeof(int));

	ec = bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (ec == SOCKET_ERROR) {
	    if (DEBUG_SOCKET)
	        printf("WIN32_SOCKET: bind failed with error:%i\n", WSAGetLastError());
		closesocket(listen_socket);

		wsaShutdown();

		return NULL;
	}

	serverSocket = (ServerSocket) GLOBAL_MALLOC(sizeof(struct sServerSocket));

	serverSocket->fd = listen_socket;
	serverSocket->backLog = 10;

    setSocketNonBlocking((Socket) serverSocket);

	socketCount++;

	return serverSocket;
}

void
ServerSocket_listen(ServerSocket self)
{
	listen(self->fd, self->backLog);
}

Socket
ServerSocket_accept(ServerSocket self)
{
	int fd;

	Socket conSocket = NULL;

	fd = accept(self->fd, NULL, NULL);

	if (fd >= 0) {
		conSocket = (Socket) GLOBAL_CALLOC(1, sizeof(struct sSocket));
		conSocket->fd = fd;

		socketCount++;

	    setSocketNonBlocking(conSocket);
	}

	return conSocket;
}

void
ServerSocket_setBacklog(ServerSocket self, int backlog)
{
	self->backLog = backlog;
}

void
ServerSocket_destroy(ServerSocket self)
{
	closesocket(self->fd);
	socketCount--;
	wsaShutdown();
	GLOBAL_FREEMEM(self);
}

Socket
TcpSocket_create()
{
    Socket self = NULL;
	int sock;

    if (wsaStartUp() == false)
        return NULL;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock != INVALID_SOCKET) {
        self = (Socket) GLOBAL_MALLOC(sizeof(struct sSocket));

        self->fd = sock;
        self->connectTimeout = 5000;

        socketCount++;
    }
    else {
        if (DEBUG_SOCKET)
            printf("SOCKET: failed to create socket (error code=%i)\n", WSAGetLastError());
    }

	return self;
}

void
Socket_setConnectTimeout(Socket self, uint32_t timeoutInMs)
{
    self->connectTimeout = timeoutInMs;
}

bool
Socket_connect(Socket self, const char* address, int port)
{
	struct sockaddr_in serverAddress;
	fd_set fdSet;
	struct timeval timeout;

	if (!prepareServerAddress(address, port, &serverAddress))
	    return false;

    setSocketNonBlocking(self);
    
    FD_ZERO(&fdSet);
    FD_SET(self->fd, &fdSet);

    if (connect(self->fd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
            return false;
    }
    
    timeout.tv_sec = self->connectTimeout / 1000;
    timeout.tv_usec = (self->connectTimeout % 1000) * 1000;

    if (select(self->fd + 1, NULL, &fdSet, NULL, &timeout) <= 0)
        return false;
    else
        return true;
}

//apa+++
/* Portable IPv6/IPv4 version of sockaddr.  Based on RFC 2553.
   Pad to force 8 byte alignment and maximum size of 128 bytes. */

/*
 * Desired design of maximum size and alignment
 */
#define _SS_MAXSIZE    128
#define _SS_ALIGNSIZE  (sizeof (__int64)) 
/*
 * Definitions used for sockaddr_storage structure paddings design.
 */
#define _SS_PAD1SIZE   (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE   (_SS_MAXSIZE - (sizeof (short) \
				       + _SS_PAD1SIZE \
				       + _SS_ALIGNSIZE))
struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[_SS_PAD1SIZE];  /* pad to 8 */
    __int64 __ss_align;  	   /* force alignment */
    char __ss_pad2[_SS_PAD2SIZE];  /*  pad to 128 */
};

#define INET6_ADDRSTRLEN 65
//end apa+++

char*
Socket_getPeerAddress(Socket self)
{
	struct sockaddr_storage addr;
	int addrLen = sizeof(addr);
	char addrString[INET6_ADDRSTRLEN + 7];
	int addrStringLen = INET6_ADDRSTRLEN + 7;
	int port;
	bool isIPv6;
	char* clientConnection;

	getpeername(self->fd, (struct sockaddr*) &addr, &addrLen);

	if (addr.ss_family == AF_INET)  {
		struct sockaddr_in* ipv4Addr = (struct sockaddr_in*) &addr;
		port = ntohs(ipv4Addr->sin_port);
		ipv4Addr->sin_port = 0;
		WSAAddressToString((LPSOCKADDR) ipv4Addr, sizeof(struct sockaddr_storage), NULL, 
			(LPSTR) addrString, (LPDWORD) &addrStringLen);
		isIPv6 = false;
	}
	else if (addr.ss_family == AF_INET6){
		struct sockaddr_in6* ipv6Addr = (struct sockaddr_in6*) &addr;
		port = ntohs(ipv6Addr->sin6_port);
		ipv6Addr->sin6_port = 0;
		WSAAddressToString((LPSOCKADDR) ipv6Addr, sizeof(struct sockaddr_storage), NULL, 
			(LPSTR) addrString, (LPDWORD) &addrStringLen);
		isIPv6 = true;
	}
	else
		return NULL;

	clientConnection = (char*) GLOBAL_MALLOC(strlen(addrString) + 9);

    if (isIPv6)
        sprintf(clientConnection, "[%s]:%i", addrString, port);
    else
        sprintf(clientConnection, "%s:%i", addrString, port);

	return clientConnection;
}

char*
Socket_getPeerAddressStatic(Socket self, char* peerAddressString)
{
    struct sockaddr_storage addr;
    int addrLen = sizeof(addr);
	char addrString[INET6_ADDRSTRLEN + 7];
    int addrStringLen = INET6_ADDRSTRLEN + 7;
    int port;

    bool isIPv6;

    getpeername(self->fd, (struct sockaddr*) &addr, &addrLen);

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in* ipv4Addr = (struct sockaddr_in*) &addr;
        port = ntohs(ipv4Addr->sin_port);
        ipv4Addr->sin_port = 0;
        WSAAddressToString((LPSOCKADDR) ipv4Addr, sizeof(struct sockaddr_storage), NULL,
                (LPSTR) addrString, (LPDWORD) & addrStringLen);
        isIPv6 = false;
    }
    else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6* ipv6Addr = (struct sockaddr_in6*) &addr;
        port = ntohs(ipv6Addr->sin6_port);
        ipv6Addr->sin6_port = 0;
        WSAAddressToString((LPSOCKADDR) ipv6Addr, sizeof(struct sockaddr_storage), NULL,
                (LPSTR) addrString, (LPDWORD) & addrStringLen);
        isIPv6 = true;
    }
    else
        return NULL;

    if (isIPv6)
        sprintf(peerAddressString, "[%s]:%i", addrString, port);
    else
        sprintf(peerAddressString, "%s:%i", addrString, port);

    return peerAddressString;
}

int
Socket_read(Socket self, uint8_t* buf, int size)
{
    int bytes_read = recv(self->fd, (char*) buf, size, 0);

    if (bytes_read == 0) // peer has closed socket
        return -1;

    if (bytes_read == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return 0;
        else
            return -1;
    }

	return bytes_read;
}

int
Socket_write(Socket self, uint8_t* buf, int size)
{
    int bytes_sent = send(self->fd, (char*) buf, size, 0);

    if (bytes_sent == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();

        if (errorCode == WSAEWOULDBLOCK)
            bytes_sent = 0;
        else
            bytes_sent = -1;
    }

	return bytes_sent;
}

void
Socket_destroy(Socket self)
{
	if (self->fd != INVALID_SOCKET) {
		closesocket(self->fd);
	}

	socketCount--;
	wsaShutdown();

	GLOBAL_FREEMEM(self);
}
