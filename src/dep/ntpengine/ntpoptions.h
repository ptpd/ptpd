/*
 * ntpdcontrol.h - definitions for the ntpd remote query and control facility
 */

#ifndef NTPOPTIONS_H_
#define NTPOPTIONS_H_

#include "../../ptp_primitives.h"

#include <netdb.h>

typedef struct {
	Boolean enableEngine;
	Boolean enableControl;
	Boolean enableFailover;
	int failoverTimeout;
	int checkInterval;
	int keyId;
	char key[20];
	Octet hostAddress[MAXHOSTNAMELEN];
} NTPoptions;

typedef struct {
	Boolean operational;
	Boolean enabled;
	Boolean isRequired;
	Boolean inControl;
	Boolean isFailOver;
	Boolean checkFailed;
	Boolean requestFailed;
	Boolean flagsCaptured;
	int originalFlags;
	Integer32 serverAddress;
	Integer32 sockFD;
} NTPcontrol;

#endif /* NTPOPTIONS_H_ */
