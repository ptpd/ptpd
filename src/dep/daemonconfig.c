/*-
 * Copyright (c) 2013-2017 Wojciech Owczarek,
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
 * @file   daemonconfig.c
 * @date   Sun May 27 00:45:32 2013
 *
 * @brief  Code to handle configuration file and default settings
 *
 * Functions in this file deal with config file parsing, reloading,
 * loading default parameters, parsing command-line options, printing
 * help output, etc.
 *
 */

#include <libcck/cck_utils.h>
#include "../ptpd.h"
#include <libcck/clockdriver/clockdriver_unix.h>

/*-
 * Helper macros - this is effectively the API for using the new config file interface.
 * The macros below cover all operations required in parsing the configuration.
 * This allows the parseConfig() function to be very straightforward and easy to expand
 * - it pretty much only uses macros at this stage. The use of macros reduces the
 * complexity of the parseConfig fuction. A lot of the macros are designed to work
 * within parseConfig - they assume the existence of the "dictionary*" dict variable.
 */

static void printComment(const char* helptext);

static int configSettingChanged(dictionary *oldConfig, dictionary *newConfig, const char *key);

static void warnRestart(const char *key, int flags);

static int configMapBoolean(int opCode, void *opArg,  dictionary* dict,
	dictionary *target, const char * key, int restartFlags, Boolean *var, Boolean def, const char* helptext);

static int configMapBool(int opCode, void *opArg,  dictionary* dict,
	dictionary *target, const char * key, int restartFlags, bool *var, bool def, const char* helptext);

static int configMapString(int opCode, void *opArg,  dictionary *dict,
	dictionary *target, const char *key, int restartFlags, char *var, int size, char *def, const char* helptext);

static int checkRangeInt(dictionary *dict, const char *key, int rangeFlags, int minBound, int maxBound);

static int configMapInt(int opCode, void *opArg,  dictionary *dict,
	dictionary *target, const char *key, int restartFlags, int intType, void *var, int def,
	const char *helptext, int rangeFlags, int minBound, int maxBound);

static int checkRangeDouble(dictionary *dict, const char *key, int rangeFlags, double minBound, double maxBound);

static int configMapDouble(int opCode, void *opArg,  dictionary *dict,
	dictionary *target, const char *key, int restartFlags, double *var, double def,
	const char *helptext, int rangeFlags, double minBound, double maxBound);

static int configMapSelectValue(int opCode, void *opArg,  dictionary *dict,
	dictionary *target, const char* key, int restartFlags, uint8_t *var, int def, const char *helptext, ...);

static void parseUserVariables(dictionary *dict, dictionary *target);

static void findUnknownSettings(int opCode, dictionary* source, dictionary* dict);


/* Basic helper macros */

#define STRING_EMPTY(string)\
	(!strcmp(string,""))

#define CONFIG_ISSET(key) \
	(strcmp(iniparser_getstring(dict, key, ""),"") != 0)

#define CONFIG_ISPRESENT(key) \
	(iniparser_find_entry(dict, key) != 0)

#define CONFIG_ISTRUE(key) \
	(iniparser_getboolean(dict,key,FALSE)==TRUE)

#define DICT_ISTRUE(dict,key) \
	(iniparser_getboolean(dict,key,FALSE)==TRUE)

/* Macros handling required settings, triggers, conflicts and dependencies */

#define CONFIG_KEY_REQUIRED(key) \
	if( !(opCode & CFGOP_PARSE_QUIET) && !(opCode & CFGOP_HELP_FULL) && !(opCode & CFGOP_HELP_SINGLE)  && !CONFIG_ISSET(key) )\
	 { \
	    ERROR("Configuration error: option \"%s\" is required\n", key); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_DEPENDENCY(key1, key2) \
	if( CONFIG_ISSET(key1) && \
	    !CONFIG_ISSET(key2)) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("Configuration error: option \"%s\" requires option \"%s\"\n", key1, key2); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONFLICT(key1, key2) \
	if( CONFIG_ISSET(key1) && \
	    CONFIG_ISSET(key2)) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("Configuration error: option \"%s\" cannot be used with option \"%s\"\n", key1, key2); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(condition,dep,messageText) \
	if ( (condition) && \
	    !CONFIG_ISSET(dep) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		WARNING("Warning: %s", messageText); \
	 }

#define CONFIG_KEY_CONDITIONAL_WARNING_ISSET(condition,dep,messageText) \
	if ( (condition) && \
	    CONFIG_ISSET(dep) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		WARNING("Warning: %s", messageText); \
	 }

#define CONFIG_KEY_CONDITIONAL_DEPENDENCY(key,condition,stringval,dep) \
	if ( (condition) && \
	    !CONFIG_ISSET(dep) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("Configuration error: option \"%s=%s\" requires option \"%s\"\n", key, stringval, dep); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONDITIONAL_CONFLICT(key,condition,stringval,dep) \
	if ( (condition) && \
	    CONFIG_ISSET(dep) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("Configuration error: option \"%s=%s\" cannot be used with option \"%s\"\n", key, stringval, dep); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_VALUE_FORBIDDEN(key,condition,stringval,message) \
	if ( (condition) && \
	    CONFIG_ISSET(key) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("Configuration error: option \"%s=%s\" cannot be used: \n%s", key, stringval, message); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_TRIGGER(key,variable,value, otherwise) \
	if (CONFIG_ISSET(key) ) \
	 { \
	    variable = value;\
	 } else { \
	    variable = otherwise;\
	 }

#define CONFIG_KEY_CONDITIONAL_TRIGGER(condition,variable,value,otherwise) \
	if ((condition)) \
	 { \
	    variable = value; \
	 } else { \
	    variable = otherwise; \
	 }

#define CONFIG_KEY_CONDITIONAL_ASSERTION(key,condition,warningtext) \
	if ( (condition) && \
	    CONFIG_ISSET(key) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("%s\n", warningtext); \
	    parseResult = FALSE;\
	 }

#define CONFIG_CONDITIONAL_ASSERTION(condition,warningtext) \
	if ( (condition) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		ERROR("%s\n", warningtext); \
	    parseResult = FALSE;\
	 }

/* Macro printing a warning for a deprecated command-line option */
#define WARN_DEPRECATED_COMMENT( old,new,long,key,comment )\
printf("Note: The use of '-%c' option is deprecated %s- consider using '-%c' (--%s) or the %s setting\n",\
	old, comment, new, long, key);

#define WARN_DEPRECATED(old,new,long,key)\
	WARN_DEPRECATED_COMMENT( old,new,long,key,"")

#define CONFIG_KEY_ALIAS(src,dest) \
	{ \
		if(!STRING_EMPTY(dictionary_get(dict,src,""))) {\
		    dictionary_set(dict,dest,dictionary_get(dict,src,""));\
		    dictionary_unset(dict,src);\
		}\
	}

/* Output a potentially multi-line string, prefixed with ;s */
static void
printComment(const char* helptext)
{

    int i, len;
    len = strlen(helptext);

    if(len > 0)
	printf("\n; ");

    for (i = 0; i < len; i++) {
	printf("%c",helptext[i]);
	if(helptext[i]=='\n') {
	    printf("; ");
	    while(i < len) {
		if( helptext[++i]!=' ' && helptext[i]!='\t') {
		    i--;
		    break;
		}
	    }
	}
    }
    printf("\n");

}

static int
configSettingChanged(dictionary *oldConfig, dictionary *newConfig, const char *key)
{

    return(strcmp(
		    dictionary_get(newConfig, key,""),
		    dictionary_get(oldConfig, key,"")
		) != 0 );

}

/* warn about restart required if needed */
static void
warnRestart(const char *key, int flags)
{
	if(flags & PTPD_RESTART_DAEMON) {
		NOTIFY("Change of %s setting requires "PTPD_PROGNAME" restart\n",key);
	} else {
		DBG("Setting %s changed, restart of subsystem %d required\n",key,flags);
	}
}

static int
configMapBoolean(int opCode, void *opArg,  dictionary* dict, dictionary *target,
		    const char * key, int restartFlags, Boolean *var, Boolean def, const char* helptext)
{

	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    return 1;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

		char *helpKey = (char*)opArg;

		if((opCode & CFGOP_HELP_SINGLE)){
		    if (strcmp(key, helpKey)) {
			return 1;
		    } else {
			/* this way we tell the caller that we have already found the setting we were looking for */
			/* ...it is a pretty shit method, I solemnly admit. */
			helpKey[0] = '\0';
		    }
		}

		printf("setting: %s (--%s)\n", key, key);
		printf("   type: BOOLEAN (value must start with t/T/y/Y/1/f/F/n/N/0)\n");
		printf("  usage: %s\n", helptext);
		printf("default: %s\n", def ? "Y" : "N");
		printf("\n");
		return 1;
	} else {
		if (!CONFIG_ISPRESENT(key)) {
		    *var = def;
		    dictionary_set(target,key,(*var)?"Y":"N");
		    if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
			    printComment(helptext);
			    printf("%s = %s\n", key,(*var)?"Y":"N");
		    }
		    return 1;
		} else if(!CONFIG_ISSET(key) || iniparser_getboolean(dict,key,-1) == -1) {
		    if(!(opCode & CFGOP_PARSE_QUIET)) {
			ERROR("Configuration error: option \"%s='%s'\" has unknown boolean value:  must start with 0/1/t/T/f/F/y/Y/n/N\n",key,iniparser_getstring(dict,key,""));
		    }
		    dictionary_set(target,key,""); /* suppress the "unknown entry" warning for malformed boolean values */ \
		    return 0;
		} else {
		    *var=iniparser_getboolean(dict,key,def);
		    dictionary_set(target,key,(*var)?"Y":"N");
		    if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
			    printComment(helptext);\
			    printf("%s = %s\n", key,(*var)?"Y":"N");
		    }
		    return 1;
		}
	}
}

static int
configMapBool(int opCode, void *opArg,  dictionary* dict, dictionary *target,
		    const char * key, int restartFlags, bool *var, bool def, const char* helptext)
{

	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    return 1;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

		char *helpKey = (char*)opArg;

		if((opCode & CFGOP_HELP_SINGLE)){
		    if (strcmp(key, helpKey)) {
			return 1;
		    } else {
			/* this way we tell the caller that we have already found the setting we were looking for */
			/* ...it is a pretty shit method, I solemnly admit. */
			helpKey[0] = '\0';
		    }
		}

		printf("setting: %s (--%s)\n", key, key);
		printf("   type: BOOLEAN (value must start with t/T/y/Y/1/f/F/n/N/0)\n");
		printf("  usage: %s\n", helptext);
		printf("default: %s\n", def ? "Y" : "N");
		printf("\n");
		return 1;
	} else {
		if (!CONFIG_ISPRESENT(key)) {
		    *var = def;
		    dictionary_set(target,key,(*var)?"Y":"N");
		    if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
			    printComment(helptext);
			    printf("%s = %s\n", key,(*var)?"Y":"N");
		    }
		    return 1;
		} else if(!CONFIG_ISSET(key) || iniparser_getboolean(dict,key,-1) == -1) {
		    if(!(opCode & CFGOP_PARSE_QUIET)) {
			ERROR("Configuration error: option \"%s='%s'\" has unknown boolean value:  must start with 0/1/t/T/f/F/y/Y/n/N\n",key,iniparser_getstring(dict,key,""));
		    }
		    dictionary_set(target,key,""); /* suppress the "unknown entry" warning for malformed boolean values */ \
		    return 0;
		} else {
		    *var=iniparser_getboolean(dict,key,def);
		    dictionary_set(target,key,(*var)?"Y":"N");
		    if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
			    printComment(helptext);\
			    printf("%s = %s\n", key,(*var)?"Y":"N");
		    }
		    return 1;
		}
	}
}

static int
configMapString(int opCode, void *opArg,  dictionary *dict, dictionary *target,
	const char *key, int restartFlags, char *var, int size, char *def, const char* helptext)
{
	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    return 1;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

		char *helpKey = (char*)opArg;

		if((opCode & CFGOP_HELP_SINGLE)){
		    if (strcmp(key, helpKey)) {
			return 1;
		    } else {
			/* this way we tell the caller that we have already found the setting we were looking for */
			helpKey[0] = '\0';
		    }
		}

		printf("setting: %s (--%s)\n", key, key);
		printf("   type: STRING\n");
		printf("  usage: %s\n", helptext);
		printf("default: %s\n", (strcmp(def, "") == 0) ? "[none]" : def);
		printf("\n");\
		return 1;
	} else {
		char *tmpstring = iniparser_getstring(dict,key,def);
		/* do not overwrite the same pointer with the same pointer */
		if (var!=tmpstring) {
		    strncpy(var, tmpstring, size);
		    var[size-1] = '\0';
		}
		dictionary_set(target, key, tmpstring);
		if(strcmp(helptext,"") && opCode & CFGOP_PRINT_DEFAULT) {
			printComment(helptext);
			printf("%s = %s\n", key,tmpstring);
		}
		return 1;
	}
}

static int
checkRangeInt(dictionary *dict, const char *key, int rangeFlags, int minBound, int maxBound)
{
	int tmpdouble = iniparser_getint(dict,key,minBound);

	int ret = 1;

	switch(rangeFlags) {
	    case RANGECHECK_NONE:
		return ret;
	    case RANGECHECK_RANGE:
		ret = !(tmpdouble < minBound || tmpdouble > maxBound);
		break;
	    case RANGECHECK_MIN:
		ret = !(tmpdouble < minBound);
		break;
	    case RANGECHECK_MAX:
		ret = !(tmpdouble > maxBound);
		break;
	    default:
		return 0;
	}

	return ret;
}

static int
configMapInt(int opCode, void *opArg,  dictionary *dict, dictionary *target, const char *key, int restartFlags, int intType,
		void *var, int def, const char *helptext, int rangeFlags,
		int minBound, int maxBound)
{
	int ret = 0;

	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    return 1;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

	    char *helpKey = (char*)opArg;

	    if((opCode & CFGOP_HELP_SINGLE)){
		if (strcmp(key, helpKey)) {
		    return 1;
		} else {
		    /* this way we tell the caller that we have already found the setting we were looking for */
		    helpKey[0] = '\0';
		}
	    }

	    switch(rangeFlags) {
	    case RANGECHECK_NONE:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: INT\n");\
		printf("  usage: %s\n", helptext);
		printf("default: %d\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_RANGE:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: INT ");
		if( minBound != maxBound )
		    printf("(min: %d, max: %d)\n", minBound, maxBound);
		else
		    printf("(only value of %d allowed)", minBound);
		printf("\n");
		printf("  usage: %s\n", helptext);
		printf("default: %d\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_MIN:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: INT (min: %d)\n", minBound);
		printf("  usage: %s\n", helptext);
		printf("default: %d\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_MAX:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: INT (max: %d)\n", maxBound);
		printf("  usage: %s\n", helptext);
		printf("default: %d\n", def);
		printf("\n");
		return 1;
	    default:
		return 0;
	    }
	} else {
		char buf[50];
		int userVar = iniparser_getint(dict,key,def);

		switch(intType) {
		    case INTTYPE_INT:
			*(int*)var = userVar;
			break;
		    case INTTYPE_I8:
			*(int8_t*)var = (int8_t)userVar;
			break;
		    case INTTYPE_U8:
			*(uint8_t*)var = (uint8_t)userVar;
			break;
		    case INTTYPE_I16:
			*(int16_t*)var = (int16_t)userVar;
			break;
		    case INTTYPE_U16:
			*(uint16_t*)var = (uint16_t)userVar;
			break;
		    case INTTYPE_I32:
			*(int32_t*)var = (int32_t)userVar;
			break;
		    case INTTYPE_U32:
			*(uint32_t*)var = (uint32_t)userVar;
			break;
		    default:
			break;
		}

		memset(buf, 0, 50);
		snprintf(buf, 50, "%d", userVar);
		dictionary_set(target,key,buf);
		if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
		    printComment(helptext);
		    printf("%s = %s\n", key,buf);
		}

		ret = checkRangeInt(dict, key, rangeFlags, minBound, maxBound);

		if(!ret && !(opCode & CFGOP_PARSE_QUIET)) {
		    switch(rangeFlags) {
			case RANGECHECK_RANGE:
				ERROR("Configuration error: option \"%s=%s\" not within allowed range: %d..%d\n", key, buf, minBound, maxBound);
			    break;
			case RANGECHECK_MIN:
				ERROR("Configuration error: option \"%s=%s\" below allowed minimum: %d\n", key, buf, minBound);
			    break;
			case RANGECHECK_MAX:
				ERROR("Configuration error: option \"%s=%s\" above allowed maximum: %d\n", key, buf, maxBound);
			    break;
			default:
			    return ret;
		    }
		}

		return ret;
	}
}

static int
checkRangeDouble(dictionary *dict, const char *key, int rangeFlags, double minBound, double maxBound)
{
	double tmpdouble = iniparser_getdouble(dict,key,minBound);

	int ret = 1;

	switch(rangeFlags) {
	    case RANGECHECK_NONE:
		return ret;
	    case RANGECHECK_RANGE:
		ret = !(tmpdouble < minBound || tmpdouble > maxBound);
		break;
	    case RANGECHECK_MIN:
		ret = !(tmpdouble < minBound);
		break;
	    case RANGECHECK_MAX:
		ret = !(tmpdouble > maxBound);
		break;
	    default:
		return 0;
	}

	return ret;
}

static int
configMapDouble(int opCode, void *opArg,  dictionary *dict, dictionary *target, const char *key, int restartFlags,
		    double *var, double def, const char *helptext, int rangeFlags,
		    double minBound, double maxBound)
{

	int ret = 0;
	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    return 1;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

	    char *helpKey = (char*)opArg;

	    if((opCode & CFGOP_HELP_SINGLE)){
		if (strcmp(key, helpKey)) {
		    return 1;
		} else {
		    /* this way we tell the caller that we have already found the setting we were looking for */
		    helpKey[0] = '\0';
		}
	    }

	    switch(rangeFlags) {
	    case RANGECHECK_NONE:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: FLOAT\n");\
		printf("  usage: %s\n", helptext);
		printf("default: %f\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_RANGE:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: FLOAT ");
		if( minBound != maxBound )
		    printf("(min: %f, max: %f)\n", minBound, maxBound);
		else
		    printf("(only value of %f allowed)", minBound);
		printf("\n");
		printf("  usage: %s\n", helptext);
		printf("default: %f\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_MIN:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: FLOAT(min: %f)\n", minBound);
		printf("  usage: %s\n", helptext);
		printf("default: %f\n", def);
		printf("\n");
		return 1;
	    case RANGECHECK_MAX:
		printf("setting: %s (--%s)\n", key, key);
		printf("   type: FLOAT(max: %f)\n", maxBound);
		printf("  usage: %s\n", helptext);
		printf("default: %f\n", def);
		printf("\n");
		return 1;
	    default:
		return 0;
	    }
	} else {
		char buf[50];
		*var = iniparser_getdouble(dict,key,def);
		memset(buf, 0, 50);
		snprintf(buf, 50, "%f", *var);
		dictionary_set(target,key,buf);
		if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
		    printComment(helptext);
		    printf("%s = %s\n", key,buf);
		}

		ret = checkRangeDouble(dict, key, rangeFlags, minBound, maxBound);

		if(!ret && !(opCode & CFGOP_PARSE_QUIET)) {
		    switch(rangeFlags) {
			case RANGECHECK_RANGE:
				ERROR("Configuration error: option \"%s=%f\" not within allowed range: %f..%f\n", key, *var, minBound, maxBound);
			    break;
			case RANGECHECK_MIN:
				ERROR("Configuration error: option \"%s=%f\" below allowed minimum: %f\n", key, *var, minBound);
			    break;
			case RANGECHECK_MAX:
				ERROR("Configuration error: option \"%s=%f\" above allowed maximum: %f\n", key, *var, maxBound);
			    break;
			default:
			    return ret;
		    }
		}
	
		return ret;
	}
}

static int
configMapSelectValue(int opCode, void *opArg,  dictionary *dict, dictionary *target,
const char* key, int restartFlags, uint8_t *var, int def, const char *helptext, ...)
{

    int ret;
    int i;
    /* fixed maximum to avoid while(1) */
    int maxCount = 100;
    char *name = NULL;
    char *keyValue;
    char *defValue = NULL;
    int value;
    va_list ap;
    int selectedValue = -1;

    char sbuf[SCREEN_BUFSZ];
    int len = 0;

    memset(sbuf, 0, sizeof(sbuf));

    keyValue = iniparser_getstring(dict, key, "");

    va_start(ap, helptext);

    for(i=0; i < maxCount; i++) {

	name = (char*)va_arg(ap, char*);
	if(name == NULL) break;

	value = (int)va_arg(ap, int);

	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s ", name);

	if(!defValue && value == def) {
	    defValue = strdup(name);
	}

	if(!strcmp(name, keyValue)) {
	    selectedValue = value;
	}

    }

    if(!strcmp(keyValue, "")) {
	selectedValue = def;
	keyValue = iniparser_getstring(dict, key, defValue);
    }

    va_end(ap);
	if(opCode & CFGOP_RESTART_FLAGS) {
	    if(CONFIG_ISSET(key)) {
		*(int*)opArg |= restartFlags;
		if(opCode & CFGOP_RESTART_FLAGS && !(opCode & CFGOP_PARSE_QUIET)) {
		    warnRestart(key, restartFlags);
		}
	    }
	    ret = 1;
	    goto result;
	} else if(opCode & CFGOP_HELP_FULL || opCode & CFGOP_HELP_SINGLE) {

		char *helpKey = (char*)opArg;

		if((opCode & CFGOP_HELP_SINGLE)){
		    if (strcmp(key, helpKey)) {
			ret = 1;
			goto result;
		    } else {
			/* this way we tell the caller that we have already found the setting we were looking for */
			helpKey[0] = '\0';
		    }
		}

		printf("setting: %s (--%s)\n", key, key);
		printf("   type: SELECT\n");
		printf("  usage: %s\n", helptext);
		printf("options: %s\n", sbuf);
		printf("default: %s\n", defValue);
		printf("\n");
		ret = 1;
		goto result;
	} else {
		dictionary_set(target, key, keyValue);
		if(selectedValue < 0) {
		    if(!(opCode & CFGOP_PARSE_QUIET)) {
			ERROR("Configuration error: option \"%s\" has unknown value: %s - allowed values: %s\n", key, keyValue, sbuf);
		    }
		    ret = 0;
		} else {
		    *var = (uint8_t)selectedValue;
		    if(strcmp(helptext, "") && opCode & CFGOP_PRINT_DEFAULT) {
		        printComment(helptext);
		        printf("; Options: %s\n", sbuf);
		        printf("%s = %s\n", key,keyValue);
		    }
		    ret = 1;
		}
	}
result:

    if(defValue != NULL) {
	free(defValue);
    }

    return ret;

}

void setConfig(dictionary *dict, const char* key, const char *value)
{

    if(dict == NULL) {
	return;
    }

    dictionary_set(dict, key, value);

}

static void
parseUserVariables(dictionary *dict, dictionary *target)
{

    int i = 0;
    char *search, *replace, *key;
    char varname[100];

    memset(varname, 0, sizeof(varname));

    if(dict == NULL) return;


    for(i = 0; i < dict->n; i++) {
	key = dict->key[i];
	/* key starts with "variables" */
        if(strstr(key,"variables:") == key) {
		/* this cannot fail if we are here */
		search = strstr(key,":");
		search++;
		replace = dictionary_get(dict, key, "");
		snprintf(varname, sizeof(varname), "@%s@", search);
		DBGV("replacing %s with %s in config\n", varname, replace);
		dictionary_replace(dict, varname, replace);
		dictionary_set(target, key, replace);
	}

    }

}

/**
 * Iterate through dictionary dict (template) and check for keys in source
 * that are not present in the template - display only.
 */
static void
findUnknownSettings(int opCode, dictionary* source, dictionary* dict)
{

    int i = 0;

    if( source == NULL || dict == NULL) return;

    for(i = 0; i < source->n; i++) {
	/* skip if the key is null or is a section */
        if(source->key[i] == NULL || strstr(source->key[i],":") == NULL)
            continue;
		    if ( !iniparser_find_entry(dict, source->key[i]) && !(opCode & CFGOP_PARSE_QUIET) )
			WARNING("Unknown configuration entry: %s - setting will be ignored\n", source->key[i]);
    }
}

/*
 * IT HAPPENS HERE
 *
 * Map all options from @dict dictionary to corresponding @rtopts fields,
 * using existing @rtopts fields as defaults. Return a dictionary free
 * of unknown options, with explicitly set defaults.
 */
dictionary*
parseConfig ( int opCode, void *opArg, dictionary* dict, GlobalConfig *global )
{

/*-
 * This function assumes that global has got all the defaults loaded,
 * hence the default values for all options are taken from global.
 * Therefore loadDefaultSettings should normally be used before parseConfig
 */



 /*-
  * WARNING: for ease of use, a limited number of keys is set
  * via getopt in loadCommanLineOptions(). When renaming settings, make sure
  * you check it  for inconsistencies. If we decide to
  * drop all short options in favour of section:key only,
  * this warning can be removed.
  */
	/**
	 * Prepare the target dictionary. Every mapped option will be set in
	 * the target dictionary, including the defaults which are typically
	 * not present in the original dictionary. As a result, the target
	 * will contain all the mapped options with explicitly set defaults
	 * if they were not present in the original dictionary. In the end
	 * of this function we return this pointer. The resulting dictionary
	 * is complete and free of any unknown options. In the end, warning
	 * is issued for unknown options. On any errors, NULL is returned
	 */

	dictionary* target = dictionary_new(0);

	Boolean parseResult = TRUE;

	PtpEnginePreset ptpPreset;

	if(!(opCode & CFGOP_PARSE_QUIET)) {
		INFO("Checking configuration\n");
	}

	/*
	 * apply the configuration template(s) as statically defined in configdefaults.c
	 * and in template files. The list of templates and files is comma, space or tab separated.
	 * names. Any templates are applied before anything else: so any settings after this can override
	 */
	if (CONFIG_ISPRESENT("global:config_templates")) {
	    applyConfigTemplates(
		    dict,
		    dictionary_get(dict, "global:config_templates", ""),
		    dictionary_get(dict, "global:template_files", "")
		);

	    /* also set the template names in the target dictionary */
	    dictionary_set(target, "global:config_templates", dictionary_get(dict, "global:config_templates", ""));
	    dictionary_set(target, "global:template_files", dictionary_get(dict, "global:template_files", ""));
	}

	/* set automatic variables */

	/* @pid@ */
	tmpsnprintf(tmpStr, 20, "%d", (uint32_t)getpid());
	dictionary_set(dict, "variables:pid", tmpStr);

	/* @hostname@ */
	char hostName[MAXHOSTNAMELEN+1];
	memset(hostName, 0, MAXHOSTNAMELEN + 1);
        gethostname(hostName, MAXHOSTNAMELEN);
	dictionary_set(dict, "variables:hostname", hostName);

	parseUserVariables(dict, target);

	dictionary_unset(dict, "variables:pid");
	dictionary_unset(target, "variables:pid");

	dictionary_unset(dict, "variables:hostname");
	dictionary_unset(target, "variables:hostname");


/* ============= BEGIN CONFIG MAPPINGS, TRIGGERS AND DEPENDENCIES =========== */

/* ===== ptpengine section ===== */

	CONFIG_KEY_REQUIRED("ptpengine:interface");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:interface",
		PTPD_RESTART_NETWORK, global->ifName, sizeof(global->ifName), global->ifName,
	"Network interface to use - eth0, igb0 etc. (required).");

	/* Preset option names have to be mapped to defined presets - no free strings here */
	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:preset",
		PTPD_RESTART_PROTOCOL, &global->selectedPreset, global->selectedPreset,
		"PTP engine preset:\n"
	"	 none	     = Defaults, no clock class restrictions\n"
	"        masteronly  = Master, passive when not best master (clock class 0..127)\n"
	"        masterslave = Full IEEE 1588 implementation:\n"
	"                      Master, slave when not best master\n"
	"	 	       (clock class 128..254)\n"
	"        slaveonly   = Slave only (clock class 255 only)\n",
#ifndef PTPD_SLAVE_ONLY
				(getPtpPreset(PTP_PRESET_NONE, global)).presetName,		PTP_PRESET_NONE,
				(getPtpPreset(PTP_PRESET_MASTERONLY, global)).presetName,	PTP_PRESET_MASTERONLY,
				(getPtpPreset(PTP_PRESET_MASTERSLAVE, global)).presetName,	PTP_PRESET_MASTERSLAVE,
#endif /* PTPD_SLAVE_ONLY */
				(getPtpPreset(PTP_PRESET_SLAVEONLY, global)).presetName,	PTP_PRESET_SLAVEONLY, NULL
				);

	ptpPreset = getPtpPreset(global->selectedPreset, global);

	parseResult &= configMapBool(opCode, opArg, dict, target, "ptpengine:master_first_lock",
		PTPD_RESTART_NONE, &global->masterFirstLock, global->masterFirstLock,
		 "Do not operate as PTP master until master port clock is in LOCKED state");

	parseResult &= configMapBool(opCode, opArg, dict, target, "ptpengine:master_locked_only",
		PTPD_RESTART_NONE, &global->masterFirstLock, global->masterFirstLock,
		 "Only operate as PTP master if master port clock is in LOCKED or HOLDOVER state");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:transport_protocol",
		PTPD_RESTART_NETWORK, &global->networkProtocol, global->networkProtocol,
		"Transport protocol used for PTP transmission. Unless ptpengine:transport_implementation\n"
	"        is used, the best available transport implementation will be selected.",
				"ipv4",		TT_FAMILY_IPV4,
				"ipv6",		TT_FAMILY_IPV6,
				"ethernet", 	TT_FAMILY_ETHERNET, NULL
				);

#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
    textname, typeenum,

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:transport_implementation",
		PTPD_RESTART_NETWORK, &global->transportType, global->transportType,
		"Use a specific transport implementation (overrides ptpengine:transport)",
				"auto",		TT_TYPE_NONE,
				#include <libcck/ttransport.def>
				NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:transport_monitor_interval",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->transportMonitorInterval, global->transportMonitorInterval,
		"Transport link state and address change monitoring interval (seconds).\n"
	"        Shorter interval provides faster reaction to changes, but more intensive polling.",
	RANGECHECK_RANGE, 1,60);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:transport_fault_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->transportFaultTimeout, global->transportFaultTimeout,
		"On transport failure, delay between attempting to test and restart the transport.",
	RANGECHECK_RANGE, 1,600);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:dot1as", PTPD_UPDATE_DATASETS, &global->dot1AS, global->dot1AS,
		"Enable TransportSpecific field compatibility with 802.1AS / AVB (requires Ethernet transport)");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disabled", PTPD_RESTART_PROTOCOL, &global->portDisabled, global->portDisabled,
		"Disable PTP port. Causes the PTP state machine to stay in PTP_DISABLED state indefinitely,\n"
	"        until it is re-enabled via configuration change or ENABLE_PORT management message.");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((global->networkProtocol != TT_FAMILY_ETHERNET) && global->dot1AS,
	 			    "ptpengine:dot1as",
				"802.1AS compatibility can only be used with the Ethernet transport\n");

	CONFIG_KEY_CONDITIONAL_TRIGGER(global->networkProtocol != TT_FAMILY_ETHERNET, global->dot1AS,FALSE, global->dot1AS);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:transport_mode",
		PTPD_RESTART_NETWORK, &global->transportMode, global->transportMode,
		"IP transmission mode (requires IP transport) - hybrid mode uses\n"
	"	 multicast for sync and announce, and unicast for delay request and\n"
	"	 response; unicast mode uses unicast for all transmission.\n"
	"	 When unicast mode is selected, destination IP(s) may need to be configured\n"
	"	(ptpengine:unicast_destinations).",
				"multicast", 	TMODE_MC,
				"unicast", 	TMODE_UC,
				"hybrid", 	TMODE_MIXED, NULL
				);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:source_address",
		PTPD_RESTART_NETWORK, global->sourceAddress, sizeof(global->sourceAddress), global->sourceAddress,
	"Source address to use (IPv4 or IPv6) - used when an interface has multiple addresses.\n"
	"        Using secondary addresses, similar to the ptpengine:bind_to_interface option,\n"
	"        may cause issues with multicast operation when using software timestamps.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:bind_to_interface",
		PTPD_RESTART_NETWORK, &global->bindToInterface, global->bindToInterface,
		"Always listen on the interface IP address, even if using multicast or hybrid mode.\n"
	"        For unicast operation, PTPd always binds to the IP address.\n"
	"        On Linux, this option should not be set for multicast on hybrid, but\n"
	"        on other systems like FreeBSD, this can help multiple PTPd instances to\n"
	"        co-exist on one system and process management messages independently.\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_negotiation",
		PTPD_RESTART_PROTOCOL, &global->unicastNegotiation, global->unicastNegotiation,
		"Enable unicast negotiation support using signaling messages\n");

	CONFIG_KEY_CONDITIONAL_CONFLICT("ptpengine:preset",
	 			    (global->selectedPreset == PTP_PRESET_MASTERSLAVE) && (global->transportMode == TMODE_UC) && (global->unicastNegotiation),
	 			    "masterslave",
	 			    "ptpengine:unicast_negotiation");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_any_master",
		PTPD_RESTART_NONE, &global->unicastAcceptAny, global->unicastAcceptAny,
		"When using unicast negotiation (slave), accept PTP messages from any master.\n"
	"        By default, only messages from acceptable masters (ptpengine:unicast_destinations)\n"
	"        are accepted, and only if transmission was granted by the master\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:unicast_port_mask",
		PTPD_RESTART_PROTOCOL, INTTYPE_U16, &global->unicastPortMask, global->unicastPortMask,
		"PTP port number wildcard mask applied onto port identities when running\n"
	"        unicast negotiation: allows multiple port identities to be accepted as one.\n"
	"	 This option can be used as a workaround where a node sends signaling messages and\n"
	"	 timing messages with different port identities", RANGECHECK_RANGE, 0,65535);

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((global->networkProtocol == TT_FAMILY_ETHERNET) && global->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation",
				"Unicast negotiation for Ethernet transport is a non-standard mode of PTP operation\n");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((global->transportMode != TMODE_UC) && global->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation",
				"Unicast negotiation can only be used with unicast transmission\n");

	/* disable unicast negotiation unless running unicast */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->transportMode != TMODE_UC, global->unicastNegotiation,FALSE, global->unicastNegotiation);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disable_bmca",
		PTPD_RESTART_PROTOCOL, &global->disableBMCA, global->disableBMCA,
		"Disable Best Master Clock Algorithm for unicast masters:\n"
	"        Only effective for masteronly preset - all Announce messages\n"
	"        will be ignored and clock will transition directly into MASTER state.\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_negotiation_listening",
		PTPD_RESTART_NONE, &global->unicastNegotiationListening, global->unicastNegotiationListening,
		"When unicast negotiation enabled on a master clock, \n"
	"	 reply to transmission requests also in LISTENING state.");

	/* libpcap is now only one of available transport implementation technologies */
	CONFIG_KEY_VALUE_FORBIDDEN("ptpengine:use_libpcap",
				    CONFIG_ISTRUE("ptpengine:use_libpcap"),
				    "y",
	    "The use of ptpengine:use_libpcap is deprecated. Please use one of the "
	    "libpcap-based transports instead if available (ptpengine:transport_implementation).\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disable_udp_checksums",
		PTPD_RESTART_NETWORK, &global->disableUdpChecksums, global->disableUdpChecksums,
		"Disable UDP checksum validation on UDP sockets (Linux only).\n"
	"        Workaround for situations where a node (like Transparent Clock).\n"
	"        does not rewrite checksums\n");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_mechanism",
		PTPD_RESTART_PROTOCOL, &global->delayMechanism, global->delayMechanism,
		 "Delay detection mode used - use DELAY_DISABLED for syntonisation only\n"
	"	 (no full synchronisation).",
				delayMechToString(E2E),		E2E,
				delayMechToString(P2P),		P2P,
				delayMechToString(DELAY_DISABLED), DELAY_DISABLED, NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:domain",
		PTPD_RESTART_PROTOCOL, INTTYPE_U8, &global->domainNumber, global->domainNumber,
		"PTP domain number.", RANGECHECK_RANGE, 0,127);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:port_number", PTPD_UPDATE_DATASETS, INTTYPE_U16, &global->portNumber, global->portNumber,
			    "PTP port number (part of PTP Port Identity - not UDP port).\n"
		    "        For ordinary clocks (single port), the default should be used, \n"
		    "        but when running multiple instances to simulate a boundary clock, \n"
		    "        The port number can be changed.",RANGECHECK_RANGE,1,65534);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:port_description",
		PTPD_UPDATE_DATASETS, global->portDescription, sizeof(global->portDescription), global->portDescription,
	"Port description (returned in the userDescription field of PORT_DESCRIPTION management message and USER_DESCRIPTION"
	"        management message) - maximum 64 characters");

	parseResult &= configMapString(opCode, opArg, dict, target, "variables:product_description",
		PTPD_UPDATE_DATASETS, global->productDescription, sizeof(global->productDescription), global->productDescription,"");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:any_domain",
		PTPD_RESTART_PROTOCOL, &global->anyDomain, global->anyDomain,
		"Usability extension: if enabled, a slave-only clock will accept\n"
	"	 masters from any domain, while preferring the configured domain,\n"
	"	 and preferring lower domain number.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:slave_only",
		PTPD_RESTART_NONE, &global->slaveOnly, ptpPreset.slaveOnly,
		 "Slave only mode (sets clock class to 255, overriding value from preset).");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:inbound_latency",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->inboundLatency.nanoseconds, global->inboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for incoming packets.", RANGECHECK_NONE, 0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:outbound_latency",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->outboundLatency.nanoseconds, global->outboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for outgoing packets.", RANGECHECK_NONE,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:offset_correction",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->ofmCorrection.nanoseconds, global->ofmCorrection.nanoseconds,
	"Apply an arbitrary shift (nanoseconds) to offset from master when\n"
	"	 in slave state. Value can be positive or negative - useful for\n"
	"	 correcting for antenna latencies, delay assymetry\n"
	"	 and IP stack latencies. This will not be visible in the offset \n"
	"	 from master value - only in the resulting clock correction. \n"
	"	 A positive value shifts the phase right (moves signal further behind reference), \n"
	"	 A negative value shifts the phase left (moves signal ahead of reference).", RANGECHECK_NONE, 0,0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:always_respect_utc_offset",
		PTPD_RESTART_NONE, &global->alwaysRespectUtcOffset, global->alwaysRespectUtcOffset,
		"Compatibility option: In slave state, always respect UTC offset\n"
	"	 announced by best master, even if the the\n"
	"	 currrentUtcOffsetValid flag is announced FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:prefer_utc_offset_valid",
		PTPD_RESTART_NONE, &global->preferUtcValid, global->preferUtcValid,
		"Compatibility extension to BMC algorithm: when enabled,\n"
	"	 BMC for both master and save clocks will prefer masters\n"
	"	 nannouncing currrentUtcOffsetValid as TRUE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:require_utc_offset_valid",
		PTPD_RESTART_NONE, &global->requireUtcValid, global->requireUtcValid,
		"Compatibility option: when enabled, ptpd will ignore\n"
	"	 Announce messages from masters announcing currentUtcOffsetValid\n"
	"	 as FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	/* from 30 seconds to 7 days */
	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:unicast_grant_duration",
		PTPD_RESTART_PROTOCOL, INTTYPE_U32, &global->unicastGrantDuration, global->unicastGrantDuration,
		"Time (seconds) unicast messages are requested for by slaves\n"
	"	 when using unicast negotiation, and maximum time unicast message\n"
	"	 transmission is granted to slaves by masters\n", RANGECHECK_RANGE, 30, 604800);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_announce_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logAnnounceInterval, global->logAnnounceInterval,
		"PTP announce message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-4,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_announce_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logMaxAnnounceInterval, global->logMaxAnnounceInterval,
		"Maximum Announce message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(global->logAnnounceInterval >= global->logMaxAnnounceInterval,
					"ptpengine:log_announce_interval value must be lower than ptpengine:log_announce_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:announce_receipt_timeout", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->announceReceiptTimeout, global->announceReceiptTimeout,
		"PTP announce receipt timeout announced in master state.",RANGECHECK_RANGE,2,255);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:announce_receipt_grace_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->announceTimeoutGracePeriod, global->announceTimeoutGracePeriod,
		"PTP announce receipt timeout grace period in slave state:\n"
	"	 when announce receipt timeout occurs, disqualify current best GM,\n"
	"	 then wait n times announce receipt timeout before resetting.\n"
	"	 Allows for a seamless GM failover when standby GMs are slow\n"
	"	 to react. When set to 0, this option is not used.", RANGECHECK_RANGE,
	0,20);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_sync_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logSyncInterval, global->logSyncInterval,
		"PTP sync message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_sync_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logMaxSyncInterval, global->logMaxSyncInterval,
		"Maximum Sync message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(global->logSyncInterval >= global->logMaxSyncInterval,
					"ptpengine:log_sync_interval value must be lower than ptpengine:log_sync_interval_max\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:log_delayreq_override", PTPD_UPDATE_DATASETS, &global->logDelayReqOverride,
	global->logDelayReqOverride,
		 "Override the Delay Request interval announced by best master.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:log_delayreq_auto", PTPD_UPDATE_DATASETS, &global->autoDelayReqInterval,
	global->autoDelayReqInterval,
		 "Automatically override the Delay Request interval\n"
	"         if the announced value is 127 (0X7F), such as in\n"
	"         unicast messages (unless using unicast negotiation)");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval_initial",
		PTPD_RESTART_NONE, INTTYPE_I8, &global->initial_delayreq, global->initial_delayreq,
		"Delay request interval used before receiving first delay response\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	/* take the delayreq_interval from config, otherwise use the initial setting as default */
	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logMinDelayReqInterval, global->initial_delayreq,
		"Minimum delay request interval announced when in master state,\n"
	"	 in slave state overrides the master interval,\n"
	"	 required in hybrid mode. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the minimum interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &global->logMaxDelayReqInterval, global->logMaxDelayReqInterval,
		"Maximum Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(global->logMinDelayReqInterval >= global->logMaxDelayReqInterval,
					"ptpengine:log_delayreq_interval value must be lower than ptpengine:log_delayreq_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_peer_delayreq_interval",
		PTPD_RESTART_NONE, INTTYPE_I8, &global->logMinPdelayReqInterval, global->logMinPdelayReqInterval,
		"Minimum peer delay request message interval in peer to peer delay mode.\n"
	"        When using unicast negotiation, this is the minimum interval requested, \n"
	"	 and the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_peer_delayreq_interval_max",
		PTPD_RESTART_NONE, INTTYPE_I8, &global->logMaxPdelayReqInterval, global->logMaxPdelayReqInterval,
		"Maximum Peer Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(global->logMinPdelayReqInterval >= global->logMaxPdelayReqInterval,
					"ptpengine:log_peer_delayreq_interval value must be lower than ptpengine:log_peer_delayreq_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:foreignrecord_capacity",
		PTPD_RESTART_DAEMON, INTTYPE_I16, &global->fmrCapacity, global->fmrCapacity,
	"Foreign master record size (Maximum number of foreign masters).",RANGECHECK_RANGE,5,10);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ptp_allan_variance", PTPD_UPDATE_DATASETS, INTTYPE_U16, &global->clockQuality.offsetScaledLogVariance, global->clockQuality.offsetScaledLogVariance,
	"Specify Allan variance announced in master state.",RANGECHECK_RANGE,0,65535);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_clock_accuracy", PTPD_UPDATE_DATASETS, &global->clockQuality.clockAccuracy, global->clockQuality.clockAccuracy,
	"Clock accuracy range announced in master state.",
				accToString(ACC_25NS),		ACC_25NS,
				accToString(ACC_100NS),		ACC_100NS,
				accToString(ACC_250NS),		ACC_250NS,
				accToString(ACC_1US),		ACC_1US,
				accToString(ACC_2_5US),		ACC_2_5US,
				accToString(ACC_10US), 		ACC_10US,
				accToString(ACC_25US), 		ACC_25US,
				accToString(ACC_100US),		ACC_100US,
				accToString(ACC_250US),		ACC_250US,
				accToString(ACC_1MS), 		ACC_1MS,
				accToString(ACC_2_5MS),		ACC_2_5MS,
				accToString(ACC_10MS), 		ACC_10MS,
				accToString(ACC_25MS),		ACC_25MS,
				accToString(ACC_100MS),		ACC_100MS,
				accToString(ACC_250MS),		ACC_250MS,
				accToString(ACC_1S), 		ACC_1S,
				accToString(ACC_10S), 		ACC_10S,
				accToString(ACC_10SPLUS),	ACC_10SPLUS,
				accToString(ACC_UNKNOWN),	ACC_UNKNOWN, NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:utc_offset", PTPD_UPDATE_DATASETS, INTTYPE_I16, &global->timeProperties.currentUtcOffset, global->timeProperties.currentUtcOffset,
		 "Underlying time source UTC offset announced in master state.", RANGECHECK_NONE,0,0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:utc_offset_valid", PTPD_UPDATE_DATASETS, &global->timeProperties.currentUtcOffsetValid,
	global->timeProperties.currentUtcOffsetValid,
		 "Underlying time source UTC offset validity announced in master state.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:time_traceable", PTPD_UPDATE_DATASETS, &global->timeProperties.timeTraceable,
	global->timeProperties.timeTraceable,
		 "Underlying time source time traceability announced in master state.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:frequency_traceable", PTPD_UPDATE_DATASETS, &global->timeProperties.frequencyTraceable,
	global->timeProperties.frequencyTraceable,
		 "Underlying time source frequency traceability announced in master state.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_timescale", PTPD_UPDATE_DATASETS, (uint8_t*)&global->timeProperties.ptpTimescale, global->timeProperties.ptpTimescale,
		"Time scale announced in master state (with ARB, UTC properties\n"
	"	 are ignored by slaves). When clock class is set to 13 (application\n"
	"	 specific), this value is ignored and ARB is used.",
				"PTP",		TRUE,
				"ARB",			FALSE, NULL
				);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_timesource", PTPD_UPDATE_DATASETS, &global->timeProperties.timeSource, global->timeProperties.timeSource,
	"Time source announced in master state.",
				"ATOMIC_CLOCK",		ATOMIC_CLOCK,
				"GPS",			GPS,
				"TERRESTRIAL_RADIO",	TERRESTRIAL_RADIO,
				"PTP",			PTP,
				"NTP",			NTP,
				"HAND_SET",		HAND_SET,
				"OTHER",		OTHER,
				"INTERNAL_OSCILLATOR",	INTERNAL_OSCILLATOR, NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:clock_class", PTPD_UPDATE_DATASETS, INTTYPE_U8, &global->clockQuality.clockClass,ptpPreset.clockClass.defaultValue,
		"Clock class - announced in master state. Always 255 for slave-only.\n"
	"	 Minimum, maximum and default values are controlled by presets.\n"
	"	 If set to 13 (application specific time source), announced \n"
	"	 time scale is always set to ARB. This setting controls the\n"
	"	 states a PTP port can be in. If below 128, port will only\n"
	"	 be in MASTER or PASSIVE states (master only). If above 127,\n"
	"	 port will be in MASTER or SLAVE states.", RANGECHECK_RANGE,
	ptpPreset.clockClass.minValue,ptpPreset.clockClass.maxValue);

	/* ClockClass = 13 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->clockQuality.clockClass==DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE,
					global->timeProperties.ptpTimescale,FALSE, global->timeProperties.ptpTimescale);

	/* ClockClass = 14 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->clockQuality.clockClass==14,
					global->timeProperties.ptpTimescale,FALSE, global->timeProperties.ptpTimescale);

	/* ClockClass = 6 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->clockQuality.clockClass==6,
					global->timeProperties.ptpTimescale,TRUE, global->timeProperties.ptpTimescale);

	/* ClockClass = 7 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->clockQuality.clockClass==7,
					global->timeProperties.ptpTimescale,TRUE, global->timeProperties.ptpTimescale);

	/* ClockClass = 255 triggers slaveOnly */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->clockQuality.clockClass==SLAVE_ONLY_CLOCK_CLASS, global->slaveOnly,TRUE,FALSE);
	/* ...and vice versa */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->slaveOnly==TRUE, global->clockQuality.clockClass,SLAVE_ONLY_CLOCK_CLASS, global->clockQuality.clockClass);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:priority1", PTPD_UPDATE_DATASETS, INTTYPE_U8, &global->priority1, global->priority1,
		"Priority 1 announced in master state,used for Best Master\n"
	"	 Clock selection.",RANGECHECK_RANGE,0,248);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:priority2", PTPD_UPDATE_DATASETS, INTTYPE_U8, &global->priority2, global->priority2,
		"Priority 2 announced in master state, used for Best Master\n"
	"	 Clock selection.",RANGECHECK_RANGE,0,248);

	/*
	 * TODO: in unicast and hybrid mode, automativally override master delayreq interval with a default,
	 * rather than require setting it manually.
	 */

	/* hybrid mode -> should specify delayreq interval: override set in the bottom of this function */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(global->transportMode == TMODE_MIXED,
    		    "ptpengine:log_delayreq_interval",
		    "It is recommended to manually set the delay request interval (ptpengine:log_delayreq_interval) to required value in hybrid mode\n"
	);

	/* unicast mode -> should specify delayreq interval if we can become a slave */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(global->transportMode == TMODE_UC &&
		    global->clockQuality.clockClass > 127,
		    "ptpengine:log_delayreq_interval",
		    "It is recommended to manually set the delay request interval (ptpengine:log_delayreq_interval) to required value in unicast mode\n"
	);

	CONFIG_KEY_ALIAS("ptpengine:unicast_address","ptpengine:unicast_destinations");
	CONFIG_KEY_ALIAS("ptpengine:ip_mode","ptpengine:transport_mode");

	/* unicast signaling slave -> must specify unicast destination(s) */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:transport_mode",
				     global->clockQuality.clockClass > 127 &&
				    global->transportMode == TMODE_UC &&
				    global->unicastNegotiation,
				    "unicast",
				    "ptpengine:unicast_destinations");



	/* unicast master without signaling - must specify unicast destinations */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:transport_mode",
				     global->clockQuality.clockClass <= 127 &&
				    global->transportMode == TMODE_UC &&
				    !global->unicastNegotiation,
				    "unicast",
				    "ptpengine:unicast_destinations");

	CONFIG_KEY_TRIGGER("ptpengine:unicast_destinations", global->unicastDestinationsSet,TRUE, global->unicastDestinationsSet);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_destinations",
		PTPD_RESTART_NETWORK, global->unicastDestinations, sizeof(global->unicastDestinations), global->unicastDestinations,
		"Specify unicast slave addresses for unicast master operation, or unicast\n"
	"	 master addresses for slave operation. Format is similar to an ACL: comma,\n"
	"        tab or space-separated IPv4 unicast addresses, one or more. For a slave,\n"
	"        when unicast negotiation is used, setting this is mandatory.");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_domains",
		PTPD_RESTART_NETWORK, global->unicastDomains, sizeof(global->unicastDomains), global->unicastDomains,
		"Specify PTP domain number for each configured unicast destination (ptpengine:unicast_destinations).\n"
	"	 This is only used by slave-only clocks using unicast destinations to allow for each master\n"
	"        to be in a separate domain, such as with Telecom Profile. The number of entries should match the number\n"
	"        of unicast destinations, otherwise unconfigured domains or domains set to 0 are set to domain configured in\n"
	"        ptpengine:domain. The format is a comma, tab or space-separated list of 8-bit unsigned integers (0 .. 255)");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_local_preference",
		PTPD_RESTART_NETWORK, global->unicastLocalPreference, sizeof(global->unicastLocalPreference), global->unicastLocalPreference,
		"Specify a local preference for each configured unicast destination (ptpengine:unicast_destinations).\n"
	"	 This is only used by slave-only clocks using unicast destinations to allow for each master's\n"
	"        BMC selection to be influenced by the slave, such as with Telecom Profile. The number of entries should match the number\n"
	"        of unicast destinations, otherwise unconfigured preference is set to 0 (highest).\n"
	"        The format is a comma, tab or space-separated list of 8-bit unsigned integers (0 .. 255)");

	/* unicast P2P - must specify unicast peer destination */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:delay_mechanism",
					global->delayMechanism == P2P &&
				    global->transportMode == TMODE_UC,
				    "P2P",
				    "ptpengine:unicast_peer_destination");

	CONFIG_KEY_TRIGGER("ptpengine:unicast_peer_destination", global->unicastPeerDestinationSet,TRUE, global->unicastPeerDestinationSet);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_peer_destination",
		PTPD_RESTART_NETWORK, global->unicastPeerDestination, sizeof(global->unicastPeerDestination), global->unicastPeerDestination,
		"Specify peer unicast adress for P2P unicast. Mandatory when\n"
	"	 running unicast mode and P2P delay mode.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:hardware_timestamping",
		PTPD_RESTART_NETWORK, &global->hwTimestamping, global->hwTimestamping,
	"Prefer hardware timestamping (if available)");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:management_enable",
		PTPD_RESTART_NONE, &global->managementEnabled, global->managementEnabled,
	"Enable handling of PTP management messages.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:management_set_enable",
		PTPD_RESTART_NONE, &global->managementSetEnable, global->managementSetEnable,
	"Accept SET and COMMAND management messages.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:ptpmon_enable",
		PTPD_RESTART_NONE, &global->ptpMonEnabled, global->ptpMonEnabled,
		"Enable support for PTPMON monitoring extensions.\n"
	"	 NOTE: if used in conjunction with hardware timestamping, timestamping\n"
	"	 support for unicast packets is required!");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:ptpmon_any_domain",
		PTPD_RESTART_NONE, &global->ptpMonAnyDomain, global->ptpMonAnyDomain,
		"Accept PTPMON Delay Request from any domain.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ptpmon_domain",
		PTPD_RESTART_NONE, INTTYPE_I8, &global->ptpMonDomainNumber, global->domainNumber,
		"Allowed PTP domain number to accept PTPMON Delay Requests from.\n"
	"	 The default is to use the PTPd domain number (ptpengine:domain).", RANGECHECK_RANGE,0,255);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:multicast_ttl",
		PTPD_RESTART_NETWORK, INTTYPE_INT, &global->ttl, global->ttl,
		"Multicast time to live for multicast PTP packets (ignored and set to 1\n"
	"	 for peer to peer messages).",RANGECHECK_RANGE,1,64);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ipv6_multicast_scope",
		PTPD_RESTART_NETWORK, &global->ipv6Scope, global->ipv6Scope,
		"IPv6 multicast scope for PTP multicast transmission.\n",

				"interface-local", IPV6_SCOPE_INT_LOCAL,
				"link-local", 	IPV6_SCOPE_LINK_LOCAL,
				"ream-local", 	IPV6_SCOPE_REALM_LOCAL,
				"admin-local", 	IPV6_SCOPE_ADMIN_LOCAL,
				"site-local", 	IPV6_SCOPE_SITE_LOCAL,
				"org-local", 	IPV6_SCOPE_ORG_LOCAL,
				"global", 	IPV6_SCOPE_GLOBAL,

				"0x01",		IPV6_SCOPE_INT_LOCAL,
				"0x02", 	IPV6_SCOPE_LINK_LOCAL,
				"0x03", 	IPV6_SCOPE_REALM_LOCAL,
				"0x04", 	IPV6_SCOPE_ADMIN_LOCAL,
				"0x05", 	IPV6_SCOPE_SITE_LOCAL,
				"0x08", 	IPV6_SCOPE_ORG_LOCAL,
				"0x0E", 	IPV6_SCOPE_GLOBAL,

				NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ip_dscp",
		PTPD_RESTART_NETWORK, INTTYPE_INT, &global->dscpValue, global->dscpValue,
		"DiffServ CodepPoint for packet prioritisation (decimal). When set to zero, \n"
	"	 this option is not used. Use 46 for Expedited Forwarding (0x2e).",RANGECHECK_RANGE,0,63);

	parseResult &= configMapBool(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_enable",
		PTPD_RESTART_FILTERS, &global->filterMSOpts.enabled, global->filterMSOpts.enabled,
		 "Enable statistical filter for Sync messages.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_type",
		PTPD_RESTART_FILTERS, &global->filterMSOpts.filterType, global->filterMSOpts.filterType,
		"Type of filter used for Sync message filtering",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->filterMSOpts.windowSize, global->filterMSOpts.windowSize,
		"Number of samples used for the Sync statistical filter",RANGECHECK_RANGE,3,STATCONTAINER_MAX_SAMPLES);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_interval",
		PTPD_RESTART_FILTERS, INTTYPE_U16, &global->filterMSOpts.samplingInterval, global->filterMSOpts.samplingInterval,
		"Sampling interval (samples) used for the Sync statistical filter\n"
		"	 when the window type is set to interval. If set to zero,\n"
		"	 the sampling window size is used as the interval.",RANGECHECK_RANGE,0, 65535);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_window_type",
		PTPD_RESTART_FILTERS, &global->filterMSOpts.windowType, global->filterMSOpts.windowType,
		"Sample window type used for Sync message statistical filter.\n"
	"        Sliding window is continuous, interval passes every n-th sample only.",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL, NULL);

	parseResult &= configMapBool(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_enable",
		PTPD_RESTART_FILTERS, &global->filterSMOpts.enabled, global->filterSMOpts.enabled,
		 "Enable statistical filter for Delay messages.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_type",
		PTPD_RESTART_FILTERS, &global->filterSMOpts.filterType, global->filterSMOpts.filterType,
		"Type of filter used for Delay message statistical filter",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->filterSMOpts.windowSize, global->filterSMOpts.windowSize,
		"Number of samples used for the Delay statistical filter",RANGECHECK_RANGE,3,5120);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_interval",
		PTPD_RESTART_FILTERS, INTTYPE_U16, &global->filterSMOpts.samplingInterval, global->filterSMOpts.samplingInterval,
		"Sampling interval (samples) used for the Delay statistical filter\n"
		"	 when the window type is set to interval. If set to zero,\n"
		"	 the sampling window size is used as the interval.",RANGECHECK_RANGE,0, 65535);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_window_type",
		PTPD_RESTART_FILTERS, &global->filterSMOpts.windowType, global->filterSMOpts.windowType,
		"Sample window type used for Delay message statistical filter\n"
	"        Sliding window is continuous, interval passes every n-th sample only",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL, NULL);


	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_enable",
		PTPD_RESTART_FILTERS, &global->oFilterSMConfig.enabled, global->oFilterSMConfig.enabled,
		 "Enable outlier filter for the Delay Response component in slave state");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_action",
		PTPD_RESTART_NONE, (uint8_t*)&global->oFilterSMConfig.discard, global->oFilterSMConfig.discard,
		"Delay Response outlier filter action. If set to 'filter', outliers are\n"
	"	 replaced with moving average.",
	"discard", TRUE,
	"filter", FALSE, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_capacity",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->oFilterSMConfig.capacity, global->oFilterSMConfig.capacity,
		"Number of samples in the Delay Response outlier filter buffer",RANGECHECK_RANGE,5, PEIRCE_MAX_SAMPLES);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_threshold",
		PTPD_RESTART_NONE, &global->oFilterSMConfig.threshold, global->oFilterSMConfig.threshold,
		"Delay Response outlier filter threshold (: multiplier for Peirce's maximum\n"
	"	 standard deviation. When set below 1.0, filter is tighter, when set above\n"
	"	 1.0, filter is looser than standard Peirce's test.\n"
	"        When autotune enabled, this is the starting threshold.", RANGECHECK_RANGE, 0.001, 1000.0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_always_filter",
		PTPD_RESTART_NONE, &global->oFilterSMConfig.alwaysFilter, global->oFilterSMConfig.alwaysFilter,
		"Always run the Delay Response outlier filter, even if clock is being slewed at maximum rate");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_enable",
		PTPD_RESTART_FILTERS, &global->oFilterSMConfig.autoTune, global->oFilterSMConfig.autoTune,
		"Enable automatic threshold control for Delay Response outlier filter.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_minpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterSMConfig.minPercent, global->oFilterSMConfig.minPercent,
		"Delay Response outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,0,99);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_maxpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterSMConfig.maxPercent, global->oFilterSMConfig.maxPercent,
		"Delay Response outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_autotune_step",
		PTPD_RESTART_NONE, &global->oFilterSMConfig.thresholdStep, global->oFilterSMConfig.thresholdStep,
		"The value the Delay Response outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_minthreshold",
		PTPD_RESTART_FILTERS, &global->oFilterSMConfig.minThreshold, global->oFilterSMConfig.minThreshold,
		"Minimum Delay Response filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_maxthreshold",
		PTPD_RESTART_FILTERS, &global->oFilterSMConfig.maxThreshold, global->oFilterSMConfig.maxThreshold,
		"Maximum Delay Response filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(global->oFilterSMConfig.maxPercent <= global->oFilterSMConfig.minPercent,
					"ptpengine:delay_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(global->oFilterSMConfig.maxThreshold <= global->oFilterSMConfig.minThreshold,
					"ptpengine:delay_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minthreshold\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_enable",
		PTPD_RESTART_FILTERS, &global->oFilterSMConfig.stepDelay, global->oFilterSMConfig.stepDelay,
		"Enable Delay filter step detection (delaySM) to block when certain level exceeded");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_threshold",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->oFilterSMConfig.stepThreshold, global->oFilterSMConfig.stepThreshold,
		"Delay Response step detection threshold. Step detection is performed\n"
	"	 only when delaySM is below this threshold (nanoseconds)", RANGECHECK_RANGE, 50000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_level",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->oFilterSMConfig.stepLevel, global->oFilterSMConfig.stepLevel,
		"Delay Response step level. When step detection enabled and operational,\n"
	"	 delaySM above this level (nanosecond) is considered a clock step and updates are paused", RANGECHECK_RANGE,50000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_credit",
		PTPD_RESTART_FILTERS, INTTYPE_I32, &global->oFilterSMConfig.delayCredit, global->oFilterSMConfig.delayCredit,
		"Initial credit (number of samples) the Delay step detection filter can block for\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",RANGECHECK_RANGE,50,1000);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_credit_increment",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterSMConfig.creditIncrement, global->oFilterSMConfig.creditIncrement,
		"Amount of credit for the Delay step detection filter restored every full sample window",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_weight",
		PTPD_RESTART_NONE, &global->oFilterSMConfig.weight, global->oFilterSMConfig.weight,
		"Delay Response outlier weight: if an outlier is detected, determines\n"
	"	 the amount of its deviation from mean that is used to build the standard\n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", RANGECHECK_RANGE, 0.01, 2.0);

    parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_enable",
		PTPD_RESTART_FILTERS, &global->oFilterMSConfig.enabled, global->oFilterMSConfig.enabled,
		"Enable outlier filter for the Sync component in slave state.");

    parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_action",
		PTPD_RESTART_NONE, (uint8_t*)&global->oFilterMSConfig.discard, global->oFilterMSConfig.discard,
		"Sync outlier filter action. If set to 'filter', outliers are replaced\n"
	"	 with moving average.",
     "discard", TRUE,
     "filter", FALSE, NULL);

     parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_capacity",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->oFilterMSConfig.capacity, global->oFilterMSConfig.capacity,
    "Number of samples in the Sync outlier filter buffer.",RANGECHECK_RANGE,5,PEIRCE_MAX_SAMPLES);

    parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_threshold",
		PTPD_RESTART_NONE, &global->oFilterMSConfig.threshold, global->oFilterMSConfig.threshold,
		"Sync outlier filter threshold: multiplier for the Peirce's maximum standard\n"
	"	 deviation. When set below 1.0, filter is tighter, when set above 1.0,\n"
	"	 filter is looser than standard Peirce's test.", RANGECHECK_RANGE, 0.001, 1000.0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_always_filter",
		PTPD_RESTART_NONE, &global->oFilterMSConfig.alwaysFilter, global->oFilterMSConfig.alwaysFilter,
		"Always run the Sync outlier filter, even if clock is being slewed at maximum rate");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_enable",
		PTPD_RESTART_FILTERS, &global->oFilterMSConfig.autoTune, global->oFilterMSConfig.autoTune,
		"Enable automatic threshold control for Sync outlier filter.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_minpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterMSConfig.minPercent, global->oFilterMSConfig.minPercent,
		"Sync outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,0,99);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_maxpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterMSConfig.maxPercent, global->oFilterMSConfig.maxPercent,
		"Sync outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_autotune_step",
		PTPD_RESTART_NONE, &global->oFilterMSConfig.thresholdStep, global->oFilterMSConfig.thresholdStep,
		"Value the Sync outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_minthreshold",
		PTPD_RESTART_FILTERS, &global->oFilterMSConfig.minThreshold, global->oFilterMSConfig.minThreshold,
		"Minimum Sync outlier filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_maxthreshold",
		PTPD_RESTART_FILTERS, &global->oFilterMSConfig.maxThreshold, global->oFilterMSConfig.maxThreshold,
		"Maximum Sync outlier filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(global->oFilterMSConfig.maxPercent <= global->oFilterMSConfig.minPercent,
					"ptpengine:sync_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(global->oFilterMSConfig.maxThreshold <= global->oFilterMSConfig.minThreshold,
					"ptpengine:sync_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minthreshold\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_enable",
		PTPD_RESTART_FILTERS, &global->oFilterMSConfig.stepDelay, global->oFilterMSConfig.stepDelay,
		"Enable Sync filter step detection (delayMS) to block when certain level exceeded.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_threshold",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->oFilterMSConfig.stepThreshold, global->oFilterMSConfig.stepThreshold,
		"Sync step detection threshold. Step detection is performed\n"
	"	 only when delayMS is below this threshold (nanoseconds)", RANGECHECK_RANGE,100000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_level",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->oFilterMSConfig.stepLevel, global->oFilterMSConfig.stepLevel,
		"Sync step level. When step detection enabled and operational,\n"
	"	 delayMS above this level (nanosecond) is considered a clock step and updates are paused", RANGECHECK_RANGE,100000, NANOSECONDS_MAX);

	CONFIG_CONDITIONAL_ASSERTION(global->oFilterMSConfig.stepThreshold <= (global->oFilterMSConfig.stepLevel + 100000),
					"ptpengine:sync_outlier_filter_stepdetect_threshold  has to be at least "
					"100 us (100000) greater than ptpengine:sync_outlier_filter_stepdetect_level\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_credit",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->oFilterMSConfig.delayCredit, global->oFilterMSConfig.delayCredit,
		"Initial credit (number of samples) the Sync step detection filter can block for.\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",RANGECHECK_RANGE,50,1000);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_credit_increment",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->oFilterMSConfig.creditIncrement, global->oFilterMSConfig.creditIncrement,
		"Amount of credit for the Sync step detection filter restored every full sample window",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_weight",
		PTPD_RESTART_NONE, &global->oFilterMSConfig.weight, global->oFilterMSConfig.weight,
		"Sync outlier weight: if an outlier is detected, this value determines the\n"
	"	 amount of its deviation from mean that is used to build the standard \n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", RANGECHECK_RANGE, 0.01, 2.0);

        parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:calibration_delay",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->calibrationDelay, global->calibrationDelay,
		"Delay between moving to slave state and enabling clock updates (seconds).\n"
	"	 This allows one-way delay to stabilise before starting clock updates.\n"
	"	 Activated when going into slave state and during slave's GM failover.\n"
	"	 0 - not used.",RANGECHECK_RANGE,0,300);

        parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:idle_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->idleTimeout, global->idleTimeout,
		"PTP idle timeout: if PTPd is in SLAVE state and there have been no clock\n"
	"	 updates for this amout of time, PTPd releases clock control.\n", RANGECHECK_RANGE,10,3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:offset_alarm_threshold", PTPD_UPDATE_DATASETS, INTTYPE_U32, &global->ofmAlarmThreshold, global->ofmAlarmThreshold,
		 "PTP slave offset from master threshold (nanoseconds - absolute value)\n"
	"	 When offset exceeds this value, an alarm is raised (also SNMP trap if configured).\n"
	"	 0 = disabled.", RANGECHECK_NONE,0,NANOSECONDS_MAX);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode",
		PTPD_RESTART_NONE, &global->enablePanicMode, global->enablePanicMode,
		"Enable panic mode: when offset from master is above 1 second, stop updating\n"
	"	 the clock for a period of time and then step the clock if offset remains\n"
	"	 above 1 second.");

    parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:panic_mode_duration",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->panicModeDuration, global->panicModeDuration,
		"Duration (seconds) of the panic mode period (no clock updates) when offset\n"
	"	 above 1 second detected.",RANGECHECK_RANGE,1,7200);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode_release_clock",
		PTPD_RESTART_NONE, &global->panicModeReleaseClock, global->panicModeReleaseClock,
		"When entering panic mode, release clock control while panic mode lasts\n"
 	"	 if ntpengine:* configured, this will fail over to NTP,\n"
	"	 if not set, PTP will hold clock control during panic mode.");

    parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:panic_mode_exit_threshold",
		PTPD_RESTART_NONE, INTTYPE_U32, &global->panicModeExitThreshold, global->panicModeExitThreshold,
		"Do not exit panic mode until offset drops below this value (nanoseconds).\n"
	"	 0 = not used.",RANGECHECK_RANGE,0,NANOSECONDS_MAX);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:pid_as_clock_identity",
		PTPD_RESTART_PROTOCOL, &global->pidAsClockId, global->pidAsClockId,
	"Use PTPd's process ID as the middle part of the PTP clock ID - useful for running multiple instances.");

/*
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:ntp_failover",
		PTPD_RESTART_NONE, &global->ntpOptions.enableFailover, global->ntpOptions.enableFailover,
		"Fail over to NTP when PTP time sync not available - requires\n"
	"	 ntpengine:enabled, but does not require the rest of NTP configuration:\n"
	"	 will warn instead of failing over if cannot control ntpd.");

	CONFIG_KEY_DEPENDENCY("ptpengine:ntp_failover", "ntpengine:enabled");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ntp_failover_timeout",
		PTPD_RESTART_NTPCONFIG, INTTYPE_INT, &global->ntpOptions.failoverTimeout,
								global->ntpOptions.failoverTimeout,	
		"NTP failover timeout in seconds: time between PTP slave going into\n"
	"	 LISTENING state, and releasing clock control. 0 = fail over immediately.", RANGECHECK_RANGE,0, 1800);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:prefer_ntp",
		PTPD_RESTART_NTPCONFIG, &global->preferNTP, global->preferNTP,
		"Prefer NTP time synchronisation. Only use PTP when NTP not available,\n"
	"	 could be used when NTP runs with a local GPS receiver or another reference");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode_ntp",
		PTPD_RESTART_NONE, &global->panicModeReleaseClock, global->panicModeReleaseClock,
		"Legacy option from 2.3.0: same as ptpengine:panic_mode_release_clock");

	CONFIG_KEY_DEPENDENCY("ptpengine:panic_mode_ntp", "ntpengine:enabled");
*/

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sigusr2_clears_counters",
		PTPD_RESTART_NONE, &global->clearCounters, global->clearCounters,
		"Clear counters after dumping all counter values on SIGUSR2.");

	/* Defining the ACLs enables ACL matching */
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_permit", global->timingAclEnabled,TRUE, global->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_deny", global->timingAclEnabled,TRUE, global->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_permit", global->managementAclEnabled,TRUE, global->managementAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_deny", global->managementAclEnabled,TRUE, global->managementAclEnabled);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:timing_acl_permit",
		PTPD_RESTART_ACLS, global->timingAclPermitText, sizeof(global->timingAclPermitText), global->timingAclPermitText,
		"Permit access control list for timing packets.\n"
        "        Format is a series of comma, space or tab separated  network prefixes:\n"
	"	- IPv4 address or full CIDR notation a.b.c.d/m.m.m.m or a.b.c.d/n\n"
	"	- IPv6 address or subnet aaaa:bbbb:cccc:dddd::fff/n\n"
	"	- Ethernet address subnet 00:aa:bb:cc:dd:ee/n\n"
        "        The match is performed on the source address of the incoming messages.\n");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:timing_acl_deny",
		PTPD_RESTART_ACLS, global->timingAclDenyText, sizeof(global->timingAclDenyText), global->timingAclDenyText,
		"Deny access control list for timing packets.\n"
        "        Format is a series of comma, space or tab separated  network prefixes:\n"
	"	- IPv4 address or full CIDR notation a.b.c.d/m.m.m.m or a.b.c.d/n\n"
	"	- IPv6 address or subnet aaaa:bbbb:cccc:dddd::fff/n\n"
	"	- Ethernet address subnet 00:aa:bb:cc:dd:ee/n\n"
        "        The match is performed on the source address of the incoming messages.\n");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:management_acl_permit",
		PTPD_RESTART_ACLS, global->managementAclPermitText, sizeof(global->managementAclPermitText), global->managementAclPermitText,
		"Permit access control list for management messages and monitoring extensions.\n"
        "        Format is a series of comma, space or tab separated  network prefixes:\n"
	"	- IPv4 address or full CIDR notation a.b.c.d/m.m.m.m or a.b.c.d/n\n"
	"	- IPv6 address or subnet aaaa:bbbb:cccc:dddd::fff/n\n"
	"	- Ethernet address subnet 00:aa:bb:cc:dd:ee/n\n"
        "        The match is performed on the source address of the incoming messages.\n");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:management_acl_deny",
		PTPD_RESTART_ACLS, global->managementAclDenyText, sizeof(global->managementAclDenyText), global->managementAclDenyText,
		"Deny access control list for management messages and monitoring extensions.\n"
        "        Format is a series of comma, space or tab separated  network prefixes:\n"
	"	- IPv4 address or full CIDR notation a.b.c.d/m.m.m.m or a.b.c.d/n\n"
	"	- IPv6 address or subnet aaaa:bbbb:cccc:dddd::fff/n\n"
	"	- Ethernet address subnet 00:aa:bb:cc:dd:ee/n\n"
        "        The match is performed on the source address of the incoming messages.\n");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:timing_acl_order",
		PTPD_RESTART_ACLS, &global->timingAclOrder, global->timingAclOrder,
		"Order in which permit and deny access lists are evaluated for timing\n"
	"	 packets, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	CCK_ACL_PERMIT_DENY,
				"deny-permit", 	CCK_ACL_DENY_PERMIT, NULL
				);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:management_acl_order",
		PTPD_RESTART_ACLS, &global->managementAclOrder, global->managementAclOrder,
		"Order in which permit and deny access lists are evaluated for management\n"
	"	 messages, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	CCK_ACL_PERMIT_DENY,
				"deny-permit", 	CCK_ACL_DENY_PERMIT, NULL
				);

/* ===== clock section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:no_adjust",
		PTPD_RESTART_NONE, &global->noAdjust,ptpPreset.noAdjust,
	"Do not adjust the clock");

	CONFIG_KEY_ALIAS("clock:no_reset","clock:no_step");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:no_step",
		PTPD_RESTART_NONE, &global->noStep, global->noStep,
	"Do not step the clock - only slew");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:calibration_time",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->clockCalibrationTime,
		global->clockCalibrationTime,
		"Frequency estimation time (seconds) after a clock has gained a reference.\n"
		"        Zero - no estimation.",
		RANGECHECK_RANGE,0,3600);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:allow_step_backwards",
		PTPD_RESTART_NONE, &global->negativeStep,global->negativeStep,
	"Allow a software clock (system clock) to be stepped backwards");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:allow_step_backwards_hw",
		PTPD_RESTART_NONE, &global->negativeStep_hw, global->negativeStep_hw,
	"Allow a hardware clock to be stepped backwards");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:step_startup_force",
		PTPD_RESTART_NONE, &global->stepForce, global->stepForce,
	"Force clock step on first sync after startup regardless of offset and clock:no_step");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:step_startup",
		PTPD_RESTART_NONE, &global->stepOnce, global->stepOnce,
		"Step clock on startup if offset >= +/-1 second, ignoring\n"
	"        panic mode and clock:no_step");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:strict_sync",
		PTPD_RESTART_NONE, &global->clockStrictSync, global->clockStrictSync,
	"Explicitly prevent clocks from sync with reference in state worse than HOLDOVER");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:min_step",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->clockMinStep,
		global->clockMinStep,
		"Minimum delta (nanoseconds) that a clock can be stepped by.\n"
		"        Zero - no limit. Stepping clock by a very small delta\n"
		"        may be imprecise for some clock drivers and cause\n"
		"        a larger offset than the delta requested.\n",
		RANGECHECK_RANGE,0,NANOSECONDS_MAX);

#ifdef HAVE_LINUX_RTC_H
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:set_rtc_on_step",
		PTPD_RESTART_NONE, &global->setRtc, global->setRtc,
	"Attempt setting the RTC when stepping clock (Linux only - FreeBSD does \n"
	"        this for us. WARNING: this will always set the RTC to OS clock time,\n"
	"        regardless of time zones, so this assumes that RTC runs in UTC or \n"
	"        at least in the same timescale as PTP. true at least on most \n"
	"        single-boot x86 Linux systems.");
#endif /* HAVE_LINUX_RTC_H */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:store_frequency",
		PTPD_RESTART_NONE, &global->storeToFile, global->storeToFile,
		"Store current clock frequency offset to file when a clock is stable and locked,:\n"
	"	 and load frequency offset from file when clock is started.");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:frequency_directory",
		PTPD_RESTART_NONE, global->frequencyDir, sizeof(global->frequencyDir), global->frequencyDir,
	"Specify the directory where frequency files are stored and read from");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:leap_second_pause_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->leapSecondPausePeriod,
		global->leapSecondPausePeriod,
		"Time (seconds) before and after midnight that clock updates should pe suspended for\n"
	"	 during a leap second event. The total duration of the pause is twice\n"
	"        the configured duration",RANGECHECK_RANGE,5,600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:leap_second_notice_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->leapSecondNoticePeriod,
		global->leapSecondNoticePeriod,
		"Time (seconds) before midnight that PTPd starts announcing the leap second\n"
	"	 if it's running as master",RANGECHECK_RANGE,3600,86399);

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:leap_seconds_file",
		PTPD_RESTART_NONE, global->leapFile, sizeof(global->leapFile), global->leapFile,
	"Specify leap second file location - up to date version can be downloaded from \n"
	"        http://www.ietf.org/timezones/data/leap-seconds.list");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "clock:leap_second_handling",
		PTPD_RESTART_NONE, &global->leapSecondHandling, global->leapSecondHandling,
		"Behaviour during a leap second event:\n"
	"	 accept: inform the OS kernel of the event\n"
	"	 ignore: do nothing - ends up with a 1-second offset which is then slewed\n"
	"	 step: similar to ignore, but steps the clock immediately after the leap second event\n"
	"        smear: do not inform kernel, gradually introduce the leap second before the event\n"
	"               by modifying clock offset (see clock:leap_second_smear_period)",
				"accept", 	LEAP_ACCEPT,
				"ignore", 	LEAP_IGNORE,
				"step", 	LEAP_STEP,
				"smear",	LEAP_SMEAR, NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:leap_second_smear_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->leapSecondSmearPeriod,
		global->leapSecondSmearPeriod,
		"Time period (Seconds) over which the leap second is introduced before the event.\n"
	"	 Example: when set to 86400 (24 hours), an extra 11.5 microseconds is added every second"
	,RANGECHECK_RANGE,3600,86400);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:lock_device",
		PTPD_RESTART_NONE, &global->lockClockDevice, global->lockClockDevice,
		"Set a write lock on the clock device handle if it's being synced\n"
	"	 (if the clock driver supports this and implements locking)");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:extra_clocks",
		PTPD_RESTART_NONE, global->extraClocks, sizeof(global->extraClocks), global->extraClocks,
	"Specify a comma, space or tab separated list of extra clocks to be controlled by PTP.\n"
	"        The format is type:path:name where \"type\" can be: \"unix\" for Unix clocks and \"linuxphc\"\n"
	"	 for Linux PHC clocks, \"path\" is either the clock device path or interface name, and\n"
	"	 \"name\" is user's name for the clock (20 characters max). If no name is given, it is\n"
	"	 extracted from the path name.");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:master_clock_name",
		PTPD_RESTART_NONE, global->masterClock, sizeof(global->masterClock), global->masterClock,
	"Specify the clock name of a clock which is to be the preferred best clock\n"
	"	 that other clock synchronise with. This clock is assumed to be controlled by an external\n"
	"	 source such as NTP. This clock can only be controlled by PTP, otherwise it is\n"
	"	 permanently in LOCKED state");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:master_clock_reference_name",
		PTPD_RESTART_NONE, global->masterClockRefName, sizeof(global->masterClockRefName), global->masterClockRefName,
	"Specify the name of the external reference controlling the master clock (clock:master_clock_name).");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:disabled_clock_names",
		PTPD_RESTART_NONE, global->disabledClocks, sizeof(global->disabledClocks), global->disabledClocks,
	"Specify a comma, space or tab separated list of names of clocks that should be disabled - disabled clocks\n"
	"	 are excluded from sync and do not show up in the clock list.\n"
	"	 NOTE: required clocks, such as system clock or PTP NIC clocks cannot be disabled.\n"
	"	 they can be set to read-only instead (clock:readonly_clock_names)");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:readonly_clock_names",
		PTPD_RESTART_NONE, global->readOnlyClocks, sizeof(global->readOnlyClocks), global->readOnlyClocks,
	"Specify a comma, space or tab separated list of names of clocks that should be read only - read only\n"
	"	 clocks are still compared to best clocks and their frequency is monitored,\n"
	"	 but they are not adjusted.\n");

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:excluded_clock_names",
		PTPD_RESTART_NONE, global->excludedClocks, sizeof(global->excludedClocks), global->excludedClocks,
	"Specify a comma, space or tab separated list of names of clocks that should be excluded from\n"
	"	 best clock selection - they will be synced unless read only, but will never be\n"
	"	 selected as best clock.\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:sync_rate",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->clockSyncRate,
		global->clockSyncRate,
		"Clock sync rate (per second) - the rate at which internal clocks are synced\n"
		"	 with each other (excluding PTP-controlled clocks)." ,RANGECHECK_RANGE,1,32);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:fault_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->clockFaultTimeout,
		global->clockFaultTimeout,
		"Clock failure suspension timeout (seconds). When one of clock functions fails unexpectedly,\n"
		"	 it is placed in HWFAIL state, its operation is suspended for this period,\n"
		"	 after which a health check is performed, and if it succeeds, clock is brought\n"
		"	 back into FREERUN state.\n", RANGECHECK_RANGE,5,600);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:frequency_step_detection",
		PTPD_RESTART_NONE, &global->clockFreqStepDetection, global->clockFreqStepDetection,
		 "In LOCKED state, react to offsets which would cause a frequency step outside LOCKED threshold,\n"
		 "	 before they are fed to the clock servo.");

	/* BEGIN inter-clock filter settings */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:stat_filter_enable",
		PTPD_RESTART_NONE, &global->clockStatFilterEnable, global->clockStatFilterEnable,
		 "Enable statistical filter for inter-clock sync (HW to system clock, HW to HW");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "clock:stat_filter_type",
		PTPD_RESTART_FILTERS, &global->clockStatFilterType, global->clockStatFilterType,
		"Type of filter used for inter-clock sync (HW to system clock, HW to HW)",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:stat_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->clockStatFilterWindowSize, global->clockStatFilterWindowSize,
		"Number of samples used for the inter-clock sync statistical filter.\n"
	"	 When set to 0, clock sync rate is used (clock:sync_rate).",RANGECHECK_RANGE,0,STATCONTAINER_MAX_SAMPLES);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "clock:stat_filter_window_type",
		PTPD_RESTART_FILTERS, &global->clockStatFilterWindowType, global->clockStatFilterWindowType,
		"Sample window type used for inter-clock sync (HW to system clock, HW to HW).\n"
	"        Sliding window is continuous, interval passes every n-th sample only.",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL, NULL);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:outlier_filter_enable",
		PTPD_RESTART_NONE, &global->clockOutlierFilterEnable, global->clockOutlierFilterEnable,
		 "Enable outlier filter for inter-clock sync (HW to system clock, HW to HW)");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:outlier_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->clockOutlierFilterWindowSize, global->clockOutlierFilterWindowSize,
		"Number of samples used for the inter-clock sync outlier filter",RANGECHECK_RANGE,3,STATCONTAINER_MAX_SAMPLES);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:outlier_filter_delay",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->clockOutlierFilterDelay, global->clockOutlierFilterDelay,
		"Number of samples after which the inter-clock sync outlier filter is activated",RANGECHECK_RANGE,0,STATCONTAINER_MAX_SAMPLES);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "clock:outlier_filter_threshold",
		PTPD_RESTART_NONE, &global->clockOutlierFilterCutoff, global->clockOutlierFilterCutoff,
		"Inter-clock sync outlier filter cutoff threshold: maximum number of MAD\n"
	"	 (Mean Absolute Deviations) from median to consider the sample an outlier", RANGECHECK_RANGE, 0.1, 100000);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:outlier_filter_block_timeout",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &global->clockOutlierFilterBlockTimeout, global->clockOutlierFilterBlockTimeout,
		"Maximum blocking time (seconds) before outlier filter is reset",RANGECHECK_RANGE,0,3600);


	/* END inter-clock filter settings */


	/* This really is clock specific - different clocks may allow different ranges */
	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:max_offset_ppm",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->servoMaxPpb, global->servoMaxPpb,
		"Maximum absolute frequency shift which can be applied to the clock servo\n"
	"	 when slewing the clock. Expressed in parts per million (1 ppm = shift of\n"
	"	 1 us per second. Values above 500 will use the tick duration correction\n"
	"	 to allow even faster slewing. Default maximum is 500 without using tick.", RANGECHECK_RANGE,
#ifdef HAVE_STRUCT_TIMEX_TICK
	50, (UNIX_MAX_FREQ * UNIX_TICKADJ_MULT) / 1000);
#else
	50, UNIX_MAX_FREQ / 1000);
#endif /* HAVE_STRUCT_TIMEX_TICK */

	/* This really is clock specific - different clocks may allow different ranges */
	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:max_offset_ppm_hardware",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->servoMaxPpb_hw, global->servoMaxPpb_hw,
		"Maximum absolute frequency shift which can be applied to the clock servo\n"
	"	 when slewing the clock. Expressed in parts per million (1 ppm = shift of\n"
	"	 1 us per second. Values above 512 will use the tick duration correction\n"
	"	 to allow even faster slewing. Default maximum is 2000 without using tick.", RANGECHECK_RANGE,
	50,2000);

	/*
	 * TimeProperties DS - in future when clock driver API is implemented,
	 * a slave PTP engine should inform a clock about this, and then that
	 * clock should pass this information to any master PTP engines, unless
	 * we override this. here. For now we just supply this to RtOpts.
	 */
	

/* ===== servo section ===== */

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:delayfilter_stiffness",
		PTPD_RESTART_NONE, INTTYPE_I16, &global->s, global->s,
	"One-way delay filter stiffness.", RANGECHECK_NONE,0,0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:kp",
		PTPD_RESTART_NONE, &global->servoKP, global->servoKP,
	"Clock servo PI controller proportional component gain (kP) used for software timestamping.", RANGECHECK_MIN, 0.000001, 0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:ki",
		PTPD_RESTART_NONE, &global->servoKI, global->servoKI,
	"Clock servo PI controller integral component gain (kI) used for software timestamping.", RANGECHECK_MIN, 0.000001,0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:kp_hardware",
		PTPD_RESTART_NONE, &global->servoKP_hw, global->servoKP_hw,
	"Clock servo PI controller proportional component gain (kP) used when syncing hardware clocks.", RANGECHECK_MIN, 0.000001, 0);
	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:ki_hardware",
		PTPD_RESTART_NONE, &global->servoKI_hw, global->servoKI_hw,
	"Clock servo PI controller integral component gain (kI) used when syncing gardware clocks.", RANGECHECK_MIN, 0.000001,0);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "servo:dt_method",
		PTPD_RESTART_NONE, &global->servoDtMethod, global->servoDtMethod,
		"How servo update interval (delta t) is calculated:\n"
	"	 none:     servo not corrected for update interval (dt always 1),\n"
	"	 constant: constant value (target servo update rate - sync interval for PTP,\n"
	"	 measured: servo measures how often it's updated and uses this interval.",
			"none", DT_NONE,
			"constant", DT_CONSTANT,
			"measured", DT_MEASURED, NULL
	);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:dt_max",
		PTPD_RESTART_NONE, &global->servoMaxdT, global->servoMaxdT,
		"Maximum servo update interval (delta t) when using measured servo update interval\n"
	"	 (servo:dt_method = measured), specified as sync interval multiplier.", RANGECHECK_RANGE, 1.5,100.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:adev_locked_threshold_low_hw",
		PTPD_RESTART_NONE, &global->stableAdev_hw, global->stableAdev_hw,
		"Minimum Allan deviation of a clock frequency (ppb = 10E-9) for a hardware clock to be considered stable (LOCKED).\n"
	"        Allan deviation is checked in intervals defined by the servo:adev_interval setting.",
		RANGECHECK_RANGE, 0.1,100000.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:adev_locked_threshold_high_hw",
		PTPD_RESTART_NONE, &global->unstableAdev_hw, global->unstableAdev_hw,
		"Allan deviation of a clock frequency (ppb = 10E-9) for a hardware clock to be considered no longer stable\n"
	"	 (transition from LOCKED to TRACKING). Allan deviation is checked in intervals defined by the servo:adev_interval setting.",
		RANGECHECK_RANGE, 0.1,100000.0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:holdover_delay_hw",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->lockedAge_hw, global->lockedAge_hw,
		"Maximum idle time allowed (no sync) before a hardware clock in LOCKED state to transitions into HOLDOVER.",
		RANGECHECK_RANGE, 5, 86400);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:holdover_timeout_hw",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->holdoverAge_hw, global->holdoverAge_hw,
		"Maximum time allowed before a hardware clock in HOLDOVER state transitions to FREERUN.",
		RANGECHECK_RANGE, 5, 86400);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:adev_locked_threshold_low",
		PTPD_RESTART_NONE, &global->stableAdev, global->stableAdev,
		"Minimum Allan deviation of a clock frequency (ppb = 10E-9) for a software-based clock to be considered stable (LOCKED).\n"
	"        Allan deviation is checked in intervals defined by the servo:adev_interval setting.",
		RANGECHECK_RANGE, 0.1,100000.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:adev_locked_threshold_high",
		PTPD_RESTART_NONE, &global->unstableAdev, global->unstableAdev,
		"Allan deviation of a clock frequency (ppb = 10E-9) for a software-based clock to be considered no longer stable\n"
	"	 (transition from LOCKED to TRACKING). Allan deviation is checked in intervals defined by the servo:adev_interval setting.",
		RANGECHECK_RANGE, 0.1,100000.0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:holdover_delay",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->lockedAge, global->lockedAge,
		"Maximum idle time allowed (no sync) before a software-based clock in LOCKED state to transitions into HOLDOVER.",
		RANGECHECK_RANGE, 5, 86400);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:holdover_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->holdoverAge, global->holdoverAge,
		"Maximum time allowed before a software-based clock in HOLDOVER state transitions to FREERUN.",
		RANGECHECK_RANGE, 5, 86400);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:adev_interval",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->adevPeriod, global->adevPeriod,
		"Interval (seconds) over which Allan deviation of a clock servo is computed for establishing clock stability.\n",
		RANGECHECK_RANGE, 5, 3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_delay",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->maxDelay, global->maxDelay,
		"Do accept master to slave delay (delayMS - from Sync message) or slave to master delay\n"
	"	 (delaySM - from Delay messages) if greater than this value (nanoseconds). 0 = not used.", RANGECHECK_RANGE,
	0,NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_delay_max_rejected",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->maxDelayMaxRejected, global->maxDelayMaxRejected,
		"Maximum number of consecutive delay measurements exceeding maxDelay threshold,\n"
	"	 before slave is reset.", RANGECHECK_MIN,0,0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "servo:max_delay_stable_only",
		PTPD_RESTART_NONE, &global->maxDelayStableOnly, global->maxDelayStableOnly,
		"If servo:max_delay is set, perform the check only if clock servo has stabilised.\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_sequence_checking",
		PTPD_RESTART_NONE, &global->syncSequenceChecking, global->syncSequenceChecking,
		"When enabled, Sync messages will only be accepted if sequence ID is increasing."
	"        This is limited to 50 dropped messages.\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:clock_update_timeout",
		PTPD_RESTART_PROTOCOL, INTTYPE_INT, &global->clockUpdateTimeout, global->clockUpdateTimeout,
		"If set to non-zero, timeout in seconds, after which the slave resets if no clock updates made. \n", RANGECHECK_RANGE,
		0, 3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_offset",
		PTPD_RESTART_NONE, INTTYPE_I32, &global->maxOffset, global->maxOffset,
		"Do not reset the clock if offset from master is greater\n"
	"        than this value (nanoseconds). 0 = not used.", RANGECHECK_RANGE,
	0,NANOSECONDS_MAX);

/* ===== global section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_alarms",
	    PTPD_RESTART_ALARMS, &global->alarmsEnabled, global->alarmsEnabled,
		 "Enable support for alarm and event notifications.\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:alarm_timeout",
		PTPD_RESTART_ALARMS, INTTYPE_INT, &global->alarmMinAge, global->alarmMinAge,
		"Mininmum alarm age (seconds) - minimal time between alarm set and clear notifications.\n"
	"	 The condition can clear while alarm lasts, but notification (log or SNMP) will only \n"
	"	 be triggered after the timeout. This option prevents from alarms flapping.", RANGECHECK_RANGE, 0, 3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:alarm_initial_delay",
		PTPD_RESTART_DAEMON, INTTYPE_INT, &global->alarmInitialDelay, global->alarmInitialDelay,
		"Delay the start of alarm processing (seconds) after ptpd startup. This option \n"
	"	 allows to avoid unnecessary alarms before PTPd starts synchronising.\n",
	RANGECHECK_RANGE, 0, 3600);

#ifdef PTPD_SNMP
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_snmp",
		PTPD_RESTART_DAEMON, &global->snmpEnabled, global->snmpEnabled,
	"Enable SNMP agent (if compiled with PTPD_SNMP).");
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_snmp_traps",
	    PTPD_RESTART_ALARMS, &global->snmpTrapsEnabled, global->snmpTrapsEnabled,
		 "Enable sending SNMP traps (only if global:enable_alarms set and global:enable_snmp set).\n");
#else
	if(!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISTRUE("global:enable_snmp"))
	    INFO("SNMP support not enabled. Please compile with PTPD_SNMP to use global:enable_snmp\n");
	if(!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISTRUE("global:enable_snmp_traps"))
	    INFO("SNMP support not enabled. Please compile with PTPD_SNMP to use global:enable_snmp_traps\n");
#endif /* PTPD_SNMP */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:use_syslog",
		PTPD_RESTART_LOGGING, &global->useSysLog, global->useSysLog,
		"Send log messages to syslog. Disabling this\n"
	"        sends all messages to stdout (or speficied log file).");

	parseResult &= configMapString(opCode, opArg, dict, target, "global:lock_file",
		PTPD_RESTART_DAEMON, global->lockFile, sizeof(global->lockFile), global->lockFile,
	"Lock file location");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:auto_lockfile",
		PTPD_RESTART_DAEMON, &global->autoLockFile, global->autoLockFile,
	"	 Use mode specific and interface specific lock file\n"
	"	 (overrides global:lock_file).");

	parseResult &= configMapString(opCode, opArg, dict, target, "global:lock_directory",
		PTPD_RESTART_DAEMON, global->lockDirectory, sizeof(global->lockDirectory), global->lockDirectory,
		 "Lock file directory: used with automatic mode-specific lock files,\n"
	"	 also used when no lock file is specified. When lock file\n"
	"	 is specified, it's expected to be an absolute path.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:ignore_lock",
		PTPD_RESTART_DAEMON, &global->ignoreLock, global->ignoreLock,
	"Skip lock file checking and locking.");

	/* if quality file specified, enable quality recording  */
	CONFIG_KEY_TRIGGER("global:quality_file", global->recordLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:quality_file",
		PTPD_RESTART_LOGGING, global->recordLog.logPath, sizeof(global->recordLog.logPath), global->recordLog.logPath,
		"File used to record data about sync packets. Enables recording when set.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:quality_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &global->recordLog.maxSize, global->recordLog.maxSize,
		"Maximum sync packet record file size (in kB) - file will be truncated\n"
	"	if size exceeds the limit. 0 - no limit.", RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:quality_file_max_files",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &global->recordLog.maxFiles, global->recordLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:quality_file_truncate",
		PTPD_RESTART_LOGGING, &global->recordLog.truncateOnReopen, global->recordLog.truncateOnReopen,
		"Truncate the sync packet record file every time it is (re) opened:\n"
	"	 startup and SIGHUP.");

	/* if status file specified, enable status logging*/
	CONFIG_KEY_TRIGGER("global:status_file", global->statusLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:status_file",
		PTPD_RESTART_LOGGING, global->statusLog.logPath, sizeof(global->statusLog.logPath), global->statusLog.logPath,
	"File used to log "PTPD_PROGNAME" status information.");
	/* status file can be disabled even if specified */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_status",
		PTPD_RESTART_NONE, &global->statusLog.logEnabled, global->statusLog.logEnabled,
		"Enable / disable writing status information to file.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:status_update_interval",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &global->statusFileUpdateInterval, global->statusFileUpdateInterval,
		"Status file update interval in seconds.", RANGECHECK_RANGE,
	1,30);

#ifdef RUNTIME_DEBUG
	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:debug_level",
		PTPD_RESTART_NONE, (uint8_t*)&global->debug_level, global->debug_level,
	"Specify debug level (if compiled with RUNTIME_DEBUG).",
				"LOG_INFO", 	LOG_INFO,
				"LOG_DEBUG", 	LOG_DEBUG,
				"LOG_DEBUG1", 	LOG_DEBUG1,
				"LOG_DEBUG2", 	LOG_DEBUG2,
				"LOG_DEBUG3", 	LOG_DEBUG3,
				"LOG_DEBUGV", 	LOG_DEBUGV, NULL
				);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:debugfilter",
		PTPD_RESTART_NONE, global->debugFilter, sizeof(global->debugFilter), global->debugFilter,
	"Only display debug messages containing this text (case sensitive, max. 100 characters)");

#else
	if (!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISSET("global:debug_level"))
	    INFO("Runtime debug not enabled. Please compile with RUNTIME_DEBUG to use global:debug_level.\n");
#endif /* RUNTIME_DEBUG */

    /*
     * Log files are reopened and / or re-created on SIGHUP anyway,
     */

	/* if log file specified, enable file logging - otherwise disable */
	CONFIG_KEY_TRIGGER("global:log_file", global->eventLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:log_file",
		PTPD_RESTART_LOGGING, global->eventLog.logPath, sizeof(global->eventLog.logPath), global->eventLog.logPath,
		"Specify log file path (event log). Setting this enables logging to file.");

	parseResult &= configMapString(opCode, opArg, dict, target, "global:log_filter",
		PTPD_RESTART_NONE, global->logFilter, sizeof(global->logFilter), global->logFilter,
	"Only display log messages containing this text (case sensitive, max. 100 characters)");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:deduplicate_log",
		PTPD_RESTART_NONE, &global->deduplicateLog, global->deduplicateLog,
		"Do not log repeated log messages");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:log_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &global->eventLog.maxSize, global->eventLog.maxSize,
		"Maximum log file size (in kB) - log file will be truncated if size exceeds\n"
	"	 the limit. 0 - no limit.", RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:log_file_max_files",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->eventLog.maxFiles, global->eventLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_file_truncate",
		PTPD_RESTART_LOGGING, &global->eventLog.truncateOnReopen, global->eventLog.truncateOnReopen,
		"Truncate the log file every time it is (re) opened: startup and SIGHUP.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:log_level",
		PTPD_RESTART_NONE, &global->logLevel, global->logLevel,
		"Specify log level (only messages at this priority or higer will be logged).\n"
	"	 The minimal level is LOG_ERR. LOG_ALL enables debug output if compiled with\n"
	"	 RUNTIME_DEBUG.",
				"LOG_ERR", 	LOG_ERR,
				"LOG_WARNING", 	LOG_WARNING,
				"LOG_NOTICE", 	LOG_NOTICE,
				"LOG_INFO", 	LOG_INFO,
				"LOG_ALL",	LOG_ALL, NULL
				);

	/* if statistics file specified, enable statistics logging - otherwise disable  - log_statistics also controlled further below*/
	CONFIG_KEY_TRIGGER("global:statistics_file", global->statisticsLog.logEnabled,TRUE,FALSE);
	CONFIG_KEY_TRIGGER("global:statistics_file", global->logStatistics,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:statistics_file",
		PTPD_RESTART_LOGGING, global->statisticsLog.logPath, sizeof(global->statisticsLog.logPath), global->statisticsLog.logPath,
		"Specify statistics log file path. Setting this enables logging of \n"
	"	 statistics, but can be overriden with global:log_statistics.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_log_interval",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->statisticsLogInterval, global->statisticsLogInterval,
		 "Log timing statistics every n seconds for Sync and Delay messages\n"
	"	 (0 - log all).",RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &global->statisticsLog.maxSize, global->statisticsLog.maxSize,
		"Maximum statistics log file size (in kB) - log file will be truncated\n"
	"	 if size exceeds the limit. 0 - no limit.",RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_file_max_files",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &global->statisticsLog.maxFiles, global->statisticsLog.maxFiles,
		"Enable log rotation of the statistics file up to n files. 0 - do not rotate.", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:statistics_file_truncate",
		PTPD_RESTART_LOGGING, &global->statisticsLog.truncateOnReopen, global->statisticsLog.truncateOnReopen,
		"Truncate the statistics file every time it is (re) opened: startup and SIGHUP.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:dump_packets",
		PTPD_RESTART_NONE, &global->displayPackets, global->displayPackets,
		"Dump the contents of every PTP packet");

	/* this also checks if the verbose_foreground flag is set correctly */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:verbose_foreground",
		PTPD_RESTART_DAEMON, &global->nonDaemon, global->nonDaemon,
		"Run in foreground with statistics and all messages logged to stdout.\n"
	"	 Overrides log file and statistics file settings and disables syslog.\n");

	if(CONFIG_ISTRUE("global:verbose_foreground")) {
		global->useSysLog    = FALSE;
		global->logStatistics = TRUE;
		global->statisticsLogInterval = 0;
		global->eventLog.logEnabled = FALSE;
		global->statisticsLog.logEnabled = FALSE;
	}

	/*
	 * this HAS to be executed after the verbose_foreground mapping because of the same
	 * default field used for both. verbose_foreground triggers nonDaemon which is OK,
	 * but we don't want foreground=y to switch on verbose_foreground if not set.
	 */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:foreground",
		PTPD_RESTART_DAEMON, &global->nonDaemon, global->nonDaemon,
		"Run in foreground - ignored when global:verbose_foreground is set");

	if(CONFIG_ISTRUE("global:verbose_foreground")) {
		global->nonDaemon = TRUE;
	}

	/* If this is processed after verbose_foreground, we can still control logStatistics */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_statistics",
		PTPD_RESTART_NONE, &global->logStatistics, global->logStatistics,
		"Log timing statistics for every PTP packet received\n");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:statistics_timestamp_format",
		PTPD_RESTART_NONE, &global->statisticsTimestamp, global->statisticsTimestamp,
		"Timestamp format used when logging timing statistics\n"
	"        (when global:log_statistics is enabled):\n"
	"        datetime - formatttted date and time: YYYY-MM-DD hh:mm:ss.uuuuuu\n"
	"        unix - Unix timestamp with nanoseconds: s.ns\n"
	"        both - Formatted date and time, followed by unix timestamp\n"
	"               (adds one extra field  to the log)\n",
		"datetime",	TIMESTAMP_DATETIME,
		"unix",		TIMESTAMP_UNIX,
		"both",		TIMESTAMP_BOTH, NULL
		);

	/* If statistics file is enabled but logStatistics isn't, disable logging to file */
	CONFIG_KEY_CONDITIONAL_TRIGGER(global->statisticsLog.logEnabled && !global->logStatistics,
					global->statisticsLog.logEnabled, FALSE, global->statisticsLog.logEnabled);

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	parseResult &= configMapInt(opCode, opArg, dict, target, "global:cpuaffinity_cpucore", PTPD_CHANGE_CPUAFFINITY, INTTYPE_INT, &global->cpuNumber, global->cpuNumber,
		"Bind "PTPD_PROGNAME" process to a selected CPU core number.\n"
	"        0 = first CPU core, etc. -1 = do not bind to a single core.", RANGECHECK_RANGE,
	-1,255);
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H */

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_update_interval",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->statsUpdateInterval,
								global->statsUpdateInterval,
		"Clock synchronisation statistics update interval in seconds\n", RANGECHECK_RANGE,1, 60);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:periodic_updates",
		PTPD_RESTART_LOGGING, &global->periodicUpdates, global->periodicUpdates,
		"Log a status update every time statistics are updated (global:statistics_update_interval).\n"
	"        The updates are logged even when ptpd is configured without statistics support");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:timingdomain_election_delay",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->electionDelay, global->electionDelay,
		" Delay (seconds) before releasing a time service (NTP or PTP)"
	"        and electing a new one to control a clock. 0 = elect immediately\n", RANGECHECK_RANGE,0, 3600);


/* ===== ntpengine section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ntpengine:enabled",
		PTPD_RESTART_NTPENGINE, &global->ntpOptions.enableEngine, global->ntpOptions.enableEngine,
	"Enable NTPd integration");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ntpengine:control_enabled",
		PTPD_RESTART_NONE, &global->ntpOptions.enableControl, global->ntpOptions.enableControl,
	"Enable control over local NTPd daemon");

	CONFIG_KEY_DEPENDENCY("ntpengine:control_enabled", "ntpengine:enabled");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ntpengine:check_interval",
		PTPD_RESTART_NTPCONFIG, INTTYPE_INT, &global->ntpOptions.checkInterval,
								global->ntpOptions.checkInterval,
		"NTP control check interval in seconds\n", RANGECHECK_RANGE, 5, 600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ntpengine:key_id",
		PTPD_RESTART_NONE, INTTYPE_INT, &global->ntpOptions.keyId, global->ntpOptions.keyId,
		 "NTP key number - must be configured as a trusted control key in ntp.conf,\n"
	"	  and be non-zero for the ntpengine:control_enabled setting to take effect.\n", RANGECHECK_RANGE,0, 65535);

	parseResult &= configMapString(opCode, opArg, dict, target, "ntpengine:key",
		PTPD_RESTART_NONE, global->ntpOptions.key, sizeof(global->ntpOptions.key), global->ntpOptions.key,
		"NTP key (plain text, max. 20 characters) - must match the key configured in\n"
	"	 ntpd's keys file, and must be non-zero for the ntpengine:control_enabled\n"
	"	 setting to take effect.\n");

	CONFIG_KEY_DEPENDENCY("ntpengine:control:enabled", "ntpengine:key_id");
	CONFIG_KEY_DEPENDENCY("ntpengine:control:enabled", "ntpengine:key");



/* ============== END CONFIG MAPPINGS, TRIGGERS AND DEPENDENCIES =========== */

/* ==== Any additional logic should go here ===== */

	/* test ACLs */

	int family = getConfiguredFamily(global);

	if(global->timingAclEnabled) {
	    if(!testCckAcl(family, global->timingAclPermitText,
				    global->timingAclDenyText,
				    global->timingAclOrder, false)) {
		parseResult = FALSE;
		global->timingAclEnabled = FALSE;
		ERROR("Error while parsing timing ACL\n");
	    }
	}

	if(global->managementAclEnabled) {
	    if(!testCckAcl(family, global->managementAclPermitText,
				    global->managementAclDenyText,
				    global->managementAclOrder, false)) {
		parseResult = FALSE;
		global->managementAclEnabled = FALSE;
		ERROR("Error while parsing management ACL\n");
	    }
	}

	/* Scale the maxPPM to PPB */
	global->servoMaxPpb *= 1000;
	global->servoMaxPpb_hw *= 1000;

	/*
	 * We're in hybrid mode and we haven't specified the delay request interval:
	 * use override with a default value
	 */
	if((global->transportMode == TMODE_MIXED) &&
	 !CONFIG_ISSET("ptpengine:log_delayreq_interval"))
		global->logDelayReqOverride=TRUE;

	/*
	 * We're in unicast slave-capable mode and we haven't specified the delay request interval:
	 * use override with a default value
	 */
	if((global->transportMode == TMODE_UC &&
	    global->clockQuality.clockClass > 127) &&
	    !CONFIG_ISSET("ptpengine:log_delayreq_interval"))
		global->logDelayReqOverride=TRUE;

	/*
	 * construct the lock file name based on operation mode:
	 * if clock class is <128 (master only), use "master" and interface name
	 * if clock class is >127 (can be slave), use clock driver and interface name
	 */
	if(global->autoLockFile) {

	    memset(global->lockFile, 0, PATH_MAX);
	    snprintf(global->lockFile, PATH_MAX,
		    "%s/"PTPD_PROGNAME"_%s_%s.lock",
		    global->lockDirectory,
		    (global->clockQuality.clockClass<128 && !global->slaveOnly) ? "master" : DEFAULT_CLOCKDRIVER,
		    global->ifName);
	    DBG("Automatic lock file name is: %s\n", global->lockFile);
	/*
	 * Otherwise use default lock file name, with the specified lock directory
	 * which will be set do default from constants_dep.h if not configured
	 */
	} else {
		if(!CONFIG_ISSET("global:lock_file"))
			snprintf(global->lockFile, PATH_MAX,
				"%s/%s", global->lockDirectory, DEFAULT_LOCKFILE_NAME);
	}

/* ==== END additional logic */

	if(opCode & CFGOP_PARSE) {

		findUnknownSettings(opCode, dict, target);

		if (parseResult)
			NOTICE("Configuration OK\n");
		    else
			ERROR("There are errors in the configuration - see previous messages\n");
	}

	if ((opCode & CFGOP_PARSE || opCode & CFGOP_PARSE_QUIET) && parseResult) {
		return target;
	}

	dictionary_del(&target);
	return NULL;


}

/**
 * Wrapper around iniparser_load, which loads the ini file
 * contents into an existing dictionary - iniparser_load
 * creates a new dictionary
 */
Boolean
loadConfigFile(dictionary **target, GlobalConfig *global)
{

	dictionary *dict;

	if ( (dict = iniparser_load(global->configFile)) == NULL) {
		ERROR("Could not load configuration file: %s\n", global->configFile);
		return FALSE;
	}

	dictionary_merge( dict, *target, 0, 0, NULL);
	dictionary_del(&dict);

	return TRUE;
}


/* -
 * this function scans argv looking for what looks like correctly formatted
 * long options in the form "--section:key=value", "-section:key=value",
 * "--section:key value" and "-section:key value".
 * it then clears them from argv so that getopt can deal with the rest.
 * This is in a way better than getopt_long() because we don't need to
 * pre-define all possible options. Unknown or malformed ones are ignored.
 */
void
loadCommandLineKeys(dictionary* dict, int argc,char** argv)
{

    int i;
    char key[PATH_MAX],val[PATH_MAX];

    for ( i=0; i<argc; i++) {

	if( strlen(argv[i]) > 3 &&
	    index(argv[i],':') != NULL ) {
	/* check if the option is passed as sec:key=value */
	if (sscanf(argv[i],"--%[-_a-zA-Z0-9:]=%s",key,val)==2 ||
	    sscanf(argv[i],"-%[-_a-zA-Z0-9:]=%s",key,val)==2) {
	/* wipe the used argv entry so that getopt doesn't get confused */
		argv[i]="";
	    } else
	/*
	 * there are no optional arguments for key:sec options - if there's no =,
	 * the next argument is the value.
	 */
	if (sscanf(argv[i],"--%[-_a-zA-Z0-9:]",key)==1 ||
	    sscanf(argv[i],"-%[-_a-zA-Z0-9:]",key)==1 ) {
		argv[i]="";
		memset(val, 0, PATH_MAX);
		if (i+1 < argc) {
		    if( (argv[i+1][0]!='-') &&
			( (strlen(argv[i+i]) > 1) && argv[i+1][1] !='-' )) {
			strncpy(val,argv[i+1],PATH_MAX);
			argv[i+1]="";
		    }
		
		}

	    }
	/*
	 * there is no argument --sec:key=val
	 */
	else {
		val[0] = '\0';
		key[0] = '\0';
	}
	
	if(strlen(key) > 0) {
		DBGV("setting loaded from command line: %s = %s\n",key,val);
		dictionary_set(dict, key, val);
	}

	}

	}
}

/**
 * Create a dummy global with defaults, create a dummy dictionary,
 * Set the "secret" key in the dictionary, causing parseConfig
 * to switch from parse mode to print default mode
 */
void
printDefaultConfig()
{

	GlobalConfig global;
	dictionary *dict;

	loadDefaultSettings(&global);
	dict = dictionary_new(0);

	printf( "; ========================================\n");
	printf( "; "USER_DESCRIPTION" version "USER_VERSION" default configuration\n");
	printf( "; ========================================\n\n");
	printf( "; NOTE: the following settings are affected by ptpengine:preset selection:\n"
		";           ptpengine:slave_only\n"
		";           clock:no_adjust\n"
		";           ptpengine:clock_class - allowed range and default value\n"
		"; To see all preset settings, run "PTPD_PROGNAME" -H (--long-help)\n");

	/* NULL will always be returned in this mode */
	parseConfig(CFGOP_PRINT_DEFAULT | CFGOP_PARSE_QUIET, NULL, dict, &global);
	dictionary_del(&dict);

	printf("\n; ========= newline required in the end ==========\n\n");

}

/**
 * Create a dummy global with defaults, create a dummy dictionary,
 * run parseConfig in help mode.
 */
void
printConfigHelp()
{

	GlobalConfig global;
	dictionary *dict;

	loadDefaultSettings(&global);
	dict = dictionary_new(0);

	printf("\n============== Full list of "PTPD_PROGNAME" settings ========\n\n");

	/* NULL will always be returned in this mode */
	parseConfig(CFGOP_HELP_FULL | CFGOP_PARSE_QUIET, NULL, dict, &global);

	dictionary_del(&dict);

}

/**
 * Create a dummy global  with defaults, create a dummy dictionary,
 * Set the "secret" key in the dictionary, causing parseConfig
 * to switch from parse mode to help mode, for a selected key only.
 */
void
printSettingHelp(char* key)
{

	GlobalConfig global;
	dictionary *dict;
	char* origKey = strdup(key);

	loadDefaultSettings(&global);
	dict = dictionary_new(0);


	printf("\n");
	/* NULL will always be returned in this mode */
	parseConfig(CFGOP_HELP_SINGLE | CFGOP_PARSE_QUIET, key, dict, &global);
	
	/* if the setting has been found (and help printed), the first byte will be cleared */
	if(key[0] != '\0') {
	    printf("Unknown setting: %s\n\n", origKey);
	}
	printf("Use -H or --long-help to show help for all settings.\n\n");
	
	dictionary_del(&dict);
	free(origKey);
}

/**
 * Handle standard options with getopt_long. Returns FALSE if ptpd should not continue.
 * If a required setting, such as interface name, or a setting
 * requiring a range check is to be set via getopts_long,
 * the respective currentConfig dictionary entry should be set,
 * instead of just setting the global field.
 */
Boolean loadCommandLineOptions(GlobalConfig* global, dictionary* dict, int argc, char** argv, int* ret) {

	int c;
#ifdef HAVE_GETOPT_LONG
	int opt_index = 0;
#endif /* HAVE_GETOPT_LONG */
	*ret = 0;

	/* there's NOTHING wrong with this */
	if(argc==1) {
			*ret = -1;
			goto short_help;
	}

#ifdef HAVE_GETOPT_LONG
	/**
	 * A minimal set of long and short CLI options,
	 * for basic operations only. Maintained compatibility
	 * with previous versions. Any other settings
	 * should be given in the config file or using
	 * --section:key options
	**/
	static struct option long_options[] = {
	    {"config-file",	required_argument, 0, 'c'},
	    {"check-config",	optional_argument, 0, 'k'},
	    {"interface",	required_argument, 0, 'i'},
	    {"domain",		required_argument, 0, 'd'},
	    {"slaveonly",	no_argument,	   0, 's'},
	    {"masterslave",	no_argument,	   0, 'm'},
	    {"masteronly",	no_argument,	   0, 'M'},
	    {"hybrid",		no_argument,	   0, 'y'},
	    {"e2e",		optional_argument, 0, 'E'},
	    {"p2p",		optional_argument, 0, 'P'},
	    {"noadjust",	optional_argument, 0, 'n'},
	    {"ignore-lock",	optional_argument, 0, 'L'},
	    {"auto-lock",	optional_argument, 0, 'A'},
	    {"lockfile",	optional_argument, 0, 'l'},
	    {"print-lockfile",	optional_argument, 0, 'p'},
	    {"lock-directory",	required_argument, 0, 'R'},
	    {"log-file",	required_argument, 0, 'f'},
	    {"statistics-file",	required_argument, 0, 'S'},
	    {"delay-interval",	required_argument, 0, 'r'},
	    {"delay-override",	no_argument, 	   0, 'a'},
	    {"debug",		no_argument,	   0, 'D'},
	    {"version",		optional_argument, 0, 'v'},
	    {"foreground",	no_argument,	   0, 'C'},
	    {"verbose",		no_argument,	   0, 'V'},
	    {"help",		optional_argument, 0, 'h'},
	    {"long-help",	no_argument,	   0, 'H'},
	    {"explain",		required_argument, 0, 'e'},
	    {"default-config",  optional_argument, 0, 'O'},
	    {"templates",	required_argument, 0, 't'},
	    {"show-templates",  no_argument,	   0, 'T'},
	    {"unicast",		optional_argument, 0, 'U'},
	    {"unicast-negotiation",		optional_argument, 0, 'g'},
	    {"unicast-destinations",		required_argument, 0, 'u'},
	    {0,			0		 , 0, 0}
	};

	while ((c = getopt_long(argc, argv, "?c:kb:i:d:sgmGMWyUu:nf:S:r:DvCVHTt:he:Y:tOLEPAaR:l:p", long_options, &opt_index)) != -1) {
#else
	while ((c = getopt(argc, argv, "?c:kb:i:d:sgmGMWyUu:nf:S:r:DvCVHTt:he:Y:tOLEPAaR:l:p")) != -1) {
#endif
	    switch(c) {
/* non-config options first */

		/* getopt error or an actual ? */
		case '?':
			printf("Run "PTPD_PROGNAME" with -h to see available command-line options.\n");
			printf("Run "PTPD_PROGNAME" with -H or --long-help to show detailed help for all settings.\n\n");
			*ret = 1;
			return FALSE;
		case 'h':
short_help:
			printf("\n");
			printShortHelp();
			return FALSE;
		case 'H':
			printLongHelp();
			return FALSE;
		case 'e':
			if(strlen(optarg) > 0)
			    printSettingHelp(optarg);
			return FALSE;
		case 'O':
			printDefaultConfig();
			return FALSE;
		case 'T':
			dumpConfigTemplates();
			return FALSE;
/* regular ptpd options */

		/* config file path */
		case 'c':
			strncpy(global->configFile, optarg, PATH_MAX);
			break;
		/* check configuration and exit */
		case 'k':
			global->checkConfigOnly = TRUE;
			break;
		/* interface */
		case 'b':
			WARN_DEPRECATED('b', 'i', "interface", "ptpengine:interface");
		case 'i':
			/* if we got a number here, we've been given the domain number */
			if( (c=='i') && strlen(optarg) > 0 && isdigit((unsigned char)optarg[0]) ) {
				WARN_DEPRECATED_COMMENT('i', 'd', "domain", "ptpengine:domain",
				"for specifying domain number ");
				dictionary_set(dict,"ptpengine:domain", optarg);
			} else
				dictionary_set(dict,"ptpengine:interface", optarg);
			break;
		/* domain */
		case 'd':
			dictionary_set(dict,"ptpengine:domain", optarg);
			break;
		case 's':
			dictionary_set(dict,"ptpengine:preset", "slaveonly");
			break;
		/* master/slave */
		case 'W':
			WARN_DEPRECATED('W', 'm', "masterslave", "ptpengine:preset=masterslave");
		case 'm':
			dictionary_set(dict,"ptpengine:preset", "masterslave");
			break;
		/* master only */
		case 'G':
			WARN_DEPRECATED('G', 'M', "masteronly", "ptpengine:preset=masteronly");
		case 'M':
			dictionary_set(dict,"ptpengine:preset", "masteronly");
			break;
		case 'y':
			dictionary_set(dict,"ptpengine:transport_mode", "hybrid");
			break;
		/* unicast */
		case 'U':
			dictionary_set(dict,"ptpengine:transport_mode", "unicast");
			break;
		case 'g':
			dictionary_set(dict,"ptpengine:unicast_negotiation", "y");
			break;
		case 'u':
			dictionary_set(dict,"ptpengine:transport_mode", "unicast");
			dictionary_set(dict,"ptpengine:unicast_destinations", optarg);
			break;
		case 'n':
			dictionary_set(dict,"clock:no_adjust", "Y");
			break;
		/* log file */
		case 'f':
			dictionary_set(dict,"global:log_file", optarg);
			break;
		/* statistics file */
		case 'S':
			dictionary_set(dict,"global:statistics_file", optarg);
			break;
		case 't':
			dictionary_set(dict,"global:config_templates", optarg);
		/* Override delay request interval from master */
		case 'a':
			dictionary_set(dict,"ptpengine:log_delayreq_override", "Y");
			break;
		/* Delay request interval - needed for hybrid mode */
		case 'Y':
			WARN_DEPRECATED('Y', 'r', "delay-interval", "ptpengine:log_delayreq_interval");
		case 'r':
			dictionary_set(dict,"ptpengine:log_delayreq_interval", optarg);
			break;
                case 'D':
#ifdef RUNTIME_DEBUG
                        (global->debug_level)++;
                        if(global->debug_level > LOG_DEBUGV ){
                                global->debug_level = LOG_DEBUGV;
                        }
#else
                        printf("Runtime debug not enabled. Please compile with RUNTIME_DEBUG\n");
#endif
			break;
		/* print version string */
		case 'v':
			printf(PTPD_PROGNAME" version "USER_VERSION
#ifdef CODE_REVISION
			CODE_REVISION
			" built on "BUILD_DATE
#endif
			"\n");
			cckVersion();
			return FALSE;
		/* run in foreground */
		case 'C':
			global->nonDaemon=1;
			dictionary_set(dict,"global:foreground", "Y");
			break;
		/* verbose mode */
		case 'V':
			global->nonDaemon=1;
			dictionary_set(dict,"global:foreground", "Y");
			dictionary_set(dict,"global:verbose_foreground", "Y");
			break;
		/* Skip locking */
		case 'L':
			dictionary_set(dict,"global:ignore_lock", "Y");
			break;
		/* End to end delay detection mode */
		case 'E':
			dictionary_set(dict,"ptpengine:delay_mechanism", "E2E");
			break;
		/* Peer to peer delay detection mode */
		case 'P':
			dictionary_set(dict,"ptpengine:delay_mechanism", "P2P");
			break;
		/* Auto-lock */
		case 'A':
			dictionary_set(dict,"global:auto_lockfile", "Y");
			break;
		/* Print lock file only */
		case 'p':
			global->printLockFile = TRUE;
			break;
		/* Lock file */
		case 'l':
			dictionary_set(dict,"global:lock_file", optarg);
			break;
		/* Lock directory */
		case 'R':
			dictionary_set(dict,"global:lock_directory", optarg);
			break;
		default:
			break;

	    }
	}

return TRUE;

}

/* Display informatin about the built-in presets */
void
printPresetHelp()
{

	int i = 0;
	PtpEnginePreset preset;
	GlobalConfig defaultOpts;

	loadDefaultSettings(&defaultOpts);

	printf("\n==================AVAILABLE PTP ENGINE PRESETS===============\n\n");

    for(i = PTP_PRESET_NONE; i < PTP_PRESET_MAX; i++) {

	preset = getPtpPreset(i, &defaultOpts);
	printf("        preset name: %s\n", preset.presetName);
	printf("         slave only: %s\n", preset.slaveOnly ? "TRUE" : "FALSE");
	printf("allow clock control: %s\n", preset.noAdjust ? "FALSE" : "TRUE");
	printf("default clock class: %d%s\n", preset.clockClass.defaultValue,
	    preset.clockClass.minValue == preset.clockClass.maxValue ?
		    " (only allowed value)" : "");
	if (preset.clockClass.minValue != preset.clockClass.maxValue) {

	printf("  clock class range: %d..%d\n",
		preset.clockClass.minValue,
		preset.clockClass.maxValue);
	}
	printf("\n");

    }

	printf("\n=============================================================\n");

}

/* print "short" help - standard parameters only */
void
printShortHelp()
{
	printf(
			USER_DESCRIPTION" "USER_VERSION"\n"
			"\n"
			"usage: "PTPD_PROGNAME" <options> < --section:key=value...>\n"
			"\n"
			"WARNING: Any command-line options take priority over options from config file!\n"
#ifndef HAVE_GETOPT_LONG
			"WARNING: *** long options below (--some-option) are not supported on this system! \n"
			"NOTE:    *** config file style options (--section:key=value) are still supported\n"
#endif
			"\n"
			"Basic options: \n"
			"\n"
			"-c --config-file [path] 	Configuration file\n"
			"-k --check-config		Check configuration and exit\n"
			"-v --version			Print version string\n"
			"-h --help			Show this help screen\n"
			"-H --long-help			Show detailed help for all settings and behaviours\n"
			"-e --explain [section:key]	Show help for a single setting\n"
			"-O --default-config		Output default configuration (usable as config file)\n"
			"-L --ignore-lock		Skip lock file checks and locking\n"
			"-A --auto-lock			Use preset / port mode specific lock files\n"
			"-l --lockfile [path]		Specify path to lock file\n"
			"-p --print-lockfile		Print path to lock file and exit (useful for init scripts)\n"
			"-R --lock-directory [path]	Directory to store lock files\n"
			"-f --log-file [path]		global:log_file=[path]		Log file\n"
			"-S --statistics-file [path]	global:statistics_file=[path]	Statistics file\n"
			"-T --show-templates		display available configuration templates\n"
			"-t --templates [name],[name],[...]\n"
			"                               apply configuration template(s) - see man(5) ptpd.conf\n"
			"\n"
			"Basic PTP protocol and daemon configuration options: \n"
			"\n"
			"Command-line option		Config key(s)			Description\n"
			"------------------------------------------------------------------------------------\n"

			"-i --interface [dev]		ptpengine:interface=<dev>	Interface to use (required)\n"
			"-d --domain [n] 		ptpengine:domain=<n>		PTP domain number\n"
			"-s --slaveonly	 	 	ptpengine:preset=slaveonly	Slave only mode\n"
			"-m --masterslave 		ptpengine:preset=masterslave	Master, slave when not best GM\n"
			"-M --masteronly 		ptpengine:preset=masteronly	Master, passive when not best GM\n"
			"-y --hybrid			ptpengine:transport_mode=hybrid	Hybrid mode (multicast for sync\n"
			"								and announce, unicast for delay\n"
			"								request and response)\n"
			"-U --unicast			ptpengine:transport_mode=unicast	Unicast mode\n"
			"-g --unicast-negotiation	ptpengine:unicast_negotiation=y Enable unicast negotiation (signaling)\n"
			"-u --unicast-destinations 	ptpengine:transport_mode=unicast	Unicast destination list\n"
			"     [ip/host, ...]		ptpengine:unicast_destinations=<ip/host, ...>\n\n"
			"-E --e2e			ptpengine:delay_mechanism=E2E	End to end delay detection\n"
			"-P --p2p			ptpengine:delay_mechanism=P2P	Peer to peer delay detection\n"
			"\n"
			"-a --delay-override 		ptpengine:log_delayreq_override Override delay request interval\n"
			"                                                               announced by master\n"
			"-r --delay-interval [n] 	ptpengine:log_delayreq_interval=<n>	Delay request interval\n"
			"									(log 2)\n"
			"\n"
			"-n --noadjust			clock:no_adjust			Do not adjust the clock\n"
			"-D<DD...> --debug		global:debug_level=<level>	Debug level\n"
			"-C --foreground			global:foreground=<Y/N>		Don't run in background\n"
			"-V --verbose			global:verbose_foreground=<Y/N>	Run in foreground, log all\n"
			"								messages to standard output\n"
			"\n"
			"------------------------------------------------------------------------------------\n"
			"\n"
			PTPD_PROGNAME" accepts a configuration file (.ini style) where settings are either\n"
			"grouped into sections:\n\n"
			"[section]\n"
			"; comment\n"
			"key = value\n\n"
			"or are specified as:\n\n"
			"section:key = value\n\n"
			"All settings can also be supplied as command-line parameters (after other options):\n"
			"\n"
			"--section:key=<value> --section:key <value> -section:key=<value> -section:key <value>\n"
			"\n"
			"To see the full help for all configurable options, run: "PTPD_PROGNAME" with -H or --long-help\n"
			"To see help for a single setting only, run: "PTPD_PROGNAME" with -e (--explain) [section:key]\n"
			"To see all default settings with descriptions, run: "PTPD_PROGNAME" with -O (--default-config)\n"
			"\n"
			"------------------------------------------------------------------------------------\n"
			"\n"
			USER_DESCRIPTION" compatibility options for migration from versions below 2.3.0:\n"
			"\n"
			"-b [dev]			Network interface to use\n"
			"-i [n]				PTP domain number\n"
			"-G				'Master mode with NTP' (ptpengine:preset=masteronly)\n"
			"-W				'Master mode without NTP' (ptpengine:preset=masterslave) \n"
			"-Y [n]				Delay request interval (log 2)\n"
			"-t 				Do not adjust the clock\n"
			"\n"
			"Note 1: the above parameters are deprecated and their use will issue a warning.\n"
			"Note 2: -U and -g options have been re-purposed in 2.3.1.\n"
			"\n\n"
		);
}

/* Print the full help */
void
printLongHelp()
{

	printShortHelp();

	printConfigHelp();

	printPresetHelp();

	printf("\n"
		"                  Configuration templates available (see man(5) ptpd.conf):\n"
		"\n usage:\n"
		"                         -t [name],[name],...\n"
		"                --templates [name],[name],...\n"
		"  --global:config_templates=[name],[name],...\n\n");

	dumpConfigTemplates();

	printf("========================\n");


	printf("\n"
		"Possible internal states:\n"
		"  init:        INITIALIZING\n"
		"  flt:         FAULTY\n"
		"  lstn_init:   LISTENING (first time)\n"
		"  lstn_reset:  LISTENING (non first time)\n"
		"  pass:        PASSIVE Master (not best in BMC, not announcing)\n"
		"  uncl:        UNCALIBRATED\n"
		"  slv:         SLAVE\n"
		"  pmst:        PRE Master\n"
		"  mst:         ACTIVE Master\n"
		"  dsbl:        DISABLED\n"
		"  ?:           UNKNOWN state\n"
		"\n"

		"Handled signals:\n"
		"  SIGHUP         Reload configuration file and close / re-open log files\n"
		"  SIGUSR1        Manually step clock to current OFM value\n"
		"                 (overides clock:no_step, but honors clock:no_adjust)\n"
		"  SIGUSR2	  Dump all PTP protocol counters to current log target\n"
		"                 (and clear if ptpengine:sigusr2_clears_counters set)\n"
		"\n"
		"  SIGINT|SIGTERM Close open files, remove lock file and exit cleanly\n"
		"  SIGKILL        Force an unclean exit\n"
				"\n"
		"BMC Algorithm defaults:\n"
		"  Software:   P1(128) > Class(13|248) > Accuracy(\"unk\"/0xFE)   > Variance(65536) > P2(128)\n"
		    );

}


/*
 * Iterate through every key in newConfig and compare to oldConfig,
 * return TRUE if both equal, otherwise FALSE;
 */
Boolean
compareConfig(dictionary* newConfig, dictionary* oldConfig)
{

    int i = 0;

    if( newConfig == NULL || oldConfig == NULL) return -1;

    for(i = 0; i < newConfig->n; i++) {

        if(newConfig->key[i] == NULL)
            continue;

	if(strcmp(newConfig->val[i], dictionary_get(oldConfig, newConfig->key[i],"")) != 0 ) {
		DBG("Setting %s changed from '%s' to '%s'\n", newConfig->key[i],dictionary_get(oldConfig, newConfig->key[i],""),
		    newConfig->val[i]);
		return FALSE;
	}
    }

return TRUE;

}

/* Compare two configurations and set flags to mark components requiring restart */
int checkSubsystemRestart(dictionary* newConfig, dictionary* oldConfig, GlobalConfig *global)
{

	int restartFlags = 0;
	dictionary *tmpDict = dictionary_new(0);

	char *a, *b, *key;

	int i;

	/* both should be post-parse so will have all settings and thus same number of keys */
	for(i = 0; i < newConfig->n; i++) {

	    key = newConfig->key[i];

	    if(key == NULL) {
		continue;
	    }

	    a = dictionary_get(oldConfig, key, "");
	    b = dictionary_get(newConfig, key, "");

	    if(strcmp(a,b)) {
		DBG("+ setting %s changed from %s to %s\n", key, a, b);
		dictionary_set(tmpDict, key, "x");
	    }

	}

	/* run parser in restart check mode */
	parseConfig(CFGOP_RESTART_FLAGS | CFGOP_PARSE_QUIET, &restartFlags, tmpDict, global);

	dictionary_del(&tmpDict);

	/* ========= Any additional logic goes here =========== */

	/* Set of possible PTP port states has changed */
	if(configSettingChanged(oldConfig, newConfig, "ptpengine:slave_only") ||
		configSettingChanged(oldConfig, newConfig, "ptpengine:clock_class")) {

		int clockClass_old = iniparser_getint(oldConfig,"ptpengine:ptp_clockclass",0);
		int clockClass_new = iniparser_getint(newConfig,"ptpengine:ptp_clockclass",0);

		/*
		 * We're changing from a mode where slave state is possible
		 * to a master only mode, or vice versa
		 */
		if ((clockClass_old < 128 && clockClass_new > 127) ||
		(clockClass_old > 127 && clockClass_new < 128)) {

			/* If we are in auto lockfile mode, trigger a check if other locks match */
			if( !configSettingChanged(oldConfig, newConfig, "global:auto_lockfile") &&
			    DICT_ISTRUE(oldConfig, "global:auto_lockfile")) {
				restartFlags|=PTPD_CHECK_LOCKS;
			}
			/* We can potentially be running in a different mode now, restart protocol */
			restartFlags|=PTPD_RESTART_PROTOCOL;
		}
	}

	/* Flag for config options that can be applied on the fly - only to trigger a log message */
	if(restartFlags == 0)
	    restartFlags = PTPD_RESTART_NONE;

	return restartFlags;

}
