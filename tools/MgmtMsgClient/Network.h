/** 
 * @file Network.h
 * @author Tomasz Kleinschmidt
 *
 * @brief Network class definition.
 */

#ifndef NETWORK_H
#define	NETWORK_H

int initNetwork(char* port, char* hostName);
void disableNetwork(int sockFd);

void sendMessage();
void receiveMessage();

#endif	/* NETWORK_H */

