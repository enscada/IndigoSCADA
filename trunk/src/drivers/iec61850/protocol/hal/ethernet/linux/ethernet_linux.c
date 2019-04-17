/*
 *  ethernet_linux.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

#include "lib_memory.h"
#include "hal_ethernet.h"

struct sEthernetSocket {
    int rawSocket;
    bool isBind;
    struct sockaddr_ll socketAddress;
};

struct sEthernetHandleSet {
    struct pollfd *handles;
    int nhandles;
};

EthernetHandleSet
EthernetHandleSet_new(void)
{
    EthernetHandleSet result = (EthernetHandleSet) GLOBAL_MALLOC(sizeof(struct sEthernetHandleSet));

    if (result != NULL) {
        result->handles = NULL;
        result->nhandles = 0;
    }

    return result;
}

void
EthernetHandleSet_addSocket(EthernetHandleSet self, const EthernetSocket sock)
{
    if (self != NULL && sock != NULL) {

        int i = self->nhandles++;

        self->handles = realloc(self->handles, self->nhandles * sizeof(struct pollfd));

        self->handles[i].fd = sock->rawSocket;
        self->handles[i].events = POLLIN;
    }
}

void
EthernetHandleSet_removeSocket(EthernetHandleSet self, const EthernetSocket sock)
{
    if ((self != NULL) && (sock != NULL)) {

        int i;

        for (i = 0; i < self->nhandles; i++) {
            if (self->handles[i].fd == sock->rawSocket) {
                memmove(&self->handles[i], &self->handles[i+1], sizeof(struct pollfd) * (self->nhandles - i - 1));
                self->nhandles--;
                return;
            }
        }
    }
}

int
EthernetHandleSet_waitReady(EthernetHandleSet self, unsigned int timeoutMs)
{
    int result;

    if ((self != NULL) && (self->nhandles >= 0)) {
        result = poll(self->handles, self->nhandles, timeoutMs);
    }
    else {
       result = -1;
    }

    return result;
}

void
EthernetHandleSet_destroy(EthernetHandleSet self)
{
    if (self->nhandles)
        free(self->handles);

    GLOBAL_FREEMEM(self);
}

static int
getInterfaceIndex(int sock, const char* deviceName)
{
    struct ifreq ifr;

    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("ETHERNET_LINUX: Failed to get interface index -> exit");
        exit(1);
    }

    int interfaceIndex = ifr.ifr_ifindex;

    if (ioctl (sock, SIOCGIFFLAGS, &ifr) == -1)
    {
        perror ("ETHERNET_LINUX: Problem getting device flags -> exit");
        exit (1);
    }

    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl (sock, SIOCSIFFLAGS, &ifr) == -1)
    {
        perror ("ETHERNET_LINUX: Setting device to promiscuous mode failed -> exit");
        exit (1);
    }

    return interfaceIndex;
}


void
Ethernet_getInterfaceMACAddress(const char* interfaceId, uint8_t* addr)
{
    struct ifreq buffer;

    int sock = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&buffer, 0x00, sizeof(buffer));

    strncpy(buffer.ifr_name, interfaceId, IFNAMSIZ);

    ioctl(sock, SIOCGIFHWADDR, &buffer);

    close(sock);

    int i;

    for(i = 0; i < 6; i++ )
    {
        addr[i] = (unsigned char)buffer.ifr_hwaddr.sa_data[i];
    }
}


EthernetSocket
Ethernet_createSocket(const char* interfaceId, uint8_t* destAddress)
{
    EthernetSocket ethernetSocket = GLOBAL_CALLOC(1, sizeof(struct sEthernetSocket));

    ethernetSocket->rawSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (ethernetSocket->rawSocket == -1) {
        printf("Error creating raw socket!\n");
        GLOBAL_FREEMEM(ethernetSocket);
        return NULL;
    }

    ethernetSocket->socketAddress.sll_family = PF_PACKET;
    ethernetSocket->socketAddress.sll_protocol = htons(ETH_P_IP);

    ethernetSocket->socketAddress.sll_ifindex = getInterfaceIndex(ethernetSocket->rawSocket, interfaceId);

    ethernetSocket->socketAddress.sll_hatype =  ARPHRD_ETHER;
    ethernetSocket->socketAddress.sll_pkttype = PACKET_OTHERHOST;

    ethernetSocket->socketAddress.sll_halen = ETH_ALEN;

    memset(ethernetSocket->socketAddress.sll_addr, 0, 8);

    if (destAddress != NULL)
        memcpy(ethernetSocket->socketAddress.sll_addr, destAddress, 6);

    ethernetSocket->isBind = false;

    return ethernetSocket;
}

void
Ethernet_setProtocolFilter(EthernetSocket ethSocket, uint16_t etherType)
{
    ethSocket->socketAddress.sll_protocol = htons(etherType);
}


/* non-blocking receive */
int
Ethernet_receivePacket(EthernetSocket self, uint8_t* buffer, int bufferSize)
{
    if (self->isBind == false) {
        if (bind(self->rawSocket, (struct sockaddr*) &self->socketAddress, sizeof(self->socketAddress)) == 0)
            self->isBind = true;
        else
            return 0;
    }

    return recvfrom(self->rawSocket, buffer, bufferSize, MSG_DONTWAIT, 0, 0);
}

void
Ethernet_sendPacket(EthernetSocket ethSocket, uint8_t* buffer, int packetSize)
{
    sendto(ethSocket->rawSocket, buffer, packetSize,
                0, (struct sockaddr*) &(ethSocket->socketAddress), sizeof(ethSocket->socketAddress));
}

void
Ethernet_destroySocket(EthernetSocket ethSocket)
{
    close(ethSocket->rawSocket);
    GLOBAL_FREEMEM(ethSocket);
}

bool
Ethernet_isSupported()
{
    return true;
}

