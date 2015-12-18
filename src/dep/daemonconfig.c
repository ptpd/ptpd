/*-
 * Copyright (c) 2013-2015 Wojciech Owczarek,
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

#include "../ptpd.h"

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
		WARNING("Warning: %s\n", messageText); \
	 }

#define CONFIG_KEY_CONDITIONAL_WARNING_ISSET(condition,dep,messageText) \
	if ( (condition) && \
	    CONFIG_ISSET(dep) ) \
	 { \
	    if(!(opCode & CFGOP_PARSE_QUIET))\
		WARNING("Warning: %s\n", messageText); \
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
		printf("   type: FLOAT (max: %d)\n", maxBound);
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

	if(value == def) {
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
 * NOTE: when adding options, also map them in checkSubsystemRestart to
 * ensure correct config reload behaviour.
 */
dictionary*
parseConfig ( int opCode, void *opArg, dictionary* dict, RunTimeOpts *rtOpts )
{

/*-
 * This function assumes that rtOpts has got all the defaults loaded,
 * hence the default values for all options are taken from rtOpts.
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
		PTPD_RESTART_NETWORK, rtOpts->primaryIfaceName, sizeof(rtOpts->primaryIfaceName), rtOpts->primaryIfaceName,
	"Network interface to use - eth0, igb0 etc. (required).");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:backup_interface",
		PTPD_RESTART_NETWORK, rtOpts->backupIfaceName, sizeof(rtOpts->backupIfaceName), rtOpts->backupIfaceName,
		"Backup network interface to use - eth0, igb0 etc. When no GM available, \n"
	"	 slave will keep alternating between primary and secondary until a GM is found.\n");

	CONFIG_KEY_TRIGGER("ptpengine:backup_interface", rtOpts->backupIfaceEnabled,TRUE,FALSE);

	/* Preset option names have to be mapped to defined presets - no free strings here */
	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:preset",
		PTPD_RESTART_PROTOCOL, &rtOpts->selectedPreset, rtOpts->selectedPreset,
		"PTP engine preset:\n"
	"	 none	     = Defaults, no clock class restrictions\n"
	"        masteronly  = Master, passive when not best master (clock class 0..127)\n"
	"        masterslave = Full IEEE 1588 implementation:\n"
	"                      Master, slave when not best master\n"
	"	 	       (clock class 128..254)\n"
	"        slaveonly   = Slave only (clock class 255 only)\n",
#ifndef PTPD_SLAVE_ONLY
				(getPtpPreset(PTP_PRESET_NONE, rtOpts)).presetName,		PTP_PRESET_NONE,
				(getPtpPreset(PTP_PRESET_MASTERONLY, rtOpts)).presetName,	PTP_PRESET_MASTERONLY,
				(getPtpPreset(PTP_PRESET_MASTERSLAVE, rtOpts)).presetName,	PTP_PRESET_MASTERSLAVE,
#endif /* PTPD_SLAVE_ONLY */
				(getPtpPreset(PTP_PRESET_SLAVEONLY, rtOpts)).presetName,		PTP_PRESET_SLAVEONLY, NULL
				);

	CONFIG_KEY_CONDITIONAL_CONFLICT("ptpengine:preset",
	 			    rtOpts->selectedPreset == PTP_PRESET_MASTERSLAVE,
	 			    "masterslave",
	 			    "ptpengine:unicast_negotiation");

	ptpPreset = getPtpPreset(rtOpts->selectedPreset, rtOpts);


	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:transport",
		PTPD_RESTART_NETWORK, &rtOpts->transport, rtOpts->transport,
		"Transport type for PTP packets. Ethernet transport requires libpcap support.",
				"ipv4",		UDP_IPV4,
#if 0
				"ipv6",		UDP_IPV6,
#endif
				"ethernet", 	IEEE_802_3, NULL
				);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:dot1as", PTPD_UPDATE_DATASETS, &rtOpts->dot1AS, rtOpts->dot1AS,
		"Enable TransportSpecific field compatibility with 802.1AS / AVB (requires Ethernet transport)");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disabled", PTPD_RESTART_PROTOCOL, &rtOpts->portDisabled, rtOpts->portDisabled,
		"Disable PTP port. Causes the PTP state machine to stay in PTP_DISABLED state indefinitely,\n"
	"        until it is re-enabled via configuration change or ENABLE_PORT management message.");


	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->transport != IEEE_802_3) && rtOpts->dot1AS,
	 			    "ptpengine:dot1as",
				"802.1AS compatibility can only be used with the Ethernet transport\n");

	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport != IEEE_802_3, rtOpts->dot1AS,FALSE, rtOpts->dot1AS);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ip_mode",
		PTPD_RESTART_NETWORK, &rtOpts->ipMode, rtOpts->ipMode,
		"IP transmission mode (requires IP transport) - hybrid mode uses\n"
	"	 multicast for sync and announce, and unicast for delay request and\n"
	"	 response; unicast mode uses unicast for all transmission.\n"
	"	 When unicast mode is selected, destination IP(s) may need to be configured\n"
	"	(ptpengine:unicast_destinations).",
				"multicast", 	IPMODE_MULTICAST,
				"unicast", 	IPMODE_UNICAST,
				"hybrid", 	IPMODE_HYBRID, NULL
				);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_negotiation",
		PTPD_RESTART_PROTOCOL, &rtOpts->unicastNegotiation, rtOpts->unicastNegotiation,
		"Enable unicast negotiation support using signaling messages\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_any_master",
		PTPD_RESTART_NONE, &rtOpts->unicastAcceptAny, rtOpts->unicastAcceptAny,
		"When using unicast negotiation (slave), accept PTP messages from any master.\n"
	"        By default, only messages from acceptable masters (ptpengine:unicast_destinations)\n"
	"        are accepted, and only if transmission was granted by the master\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:unicast_port_mask",
		PTPD_RESTART_PROTOCOL, INTTYPE_U16, &rtOpts->unicastPortMask, rtOpts->unicastPortMask,
		"PTP port number wildcard mask applied onto port identities when running\n"
	"        unicast negotiation: allows multiple port identities to be accepted as one.\n"
	"	 This option can be used as a workaround where a node sends signaling messages and\n"
	"	 timing messages with different port identities", RANGECHECK_RANGE, 0,65535);

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->transport == IEEE_802_3) && rtOpts->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation",
				"Unicast negotiation cannot be used with Ethernet transport\n");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->ipMode != IPMODE_UNICAST) && rtOpts->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation",
				"Unicast negotiation can only be used with unicast transmission\n");

	/* disable unicast negotiation unless running unicast */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3, rtOpts->unicastNegotiation,FALSE, rtOpts->unicastNegotiation);
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->ipMode != IPMODE_UNICAST, rtOpts->unicastNegotiation,FALSE, rtOpts->unicastNegotiation);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disable_bmca",
		PTPD_RESTART_PROTOCOL, &rtOpts->disableBMCA, rtOpts->disableBMCA,
		"Disable Best Master Clock Algorithm for unicast masters:\n"
	"        Only effective for masteronly preset - all Announce messages\n"
	"        will be ignored and clock will transition directly into MASTER state.\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:unicast_negotiation_listening",
		PTPD_RESTART_NONE, &rtOpts->unicastNegotiationListening, rtOpts->unicastNegotiationListening,
		"When unicast negotiation enabled on a master clock, \n"
	"	 reply to transmission requests also in LISTENING state.");

#if defined(PTPD_PCAP) && defined(__sun) && !defined(PTPD_EXPERIMENTAL)
	if(CONFIG_ISTRUE("ptpengine:use_libpcap"))
	INFO("Libpcap support is currently marked broken/experimental on Solaris platforms.\n"
	     "To test it, please build with --enable-experimental-options\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:use_libpcap",
		PTPD_RESTART_NETWORK, &rtOpts->pcap,FALSE,
		"Use libpcap for sending and receiving traffic (automatically enabled\n"
	"	 in Ethernet mode).");

	/* cannot set ethernet transport without libpcap */
	CONFIG_KEY_VALUE_FORBIDDEN("ptpengine:transport",
				    rtOpts->transport == IEEE_802_3,
				    "ethernet",
	    "Libpcap support is currently marked broken/experimental on Solaris platforms.\n"
	    "To test it and use the Ethernet transport, please build with --enable-experimental-options\n");
#elif defined(PTPD_PCAP)
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:use_libpcap",
		PTPD_RESTART_NETWORK, &rtOpts->pcap, rtOpts->pcap,
		"Use libpcap for sending and receiving traffic (automatically enabled\n"
	"	 in Ethernet mode).");

	/* in ethernet mode, activate pcap and overwrite previous setting */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport==IEEE_802_3, rtOpts->pcap,TRUE, rtOpts->pcap);
#else
	if(CONFIG_ISTRUE("ptpengine:use_libpcap"))
	INFO("Libpcap support disabled or not available. Please install libpcap,\n"
	     "build without --disable-pcap, or try building with ---with-pcap-config\n"
	     " to use ptpengine:use_libpcap.\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:use_libpcap",
		PTPD_RESTART_NETWORK, &rtOpts->pcap,FALSE,
		"Use libpcap for sending and receiving traffic (automatically enabled\n"
	"	 in Ethernet mode).");

	/* cannot set ethernet transport without libpcap */
	CONFIG_KEY_VALUE_FORBIDDEN("ptpengine:transport",
				    rtOpts->transport == IEEE_802_3,
				    "ethernet",
	    "Libpcap support disabled or not available. Please install libpcap,\n"
	     "build without --disable-pcap, or try building with ---with-pcap-config\n"
	     "to use Ethernet transport. "PTPD_PROGNAME" was built with no libpcap support.\n");

#endif /* PTPD_PCAP */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:disable_udp_checksums",
		PTPD_RESTART_NETWORK, &rtOpts->disableUdpChecksums, rtOpts->disableUdpChecksums,
		"Disable UDP checksum validation on UDP sockets (Linux only).\n"
	"        Workaround for situations where a node (like Transparent Clock).\n"
	"        does not rewrite checksums\n");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_mechanism",
		PTPD_RESTART_PROTOCOL, &rtOpts->delayMechanism, rtOpts->delayMechanism,
		 "Delay detection mode used - use DELAY_DISABLED for syntonisation only\n"
	"	 (no full synchronisation).",
				delayMechToString(E2E),		E2E,
				delayMechToString(P2P),		P2P,
				delayMechToString(DELAY_DISABLED), DELAY_DISABLED, NULL
				);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:domain",
		PTPD_RESTART_PROTOCOL, INTTYPE_U8, &rtOpts->domainNumber, rtOpts->domainNumber,
		"PTP domain number.", RANGECHECK_RANGE, 0,127);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:port_number", PTPD_UPDATE_DATASETS, INTTYPE_U16, &rtOpts->portNumber, rtOpts->portNumber,
			    "PTP port number (part of PTP Port Identity - not UDP port).\n"
		    "        For ordinary clocks (single port), the default should be used, \n"
		    "        but when running multiple instances to simulate a boundary clock, \n"
		    "        The port number can be changed.",RANGECHECK_RANGE,1,65534);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:port_description",
		PTPD_UPDATE_DATASETS, rtOpts->portDescription, sizeof(rtOpts->portDescription), rtOpts->portDescription,
	"Port description (returned in the userDescription field of PORT_DESCRIPTION management message and USER_DESCRIPTION"
	"        management message) - maximum 64 characters");

	parseResult &= configMapString(opCode, opArg, dict, target, "variables:product_description",
		PTPD_UPDATE_DATASETS, rtOpts->productDescription, sizeof(rtOpts->productDescription), rtOpts->productDescription,"");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:any_domain",
		PTPD_RESTART_PROTOCOL, &rtOpts->anyDomain, rtOpts->anyDomain,
		"Usability extension: if enabled, a slave-only clock will accept\n"
	"	 masters from any domain, while preferring the configured domain,\n"
	"	 and preferring lower domain number.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:slave_only",
		PTPD_RESTART_NONE, &rtOpts->slaveOnly, ptpPreset.slaveOnly,
		 "Slave only mode (sets clock class to 255, overriding value from preset).");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:inbound_latency",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->inboundLatency.nanoseconds, rtOpts->inboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for incoming packets.", RANGECHECK_NONE, 0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:outbound_latency",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->outboundLatency.nanoseconds, rtOpts->outboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for outgoing packets.", RANGECHECK_NONE,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:offset_shift",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->ofmShift.nanoseconds, rtOpts->ofmShift.nanoseconds,
	"Apply an arbitrary shift (nanoseconds) to offset from master when\n"
	"	 in slave state. Value can be positive or negative - useful for\n"
	"	 correcting for antenna latencies, delay assymetry\n"
	"	 and IP stack latencies. This will not be visible in the offset \n"
	"	 from master value - only in the resulting clock correction.", RANGECHECK_NONE, 0,0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:always_respect_utc_offset",
		PTPD_RESTART_NONE, &rtOpts->alwaysRespectUtcOffset, rtOpts->alwaysRespectUtcOffset,
		"Compatibility option: In slave state, always respect UTC offset\n"
	"	 announced by best master, even if the the\n"
	"	 currrentUtcOffsetValid flag is announced FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:prefer_utc_offset_valid",
		PTPD_RESTART_NONE, &rtOpts->preferUtcValid, rtOpts->preferUtcValid,
		"Compatibility extension to BMC algorithm: when enabled,\n"
	"	 BMC for both master and save clocks will prefer masters\n"
	"	 nannouncing currrentUtcOffsetValid as TRUE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:require_utc_offset_valid",
		PTPD_RESTART_NONE, &rtOpts->requireUtcValid, rtOpts->requireUtcValid,
		"Compatibility option: when enabled, ptpd2 will ignore\n"
	"	 Announce messages from masters announcing currentUtcOffsetValid\n"
	"	 as FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	/* from 30 seconds to 7 days */
	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:unicast_grant_duration",
		PTPD_RESTART_PROTOCOL, INTTYPE_U32, &rtOpts->unicastGrantDuration, rtOpts->unicastGrantDuration,
		"Time (seconds) unicast messages are requested for by slaves\n"
	"	 when using unicast negotiation, and maximum time unicast message\n"
	"	 transmission is granted to slaves by masters\n", RANGECHECK_RANGE, 30, 604800);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_announce_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logAnnounceInterval, rtOpts->logAnnounceInterval,
		"PTP announce message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-4,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_announce_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logMaxAnnounceInterval, rtOpts->logMaxAnnounceInterval,
		"Maximum Announce message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logAnnounceInterval >= rtOpts->logMaxAnnounceInterval,
					"ptpengine:log_announce_interval value must be lower than ptpengine:log_announce_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:announce_receipt_timeout", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->announceReceiptTimeout, rtOpts->announceReceiptTimeout,
		"PTP announce receipt timeout announced in master state.",RANGECHECK_RANGE,2,255);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:announce_receipt_grace_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->announceTimeoutGracePeriod, rtOpts->announceTimeoutGracePeriod,
		"PTP announce receipt timeout grace period in slave state:\n"
	"	 when announce receipt timeout occurs, disqualify current best GM,\n"
	"	 then wait n times announce receipt timeout before resetting.\n"
	"	 Allows for a seamless GM failover when standby GMs are slow\n"
	"	 to react. When set to 0, this option is not used.", RANGECHECK_RANGE,
	0,20);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_sync_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logSyncInterval, rtOpts->logSyncInterval,
		"PTP sync message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_sync_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logMaxSyncInterval, rtOpts->logMaxSyncInterval,
		"Maximum Sync message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logSyncInterval >= rtOpts->logMaxSyncInterval,
					"ptpengine:log_sync_interval value must be lower than ptpengine:log_sync_interval_max\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:log_delayreq_override", PTPD_UPDATE_DATASETS, &rtOpts->ignore_delayreq_interval_master,
	rtOpts->ignore_delayreq_interval_master,
		 "Override the Delay Request interval announced by best master.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:log_delayreq_auto", PTPD_UPDATE_DATASETS, &rtOpts->autoDelayReqInterval,
	rtOpts->autoDelayReqInterval,
		 "Automatically override the Delay Request interval\n"
	"         if the announced value is 127 (0X7F), such as in\n"
	"         unicast messages (unless using unicast negotiation)");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval_initial",
		PTPD_RESTART_NONE, INTTYPE_I8, &rtOpts->initial_delayreq, rtOpts->initial_delayreq,
		"Delay request interval used before receiving first delay response\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	/* take the delayreq_interval from config, otherwise use the initial setting as default */
	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logMinDelayReqInterval, rtOpts->initial_delayreq,
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

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_delayreq_interval_max", PTPD_UPDATE_DATASETS, INTTYPE_I8, &rtOpts->logMaxDelayReqInterval, rtOpts->logMaxDelayReqInterval,
		"Maximum Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logMinDelayReqInterval >= rtOpts->logMaxDelayReqInterval,
					"ptpengine:log_delayreq_interval value must be lower than ptpengine:log_delayreq_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_peer_delayreq_interval",
		PTPD_RESTART_NONE, INTTYPE_I8, &rtOpts->logMinPdelayReqInterval, rtOpts->logMinPdelayReqInterval,
		"Minimum peer delay request message interval in peer to peer delay mode.\n"
	"        When using unicast negotiation, this is the minimum interval requested, \n"
	"	 and the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-7,7);
#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:log_peer_delayreq_interval_max",
		PTPD_RESTART_NONE, INTTYPE_I8, &rtOpts->logMaxPdelayReqInterval, rtOpts->logMaxPdelayReqInterval,
		"Maximum Peer Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,RANGECHECK_RANGE,-30,30);
#else
    "	"LOG2_HELP,RANGECHECK_RANGE,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logMinPdelayReqInterval >= rtOpts->logMaxPdelayReqInterval,
					"ptpengine:log_peer_delayreq_interval value must be lower than ptpengine:log_peer_delayreq_interval_max\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:foreignrecord_capacity",
		PTPD_RESTART_DAEMON, INTTYPE_I16, &rtOpts->max_foreign_records, rtOpts->max_foreign_records,
	"Foreign master record size (Maximum number of foreign masters).",RANGECHECK_RANGE,5,10);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ptp_allan_variance", PTPD_UPDATE_DATASETS, INTTYPE_U16, &rtOpts->clockQuality.offsetScaledLogVariance, rtOpts->clockQuality.offsetScaledLogVariance,
	"Specify Allan variance announced in master state.",RANGECHECK_RANGE,0,65535);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_clock_accuracy", PTPD_UPDATE_DATASETS, &rtOpts->clockQuality.clockAccuracy, rtOpts->clockQuality.clockAccuracy,
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

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:utc_offset", PTPD_UPDATE_DATASETS, INTTYPE_I16, &rtOpts->timeProperties.currentUtcOffset, rtOpts->timeProperties.currentUtcOffset,
		 "Underlying time source UTC offset announced in master state.", RANGECHECK_NONE,0,0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:utc_offset_valid", PTPD_UPDATE_DATASETS, &rtOpts->timeProperties.currentUtcOffsetValid,
	rtOpts->timeProperties.currentUtcOffsetValid,
		 "Underlying time source UTC offset validity announced in master state.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:time_traceable", PTPD_UPDATE_DATASETS, &rtOpts->timeProperties.timeTraceable,
	rtOpts->timeProperties.timeTraceable,
		 "Underlying time source time traceability announced in master state.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:frequency_traceable", PTPD_UPDATE_DATASETS, &rtOpts->timeProperties.frequencyTraceable,
	rtOpts->timeProperties.frequencyTraceable,
		 "Underlying time source frequency traceability announced in master state.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_timescale", PTPD_UPDATE_DATASETS, (uint8_t*)&rtOpts->timeProperties.ptpTimescale, rtOpts->timeProperties.ptpTimescale,
		"Time scale announced in master state (with ARB, UTC properties\n"
	"	 are ignored by slaves). When clock class is set to 13 (application\n"
	"	 specific), this value is ignored and ARB is used.",
				"PTP",		TRUE,
				"ARB",			FALSE, NULL
				);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:ptp_timesource", PTPD_UPDATE_DATASETS, &rtOpts->timeProperties.timeSource, rtOpts->timeProperties.timeSource,
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

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:clock_class", PTPD_UPDATE_DATASETS, INTTYPE_U8, &rtOpts->clockQuality.clockClass,ptpPreset.clockClass.defaultValue,
		"Clock class - announced in master state. Always 255 for slave-only.\n"
	"	 Minimum, maximum and default values are controlled by presets.\n"
	"	 If set to 13 (application specific time source), announced \n"
	"	 time scale is always set to ARB. This setting controls the\n"
	"	 states a PTP port can be in. If below 128, port will only\n"
	"	 be in MASTER or PASSIVE states (master only). If above 127,\n"
	"	 port will be in MASTER or SLAVE states.", RANGECHECK_RANGE,
	ptpPreset.clockClass.minValue,ptpPreset.clockClass.maxValue);

	/* ClockClass = 13 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE,
					rtOpts->timeProperties.ptpTimescale,FALSE, rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 14 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==14,
					rtOpts->timeProperties.ptpTimescale,FALSE, rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 6 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==6,
					rtOpts->timeProperties.ptpTimescale,TRUE, rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 7 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==7,
					rtOpts->timeProperties.ptpTimescale,TRUE, rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 255 triggers slaveOnly */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==SLAVE_ONLY_CLOCK_CLASS, rtOpts->slaveOnly,TRUE,FALSE);
	/* ...and vice versa */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->slaveOnly==TRUE, rtOpts->clockQuality.clockClass,SLAVE_ONLY_CLOCK_CLASS, rtOpts->clockQuality.clockClass);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:priority1", PTPD_UPDATE_DATASETS, INTTYPE_U8, &rtOpts->priority1, rtOpts->priority1,
		"Priority 1 announced in master state,used for Best Master\n"
	"	 Clock selection.",RANGECHECK_RANGE,0,248);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:priority2", PTPD_UPDATE_DATASETS, INTTYPE_U8, &rtOpts->priority2, rtOpts->priority2,
		"Priority 2 announced in master state, used for Best Master\n"
	"	 Clock selection.",RANGECHECK_RANGE,0,248);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:max_listen",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->maxListen, rtOpts->maxListen,
		 "Number of consecutive resets to LISTENING before full network reset\n",RANGECHECK_MIN,1,0);

	/*
	 * TODO: in unicast and hybrid mode, automativally override master delayreq interval with a default,
	 * rather than require setting it manually.
	 */

	/* hybrid mode -> should specify delayreq interval: override set in the bottom of this function */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(rtOpts->ipMode == IPMODE_HYBRID,
    		    "ptpengine:log_delayreq_interval",
		    "It is recommended to manually set the delay request interval (ptpengine:log_delayreq_interval) to required value in hybrid mode"
	);

	/* unicast mode -> should specify delayreq interval if we can become a slave */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(rtOpts->ipMode == IPMODE_UNICAST &&
		    rtOpts->clockQuality.clockClass > 127,
		    "ptpengine:log_delayreq_interval",
		    "It is recommended to manually set the delay request interval (ptpengine:log_delayreq_interval) to required value in unicast mode"
	);

	CONFIG_KEY_ALIAS("ptpengine:unicast_address","ptpengine:unicast_destinations");

	/* unicast signaling slave -> must specify unicast destination(s) */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:ip_mode",
				     rtOpts->clockQuality.clockClass > 127 &&
				    rtOpts->ipMode == IPMODE_UNICAST &&
				    rtOpts->unicastNegotiation,
				    "unicast",
				    "ptpengine:unicast_destinations");

	/* unicast master without signaling - must specify unicast destinations */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:ip_mode",
				     rtOpts->clockQuality.clockClass <= 127 &&
				    rtOpts->ipMode == IPMODE_UNICAST &&
				    !rtOpts->unicastNegotiation,
				    "unicast",
				    "ptpengine:unicast_destinations");

	CONFIG_KEY_TRIGGER("ptpengine:unicast_destinations", rtOpts->unicastDestinationsSet,TRUE, rtOpts->unicastDestinationsSet);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_destinations",
		PTPD_RESTART_NETWORK, rtOpts->unicastDestinations, sizeof(rtOpts->unicastDestinations), rtOpts->unicastDestinations,
		"Specify unicast slave addresses for unicast master operation, or unicast\n"
	"	 master addresses for slave operation. Format is similar to an ACL: comma,\n"
	"        tab or space-separated IPv4 unicast addresses, one or more. For a slave,\n"
	"        when unicast negotiation is used, setting this is mandatory.");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_domains",
		PTPD_RESTART_NETWORK, rtOpts->unicastDomains, sizeof(rtOpts->unicastDomains), rtOpts->unicastDomains,
		"Specify PTP domain number for each configured unicast destination (ptpengine:unicast_destinations).\n"
	"	 This is only used by slave-only clocks using unicast destinations to allow for each master\n"
	"        to be in a separate domain, such as with Telecom Profile. The number of entries should match the number\n"
	"        of unicast destinations, otherwise unconfigured domains or domains set to 0 are set to domain configured in\n"
	"        ptpengine:domain. The format is a comma, tab or space-separated list of 8-bit unsigned integers (0 .. 255)");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_local_preference",
		PTPD_RESTART_NETWORK, rtOpts->unicastLocalPreference, sizeof(rtOpts->unicastLocalPreference), rtOpts->unicastLocalPreference,
		"Specify a local preference for each configured unicast destination (ptpengine:unicast_destinations).\n"
	"	 This is only used by slave-only clocks using unicast destinations to allow for each master's\n"
	"        BMC selection to be influenced by the slave, such as with Telecom Profile. The number of entries should match the number\n"
	"        of unicast destinations, otherwise unconfigured preference is set to 0 (highest).\n"
	"        The format is a comma, tab or space-separated list of 8-bit unsigned integers (0 .. 255)");

	/* unicast P2P - must specify unicast peer destination */
	CONFIG_KEY_CONDITIONAL_DEPENDENCY("ptpengine:delay_mechanism",
					rtOpts->delayMechanism == P2P &&
				    rtOpts->ipMode == IPMODE_UNICAST,
				    "P2P",
				    "ptpengine:unicast_peer_destination");

	CONFIG_KEY_TRIGGER("ptpengine:unicast_peer_destination", rtOpts->unicastPeerDestinationSet,TRUE, rtOpts->unicastPeerDestinationSet);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:unicast_peer_destination",
		PTPD_RESTART_NETWORK, rtOpts->unicastPeerDestination, sizeof(rtOpts->unicastPeerDestination), rtOpts->unicastPeerDestination,
		"Specify peer unicast adress for P2P unicast. Mandatory when\n"
	"	 running unicast mode and P2P delay mode.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:management_enable",
		PTPD_RESTART_NONE, &rtOpts->managementEnabled, rtOpts->managementEnabled,
	"Enable handling of PTP management messages.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:management_set_enable",
		PTPD_RESTART_NONE, &rtOpts->managementSetEnable, rtOpts->managementSetEnable,
	"Accept SET and COMMAND management messages.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:igmp_refresh",
		PTPD_RESTART_NONE, &rtOpts->do_IGMP_refresh, rtOpts->do_IGMP_refresh,
	"Send explicit IGMP joins between engine resets and periodically\n"
	"	 in master state.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:master_igmp_refresh_interval",
		PTPD_RESTART_PROTOCOL, INTTYPE_I8, &rtOpts->masterRefreshInterval, rtOpts->masterRefreshInterval,
		"Periodic IGMP join interval (seconds) in master state when running\n"
		"	 IPv4 multicast: when set below 10 or when ptpengine:igmp_refresh\n"
		"	 is disabled, this setting has no effect.",RANGECHECK_RANGE,0,255);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:multicast_ttl",
		PTPD_RESTART_NETWORK, INTTYPE_INT, &rtOpts->ttl, rtOpts->ttl,
		"Multicast time to live for multicast PTP packets (ignored and set to 1\n"
	"	 for peer to peer messages).",RANGECHECK_RANGE,1,64);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ip_dscp",
		PTPD_RESTART_NETWORK, INTTYPE_INT, &rtOpts->dscpValue, rtOpts->dscpValue,
		"DiffServ CodepPoint for packet prioritisation (decimal). When set to zero, \n"
	"	 this option is not used. Use 46 for Expedited Forwarding (0x2e).",RANGECHECK_RANGE,0,63);

#ifdef PTPD_STATISTICS

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_enable",
		PTPD_RESTART_FILTERS, &rtOpts->filterMSOpts.enabled, rtOpts->filterMSOpts.enabled,
		 "Enable statistical filter for Sync messages.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_type",
		PTPD_RESTART_FILTERS, &rtOpts->filterMSOpts.filterType, rtOpts->filterMSOpts.filterType,
		"Type of filter used for Sync message filtering",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &rtOpts->filterMSOpts.windowSize, rtOpts->filterMSOpts.windowSize,
		"Number of samples used for the Sync statistical filter",RANGECHECK_RANGE,3,128);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_stat_filter_window_type",
		PTPD_RESTART_FILTERS, &rtOpts->filterMSOpts.windowType, rtOpts->filterMSOpts.windowType,
		"Sample window type used for Sync message statistical filter. Delay Response outlier filter action.\n"
	"        Sliding window is continuous, interval passes every n-th sample only.",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL, NULL);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_enable",
		PTPD_RESTART_FILTERS, &rtOpts->filterSMOpts.enabled, rtOpts->filterSMOpts.enabled,
		 "Enable statistical filter for Delay messages.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_type",
		PTPD_RESTART_FILTERS, &rtOpts->filterSMOpts.filterType, rtOpts->filterSMOpts.filterType,
		"Type of filter used for Delay message statistical filter",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_window",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &rtOpts->filterSMOpts.windowSize, rtOpts->filterSMOpts.windowSize,
		"Number of samples used for the Delay statistical filter",RANGECHECK_RANGE,3,128);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_stat_filter_window_type",
		PTPD_RESTART_FILTERS, &rtOpts->filterSMOpts.windowType, rtOpts->filterSMOpts.windowType,
		"Sample window type used for Delay message statistical filter\n"
	"        Sliding window is continuous, interval passes every n-th sample only",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL, NULL);


	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterSMConfig.enabled, rtOpts->oFilterSMConfig.enabled,
		 "Enable outlier filter for the Delay Response component in slave state");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_action",
		PTPD_RESTART_NONE, (uint8_t*)&rtOpts->oFilterSMConfig.discard, rtOpts->oFilterSMConfig.discard,
		"Delay Response outlier filter action. If set to 'filter', outliers are\n"
	"	 replaced with moving average.",
	"discard", TRUE,
	"filter", FALSE, NULL);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_capacity",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &rtOpts->oFilterSMConfig.capacity, rtOpts->oFilterSMConfig.capacity,
		"Number of samples in the Delay Response outlier filter buffer",RANGECHECK_RANGE,10,STATCONTAINER_MAX_SAMPLES);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_threshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.threshold, rtOpts->oFilterSMConfig.threshold,
		"Delay Response outlier filter threshold (: multiplier for Peirce's maximum\n"
	"	 standard deviation. When set below 1.0, filter is tighter, when set above\n"
	"	 1.0, filter is looser than standard Peirce's test.\n"
	"        When autotune enabled, this is the starting threshold.", RANGECHECK_RANGE, 0.001, 1000.0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_always_filter",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.alwaysFilter, rtOpts->oFilterSMConfig.alwaysFilter,
		"Always run the Delay Response outlier filter, even if clock is being slewed at maximum rate");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterSMConfig.autoTune, rtOpts->oFilterSMConfig.autoTune,
		"Enable automatic threshold control for Delay Response outlier filter.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_minpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterSMConfig.minPercent, rtOpts->oFilterSMConfig.minPercent,
		"Delay Response outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,0,99);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_maxpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterSMConfig.maxPercent, rtOpts->oFilterSMConfig.maxPercent,
		"Delay Response outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_autotune_step",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.thresholdStep, rtOpts->oFilterSMConfig.thresholdStep,
		"The value the Delay Response outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_minthreshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.minThreshold, rtOpts->oFilterSMConfig.minThreshold,
		"Minimum Delay Response filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_autotune_maxthreshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.maxThreshold, rtOpts->oFilterSMConfig.maxThreshold,
		"Maximum Delay Response filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterSMConfig.maxPercent <= rtOpts->oFilterSMConfig.minPercent,
					"ptpengine:delay_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterSMConfig.maxThreshold <= rtOpts->oFilterSMConfig.minThreshold,
					"ptpengine:delay_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minthreshold\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterSMConfig.stepDelay, rtOpts->oFilterSMConfig.stepDelay,
		"Enable Delay filter step detection (delaySM) to block when certain level exceeded");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_threshold",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->oFilterSMConfig.stepThreshold, rtOpts->oFilterSMConfig.stepThreshold,
		"Delay Response step detection threshold. Step detection is performed\n"
	"	 only when delaySM is below this threshold (nanoseconds)", RANGECHECK_RANGE, 50000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_level",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->oFilterSMConfig.stepLevel, rtOpts->oFilterSMConfig.stepLevel,
		"Delay Response step level. When step detection enabled and operational,\n"
	"	 delaySM above this level (nanosecond) is considered a clock step and updates are paused", RANGECHECK_RANGE,50000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_credit",
		PTPD_RESTART_FILTERS, INTTYPE_I32, &rtOpts->oFilterSMConfig.delayCredit, rtOpts->oFilterSMConfig.delayCredit,
		"Initial credit (number of samples) the Delay step detection filter can block for\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",RANGECHECK_RANGE,50,1000);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:delay_outlier_filter_stepdetect_credit_increment",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterSMConfig.creditIncrement, rtOpts->oFilterSMConfig.creditIncrement,
		"Amount of credit for the Delay step detection filter restored every full sample window",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:delay_outlier_weight",
		PTPD_RESTART_NONE, &rtOpts->oFilterSMConfig.weight, rtOpts->oFilterSMConfig.weight,
		"Delay Response outlier weight: if an outlier is detected, determines\n"
	"	 the amount of its deviation from mean that is used to build the standard\n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", RANGECHECK_RANGE, 0.01, 2.0);

    parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterMSConfig.enabled, rtOpts->oFilterMSConfig.enabled,
		"Enable outlier filter for the Sync component in slave state.");

    parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_action",
		PTPD_RESTART_NONE, (uint8_t*)&rtOpts->oFilterMSConfig.discard, rtOpts->oFilterMSConfig.discard,
		"Sync outlier filter action. If set to 'filter', outliers are replaced\n"
	"	 with moving average.",
     "discard", TRUE,
     "filter", FALSE, NULL);

     parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_capacity",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &rtOpts->oFilterMSConfig.capacity, rtOpts->oFilterMSConfig.capacity,
    "Number of samples in the Sync outlier filter buffer.",RANGECHECK_RANGE,10,STATCONTAINER_MAX_SAMPLES);

    parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_threshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.threshold, rtOpts->oFilterMSConfig.threshold,
		"Sync outlier filter threshold: multiplier for the Peirce's maximum standard\n"
	"	 deviation. When set below 1.0, filter is tighter, when set above 1.0,\n"
	"	 filter is looser than standard Peirce's test.", RANGECHECK_RANGE, 0.001, 1000.0);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_always_filter",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.alwaysFilter, rtOpts->oFilterMSConfig.alwaysFilter,
		"Always run the Sync outlier filter, even if clock is being slewed at maximum rate");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterMSConfig.autoTune, rtOpts->oFilterMSConfig.autoTune,
		"Enable automatic threshold control for Sync outlier filter.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_minpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterMSConfig.minPercent, rtOpts->oFilterMSConfig.minPercent,
		"Sync outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,0,99);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_maxpercent",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterMSConfig.maxPercent, rtOpts->oFilterMSConfig.maxPercent,
		"Sync outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_autotune_step",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.thresholdStep, rtOpts->oFilterMSConfig.thresholdStep,
		"Value the Sync outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_minthreshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.minThreshold, rtOpts->oFilterMSConfig.minThreshold,
		"Minimum Sync outlier filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_autotune_maxthreshold",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.maxThreshold, rtOpts->oFilterMSConfig.maxThreshold,
		"Maximum Sync outlier filter threshold value used when auto-tuning", RANGECHECK_RANGE, 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.maxPercent <= rtOpts->oFilterMSConfig.minPercent,
					"ptpengine:sync_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.maxThreshold <= rtOpts->oFilterMSConfig.minThreshold,
					"ptpengine:sync_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minthreshold\n");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_enable",
		PTPD_RESTART_FILTERS, &rtOpts->oFilterMSConfig.stepDelay, rtOpts->oFilterMSConfig.stepDelay,
		"Enable Sync filter step detection (delayMS) to block when certain level exceeded.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_threshold",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->oFilterMSConfig.stepThreshold, rtOpts->oFilterMSConfig.stepThreshold,
		"Sync step detection threshold. Step detection is performed\n"
	"	 only when delayMS is below this threshold (nanoseconds)", RANGECHECK_RANGE,100000, NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_level",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->oFilterMSConfig.stepLevel, rtOpts->oFilterMSConfig.stepLevel,
		"Sync step level. When step detection enabled and operational,\n"
	"	 delayMS above this level (nanosecond) is considered a clock step and updates are paused", RANGECHECK_RANGE,100000, NANOSECONDS_MAX);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.stepThreshold <= (rtOpts->oFilterMSConfig.stepLevel + 100000),
					"ptpengine:sync_outlier_filter_stepdetect_threshold  has to be at least "
					"100 us (100000) greater than ptpengine:sync_outlier_filter_stepdetect_level\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_credit",
		PTPD_RESTART_FILTERS, INTTYPE_INT, &rtOpts->oFilterMSConfig.delayCredit, rtOpts->oFilterMSConfig.delayCredit,
		"Initial credit (number of samples) the Sync step detection filter can block for.\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",RANGECHECK_RANGE,50,1000);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:sync_outlier_filter_stepdetect_credit_increment",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->oFilterMSConfig.creditIncrement, rtOpts->oFilterMSConfig.creditIncrement,
		"Amount of credit for the Sync step detection filter restored every full sample window",RANGECHECK_RANGE,1,100);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "ptpengine:sync_outlier_weight",
		PTPD_RESTART_NONE, &rtOpts->oFilterMSConfig.weight, rtOpts->oFilterMSConfig.weight,
		"Sync outlier weight: if an outlier is detected, this value determines the\n"
	"	 amount of its deviation from mean that is used to build the standard \n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", RANGECHECK_RANGE, 0.01, 2.0);

        parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:calibration_delay",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->calibrationDelay, rtOpts->calibrationDelay,
		"Delay between moving to slave state and enabling clock updates (seconds).\n"
	"	 This allows one-way delay to stabilise before starting clock updates.\n"
	"	 Activated when going into slave state and during slave's GM failover.\n"
	"	 0 - not used.",RANGECHECK_RANGE,0,300);

#endif /* PTPD_STATISTICS */


        parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:idle_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->idleTimeout, rtOpts->idleTimeout,
		"PTP idle timeout: if PTPd is in SLAVE state and there have been no clock\n"
	"	 updates for this amout of time, PTPd releases clock control.\n", RANGECHECK_RANGE,10,3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:offset_alarm_threshold", PTPD_UPDATE_DATASETS, INTTYPE_U32, &rtOpts->ofmAlarmThreshold, rtOpts->ofmAlarmThreshold,
		 "PTP slave offset from master threshold (nanoseconds - absolute value)\n"
	"	 When offset exceeds this value, an alarm is raised (also SNMP trap if configured).\n"
	"	 0 = disabled.", RANGECHECK_NONE,0,NANOSECONDS_MAX);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode",
		PTPD_RESTART_NONE, &rtOpts->enablePanicMode, rtOpts->enablePanicMode,
		"Enable panic mode: when offset from master is above 1 second, stop updating\n"
	"	 the clock for a period of time and then step the clock if offset remains\n"
	"	 above 1 second.");

    parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:panic_mode_duration",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->panicModeDuration, rtOpts->panicModeDuration,
		"Duration (minutes) of the panic mode period (no clock updates) when offset\n"
	"	 above 1 second detected.",RANGECHECK_RANGE,1,60);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode_release_clock",
		PTPD_RESTART_NONE, &rtOpts->panicModeReleaseClock, rtOpts->panicModeReleaseClock,
		"When entering panic mode, release clock control while panic mode lasts\n"
 	"	 if ntpengine:* configured, this will fail over to NTP,\n"
	"	 if not set, PTP will hold clock control during panic mode.");

    parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:panic_mode_exit_threshold",
		PTPD_RESTART_NONE, INTTYPE_U32, &rtOpts->panicModeExitThreshold, rtOpts->panicModeExitThreshold,
		"Do not exit panic mode until offset drops below this value (nanoseconds).\n"
	"	 0 = not used.",RANGECHECK_RANGE,0,NANOSECONDS_MAX);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:pid_as_clock_identity",
		PTPD_RESTART_PROTOCOL, &rtOpts->pidAsClockId, rtOpts->pidAsClockId,
	"Use PTPd's process ID as the middle part of the PTP clock ID - useful for running multiple instances.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:ntp_failover",
		PTPD_RESTART_NONE, &rtOpts->ntpOptions.enableFailover, rtOpts->ntpOptions.enableFailover,
		"Fail over to NTP when PTP time sync not available - requires\n"
	"	 ntpengine:enabled, but does not require the rest of NTP configuration:\n"
	"	 will warn instead of failing over if cannot control ntpd.");

	CONFIG_KEY_DEPENDENCY("ptpengine:ntp_failover", "ntpengine:enabled");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:ntp_failover_timeout",
		PTPD_RESTART_NTPCONFIG, INTTYPE_INT, &rtOpts->ntpOptions.failoverTimeout,
								rtOpts->ntpOptions.failoverTimeout,	
		"NTP failover timeout in seconds: time between PTP slave going into\n"
	"	 LISTENING state, and releasing clock control. 0 = fail over immediately.", RANGECHECK_RANGE,0, 1800);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:prefer_ntp",
		PTPD_RESTART_NTPCONFIG, &rtOpts->preferNTP, rtOpts->preferNTP,
		"Prefer NTP time synchronisation. Only use PTP when NTP not available,\n"
	"	 could be used when NTP runs with a local GPS receiver or another reference");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:panic_mode_ntp",
		PTPD_RESTART_NONE, &rtOpts->panicModeReleaseClock, rtOpts->panicModeReleaseClock,
		"Legacy option from 2.3.0: same as ptpengine:panic_mode_release_clock");

	CONFIG_KEY_DEPENDENCY("ptpengine:panic_mode_ntp", "ntpengine:enabled");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sigusr2_clears_counters",
		PTPD_RESTART_NONE, &rtOpts->clearCounters, rtOpts->clearCounters,
		"Clear counters after dumping all counter values on SIGUSR2.");

	/* Defining the ACLs enables ACL matching */
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_permit", rtOpts->timingAclEnabled,TRUE, rtOpts->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_deny", rtOpts->timingAclEnabled,TRUE, rtOpts->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_permit", rtOpts->managementAclEnabled,TRUE, rtOpts->managementAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_deny", rtOpts->managementAclEnabled,TRUE, rtOpts->managementAclEnabled);

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:timing_acl_permit",
		PTPD_RESTART_ACLS, rtOpts->timingAclPermitText, sizeof(rtOpts->timingAclPermitText), rtOpts->timingAclPermitText,
		"Permit access control list for timing packets. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:timing_acl_deny",
		PTPD_RESTART_ACLS, rtOpts->timingAclDenyText, sizeof(rtOpts->timingAclDenyText), rtOpts->timingAclDenyText,
		"Deny access control list for timing packets. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:management_acl_permit",
		PTPD_RESTART_ACLS, rtOpts->managementAclPermitText, sizeof(rtOpts->managementAclPermitText), rtOpts->managementAclPermitText,
		"Permit access control list for management messages. Format is a series of \n"
	"	 comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
	"	 where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
	"        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
	"        incoming messages. IP access lists are only supported when using the IP transport.");

	parseResult &= configMapString(opCode, opArg, dict, target, "ptpengine:management_acl_deny",
		PTPD_RESTART_ACLS, rtOpts->managementAclDenyText, sizeof(rtOpts->managementAclDenyText), rtOpts->managementAclDenyText,
		"Deny access control list for management messages. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");


	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:timing_acl_order",
		PTPD_RESTART_ACLS, &rtOpts->timingAclOrder, rtOpts->timingAclOrder,
		"Order in which permit and deny access lists are evaluated for timing\n"
	"	 packets, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	ACL_PERMIT_DENY,
				"deny-permit", 	ACL_DENY_PERMIT, NULL
				);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "ptpengine:management_acl_order",
		PTPD_RESTART_ACLS, &rtOpts->managementAclOrder, rtOpts->managementAclOrder,
		"Order in which permit and deny access lists are evaluated for management\n"
	"	 messages, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	ACL_PERMIT_DENY,
				"deny-permit", 	ACL_DENY_PERMIT, NULL
				);


	/* Ethernet mode disables ACL processing*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3, rtOpts->timingAclEnabled,FALSE, rtOpts->timingAclEnabled);
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3, rtOpts->managementAclEnabled,FALSE, rtOpts->managementAclEnabled);



/* ===== clock section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:no_adjust",
		PTPD_RESTART_NONE, &rtOpts->noAdjust,ptpPreset.noAdjust,
	"Do not adjust the clock");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:no_reset",
		PTPD_RESTART_NONE, &rtOpts->noResetClock, rtOpts->noResetClock,
	"Do not step the clock - only slew");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:step_startup_force",
		PTPD_RESTART_NONE, &rtOpts->stepForce, rtOpts->stepForce,
	"Force clock step on first sync after startup regardless of offset and clock:no_reset");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:step_startup",
		PTPD_RESTART_NONE, &rtOpts->stepOnce, rtOpts->stepOnce,
		"Step clock on startup if offset >= 1 second, ignoring\n"
	"        panic mode and clock:no_reset");


#ifdef HAVE_LINUX_RTC_H
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "clock:set_rtc_on_step",
		PTPD_RESTART_NONE, &rtOpts->setRtc, rtOpts->setRtc,
	"Attempt setting the RTC when stepping clock (Linux only - FreeBSD does \n"
	"        this for us. WARNING: this will always set the RTC to OS clock time,\n"
	"        regardless of time zones, so this assumes that RTC runs in UTC or \n"
	"        at least in the same timescale as PTP. true at least on most \n"
	"        single-boot x86 Linux systems.");
#endif /* HAVE_LINUX_RTC_H */

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "clock:drift_handling",
		PTPD_RESTART_NONE, &rtOpts->drift_recovery_method, rtOpts->drift_recovery_method,
		"Observed drift handling method between servo restarts:\n"
	"	 reset: set to zero (not recommended)\n"
	"	 preserve: use kernel value,\n"
	"	 file: load/save to drift file on startup/shutdown, use kernel\n"
	"	 value inbetween. To specify drift file, use the clock:drift_file setting."
	,
				"reset", 	DRIFT_RESET,
				"preserve", 	DRIFT_KERNEL,
				"file", 	DRIFT_FILE, NULL
				);

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:drift_file",
		PTPD_RESTART_NONE, rtOpts->driftFile, sizeof(rtOpts->driftFile), rtOpts->driftFile,
	"Specify drift file");

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:leap_second_pause_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->leapSecondPausePeriod,
		rtOpts->leapSecondPausePeriod,
		"Time (seconds) before and after midnight that clock updates should pe suspended for\n"
	"	 during a leap second event. The total duration of the pause is twice\n"
	"        the configured duration",RANGECHECK_RANGE,5,600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:leap_second_notice_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->leapSecondNoticePeriod,
		rtOpts->leapSecondNoticePeriod,
		"Time (seconds) before midnight that PTPd starts announcing the leap second\n"
	"	 if it's running as master",RANGECHECK_RANGE,3600,86400);

	parseResult &= configMapString(opCode, opArg, dict, target, "clock:leap_seconds_file",
		PTPD_RESTART_NONE, rtOpts->leapFile, sizeof(rtOpts->leapFile), rtOpts->leapFile,
	"Specify leap second file location - up to date version can be downloaded from \n"
	"        http://www.ietf.org/timezones/data/leap-seconds.list");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "clock:leap_second_handling",
		PTPD_RESTART_NONE, &rtOpts->leapSecondHandling, rtOpts->leapSecondHandling,
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
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->leapSecondSmearPeriod,
		rtOpts->leapSecondSmearPeriod,
		"Time period (Seconds) over which the leap second is introduced before the event.\n"
	"	 Example: when set to 86400 (24 hours), an extra 11.5 microseconds is added every second"
	,RANGECHECK_RANGE,3600,86400);


#ifdef HAVE_STRUCT_TIMEX_TICK
	/* This really is clock specific - different clocks may allow different ranges */
	parseResult &= configMapInt(opCode, opArg, dict, target, "clock:max_offset_ppm",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->servoMaxPpb, rtOpts->servoMaxPpb,
		"Maximum absolute frequency shift which can be applied to the clock servo\n"
	"	 when slewing the clock. Expressed in parts per million (1 ppm = shift of\n"
	"	 1 us per second. Values above 512 will use the tick duration correction\n"
	"	 to allow even faster slewing. Default maximum is 512 without using tick.", RANGECHECK_RANGE,
	ADJ_FREQ_MAX/1000,ADJ_FREQ_MAX/500);
#endif /* HAVE_STRUCT_TIMEX_TICK */

	/*
	 * TimeProperties DS - in future when clock driver API is implemented,
	 * a slave PTP engine should inform a clock about this, and then that
	 * clock should pass this information to any master PTP engines, unless
	 * we override this. here. For now we just supply this to RtOpts.
	 */
	

/* ===== servo section ===== */

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:delayfilter_stiffness",
		PTPD_RESTART_NONE, INTTYPE_I16, &rtOpts->s, rtOpts->s,
	"One-way delay filter stiffness.", RANGECHECK_NONE,0,0);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:kp",
		PTPD_RESTART_NONE, &rtOpts->servoKP, rtOpts->servoKP,
	"Clock servo PI controller proportional component gain (kP).", RANGECHECK_MIN, 0.000001, 0);
	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:ki",
		PTPD_RESTART_NONE, &rtOpts->servoKI, rtOpts->servoKI,
	"Clock servo PI controller integral component gain (kI).", RANGECHECK_MIN, 0.000001,0);

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "servo:dt_method",
		PTPD_RESTART_NONE, &rtOpts->servoDtMethod, rtOpts->servoDtMethod,
		"How servo update interval (delta t) is calculated:\n"
	"	 none:     servo not corrected for update interval (dt always 1),\n"
	"	 constant: constant value (target servo update rate - sync interval for PTP,\n"
	"	 measured: servo measures how often it's updated and uses this interval.",
			"none", DT_NONE,
			"constant", DT_CONSTANT,
			"measured", DT_MEASURED, NULL
	);

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:dt_max",
		PTPD_RESTART_NONE, &rtOpts->servoMaxdT, rtOpts->servoMaxdT,
		"Maximum servo update interval (delta t) when using measured servo update interval\n"
	"	 (servo:dt_method = measured), specified as sync interval multiplier.", RANGECHECK_RANGE, 1.5,100.0);

#ifdef PTPD_STATISTICS
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "servo:stability_detection",
		PTPD_RESTART_NONE, &rtOpts->servoStabilityDetection,
							rtOpts->servoStabilityDetection,
		 "Enable clock synchronisation servo stability detection\n"
	"	 (based on standard deviation of the observed drift value)\n"
	"	 - drift will be saved to drift file / cached when considered stable,\n"
	"	 also clock stability status will be logged.");

	parseResult &= configMapDouble(opCode, opArg, dict, target, "servo:stability_threshold",
		PTPD_RESTART_NONE, &rtOpts->servoStabilityThreshold,
							rtOpts->servoStabilityThreshold,
		"Specify the observed drift standard deviation threshold in parts per\n"
	"	 billion (ppb) - if stanard deviation is within the threshold, servo\n"
	"	 is considered stable.", RANGECHECK_RANGE, 1.0,10000.0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:stability_period",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->servoStabilityPeriod,
							rtOpts->servoStabilityPeriod,
		"Specify for how many statistics update intervals the observed drift\n"
	"	standard deviation has to stay within threshold to be considered stable.", RANGECHECK_RANGE,
	1,100);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:stability_timeout",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->servoStabilityTimeout,
							rtOpts->servoStabilityTimeout,
		"Specify after how many minutes without stabilisation servo is considered\n"
	"	 unstable. Assists with logging servo stability information and\n"
	"	 allows to preserve observed drift if servo cannot stabilise.\n", RANGECHECK_RANGE,
	1,60);

#endif

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_delay",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->maxDelay, rtOpts->maxDelay,
		"Do accept master to slave delay (delayMS - from Sync message) or slave to master delay\n"
	"	 (delaySM - from Delay messages) if greater than this value (nanoseconds). 0 = not used.", RANGECHECK_RANGE,
	0,NANOSECONDS_MAX);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_delay_max_rejected",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->maxDelayMaxRejected, rtOpts->maxDelayMaxRejected,
		"Maximum number of consecutive delay measurements exceeding maxDelay threshold,\n"
	"	 before slave is reset.", RANGECHECK_MIN,0,0);

#ifdef PTPD_STATISTICS
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "servo:max_delay_stable_only",
		PTPD_RESTART_NONE, &rtOpts->maxDelayStableOnly, rtOpts->maxDelayStableOnly,
		"If servo:max_delay is set, perform the check only if clock servo has stabilised.\n");
#endif

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ptpengine:sync_sequence_checking",
		PTPD_RESTART_NONE, &rtOpts->syncSequenceChecking, rtOpts->syncSequenceChecking,
		"When enabled, Sync messages will only be accepted if sequence ID is increasing."
	"        This is limited to 50 dropped messages.\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ptpengine:clock_update_timeout",
		PTPD_RESTART_PROTOCOL, INTTYPE_INT, &rtOpts->clockUpdateTimeout, rtOpts->clockUpdateTimeout,
		"If set to non-zero, timeout in seconds, after which the slave resets if no clock updates made. \n", RANGECHECK_RANGE,
		0, 3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "servo:max_offset",
		PTPD_RESTART_NONE, INTTYPE_I32, &rtOpts->maxOffset, rtOpts->maxOffset,
		"Do not reset the clock if offset from master is greater\n"
	"        than this value (nanoseconds). 0 = not used.", RANGECHECK_RANGE,
	0,NANOSECONDS_MAX);

/* ===== global section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_alarms",
	    PTPD_RESTART_ALARMS, &rtOpts->alarmsEnabled, rtOpts->alarmsEnabled,
		 "Enable support for alarm and event notifications.\n");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:alarm_timeout",
		PTPD_RESTART_ALARMS, INTTYPE_INT, &rtOpts->alarmMinAge, rtOpts->alarmMinAge,
		"Mininmum alarm age (seconds) - minimal time between alarm set and clear notifications.\n"
	"	 The condition can clear while alarm lasts, but notification (log or SNMP) will only \n"
	"	 be triggered after the timeout. This option prevents from alarms flapping.", RANGECHECK_RANGE, 0, 3600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:alarm_initial_delay",
		PTPD_RESTART_DAEMON, INTTYPE_INT, &rtOpts->alarmInitialDelay, rtOpts->alarmInitialDelay,
		"Delay the start of alarm processing (seconds) after ptpd startup. This option \n"
	"	 allows to avoid unnecessary alarms before PTPd starts synchronising.\n",
	RANGECHECK_RANGE, 0, 3600);

#ifdef PTPD_SNMP
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_snmp",
		PTPD_RESTART_DAEMON, &rtOpts->snmpEnabled, rtOpts->snmpEnabled,
	"Enable SNMP agent (if compiled with PTPD_SNMP).");
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:enable_snmp_traps",
	    PTPD_RESTART_ALARMS, &rtOpts->snmpTrapsEnabled, rtOpts->snmpTrapsEnabled,
		 "Enable sending SNMP traps (only if global:enable_alarms set and global:enable_snmp set).\n");
#else
	if(!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISTRUE("global:enable_snmp"))
	    INFO("SNMP support not enabled. Please compile with PTPD_SNMP to use global:enable_snmp\n");
	if(!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISTRUE("global:enable_snmp_traps"))
	    INFO("SNMP support not enabled. Please compile with PTPD_SNMP to use global:enable_snmp_traps\n");
#endif /* PTPD_SNMP */



	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:use_syslog",
		PTPD_RESTART_LOGGING, &rtOpts->useSysLog, rtOpts->useSysLog,
		"Send log messages to syslog. Disabling this\n"
	"        sends all messages to stdout (or speficied log file).");

	parseResult &= configMapString(opCode, opArg, dict, target, "global:lock_file",
		PTPD_RESTART_DAEMON, rtOpts->lockFile, sizeof(rtOpts->lockFile), rtOpts->lockFile,
	"Lock file location");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:auto_lockfile",
		PTPD_RESTART_DAEMON, &rtOpts->autoLockFile, rtOpts->autoLockFile,
	"	 Use mode specific and interface specific lock file\n"
	"	 (overrides global:lock_file).");

	parseResult &= configMapString(opCode, opArg, dict, target, "global:lock_directory",
		PTPD_RESTART_DAEMON, rtOpts->lockDirectory, sizeof(rtOpts->lockDirectory), rtOpts->lockDirectory,
		 "Lock file directory: used with automatic mode-specific lock files,\n"
	"	 also used when no lock file is specified. When lock file\n"
	"	 is specified, it's expected to be an absolute path.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:ignore_lock",
		PTPD_RESTART_DAEMON, &rtOpts->ignore_daemon_lock, rtOpts->ignore_daemon_lock,
	"Skip lock file checking and locking.");

	/* if quality file specified, enable quality recording  */
	CONFIG_KEY_TRIGGER("global:quality_file", rtOpts->recordLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:quality_file",
		PTPD_RESTART_LOGGING, rtOpts->recordLog.logPath, sizeof(rtOpts->recordLog.logPath), rtOpts->recordLog.logPath,
		"File used to record data about sync packets. Enables recording when set.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:quality_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &rtOpts->recordLog.maxSize, rtOpts->recordLog.maxSize,
		"Maximum sync packet record file size (in kB) - file will be truncated\n"
	"	if size exceeds the limit. 0 - no limit.", RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:quality_file_max_files",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &rtOpts->recordLog.maxFiles, rtOpts->recordLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:quality_file_truncate",
		PTPD_RESTART_LOGGING, &rtOpts->recordLog.truncateOnReopen, rtOpts->recordLog.truncateOnReopen,
		"Truncate the sync packet record file every time it is (re) opened:\n"
	"	 startup and SIGHUP.");

	/* if status file specified, enable status logging*/
	CONFIG_KEY_TRIGGER("global:status_file", rtOpts->statusLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:status_file",
		PTPD_RESTART_LOGGING, rtOpts->statusLog.logPath, sizeof(rtOpts->statusLog.logPath), rtOpts->statusLog.logPath,
	"File used to log "PTPD_PROGNAME" status information.");
	/* status file can be disabled even if specified */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_status",
		PTPD_RESTART_NONE, &rtOpts->statusLog.logEnabled, rtOpts->statusLog.logEnabled,
		"Enable / disable writing status information to file.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:status_update_interval",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &rtOpts->statusFileUpdateInterval, rtOpts->statusFileUpdateInterval,
		"Status file update interval in seconds.", RANGECHECK_RANGE,
	1,30);

#ifdef RUNTIME_DEBUG
	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:debug_level",
		PTPD_RESTART_NONE, (uint8_t*)&rtOpts->debug_level, rtOpts->debug_level,
	"Specify debug level (if compiled with RUNTIME_DEBUG).",
				"LOG_INFO", 	LOG_INFO,
				"LOG_DEBUG", 	LOG_DEBUG,
				"LOG_DEBUG1", 	LOG_DEBUG1,
				"LOG_DEBUG2", 	LOG_DEBUG2,
				"LOG_DEBUG3", 	LOG_DEBUG3,
				"LOG_DEBUGV", 	LOG_DEBUGV, NULL
				);
#else
	if (!(opCode & CFGOP_PARSE_QUIET) && CONFIG_ISSET("global:debug_level"))
	    INFO("Runtime debug not enabled. Please compile with RUNTIME_DEBUG to use global:debug_level.\n");
#endif /* RUNTIME_DEBUG */

    /*
     * Log files are reopened and / or re-created on SIGHUP anyway,
     */

	/* if log file specified, enable file logging - otherwise disable */
	CONFIG_KEY_TRIGGER("global:log_file", rtOpts->eventLog.logEnabled,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:log_file",
		PTPD_RESTART_LOGGING, rtOpts->eventLog.logPath, sizeof(rtOpts->eventLog.logPath), rtOpts->eventLog.logPath,
		"Specify log file path (event log). Setting this enables logging to file.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:log_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &rtOpts->eventLog.maxSize, rtOpts->eventLog.maxSize,
		"Maximum log file size (in kB) - log file will be truncated if size exceeds\n"
	"	 the limit. 0 - no limit.", RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:log_file_max_files",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->eventLog.maxFiles, rtOpts->eventLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_file_truncate",
		PTPD_RESTART_LOGGING, &rtOpts->eventLog.truncateOnReopen, rtOpts->eventLog.truncateOnReopen,
		"Truncate the log file every time it is (re) opened: startup and SIGHUP.");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:log_level",
		PTPD_RESTART_NONE, &rtOpts->logLevel, rtOpts->logLevel,
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
	CONFIG_KEY_TRIGGER("global:statistics_file", rtOpts->statisticsLog.logEnabled,TRUE,FALSE);
	CONFIG_KEY_TRIGGER("global:statistics_file", rtOpts->logStatistics,TRUE,FALSE);
	parseResult &= configMapString(opCode, opArg, dict, target, "global:statistics_file",
		PTPD_RESTART_LOGGING, rtOpts->statisticsLog.logPath, sizeof(rtOpts->statisticsLog.logPath), rtOpts->statisticsLog.logPath,
		"Specify statistics log file path. Setting this enables logging of \n"
	"	 statistics, but can be overriden with global:log_statistics.");

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_log_interval",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->statisticsLogInterval, rtOpts->statisticsLogInterval,
		 "Log timing statistics every n seconds for Sync and Delay messages\n"
	"	 (0 - log all).",RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_file_max_size",
		PTPD_RESTART_LOGGING, INTTYPE_U32, &rtOpts->statisticsLog.maxSize, rtOpts->statisticsLog.maxSize,
		"Maximum statistics log file size (in kB) - log file will be truncated\n"
	"	 if size exceeds the limit. 0 - no limit.",RANGECHECK_MIN,0,0);

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_file_max_files",
		PTPD_RESTART_LOGGING, INTTYPE_INT, &rtOpts->statisticsLog.maxFiles, rtOpts->statisticsLog.maxFiles,
		"Enable log rotation of the statistics file up to n files. 0 - do not rotate.", RANGECHECK_RANGE,0, 100);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:statistics_file_truncate",
		PTPD_RESTART_LOGGING, &rtOpts->statisticsLog.truncateOnReopen, rtOpts->statisticsLog.truncateOnReopen,
		"Truncate the statistics file every time it is (re) opened: startup and SIGHUP.");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:dump_packets",
		PTPD_RESTART_NONE, &rtOpts->displayPackets, rtOpts->displayPackets,
		"Dump the contents of every PTP packet");

	/* this also checks if the verbose_foreground flag is set correctly */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:verbose_foreground",
		PTPD_RESTART_DAEMON, &rtOpts->nonDaemon, rtOpts->nonDaemon,
		"Run in foreground with statistics and all messages logged to stdout.\n"
	"	 Overrides log file and statistics file settings and disables syslog.\n");

	if(CONFIG_ISTRUE("global:verbose_foreground")) {
		rtOpts->useSysLog    = FALSE;
		rtOpts->logStatistics = TRUE;
		rtOpts->statisticsLogInterval = 0;
		rtOpts->eventLog.logEnabled = FALSE;
		rtOpts->statisticsLog.logEnabled = FALSE;
	}

	/*
	 * this HAS to be executed after the verbose_foreground mapping because of the same
	 * default field used for both. verbose_foreground triggers nonDaemon which is OK,
	 * but we don't want foreground=y to switch on verbose_foreground if not set.
	 */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:foreground",
		PTPD_RESTART_DAEMON, &rtOpts->nonDaemon, rtOpts->nonDaemon,
		"Run in foreground - ignored when global:verbose_foreground is set");

	if(CONFIG_ISTRUE("global:verbose_foreground")) {
		rtOpts->nonDaemon = TRUE;
	}

	/* If this is processed after verbose_foreground, we can still control logStatistics */
	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:log_statistics",
		PTPD_RESTART_NONE, &rtOpts->logStatistics, rtOpts->logStatistics,
		"Log timing statistics for every PTP packet received\n");

	parseResult &= configMapSelectValue(opCode, opArg, dict, target, "global:statistics_timestamp_format",
		PTPD_RESTART_NONE, &rtOpts->statisticsTimestamp, rtOpts->statisticsTimestamp,
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
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->statisticsLog.logEnabled && !rtOpts->logStatistics,
					rtOpts->statisticsLog.logEnabled, FALSE, rtOpts->statisticsLog.logEnabled);

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	parseResult &= configMapInt(opCode, opArg, dict, target, "global:cpuaffinity_cpucore", PTPD_CHANGE_CPUAFFINITY, INTTYPE_INT, &rtOpts->cpuNumber, rtOpts->cpuNumber,
		"Bind "PTPD_PROGNAME" process to a selected CPU core number.\n"
	"        0 = first CPU core, etc. -1 = do not bind to a single core.", RANGECHECK_RANGE,
	-1,255);
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H */

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:statistics_update_interval",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->statsUpdateInterval,
								rtOpts->statsUpdateInterval,
		"Clock synchronisation statistics update interval in seconds\n", RANGECHECK_RANGE,1, 60);

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "global:periodic_updates",
		PTPD_RESTART_LOGGING, &rtOpts->periodicUpdates, rtOpts->periodicUpdates,
		"Log a status update every time statistics are updated (global:statistics_update_interval).\n"
	"        The updates are logged even when ptpd is configured without statistics support");

#ifdef PTPD_STATISTICS

	CONFIG_CONDITIONAL_ASSERTION( rtOpts->servoStabilityDetection && (
				    (rtOpts->statsUpdateInterval * rtOpts->servoStabilityPeriod) / 60 >=
				    rtOpts->servoStabilityTimeout),
			"The configured servo stabilisation timeout has to be longer than\n"
		"	 servo stabilisation period");

#endif /* PTPD_STATISTICS */

	parseResult &= configMapInt(opCode, opArg, dict, target, "global:timingdomain_election_delay",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->electionDelay, rtOpts->electionDelay,
		" Delay (seconds) before releasing a time service (NTP or PTP)"
	"        and electing a new one to control a clock. 0 = elect immediately\n", RANGECHECK_RANGE,0, 3600);


/* ===== ntpengine section ===== */

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ntpengine:enabled",
		PTPD_RESTART_NTPENGINE, &rtOpts->ntpOptions.enableEngine, rtOpts->ntpOptions.enableEngine,
	"Enable NTPd integration");

	parseResult &= configMapBoolean(opCode, opArg, dict, target, "ntpengine:control_enabled",
		PTPD_RESTART_NONE, &rtOpts->ntpOptions.enableControl, rtOpts->ntpOptions.enableControl,
	"Enable control over local NTPd daemon");

	CONFIG_KEY_DEPENDENCY("ntpengine:control_enabled", "ntpengine:enabled");

	parseResult &= configMapInt(opCode, opArg, dict, target, "ntpengine:check_interval",
		PTPD_RESTART_NTPCONFIG, INTTYPE_INT, &rtOpts->ntpOptions.checkInterval,
								rtOpts->ntpOptions.checkInterval,
		"NTP control check interval in seconds\n", RANGECHECK_RANGE, 5, 600);

	parseResult &= configMapInt(opCode, opArg, dict, target, "ntpengine:key_id",
		PTPD_RESTART_NONE, INTTYPE_INT, &rtOpts->ntpOptions.keyId, rtOpts->ntpOptions.keyId,
		 "NTP key number - must be configured as a trusted control key in ntp.conf,\n"
	"	  and be non-zero for the ntpengine:control_enabled setting to take effect.\n", RANGECHECK_RANGE,0, 65535);

	parseResult &= configMapString(opCode, opArg, dict, target, "ntpengine:key",
		PTPD_RESTART_NONE, rtOpts->ntpOptions.key, sizeof(rtOpts->ntpOptions.key), rtOpts->ntpOptions.key,
		"NTP key (plain text, max. 20 characters) - must match the key configured in\n"
	"	 ntpd's keys file, and must be non-zero for the ntpengine:control_enabled\n"
	"	 setting to take effect.\n");

	CONFIG_KEY_DEPENDENCY("ntpengine:control:enabled", "ntpengine:key_id");
	CONFIG_KEY_DEPENDENCY("ntpengine:control:enabled", "ntpengine:key");


/* ============== END CONFIG MAPPINGS, TRIGGERS AND DEPENDENCIES =========== */

/* ==== Any additional logic should go here ===== */

	rtOpts->ifaceName = rtOpts->primaryIfaceName;

	/* Check timing packet ACLs */
	if(rtOpts->timingAclEnabled) {

		int pResult, dResult;

		if((pResult = maskParser(rtOpts->timingAclPermitText, NULL)) == -1)
			ERROR("Error while parsing timing permit access list: \"%s\"\n",
				rtOpts->timingAclPermitText);
		if((dResult = maskParser(rtOpts->timingAclDenyText, NULL)) == -1)
			ERROR("Error while parsing timing deny access list: \"%s\"\n",
				rtOpts->timingAclDenyText);

		/* -1 = ACL format error*/
		if(pResult == -1 || dResult == -1) {
			parseResult = FALSE;
			rtOpts->timingAclEnabled = FALSE;
		}
		/* 0 = no entries - we simply don't match */
		if(pResult == 0 && dResult == 0) {
			rtOpts->timingAclEnabled = FALSE;
		}
	}


	/* Check management message ACLs */
	if(rtOpts->managementAclEnabled) {

		int pResult, dResult;

		if((pResult = maskParser(rtOpts->managementAclPermitText, NULL)) == -1)
			ERROR("Error while parsing management permit access list: \"%s\"\n",
				rtOpts->managementAclPermitText);
		if((dResult = maskParser(rtOpts->managementAclDenyText, NULL)) == -1)
			ERROR("Error while parsing management deny access list: \"%s\"\n",
				rtOpts->managementAclDenyText);

		/* -1 = ACL format error*/
		if(pResult == -1 || dResult == -1) {
			parseResult = FALSE;
			rtOpts->managementAclEnabled = FALSE;
		}
		/* 0 = no entries - we simply don't match */
		if(pResult == 0 && dResult == 0) {
			rtOpts->managementAclEnabled = FALSE;
		}
	}

	/* Scale the maxPPM to PPB */
	rtOpts->servoMaxPpb *= 1000;

	/* Shift DSCP to accept the 6-bit value */
	rtOpts->dscpValue = rtOpts->dscpValue << 2;

	/*
	 * We're in hybrid mode and we haven't specified the delay request interval:
	 * use override with a default value
	 */
	if((rtOpts->ipMode == IPMODE_HYBRID) &&
	 !CONFIG_ISSET("ptpengine:log_delayreq_interval"))
		rtOpts->ignore_delayreq_interval_master=TRUE;

	/*
	 * We're in unicast slave-capable mode and we haven't specified the delay request interval:
	 * use override with a default value
	 */
	if((rtOpts->ipMode == IPMODE_UNICAST &&
	    rtOpts->clockQuality.clockClass > 127) &&
	    !CONFIG_ISSET("ptpengine:log_delayreq_interval"))
		rtOpts->ignore_delayreq_interval_master=TRUE;

	/*
	 * construct the lock file name based on operation mode:
	 * if clock class is <128 (master only), use "master" and interface name
	 * if clock class is >127 (can be slave), use clock driver and interface name
	 */
	if(rtOpts->autoLockFile) {

	    memset(rtOpts->lockFile, 0, PATH_MAX);
	    snprintf(rtOpts->lockFile, PATH_MAX,
		    "%s/"PTPD_PROGNAME"_%s_%s.lock",
		    rtOpts->lockDirectory,
		    (rtOpts->clockQuality.clockClass<128 && !rtOpts->slaveOnly) ? "master" : DEFAULT_CLOCKDRIVER,
		    rtOpts->ifaceName);
	    DBG("Automatic lock file name is: %s\n", rtOpts->lockFile);
	/*
	 * Otherwise use default lock file name, with the specified lock directory
	 * which will be set do default from constants_dep.h if not configured
	 */
	} else {
		if(!CONFIG_ISSET("global:lock_file"))
			snprintf(rtOpts->lockFile, PATH_MAX,
				"%s/%s", rtOpts->lockDirectory, DEFAULT_LOCKFILE_NAME);
	}

/* ==== END additional logic */

	if(opCode & CFGOP_PARSE) {

		findUnknownSettings(opCode, dict, target);

		if (parseResult)
			INFO("Configuration OK\n");
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
loadConfigFile(dictionary **target, RunTimeOpts *rtOpts)
{

	dictionary *dict;

	if ( (dict = iniparser_load(rtOpts->configFile)) == NULL) {
		ERROR("Could not load configuration file: %s\n", rtOpts->configFile);
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
 * Create a dummy rtOpts with defaults, create a dummy dictionary,
 * Set the "secret" key in the dictionary, causing parseConfig
 * to switch from parse mode to print default mode
 */
void
printDefaultConfig()
{

	RunTimeOpts rtOpts;
	dictionary *dict;

	loadDefaultSettings(&rtOpts);
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
	parseConfig(CFGOP_PRINT_DEFAULT | CFGOP_PARSE_QUIET, NULL, dict, &rtOpts);
	dictionary_del(&dict);

	printf("\n; ========= newline required in the end ==========\n\n");

}



/**
 * Create a dummy rtOpts  with defaults, create a dummy dictionary,
 * Set the "secret" key in the dictionary, causing parseConfig
 * to switch from parse mode to help mode.
 */
void
printConfigHelp()
{

	RunTimeOpts rtOpts;
	dictionary *dict;

	loadDefaultSettings(&rtOpts);
	dict = dictionary_new(0);

	printf("\n============== Full list of "PTPD_PROGNAME" settings ========\n\n");

	/* NULL will always be returned in this mode */
	parseConfig(CFGOP_HELP_FULL | CFGOP_PARSE_QUIET, NULL, dict, &rtOpts);

	dictionary_del(&dict);

}

/**
 * Create a dummy rtOpts  with defaults, create a dummy dictionary,
 * Set the "secret" key in the dictionary, causing parseConfig
 * to switch from parse mode to help mode, for a selected key only.
 */
void
printSettingHelp(char* key)
{

	RunTimeOpts rtOpts;
	dictionary *dict;
	char* origKey = strdup(key);

	loadDefaultSettings(&rtOpts);
	dict = dictionary_new(0);


	printf("\n");
	/* NULL will always be returned in this mode */
	parseConfig(CFGOP_HELP_SINGLE | CFGOP_PARSE_QUIET, key, dict, &rtOpts);
	
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
 * instead of just setting the rtOpts field.
 */
Boolean loadCommandLineOptions(RunTimeOpts* rtOpts, dictionary* dict, int argc, char** argv, Integer16* ret) {

	int c;
#ifdef HAVE_GETOPT_LONG
	int opt_index = 0;
#endif /* HAVE_GETOPT_LONG */
	*ret = 0;

	/* there's NOTHING wrong with this */
	if(argc==1) {
			*ret = 1;
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
			strncpy(rtOpts->configFile, optarg, PATH_MAX);
			break;
		/* check configuration and exit */
		case 'k':
			rtOpts->checkConfigOnly = TRUE;
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
			dictionary_set(dict,"ptpengine:ip_mode", "hybrid");
			break;
		/* unicast */
		case 'U':
			dictionary_set(dict,"ptpengine:ip_mode", "unicast");
			break;
		case 'g':
			dictionary_set(dict,"ptpengine:unicast_negotiation", "y");
			break;
		case 'u':
			dictionary_set(dict,"ptpengine:ip_mode", "unicast");
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
                        (rtOpts->debug_level)++;
                        if(rtOpts->debug_level > LOG_DEBUGV ){
                                rtOpts->debug_level = LOG_DEBUGV;
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

			return FALSE;
		/* run in foreground */
		case 'C':
			rtOpts->nonDaemon=1;
			dictionary_set(dict,"global:foreground", "Y");
			break;
		/* verbose mode */
		case 'V':
			rtOpts->nonDaemon=1;
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
			dictionary_set(dict,"global:auto_lock", "Y");
			break;
		/* Print lock file only */
		case 'p':
			rtOpts->printLockFile = TRUE;
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
	RunTimeOpts defaultOpts;

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
			"                               apply configuration template(s) - see man(5) ptpd2.conf\n"
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
			"-y --hybrid			ptpengine:ip_mode=hybrid	Hybrid mode (multicast for sync\n"
			"								and announce, unicast for delay\n"
			"								request and response)\n"
			"-U --unicast			ptpengine:ip_mode=unicast	Unicast mode\n"
			"-g --unicast-negotiation	ptpengine:unicast_negotiation=y Enable unicast negotiation (signaling)\n"
			"-u --unicast-destinations 	ptpengine:ip_mode=unicast	Unicast destination list\n"
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
		"                  Configuration templates available (see man(5) ptpd2.conf):\n"
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
		"                 (overides clock:no_reset, but honors clock:no_adjust)\n"
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
int checkSubsystemRestart(dictionary* newConfig, dictionary* oldConfig, RunTimeOpts *rtOpts)
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
	parseConfig(CFGOP_RESTART_FLAGS | CFGOP_PARSE_QUIET, &restartFlags, tmpDict, rtOpts);

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
