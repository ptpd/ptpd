/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments,
 *                         Tomasz Kleinschmidt
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** 
 * @file        network.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Network functions.
 * 
 * Functions to initialize and manage network connection between 
 * client and server.
 */

#include "network.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <net/if.h>
#ifdef linux
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#else /* BSD */
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#endif

#include "MgmtMsgClient.h"

#include "constants.h"
#include "constants_dep.h"

/**
 * @brief Initialize network connection between client and server.
 * 
 * @param hostName      Hostname or IP address of the server.
 * @param port          Port number that the server is listening on.
 * @param addrInfo      Structure to store address info of the server.
 */
int initNetwork(char* hostName, char* port, char* ifaceName, struct addrinfo** addrInfo) {
    int error, sockFd, yes = 1;
    struct addrinfo sockHints, *sockRes;
    
    memset(&sockHints, 0, sizeof(sockHints)); 
            
    sockHints.ai_family = PF_INET;
    sockHints.ai_socktype = SOCK_DGRAM;
    sockHints.ai_protocol = IPPROTO_UDP;
	
    //get host address data
    error = getaddrinfo(hostName, port, &sockHints, &sockRes);
	
    if (error) {		
        errx(1, "%s", gai_strerror(error));		
        exit(1);	
    }
    
    //store address data for future use
    *addrInfo = sockRes;

    //make a socket with characteristics identical to server's
    sockFd = socket(sockRes->ai_family, sockRes->ai_socktype, sockRes->ai_protocol);

    if (sockFd == -1) {
        perror("socket()");
        exit(1);
    }
    
    //set reuse port to on to allow multiple binds per host
    error = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
    if (error != 0) {		
        perror("setsockopt()");		
        exit(1);	
    }
    
    memset(&sockHints, 0, sizeof(sockHints)); 
            
    sockHints.ai_socktype = SOCK_DGRAM;
    sockHints.ai_family = AF_INET;
    sockHints.ai_flags = AI_PASSIVE; // use my IP

    //get host address data
    error = getaddrinfo(NULL, port, &sockHints, &sockRes);
	
    if (error) {		
        errx(1, "%s", gai_strerror(error));		
        exit(1);	
    }
    
    //bind to a local port
    error = bind(sockFd, sockRes->ai_addr, sockRes->ai_addrlen);
	
    if (error != 0) {		
        perror("bind()");		
        exit(1);	
    }
    
#ifdef linux
   /*
    * The following code makes sure that the data is only received on the specified interface.
    * Without this option, it's possible to receive PTP from another interface, and confuse the protocol.
    * Calling bind() with the IP address of the device instead of INADDR_ANY does not work.
    *
    * More info:
    *   http://developerweb.net/viewtopic.php?id=6471
    *   http://stackoverflow.com/questions/1207746/problems-with-so-bindtodevice-linux-socket-option
    */
    
    error = setsockopt(sockFd, SOL_SOCKET, SO_BINDTODEVICE, ifaceName, strlen(ifaceName));
    
    if (error != 0) {		
        perror("setsockopt()");		
        exit(1);	
    }
    
#endif
    
    DBG("Client connected to %s on port %s\n", hostName, port);
    
    return sockFd;
}

/**
 * @brief Close network connection.
 * 
 * @param sockFd        A descriptor identifying a socket.
 * @param addrInfo      A structure containing the destination address.
 */
void disableNetwork(int sockFd, struct addrinfo** addrInfo) {
    freeaddrinfo(*addrInfo);
    close(sockFd);
}

/**
 * @brief Send message to the server.
 * 
 * @param sockFd        A descriptor identifying a socket.
 * @param buf           A buffer containing the message to be sent.
 * @param length        Specifies the size of the message.
 * @param addrInfo      A structure containing the destination address.
 */
void sendMessage(int sockFd, Octet* buf, UInteger16 length, struct addrinfo* addrInfo) {
    ssize_t ret;
    
    DBG("sending management message \n");
    
    if ((ret = sendto(sockFd, buf, length, 0, addrInfo->ai_addr, addrInfo->ai_addrlen)) <= 0) {
        perror("send()");
	exit(1);
    }
}

/**
 * @brief Receive message from the server.
 * 
 * @param sockFd        A descriptor identifying a bound socket.
 * @param buf           A buffer for the incoming data.
 * @param length        The length of the buffer.
 * @param addr          A buffer in a sockaddr structure that will hold the source address.
 * @param len           A pointer to the size of the source address buffer.
 * @param isNullMgmt    A bool indicating whether the Null Management message is being handled.
 * @param recvTimeout   A receive timeout.
 */
void receiveMessage(int sockFd, Octet* buf, UInteger16 length, struct sockaddr_storage* addr, socklen_t* len, bool isNullMgmt, unsigned int recvTimeout) {
    int result;
    fd_set socks;
    ssize_t ret;
    struct timeval timeout;
    
    *len = sizeof(*addr);
    
    FD_ZERO(&socks);
    FD_SET(sockFd, &socks);
    
    timeout.tv_sec = recvTimeout;
    timeout.tv_usec = 0;
    
    DBG("receiving management message \n");
    
    if ((result = select(sockFd + 1, &socks, NULL, NULL, &timeout)) != 0) {
        if ((ret = recvfrom(sockFd, buf, length, 0, (struct sockaddr *)addr, len)) == -1) {
            perror("recvfrom()");
            exit(1);
        }
    }
    else if (result < 0) {
        perror("select()");
        exit(1);
    }
    else {
        /* We should except no response to NULL_MANAGEMENT unless some Error
         is reported. In this case no reponse is considered a success. In other
         way, timeout exceeding is reported. */
        if (isNullMgmt)
            printf("NULL_MANAGEMENT message receives no response by default\n");
        else
            printf("timeout exceeded...\n");
        exit(0);
    }
}

/**
 * @brief Test if network layer is OK for PTP.
 * 
 * @param communicationTechnology       Communication technology to check.
 */
UInteger8 lookupCommunicationTechnology(UInteger8 communicationTechnology)
{
#ifdef linux
    switch (communicationTechnology) {
	case ARPHRD_ETHER:
	case ARPHRD_EETHER:
	case ARPHRD_IEEE802:
            return PTP_ETHER;

	default:
            break;
    }
#endif  /* defined(linux) */

    return PTP_DEFAULT;
}

/**
 * @brief Resolve hardware address of a given interface.
 * 
 * @param sockFd        A descriptor identifying a bound socket.
 * @param ifaceName     A buffer with the interface name.
 * @param hwAddr        A buffer for the hardware address.
 */
void findIface(int sockFd, Octet* ifaceName, Octet* hwAddr)
{
#ifdef linux
    UInteger8 communicationTechnology;
    
    struct ifreq device;
    memcpy(device.ifr_name, ifaceName, 16);
    
    if (ioctl(sockFd, SIOCGIFHWADDR, &device) < 0)
        WARNING("failed to get hardware address\n");
    else if ((communicationTechnology = (lookupCommunicationTechnology(device.ifr_hwaddr.sa_family))) == PTP_DEFAULT)
        WARNING("unsupported communication technology (%d)\n", communicationTechnology);
    else {
        DBG("Local MAC address used : %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
                device.ifr_ifru.ifru_hwaddr.sa_data[0], device.ifr_ifru.ifru_hwaddr.sa_data[1], 
                device.ifr_ifru.ifru_hwaddr.sa_data[2], device.ifr_ifru.ifru_hwaddr.sa_data[3], 
                device.ifr_ifru.ifru_hwaddr.sa_data[4], device.ifr_ifru.ifru_hwaddr.sa_data[5]);
        memcpy(hwAddr, &device.ifr_ifru.ifru_hwaddr.sa_data, MAC_ADDR_STR_LEN);
    }

#else /* usually *BSD */
    struct ifaddrs *if_list, *ifv4, *ifh;

    if (getifaddrs(&if_list) < 0) {
        perror("getifaddrs() failed");
        exit(1);
    }
    /* find an IPv4, multicast, UP interface, right name(if supplied) */
    for (ifv4 = if_list; ifv4 != NULL; ifv4 = ifv4->ifa_next) {
        if ((ifv4->ifa_flags & IFF_UP) == 0)
            continue;
        if ((ifv4->ifa_flags & IFF_RUNNING) == 0)
            continue;
        if ((ifv4->ifa_flags & IFF_LOOPBACK))
            continue;
        if ((ifv4->ifa_flags & IFF_MULTICAST) == 0)
            continue;
        /* must have IPv4 address */
        if (ifv4->ifa_addr->sa_family != AF_INET)
            continue;
        if (ifaceName[0] && strncmp(ifv4->ifa_name, ifaceName, IF_NAMESIZE) != 0)
            continue;
        break;
    }

    if (ifv4 == NULL) {
        if (ifaceName[0]) {
            ERROR("interface \"%s\" does not exist,"
                    "or is not appropriate\n", ifaceName);
        }
        ERROR("no suitable interfaces found!");
    }
    
    /* find the AF_LINK info associated with the chosen interface */
    for (ifh = if_list; ifh != NULL; ifh = ifh->ifa_next) {
        if (ifh->ifa_addr->sa_family != AF_LINK)
            continue;
        if (strncmp(ifv4->ifa_name, ifh->ifa_name, IF_NAMESIZE) == 0)
            break;
    }

    if (ifh == NULL) {
        WARNING("could not get hardware address for interface \"%s\"\n", ifv4->ifa_name);
        return;
    }
    
    /* check that the interface TYPE is OK */
    if (((struct sockaddr_dl *)ifh->ifa_addr)->sdl_type != IFT_ETHER) {
        WARNING("\"%s\" is not an ethernet interface!\n", ifh->ifa_name);
        return;
    }
    
    DBG("Local MAC address used : %s\n",
            ether_ntoa((struct ether_addr *)LLADDR((struct sockaddr_dl *)ifh->ifa_addr))
            );

    memcpy(hwAddr, ether_ntoa((struct ether_addr *)LLADDR((struct sockaddr_dl *)ifh->ifa_addr)), MAC_ADDR_STR_LEN);

#endif
}

