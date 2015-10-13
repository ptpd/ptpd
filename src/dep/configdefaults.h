/**
 * @file   configdefaults.h
 * 
 * @brief  definitions related to config templates and defaults
 * 
 * 
 */

#ifndef PTPD_CONFIGDEFAULTS_H_
#define PTPD_CONFIGDEFAULTS_H_

#include <stddef.h>
#include "iniparser/iniparser.h"

typedef struct {
    char * name;
    char * value;
} TemplateOption;

typedef struct {
    char * name;
    TemplateOption options[100];
} ConfigTemplate;

/* Structure defining a PTP engine preset */
typedef struct {

    char* presetName;
    Boolean slaveOnly;
    Boolean noAdjust;
    UInteger8_option clockClass;

} PtpEnginePreset;

/* Preset definitions */
enum {
    PTP_PRESET_NONE,
    PTP_PRESET_SLAVEONLY,
    PTP_PRESET_MASTERSLAVE,
    PTP_PRESET_MASTERONLY,
    PTP_PRESET_MAX
};

void loadDefaultSettings( RunTimeOpts* rtOpts );
int applyConfigTemplates(char *templateNames, dictionary *dict);
PtpEnginePreset getPtpPreset(int presetNumber, RunTimeOpts* rtOpts);
void dumpConfigTemplates();

#endif /* PTPD_CONFIGDEFAULTS_H_ */
