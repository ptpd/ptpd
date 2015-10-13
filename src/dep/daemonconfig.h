/**
 * @file   daemonconfig.h
 * 
 * @brief  definitions related to config file handling.
 * 
 * 
 */

#ifndef PTPD_DAEMONCONFIG_H_
#define PTPD_DAEMONCONFIG_H_

#include "iniparser/iniparser.h"
#include "configdefaults.h"

/* Config reload - component restart status flags */

/* No restart required - can continue with new config */
#define PTPD_RESTART_NONE	1 << 0
/* PTP port datasetw can be updated without restarting FSM */
#define PTPD_UPDATE_DATASETS	1 << 1
/* PTP FSM port re-initialisation required (PTP_INITIALIZING) */
#define PTPD_RESTART_PROTOCOL	1 << 2
/* Network config changed: PTP_INITIALIZING handles this so far */
#define PTPD_RESTART_NETWORK	1 << 3
/* Logging config changes: log files need closed / reopened */
#define PTPD_RESTART_LOGGING	1 << 4
/* Configuration changes need checking lock files */
#define PTPD_CHECK_LOCKS	1 << 5
/* CPU core has changed */
#define PTPD_CHANGE_CPUAFFINITY 1 << 6
/* Configuration changes require daemon restart */
#define PTPD_RESTART_DAEMON	1 << 7

#ifdef PTPD_STATISTICS
/* Configuration changes require filter restart */
#define PTPD_RESTART_FILTERS	1 << 8
#endif

#define PTPD_RESTART_ACLS	1 << 9
#define PTPD_RESTART_NTPENGINE	1 << 10
#define PTPD_RESTART_NTPCONFIG	1 << 11

#define LOG2_HELP "(expressed as log 2 i.e. -1=0.5s, 0=1s, 1=2s etc.)"

Boolean loadConfigFile (dictionary**, RunTimeOpts*);
void loadCommandLineKeys(dictionary*, int, char**);
Boolean loadCommandLineOptions(RunTimeOpts*, dictionary*, int, char** , Integer16*);
dictionary* parseConfig (dictionary*, RunTimeOpts*);
int reloadConfig ( RunTimeOpts*, PtpClock* );
Boolean compareConfig(dictionary* source, dictionary* target);
int checkSubsystemRestart(dictionary* newConfig, dictionary* oldConfig);
void printConfigHelp();
void printDefaultConfig();
void printShortHelp();
void printLongHelp();
void printSettingHelp(char*);

#endif /*PTPD_DAEMONCONFIG_H_*/
