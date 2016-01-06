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
#define PTPD_RESTART_ALARMS	1 << 12

#define LOG2_HELP "(expressed as log 2 i.e. -1=0.5s, 0=1s, 1=2s etc.)"
#define MAX_LINE_SIZE 1024

/* int/double range check flags */
enum {
    RANGECHECK_NONE,
    RANGECHECK_RANGE,
    RANGECHECK_MIN,
    RANGECHECK_MAX
};

/* int type to cast parsed config data to */
enum {
    INTTYPE_INT,
    INTTYPE_I8,
    INTTYPE_U8,
    INTTYPE_I16,
    INTTYPE_U16,
    INTTYPE_I32,
    INTTYPE_U32
};

/* config parser operations */

#define    CFGOP_PARSE 		1<<0	/* parse all config options, return success/failure */
#define    CFGOP_PARSE_QUIET	1<<1	/* parse config but display no warnings */
#define    CFGOP_PRINT_DEFAULT	1<<2	/* print default settings only */
#define    CFGOP_HELP_FULL	1<<3	/* print help for all settings */
#define    CFGOP_HELP_SINGLE	1<<4	/* print help for one entry */
#define    CFGOP_RESTART_FLAGS	1<<5	/* return subsystems affected by config changes */

Boolean loadConfigFile (dictionary**, RunTimeOpts*);
void loadCommandLineKeys(dictionary*, int, char**);
Boolean loadCommandLineOptions(RunTimeOpts*, dictionary*, int, char** , Integer16*);
dictionary* parseConfig (int, void*, dictionary*, RunTimeOpts*);
int reloadConfig ( RunTimeOpts*, PtpClock* );
Boolean compareConfig(dictionary* source, dictionary* target);
int checkSubsystemRestart(dictionary* newConfig, dictionary* oldConfig, RunTimeOpts *rtOpts);
void printConfigHelp();
void printDefaultConfig();
void printShortHelp();
void printLongHelp();
void printSettingHelp(char*);
void setConfig(dictionary *dict, const char* key, const char *value);

#endif /*PTPD_DAEMONCONFIG_H_*/
