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

/* -
 * Variadic macro argument count trick -> comp.std.c __VA_NARG__
 * Laurent Deniau et al, January 2006. Works from 1 argument up,
 * but in this code you will always have at least 2.
 */

#define NUMARGS(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

/*-
 * Helper macros - this is effectively the API for using the new config file interface.
 * The macros below cover all operations required in parsing the configuration.
 * This allows the parseConfig() function to be very straightforward and easy to expand
 * - it pretty much only uses macros at this stage. The use of macros reduces the
 * complexity of the parseConfig fuction. A lot of the macros are designed to work
 * within parseConfig - they assume the existence of the "dictionary*" dict variable.
 */


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


/* 
 * Macros controlling the behaviour of parseConfig - using "hidden" keys.
 * Thanks to this, a single line in parseConfig can map settings and print help.
 * the SET_QUIET / END_QUIET macros suppress info and error output of parseConfig
 */

#define SET_QUIET() \
	dictionary_set(dict,"%quiet%:%quiet%","Y");

#define END_QUIET() \
	dictionary_unset(dict,"%quiet%:%quiet%");

#define IS_QUIET()\
	( CONFIG_ISTRUE("%quiet%:%quiet%") )

#define SET_SHOWDEFAULT() \
	SET_QUIET();\
	dictionary_set(dict,"%showdefault%:%showdefault%","Y");

#define END_SHOWDEFAULT() \
	END_QUIET();\
	dictionary_unset(dict,"%showdefault%:%showdefault%");

#define IS_SHOWDEFAULT()\
	( CONFIG_ISTRUE("%showdefault%:%showdefault%") )

#define HELP_START() \
	SET_QUIET();\
	dictionary_set(dict,"%helponly%:%helponly%","T");

#define HELP_ITEM(key) \
	SET_QUIET();\
	dictionary_set(dict,"%helponly%:%helpitem%",key);

#define HELP_ITEM_COMPLETE() \
	dictionary_set(dict,"%helponly%:%helpitem%","%complete%");

#define HELP_ITEM_FOUND() \
	(strcmp(iniparser_getstring(dict, "%helponly%:%helpitem%" , ""),"%complete%") == 0)

#define HELP_END() \
	dictionary_unset(dict,"%helponly%:%helponly%");\
	dictionary_unset(dict,"%helponly%:%helpitem%");\
	END_QUIET();

#define IS_HELP(key, helptext)\
    ( ( (strcmp(iniparser_getstring(dict, "%helponly%:%helpitem%", ""),key) == 0) ||\
    CONFIG_ISTRUE("%helponly%:%helponly%")) && (strcmp(helptext, "") != 0) )

#define HELP_ON()\
    ( CONFIG_ISTRUE("%helponly%:%helponly%") || \
    CONFIG_ISSET("%helponly%:%helpitem%") )

/* Macros handling required settings, triggers, conflicts and dependencies */

#define CONFIG_KEY_REQUIRED(key) \
	if( !IS_QUIET() && !HELP_ON() && !CONFIG_ISSET(key) )\
	 { \
	    ERROR("Configuration error: option \"%s\" is required\n", key); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_DEPENDENCY(key1, key2) \
	if( CONFIG_ISSET(key1) && \
	    !CONFIG_ISSET(key2)) \
	 { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s\" requires option \"%s\"\n", key1, key2); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONFLICT(key1, key2) \
	if( CONFIG_ISSET(key1) && \
	    CONFIG_ISSET(key2)) \
	 { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s\" cannot be used with option \"%s\"\n", key1, key2); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(condition,dep,messageText) \
	if ( (condition) && \
	    !CONFIG_ISSET(dep) ) \
	 { \
	    if(!IS_QUIET())\
		WARNING("Warning: %s\n", messageText); \
	 }

#define CONFIG_KEY_CONDITIONAL_WARNING_ISSET(condition,dep,messageText) \
	if ( (condition) && \
	    CONFIG_ISSET(dep) ) \
	 { \
	    if(!IS_QUIET())\
		WARNING("Warning: %s\n", messageText); \
	 }

#define CONFIG_KEY_CONDITIONAL_DEPENDENCY(key,condition,stringval,dep) \
	if ( (condition) && \
	    !CONFIG_ISSET(dep) ) \
	 { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%s\" requires option \"%s\"\n", key, stringval, dep); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_CONDITIONAL_CONFLICT(key,condition,stringval,dep) \
	if ( (condition) && \
	    CONFIG_ISSET(dep) ) \
	 { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%s\" cannot be used with option \"%s\"\n", key, stringval, dep); \
	    parseResult = FALSE;\
	 }

#define CONFIG_KEY_VALUE_FORBIDDEN(key,condition,stringval,message) \
	if ( (condition) && \
	    CONFIG_ISSET(key) ) \
	 { \
	    if(!IS_QUIET())\
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
	    if(!IS_QUIET())\
		ERROR("%s\n", warningtext); \
	    parseResult = FALSE;\
	 }

#define CONFIG_CONDITIONAL_ASSERTION(condition,warningtext) \
	if ( (condition) ) \
	 { \
	    if(!IS_QUIET())\
		ERROR("%s\n", warningtext); \
	    parseResult = FALSE;\
	 }

/* Range check macros */

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_RANGECHECK_INT(key,min,max) \
	{\
	int tmpint = iniparser_getint(dict,key,min);\
	if (tmpint < min || tmpint > max) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%d\" not within allowed range: %d..%d\n", key, tmpint, min, max); \
	    parseResult = FALSE;\
	}\
	}

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_MIN_INT(key,min) \
	{\
	int tmpint = iniparser_getint(dict,key,min);\
	if (tmpint < min) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%d\" should be equal or greater than %d\n", key, tmpint, min); \
	    parseResult = FALSE;\
	}\
	}

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_MAX_INT(key,max) \
	{\
	int tmpint = iniparser_getint(dict,key,max);\
	if (tmpint > max) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%d\" should be equal or less than %d\n", key, tmpint, max); \
	    parseResult = FALSE;\
	}\
	}

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_RANGECHECK_DOUBLE(key,min,max) \
	{\
	double tmpdouble = iniparser_getdouble(dict,key,min);\
	if (tmpdouble < min || tmpdouble > max) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%f\" not within allowed range: %f..%f\n", key, tmpdouble, min, max); \
	    parseResult = FALSE;\
	}\
	}

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_MIN_DOUBLE(key,min) \
	{\
	double tmpdouble = iniparser_getdouble(dict,key,min);\
	if (tmpdouble < min) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%f\" should be equal or greater than %f\n", key, tmpdouble, min); \
	    parseResult = FALSE;\
	}\
	}

/* surrounded with {}s to limit the scope of the temporary variable */
#define CONFIG_KEY_MAX_DOUBLE(key,max) \
	{\
	double tmpdouble = iniparser_getdouble(dict,key,max);\
	if (tmpdouble > max) { \
	    if(!IS_QUIET())\
		ERROR("Configuration error: option \"%s=%f\" should be equal or less than %f\n", key, tmpdouble, max); \
	    parseResult = FALSE;\
	}\
	}

/* 
 * Config mapping macros - effectively functions turned into macros. Because of
 * the use of varargs and argument counting, it made sense to make at least some
 * of them macros.  To make parseConfig more consistent, all the mapping is done
 * via macros. Moving to functions would require handling the return values for
 * range checks, so again different calls for different mappings. With macros,
 * parseResult is set where needed. There may be a better way of doing this
 * (macros generate a huge amount of code), but this makes parseConfig very
 * simple
 */

#define CONFIG_MAP_STRING(key,variable,default,helptext) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: STRING\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %s\n", (strcmp(default, "") == 0) ? "[none]" : default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {\
		variable = iniparser_getstring(dict,key,default);\
		dictionary_set(target, key, variable);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
			printComment(helptext);\
			printf("%s = %s\n\n", key,variable);\
		}\
	}

/* double {}s to limit the scope of the temporary variable */
#define CONFIG_MAP_CHARARRAY(key,variable,default,helptext) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: STRING\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %s\n", (strcmp(default, "") == 0) ? "[none]" : default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		char *tmpstring = iniparser_getstring(dict,key,default); \
		if (variable!=tmpstring) strncpy(variable,tmpstring,sizeof(variable) / sizeof(char));\
		dictionary_set(target, key, tmpstring);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
			printComment(helptext);\
			printf("%s = %s\n", key,tmpstring);\
		}\
	}}

#define CONFIG_MAP_BOOLEAN(key,variable,default,helptext) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: BOOLEAN (value must start with t/T/y/Y/1/f/F/n/N/0)\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %s\n", default ? "Y" : "N");\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {\
		if (!CONFIG_ISPRESENT(key)) {\
		    variable = default;\
		    dictionary_set(target,key,(variable)?"Y":"N");\
		    if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
			    printComment(helptext);\
			    printf("%s = %s\n", key,(variable)?"Y":"N");\
		    }\
		} else if(!CONFIG_ISSET(key) || iniparser_getboolean(dict,key,-1) == -1) {\
		    ERROR("Configuration error: option \"%s='%s'\" has unknown boolean value:  must start with 0/1/t/T/f/F/y/Y/n/N\n",key,iniparser_getstring(dict,key,""));\
		    dictionary_set(target,key,""); /* suppress the "unknown entry" warning for malformed boolean values */ \
		    parseResult = FALSE;\
		} else {\
		    variable=iniparser_getboolean(dict,key,default);\
		    dictionary_set(target,key,(variable)?"Y":"N");\
		    if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
			    printComment(helptext);\
			    printf("%s = %s\n", key,(variable)?"Y":"N");\
		    }\
		}\
	}

#define CONFIG_MAP_INT(key,variable,default,helptext) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: INT\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %d\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getint(dict,key,default);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%d", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_INT_RANGE(key,variable,default,helptext,min,max) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: INT ");\
		if( min!=max )\
		    printf("(min: %d, max: %d)\n", min, max);\
		else\
		    printf("(only value of %d allowed)\n", min);\
		printf("  usage: %s\n", helptext);\
		printf("default: %d\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getint(dict,key,default);\
		CONFIG_KEY_RANGECHECK_INT(key,min,max);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%d", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_INT_MIN(key,variable,default,helptext,min) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: INT (min: %d)\n",min);\
		printf("  usage: %s\n", helptext);\
		printf("default: %d\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getint(dict,key,default);\
		CONFIG_KEY_MIN_INT(key,min);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%d", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_INT_MAX(key,variable,default,helptext,max) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: INT (max: %d)\n",max);\
		printf("  usage: %s\n", helptext);\
		printf("default: %d\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getint(dict,key,default);\
		CONFIG_KEY_MAX_INT(key,max);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%d", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_DOUBLE(key,variable,default,helptext) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: FLOAT\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %d\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getdouble(dict,key,default);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%f", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_DOUBLE_RANGE(key,variable,default,helptext,min,max) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: FLOAT ");\
		if( min!=max )\
		    printf("(min: %f, max: %f)\n", min, max);\
		else\
		    printf("(only value of %f allowed)", min);\
		printf("\n");\
		printf("  usage: %s\n", helptext);\
		printf("default: %f\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getdouble(dict,key,default);\
		CONFIG_KEY_RANGECHECK_DOUBLE(key,min,max);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%f", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_DOUBLE_MIN(key,variable,default,helptext,min) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: FLOAT(min: %f)\n", min);\
		printf("  usage: %s\n", helptext);\
		printf("default: %f\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getdouble(dict,key,default);\
		CONFIG_KEY_MIN_DOUBLE(key,min); \
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%f", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
	}}

#define CONFIG_MAP_DOUBLE_MAX(key,variable,default,helptext,max) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: FLOAT(max: %f)\n", max);\
		printf("  usage: %s\n", helptext);\
		printf("default: %f\n", default);\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {{\
		variable = iniparser_getdouble(dict,key,default);\
		CONFIG_KEY_MIN_DOUBLE(key,max);\
		char buf[50];\
		memset(buf, 0, 50);\
		snprintf(buf, 50, "%f", variable);\
		dictionary_set(target,key,buf);\
		if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
		    printComment(helptext);\
		    printf("%s = %s\n", key,buf);\
		}\
		}}

#define CONFIG_MAP_SELECTVALUE(key,variable,default,helptext, ... ) \
	if(IS_HELP(key, helptext)) {\
		printf("setting: %s (--%s)\n", key, key);\
		printf("   type: SELECT\n");\
		printf("  usage: %s\n", helptext);\
		printf("options: ");\
		printKeyOptions (NUMARGS(__VA_ARGS__), __VA_ARGS__);\
		printf("\n");\
		printf("default: %s\n",\
			selectOptionName(default,NUMARGS(__VA_ARGS__),\
			    __VA_ARGS__));\
		printf("\n");\
		HELP_ITEM_COMPLETE(); \
	} else {\
		int res;\
		if ( (res = selectKeyValue(\
			dict, key, default,NUMARGS(__VA_ARGS__),\
			    __VA_ARGS__\
			 )) == -1) {\
				    variable = default;\
				    parseResult = FALSE;\
					} else {\
						variable=res;\
					}\
			    {\
				dictionary_set(target, key, selectOptionName(\
				    variable,NUMARGS(__VA_ARGS__), __VA_ARGS__));\
				if(!STRING_EMPTY(helptext) && IS_SHOWDEFAULT()) {\
				    printComment(helptext);\
				    printf("; Options: ");\
				    printKeyOptions (NUMARGS(__VA_ARGS__), __VA_ARGS__);\
				    printf("\n%s = %s\n", key,selectOptionName(\
				    variable,NUMARGS(__VA_ARGS__), __VA_ARGS__));\
				}\
			    }\
		}

/* Macro printing a warning for a deprecated command-line option */
#define WARN_DEPRECATED_COMMENT( old,new,long,key,comment )\
printf("Note: The use of '-%c' option is deprecated %s- consider using '-%c' (--%s) or the %s setting\n",\
	old, comment, new, long, key);

#define WARN_DEPRECATED(old,new,long,key)\
	WARN_DEPRECATED_COMMENT( old,new,long,key,"")

#define SETTING_CHANGED(key) \
	(strcmp(\
		dictionary_get(newConfig, key,""),\
		dictionary_get(oldConfig, key,"")\
	    ) != 0 )
/* Macro flagging component restart if the given option has changed */
#define COMPONENT_RESTART_REQUIRED(key,flag)\
	if(SETTING_CHANGED(key)) {\
		if(flag == PTPD_RESTART_DAEMON) {\
			DBG("Setting %s changed, restart of ptpd process required\n",key,flag);\
			NOTIFY("Change of %s setting requires "PTPD_PROGNAME" restart\n",key);\
		} else\
			DBG("Setting %s changed, restart of subystem %d required\n",key,flag);\
		restartFlags |= flag;\
	}

#define CONFIG_KEY_ALIAS(src,dest) \
	{ \
		if(!STRING_EMPTY(dictionary_get(dict,src,""))) {\
		    dictionary_set(dict,dest,dictionary_get(dict,src,""));\
		    dictionary_unset(dict,src);\
		}\
	}

/* concatenate every second vararg argument for string, int pairs and print it */
static void
printKeyOptions( int count, ... )
{


    char *optionvalue;
    va_list va;
    int i;

    /* if we got an incomplete argument list, cut it down to the last pair or nothing */
    if(count % 2 == 1) {

	count--;

    }

    count/=2;

    va_start(va, count);

    for(i = 0; i < count; i++) {

	optionvalue = (char *)va_arg(va, char *);
	(void)va_arg(va, int);

	printf("%s ", optionvalue);

    }

    va_end(va);

}

/* Return the number corresponding to a string from a number of string, int pairs */
static int
selectKeyValue( dictionary* dict, const char* keyname, int default_option, int count, ... )
{

    char sbuf[SCREEN_BUFSZ];
    int len = 0;

    char *keyvalue, *optionvalue;
    va_list va;
    int ret, i, optionnumber;

    /* if we got an incomplete argument list, cut it down to the last pair or nothing */
    if(count % 2 == 1) {

	count--;

    }

    count/=2;

    if ( !CONFIG_ISPRESENT(keyname) ) {
	ret = default_option;
	goto end;
    }

    memset(sbuf, 0, sizeof(sbuf));

    keyvalue = iniparser_getstring(dict, keyname, "");

    va_start(va, count);

    for(i = 0; i < count; i++) {

	optionvalue = (char *)va_arg(va, char *);
	optionnumber = (int)va_arg(va, int);

	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s ", optionvalue);

	if ( strcmp(keyvalue, optionvalue) == 0) {

	    ret = optionnumber;
	    goto end; /* must call va_end */

	}

    }

    ERROR("Configuration error: option \"%s\" has unknown value: %s - allowed values: %s\n", keyname, keyvalue,sbuf);

    ret = -1;

end:

    va_end(va);
    return ret;

}

/* Return the string value for the given number value of string, int pairs */
static char*
selectOptionName( int value, int count, ... )
{
    char *ret, *optionvalue;
    int i, optionnumber;
    va_list va;

    /* if we got an incomplete argument list, cut it down to the last pair or nothing */
    if(count % 2 == 1) {

	count--;

    }

    ret = "[none]";

    count/=2;

    va_start(va, count);

    for(i = 0; i < count; i++) {

	optionvalue = (char *)va_arg(va, char *);
	optionnumber = (int)va_arg(va, int);

	if ( optionnumber == value ) {

	    ret = optionvalue;
	    goto end; /* must call va_end */

	}

    }

end:

    va_end(va);
    return ret;

}

/* Output a potentially multi-line string, prefixed with ;s */
static void printComment(char* helptext)
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

/* Load all rtOpts defaults */
void
loadDefaultSettings( RunTimeOpts* rtOpts )
{

	/* Wipe the memory first to avoid unconsistent behaviour - no need to set Boolean to FALSE, int to 0 etc. */
	memset(rtOpts, 0, sizeof(RunTimeOpts));

	rtOpts->logAnnounceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts->logSyncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	rtOpts->clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts->clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts->clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts->priority1 = DEFAULT_PRIORITY1;
	rtOpts->priority2 = DEFAULT_PRIORITY2;
	rtOpts->domainNumber = DEFAULT_DOMAIN_NUMBER;
	rtOpts->portNumber = NUMBER_PORTS;

	rtOpts->anyDomain = FALSE;

	rtOpts->transport = UDP_IPV4;

	/* timePropertiesDS */
	rtOpts->timeProperties.currentUtcOffsetValid = DEFAULT_UTC_VALID;
	rtOpts->timeProperties.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts->timeProperties.timeSource = INTERNAL_OSCILLATOR;
	rtOpts->timeProperties.timeTraceable = FALSE;
	rtOpts->timeProperties.frequencyTraceable = FALSE;
	rtOpts->timeProperties.ptpTimescale = TRUE;

	rtOpts->ipMode = IPMODE_MULTICAST;
	rtOpts->dot2AS = FALSE;

	rtOpts->unicastNegotiation = FALSE;
	rtOpts->unicastNegotiationListening = FALSE;
	rtOpts->disableBMCA = FALSE;
	rtOpts->unicastGrantDuration = 300;

	rtOpts->noAdjust = NO_ADJUST;  // false
	rtOpts->logStatistics = TRUE;
	rtOpts->statisticsTimestamp = TIMESTAMP_DATETIME;

	rtOpts->periodicUpdates = FALSE; /* periodically log a status update */

	/* Deep display of all packets seen by the daemon */
	rtOpts->displayPackets = FALSE;

	rtOpts->s = DEFAULT_DELAY_S;
	rtOpts->inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts->outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts->max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	rtOpts->nonDaemon = FALSE;

	/*
	 * defaults for new options
	 */
	rtOpts->ignore_delayreq_interval_master = FALSE;
	rtOpts->do_IGMP_refresh = TRUE;
	rtOpts->useSysLog       = FALSE;
	rtOpts->announceReceiptTimeout  = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
#ifdef RUNTIME_DEBUG
	rtOpts->debug_level = LOG_INFO;			/* by default debug messages as disabled, but INFO messages and below are printed */
#endif
	rtOpts->ttl = 64;
	rtOpts->delayMechanism   = DEFAULT_DELAY_MECHANISM;
	rtOpts->noResetClock     = DEFAULT_NO_RESET_CLOCK;
	rtOpts->stepOnce	 = FALSE;
	rtOpts->stepForce	 = FALSE;
#ifdef HAVE_LINUX_RTC_H
	rtOpts->setRtc		 = FALSE;
#endif /* HAVE_LINUX_RTC_H */

	rtOpts->clearCounters = FALSE;
	rtOpts->statisticsLogInterval = 0;

	rtOpts->initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts->logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts->autoDelayReqInterval = TRUE;
	rtOpts->masterRefreshInterval = 60;

	/* maximum values for unicast negotiation */	
    	rtOpts->logMaxPdelayReqInterval = 5;
	rtOpts->logMaxDelayReqInterval = 5;
	rtOpts->logMaxSyncInterval = 5;
	rtOpts->logMaxAnnounceInterval = 5;



	rtOpts->drift_recovery_method = DRIFT_KERNEL;
	strncpy(rtOpts->lockDirectory, DEFAULT_LOCKDIR, PATH_MAX);
	strncpy(rtOpts->driftFile, DEFAULT_DRIFTFILE, PATH_MAX);
/*	strncpy(rtOpts->lockFile, DEFAULT_LOCKFILE, PATH_MAX); */
	rtOpts->autoLockFile = FALSE;
	rtOpts->snmp_enabled = FALSE;
	/* This will only be used if the "none" preset is configured */
#ifndef PTPD_SLAVE_ONLY
	rtOpts->slaveOnly = FALSE;
#else
	rtOpts->slaveOnly = TRUE;
#endif /* PTPD_SLAVE_ONLY */
	/* Otherwise default to slave only via the preset */
	rtOpts->selectedPreset = PTP_PRESET_SLAVEONLY;
	rtOpts->pidAsClockId = FALSE;

	/* highest possible */
	rtOpts->logLevel = LOG_ALL;

	/* ADJ_FREQ_MAX by default */
	rtOpts->servoMaxPpb = ADJ_FREQ_MAX / 1000;
	/* kP and kI are scaled to 10000 and are gains now - values same as originally */
	rtOpts->servoKP = 0.1;
	rtOpts->servoKI = 0.001;

	rtOpts->servoDtMethod = DT_CONSTANT;
	/* when measuring dT, use a maximum of 5 sync intervals (would correspond to avg 20% discard rate) */
	rtOpts->servoMaxdT = 5.0;

	/* disabled by default */
	rtOpts->announceTimeoutGracePeriod = 0;
	rtOpts->alwaysRespectUtcOffset=FALSE;
	rtOpts->preferUtcValid=FALSE;
	rtOpts->requireUtcValid=FALSE;

	/* Try 46 for expedited forwarding */
	rtOpts->dscpValue = 0;

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H)
	rtOpts-> cpuNumber = -1;
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H*/

#ifdef PTPD_STATISTICS

	rtOpts->oFilterMSConfig.enabled = FALSE;
	rtOpts->oFilterMSConfig.discard = TRUE;
	rtOpts->oFilterMSConfig.autoTune = TRUE;
	rtOpts->oFilterMSConfig.stepDelay = FALSE;
	rtOpts->oFilterMSConfig.alwaysFilter = FALSE;
	rtOpts->oFilterMSConfig.stepThreshold = 1000000;
	rtOpts->oFilterMSConfig.stepLevel = 500000;
	rtOpts->oFilterMSConfig.capacity = 20;
	rtOpts->oFilterMSConfig.threshold = 1.0;
	rtOpts->oFilterMSConfig.weight = 1;
	rtOpts->oFilterMSConfig.minPercent = 20;
	rtOpts->oFilterMSConfig.maxPercent = 95;
	rtOpts->oFilterMSConfig.thresholdStep = 0.1;
	rtOpts->oFilterMSConfig.minThreshold = 0.1;
	rtOpts->oFilterMSConfig.maxThreshold = 5.0;
	rtOpts->oFilterMSConfig.delayCredit = 200;
	rtOpts->oFilterMSConfig.creditIncrement = 10;
	rtOpts->oFilterMSConfig.maxDelay = 1500;

	rtOpts->oFilterSMConfig.enabled = FALSE;
	rtOpts->oFilterSMConfig.discard = TRUE;
	rtOpts->oFilterSMConfig.autoTune = TRUE;
	rtOpts->oFilterSMConfig.stepDelay = FALSE;
	rtOpts->oFilterSMConfig.alwaysFilter = FALSE;
	rtOpts->oFilterSMConfig.stepThreshold = 1000000;
	rtOpts->oFilterSMConfig.stepLevel = 500000;
	rtOpts->oFilterSMConfig.capacity = 20;
	rtOpts->oFilterSMConfig.threshold = 1.0;
	rtOpts->oFilterSMConfig.weight = 1;
	rtOpts->oFilterSMConfig.minPercent = 20;
	rtOpts->oFilterSMConfig.maxPercent = 95;
	rtOpts->oFilterSMConfig.thresholdStep = 0.1;
	rtOpts->oFilterSMConfig.minThreshold = 0.1;
	rtOpts->oFilterSMConfig.maxThreshold = 5.0;
	rtOpts->oFilterSMConfig.delayCredit = 200;
	rtOpts->oFilterSMConfig.creditIncrement = 10;
	rtOpts->oFilterSMConfig.maxDelay = 1500;

	rtOpts->filterMSOpts.enabled = FALSE;
	rtOpts->filterMSOpts.filterType = FILTER_MIN;
	rtOpts->filterMSOpts.windowSize = 4;
	rtOpts->filterMSOpts.windowType = WINDOW_SLIDING;

	rtOpts->filterSMOpts.enabled = FALSE;
	rtOpts->filterSMOpts.filterType = FILTER_MIN;
	rtOpts->filterSMOpts.windowSize = 4;
	rtOpts->filterSMOpts.windowType = WINDOW_SLIDING;

	/* How often refresh statistics (seconds) */
	rtOpts->statsUpdateInterval = 30;
	/* Servo stability detection settings follow */
	rtOpts->servoStabilityDetection = FALSE;
	/* Stability threshold (ppb) - observed drift std dev value considered stable */
	rtOpts->servoStabilityThreshold = 10;
	/* How many consecutive statsUpdateInterval periods of observed drift std dev within threshold  means stable servo */
	rtOpts->servoStabilityPeriod = 1;
	/* How many minutes without servo stabilisation means servo has not stabilised */
	rtOpts->servoStabilityTimeout = 10;
	/* How long to wait for one-way delay prefiltering */
	rtOpts->calibrationDelay = 0;
	/* if set to TRUE and maxDelay is defined, only check against threshold if servo is stable */
	rtOpts->maxDelayStableOnly = FALSE;
	/* if set to non-zero, reset slave if more than this amount of consecutive delay measurements was above maxDelay */
	rtOpts->maxDelayMaxRejected = 0;
#endif

	/* status file options */
	rtOpts->statusFileUpdateInterval = 1;

	/* panic mode options */
	rtOpts->enablePanicMode = FALSE;
	rtOpts->panicModeDuration = 2;
	rtOpts->panicModeExitThreshold = 0;

	/* full network reset after 5 times in listening */
	rtOpts->maxListen = 5;

	rtOpts->panicModeReleaseClock = FALSE;
	rtOpts->ntpOptions.enableEngine = FALSE;
	rtOpts->ntpOptions.enableControl = FALSE;
	rtOpts->ntpOptions.enableFailover = FALSE;
	rtOpts->ntpOptions.failoverTimeout = 120;
	rtOpts->ntpOptions.checkInterval = 15;
	rtOpts->ntpOptions.keyId = 0;
	strncpy(rtOpts->ntpOptions.hostAddress,"localhost",MAXHOSTNAMELEN); 	/* not configurable, but could be */
	rtOpts->preferNTP = FALSE;

	rtOpts->leapSecondPausePeriod = 5;
	/* by default, announce the leap second 12 hours before the event:
	 * Clause 9.4 paragraph 5 */
	rtOpts->leapSecondNoticePeriod = 43200;
	rtOpts->leapSecondHandling = LEAP_ACCEPT;
	rtOpts->leapSecondSmearPeriod = 86400;

/* timing domain */
	rtOpts->idleTimeout = 120; /* idle timeout */
	rtOpts->electionDelay = 15; /* anti-flapping delay */

/* Log file settings */

	rtOpts->statisticsLog.logID = "statistics";
	rtOpts->statisticsLog.openMode = "a+";
	rtOpts->statisticsLog.logFP = NULL;
	rtOpts->statisticsLog.truncateOnReopen = FALSE;
	rtOpts->statisticsLog.unlinkOnClose = FALSE;
	rtOpts->statisticsLog.maxSize = 0;

	rtOpts->recordLog.logID = "record";
	rtOpts->recordLog.openMode = "a+";
	rtOpts->recordLog.logFP = NULL;
	rtOpts->recordLog.truncateOnReopen = FALSE;
	rtOpts->recordLog.unlinkOnClose = FALSE;
	rtOpts->recordLog.maxSize = 0;

	rtOpts->eventLog.logID = "log";
	rtOpts->eventLog.openMode = "a+";
	rtOpts->eventLog.logFP = NULL;
	rtOpts->eventLog.truncateOnReopen = FALSE;
	rtOpts->eventLog.unlinkOnClose = FALSE;
	rtOpts->eventLog.maxSize = 0;

	rtOpts->statusLog.logID = "status";
	rtOpts->statusLog.openMode = "w";
	strncpy(rtOpts->statusLog.logPath, DEFAULT_STATUSFILE, PATH_MAX);
	rtOpts->statusLog.logFP = NULL;
	rtOpts->statusLog.truncateOnReopen = FALSE;
	rtOpts->statusLog.unlinkOnClose = TRUE;

/* Management message support settings */
	rtOpts->managementEnabled = TRUE;
	rtOpts->managementSetEnable = FALSE;

/* IP ACL settings */

	rtOpts->timingAclEnabled = FALSE;
	rtOpts->managementAclEnabled = FALSE;
	rtOpts->timingAclOrder = ACL_DENY_PERMIT;
	rtOpts->managementAclOrder = ACL_DENY_PERMIT;

	// by default we don't check Sync message sequence continuity
	rtOpts->syncSequenceChecking = FALSE;
	rtOpts->clockUpdateTimeout = 0;

}

/* The PtpEnginePreset structure for reference: 

typedef struct {

    char* presetName;
    Boolean slaveOnly;
    Boolean noAdjust;
    UInteger8_option clockClass;

} PtpEnginePreset;
*/

PtpEnginePreset
getPtpPreset(int presetNumber, RunTimeOpts* rtOpts)
{

	PtpEnginePreset ret;

	memset(&ret,0,sizeof(ret));

	switch(presetNumber) {

	case PTP_PRESET_SLAVEONLY:
		ret.presetName="slaveonly";
		ret.slaveOnly = TRUE;
		ret.noAdjust = FALSE;
		ret.clockClass.minValue = SLAVE_ONLY_CLOCK_CLASS;
		ret.clockClass.maxValue = SLAVE_ONLY_CLOCK_CLASS;
		ret.clockClass.defaultValue = SLAVE_ONLY_CLOCK_CLASS;
		break;
	case PTP_PRESET_MASTERSLAVE:
		ret.presetName = "masterslave";
		ret.slaveOnly = FALSE;
		ret.noAdjust = FALSE;
		ret.clockClass.minValue = 128;
		ret.clockClass.maxValue = 254;
		ret.clockClass.defaultValue = DEFAULT_CLOCK_CLASS;
		break;
	case PTP_PRESET_MASTERONLY:
		ret.presetName = "masteronly";
		ret.slaveOnly = FALSE;
		ret.noAdjust = TRUE;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 127;
		ret.clockClass.defaultValue = DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE;
		break;
	default:
		ret.presetName = "none";
		ret.slaveOnly = rtOpts->slaveOnly;
		ret.noAdjust = rtOpts->noAdjust;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 255;
		ret.clockClass.defaultValue = rtOpts->clockQuality.clockClass;
	}

	return ret;
}

/**
 * Iterate through dictionary dict (template) and check for keys in source
 * that are not present in the template - display only.
 */
static void
findUnknownSettings(dictionary* source, dictionary* dict)
{

    int i = 0;

    if( source == NULL || dict == NULL) return;

    for(i = 0; i < source->n; i++) {
	/* skip if the key is null or is a section */
        if(source->key[i] == NULL || strstr(source->key[i],":") == NULL)
            continue;
		    if ( !iniparser_find_entry(dict, source->key[i]) && !IS_QUIET() )
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
parseConfig ( dictionary* dict, RunTimeOpts *rtOpts )
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

	if(!IS_QUIET()) {
		INFO("Checking configuration\n");
	}

/* ============= BEGIN CONFIG MAPPINGS, TRIGGERS AND DEPENDENCIES =========== */

/* ===== ptpengine section ===== */

	CONFIG_KEY_REQUIRED("ptpengine:interface");

	CONFIG_MAP_CHARARRAY("ptpengine:interface",rtOpts->primaryIfaceName,rtOpts->primaryIfaceName,
	"Network interface to use - eth0, igb0 etc. (required).");

	CONFIG_MAP_CHARARRAY("ptpengine:backup_interface",rtOpts->backupIfaceName,rtOpts->backupIfaceName,
		"Backup network interface to use - eth0, igb0 etc. When no GM available, \n"
	"	 slave will keep alternating between primary and secondary until a GM is found.\n");

	CONFIG_KEY_TRIGGER("ptpengine:backup_interface",rtOpts->backupIfaceEnabled,TRUE,FALSE);

	/* Preset option names have to be mapped to defined presets - no free strings here */
	CONFIG_MAP_SELECTVALUE("ptpengine:preset",rtOpts->selectedPreset,rtOpts->selectedPreset,
		"PTP engine preset:\n"
	"	 none	     = Defaults, no clock class restrictions\n"
	"        masteronly  = Master, passive when not best master (clock class 0..127)\n"
	"        masterslave = Full IEEE 1588 implementation:\n"
	"                      Master, slave when not best master\n"
	"	 	       (clock class 128..254)\n"
	"        slaveonly   = Slave only (clock class 255 only)\n",
#ifndef PTPD_SLAVE_ONLY
				(getPtpPreset(PTP_PRESET_NONE,rtOpts)).presetName,		PTP_PRESET_NONE,
				(getPtpPreset(PTP_PRESET_MASTERONLY,rtOpts)).presetName,	PTP_PRESET_MASTERONLY,
				(getPtpPreset(PTP_PRESET_MASTERSLAVE,rtOpts)).presetName,	PTP_PRESET_MASTERSLAVE,
#endif /* PTPD_SLAVE_ONLY */
				(getPtpPreset(PTP_PRESET_SLAVEONLY,rtOpts)).presetName,		PTP_PRESET_SLAVEONLY
				);

	CONFIG_KEY_CONDITIONAL_CONFLICT("ptpengine:preset",
	 			    rtOpts->selectedPreset == PTP_PRESET_MASTERSLAVE,
	 			    "masterslave",
	 			    "ptpengine:unicast_negotiation");

	ptpPreset = getPtpPreset(rtOpts->selectedPreset, rtOpts);


	CONFIG_MAP_SELECTVALUE("ptpengine:transport",rtOpts->transport,rtOpts->transport,
		"Transport type for PTP packets. Ethernet transport requires libpcap support.",
				"ipv4",		UDP_IPV4,
#if 0
				"ipv6",		UDP_IPV6,
#endif
				"ethernet", 	IEEE_802_3
				);

	CONFIG_MAP_BOOLEAN("ptpengine:dot2as",rtOpts->dot2AS,rtOpts->dot2AS,
		"Enable TransportSpecific field compatibility with 802.1AS / AVB (requires Ethernet transport)");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->transport != IEEE_802_3) && rtOpts->dot2AS,
	 			    "ptpengine:dot2as",
				"802.1AS compatibility can only be used with the Ethernet transport\n");

	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport != IEEE_802_3,rtOpts->dot2AS,FALSE,rtOpts->dot2AS);

	CONFIG_MAP_SELECTVALUE("ptpengine:ip_mode", rtOpts->ipMode, rtOpts->ipMode,
		"IP transmission mode (requires IP transport) - hybrid mode uses\n"
	"	 multicast for sync and announce, and unicast for delay request and\n"
	"	 response; unicast mode uses unicast for all transmission.\n"
	"	 When unicast mode is selected, destination IP(s) may need to be configured\n"
	"	(ptpengine:unicast_destinations).",
				"multicast", 	IPMODE_MULTICAST,
				"unicast", 	IPMODE_UNICAST,
				"hybrid", 	IPMODE_HYBRID
				);

	CONFIG_MAP_BOOLEAN("ptpengine:unicast_negotiation",rtOpts->unicastNegotiation,rtOpts->unicastNegotiation,
		"Enable unicast negotiation support using signaling messages\n");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->transport == IEEE_802_3) && rtOpts->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation", 
				"Unicast negotiation cannot be used with Ethernet transport\n");

	CONFIG_KEY_CONDITIONAL_WARNING_ISSET((rtOpts->ipMode != IPMODE_UNICAST) && rtOpts->unicastNegotiation,
	 			    "ptpengine:unicast_negotiation", 
				"Unicast negotiation can only be used with unicast transmission\n");

	/* disable unicast negotiation unless running unicast */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3,rtOpts->unicastNegotiation,FALSE,rtOpts->unicastNegotiation);
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->ipMode != IPMODE_UNICAST,rtOpts->unicastNegotiation,FALSE,rtOpts->unicastNegotiation);

	CONFIG_MAP_BOOLEAN("ptpengine:disable_bmca",rtOpts->disableBMCA,rtOpts->disableBMCA,
		"Disable Best Master Clock Algorithm for unicast masters:\n"
	"        Only effective for masteronly preset - all Announce messages\n"
	"        will be ignored and clock will transition directly into MASTER state.\n");

	CONFIG_MAP_BOOLEAN("ptpengine:unicast_negotiation_listening",rtOpts->unicastNegotiationListening,rtOpts->unicastNegotiationListening,
		"When unicast negotiation enabled on a master clock, \n"
	"	 reply to transmission requests also in LISTENING state.");

#if defined(PTPD_PCAP) && defined(__sun) && !defined(PTPD_EXPERIMENTAL)
	if(CONFIG_ISTRUE("ptpengine:use_libpcap"))
	INFO("Libpcap support is currently marked broken/experimental on Solaris platforms.\n"
	     "To test it, please build with --enable-experimental-options\n");

	CONFIG_MAP_BOOLEAN("ptpengine:use_libpcap",rtOpts->pcap,FALSE,
		"Use libpcap for sending and receiving traffic (automatically enabled\n"
	"	 in Ethernet mode).");

	/* cannot set ethernet transport without libpcap */
	CONFIG_KEY_VALUE_FORBIDDEN("ptpengine:transport",
				    rtOpts->transport == IEEE_802_3,
				    "ethernet",
	    "Libpcap support is currently marked broken/experimental on Solaris platforms.\n"
	    "To test it and use the Ethernet transport, please build with --enable-experimental-options\n");
#elif defined(PTPD_PCAP)
	CONFIG_MAP_BOOLEAN("ptpengine:use_libpcap",rtOpts->pcap,rtOpts->pcap,
		"Use libpcap for sending and receiving traffic (automatically enabled\n"
	"	 in Ethernet mode).");

	/* in ethernet mode, activate pcap and overwrite previous setting */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport==IEEE_802_3,rtOpts->pcap,TRUE,rtOpts->pcap);
#else
	if(CONFIG_ISTRUE("ptpengine:use_libpcap"))
	INFO("Libpcap support disabled or not available. Please install libpcap,\n"
	     "build without --disable-pcap, or try building with ---with-pcap-config\n"
	     " to use ptpengine:use_libpcap.\n");

	CONFIG_MAP_BOOLEAN("ptpengine:use_libpcap",rtOpts->pcap,FALSE,
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

	CONFIG_MAP_SELECTVALUE("ptpengine:delay_mechanism",rtOpts->delayMechanism,rtOpts->delayMechanism,
		 "Delay detection mode used - use DELAY_DISABLED for syntonisation only\n"
	"	 (no full synchronisation).",
				"E2E",		E2E,	
				"P2P",		P2P,
				"DELAY_DISABLED", DELAY_DISABLED
				);

	CONFIG_MAP_INT_RANGE("ptpengine:domain",rtOpts->domainNumber,rtOpts->domainNumber,
		"PTP domain number.",0,127);

	CONFIG_MAP_INT_RANGE("ptpengine:port_number",rtOpts->portNumber,rtOpts->portNumber,
			    "PTP port number (part of PTP Port Identity - not UDP port).\n"
		    "        For ordinary clocks (single port), the default should be used, \n"
		    "        but when running multiple instances to simulate a boundary clock, \n"
		    "        The port number can be changed.",1,65534);

	CONFIG_MAP_BOOLEAN("ptpengine:any_domain",rtOpts->anyDomain, rtOpts->anyDomain,
		"Usability extension: if enabled, a slave-only clock will accept\n"
	"	 masters from any domain, while preferring the configured domain,\n"
	"	 and preferring lower domain number.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	CONFIG_MAP_BOOLEAN("ptpengine:slave_only",rtOpts->slaveOnly, ptpPreset.slaveOnly,
		 "Slave only mode (sets clock class to 255, overriding value from preset).");

	CONFIG_MAP_INT( "ptpengine:inbound_latency",rtOpts->inboundLatency.nanoseconds,rtOpts->inboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for incoming packets.");

	CONFIG_MAP_INT( "ptpengine:outbound_latency",rtOpts->outboundLatency.nanoseconds,rtOpts->outboundLatency.nanoseconds,
	"Specify latency correction (nanoseconds) for outgoing packets.");

	CONFIG_MAP_INT( "ptpengine:offset_shift",rtOpts->ofmShift.nanoseconds,rtOpts->ofmShift.nanoseconds,
	"Apply an arbitrary shift (nanoseconds) to offset from master when\n"
	"	 in slave state. Value can be positive or negative - useful for\n"
	"	 correcting for antenna latencies, delay assymetry\n"
	"	 and IP stack latencies. This will not be visible in the offset \n"
	"	 from master value - only in the resulting clock correction.");

	CONFIG_MAP_BOOLEAN("ptpengine:always_respect_utc_offset",rtOpts->alwaysRespectUtcOffset, rtOpts->alwaysRespectUtcOffset,
		"Compatibility option: In slave state, always respect UTC offset\n"
	"	 announced by best master, even if the the\n"
	"	 currrentUtcOffsetValid flag is announced FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	CONFIG_MAP_BOOLEAN("ptpengine:prefer_utc_offset_valid",rtOpts->preferUtcValid, rtOpts->preferUtcValid,
		"Compatibility extension to BMC algorithm: when enabled,\n"
	"	 BMC for both master and save clocks will prefer masters\n"
	"	 nannouncing currrentUtcOffsetValid as TRUE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	CONFIG_MAP_BOOLEAN("ptpengine:require_utc_offset_valid",rtOpts->requireUtcValid, rtOpts->requireUtcValid,
		"Compatibility option: when enabled, ptpd2 will ignore\n"
	"	 Announce messages from masters announcing currentUtcOffsetValid\n"
	"	 as FALSE.\n"
	"	 NOTE: this behaviour is not part of the standard.");

	/* from 30 seconds to 7 days */
	CONFIG_MAP_INT_RANGE("ptpengine:unicast_grant_duration", rtOpts->unicastGrantDuration, rtOpts->unicastGrantDuration,
		"Time (seconds) unicast messages are requested for by slaves\n"
	"	 when using unicast negotiation, and maximum time unicast message\n"
	"	 transmission is granted to slaves by masters\n", 30, 604800);

	CONFIG_MAP_INT_RANGE("ptpengine:log_announce_interval",rtOpts->logAnnounceInterval,rtOpts->logAnnounceInterval,
		"PTP announce message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-4,7);
#endif

	CONFIG_MAP_INT_RANGE("ptpengine:log_announce_interval_max",rtOpts->logMaxAnnounceInterval,rtOpts->logMaxAnnounceInterval,
		"Maximum Announce message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logAnnounceInterval >= rtOpts->logMaxAnnounceInterval,
					"ptpengine:log_announce_interval value must be lower than ptpengine:log_announce_interval_max\n");

	CONFIG_MAP_INT_RANGE("ptpengine:announce_receipt_timeout",rtOpts->announceReceiptTimeout,rtOpts->announceReceiptTimeout,
		"PTP announce receipt timeout announced in master state.",2,255);

	CONFIG_MAP_INT_RANGE("ptpengine:announce_receipt_grace_period",rtOpts->announceTimeoutGracePeriod,rtOpts->announceTimeoutGracePeriod,
		"PTP announce receipt timeout grace period in slave state:\n"
	"	 when announce receipt timeout occurs, disqualify current best GM,\n"
	"	 then wait n times announce receipt timeout before resetting.\n"
	"	 Allows for a seamless GM failover when standby GMs are slow\n"
	"	 to react. When set to 0, this option is not used.",
	0,20);

	CONFIG_MAP_INT_RANGE("ptpengine:log_sync_interval",rtOpts->logSyncInterval,rtOpts->logSyncInterval,
		"PTP sync message interval in master state. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-7,7);
#endif

	CONFIG_MAP_INT_RANGE("ptpengine:log_sync_interval_max",rtOpts->logMaxSyncInterval,rtOpts->logMaxSyncInterval,
		"Maximum Sync message interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logSyncInterval >= rtOpts->logMaxSyncInterval,
					"ptpengine:log_sync_interval value must be lower than ptpengine:log_sync_interval_max\n");

	CONFIG_MAP_BOOLEAN("ptpengine:log_delayreq_override", rtOpts->ignore_delayreq_interval_master,
	rtOpts->ignore_delayreq_interval_master,
		 "Override the Delay Request interval announced by best master.");

	CONFIG_MAP_BOOLEAN("ptpengine:log_delayreq_auto", rtOpts->autoDelayReqInterval,
	rtOpts->autoDelayReqInterval,
		 "Automatically override the Delay Request interval\n"
	"         if the announced value is 127 (0X7F), such as in\n"
	"         unicast messages (unless using unicast negotiation)");

	CONFIG_MAP_INT_RANGE("ptpengine:log_delayreq_interval_initial",rtOpts->initial_delayreq,rtOpts->initial_delayreq,
		"Delay request interval used before receiving first delay response\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-7,7);
#endif

	/* take the delayreq_interval from config, otherwise use the initial setting as default */
	CONFIG_MAP_INT_RANGE("ptpengine:log_delayreq_interval",rtOpts->logMinDelayReqInterval,rtOpts->initial_delayreq,
		"Minimum delay request interval announced when in master state,\n"
	"	 in slave state overrides the master interval,\n"
	"	 required in hybrid mode. When using unicast negotiation, for\n"
	"	 slaves this is the minimum interval requested, and for masters\n"
	"	 this is the minimum interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-7,7);
#endif

	CONFIG_MAP_INT_RANGE("ptpengine:log_delayreq_interval_max",rtOpts->logMaxDelayReqInterval,rtOpts->logMaxDelayReqInterval,
		"Maximum Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logMinDelayReqInterval >= rtOpts->logMaxDelayReqInterval,
					"ptpengine:log_delayreq_interval value must be lower than ptpengine:log_delayreq_interval_max\n");

	CONFIG_MAP_INT_RANGE("ptpengine:log_peer_delayreq_interval",rtOpts->logMinPdelayReqInterval,rtOpts->logMinPdelayReqInterval,
		"Minimum peer delay request message interval in peer to peer delay mode.\n"
	"        When using unicast negotiation, this is the minimum interval requested, \n"
	"	 and the only interval granted.\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-7,7);
#endif

	CONFIG_MAP_INT_RANGE("ptpengine:log_peer_delayreq_interval_max",rtOpts->logMaxPdelayReqInterval,rtOpts->logMaxPdelayReqInterval,
		"Maximum Peer Delay Response interval requested by slaves "
		"when using unicast negotiation,\n"
#ifdef PTPD_EXPERIMENTAL
    "	"LOG2_HELP,-30,30);
#else
    "	"LOG2_HELP,-1,7);
#endif

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->logMinPdelayReqInterval >= rtOpts->logMaxPdelayReqInterval,
					"ptpengine:log_peer_delayreq_interval value must be lower than ptpengine:log_peer_delayreq_interval_max\n");

	CONFIG_MAP_INT_RANGE("ptpengine:foreignrecord_capacity",rtOpts->max_foreign_records,rtOpts->max_foreign_records,
	"Foreign master record size (Maximum number of foreign masters).",5,10);

	CONFIG_MAP_INT_RANGE("ptpengine:ptp_allan_variance",rtOpts->clockQuality.offsetScaledLogVariance,rtOpts->clockQuality.offsetScaledLogVariance,
	"Specify Allan variance announced in master state.",0,65535);

	CONFIG_MAP_SELECTVALUE("ptpengine:ptp_clock_accuracy",rtOpts->clockQuality.clockAccuracy,rtOpts->clockQuality.clockAccuracy,
	"Clock accuracy range announced in master state.",
				"ACC_25NS",		0x20,
				"ACC_100NS",		0x21,
				"ACC_250NS",		0x22,
				"ACC_1US",		0x23,
				"ACC_2.5US", 		0x24,
				"ACC_10US", 		0x25,
				"ACC_25US", 		0x26,
				"ACC_100US", 		0x27,
				"ACC_250US", 		0x28,
				"ACC_1MS", 		0x29,
				"ACC_2.5MS", 		0x2A,
				"ACC_10MS", 		0x2B,
				"ACC_25MS",		0X2C,
				"ACC_100MS", 		0x2D,
				"ACC_250MS", 		0x2E,
				"ACC_1S", 		0x2F,
				"ACC_10S", 		0x30,
				"ACC_10SPLUS", 		0x31,
				"ACC_UNKNOWN", 		0xFE
				);

	CONFIG_MAP_INT( "ptpengine:utc_offset",rtOpts->timeProperties.currentUtcOffset,rtOpts->timeProperties.currentUtcOffset,
		 "Underlying time source UTC offset announced in master state.");

	CONFIG_MAP_BOOLEAN("ptpengine:utc_offset_valid",rtOpts->timeProperties.currentUtcOffsetValid,
	rtOpts->timeProperties.currentUtcOffsetValid,
		 "Underlying time source UTC offset validity announced in master state.");

	CONFIG_MAP_BOOLEAN("ptpengine:time_traceable",rtOpts->timeProperties.timeTraceable,
	rtOpts->timeProperties.timeTraceable,
		 "Underlying time source time traceability announced in master state.");

	CONFIG_MAP_BOOLEAN("ptpengine:frequency_traceable",rtOpts->timeProperties.frequencyTraceable,
	rtOpts->timeProperties.frequencyTraceable,
		 "Underlying time source frequency traceability announced in master state.");

	CONFIG_MAP_SELECTVALUE("ptpengine:ptp_timescale",rtOpts->timeProperties.ptpTimescale,rtOpts->timeProperties.ptpTimescale,
		"Time scale announced in master state (with ARB, UTC properties\n"
	"	 are ignored by slaves). When clock class is set to 13 (application\n"
	"	 specific), this value is ignored and ARB is used.",
				"PTP",		TRUE,
				"ARB",			FALSE
				);

	CONFIG_MAP_SELECTVALUE("ptpengine:ptp_timesource",rtOpts->timeProperties.timeSource,rtOpts->timeProperties.timeSource,
	"Time source announced in master state.",
				"ATOMIC_CLOCK",		ATOMIC_CLOCK,
				"GPS",			GPS,
				"TERRESTRIAL_RADIO",	TERRESTRIAL_RADIO,
				"PTP",			PTP,
				"NTP",			NTP,
				"HAND_SET",		HAND_SET,
				"OTHER",		OTHER,
				"INTERNAL_OSCILLATOR",	INTERNAL_OSCILLATOR
				);

	CONFIG_MAP_INT_RANGE("ptpengine:clock_class",rtOpts->clockQuality.clockClass,ptpPreset.clockClass.defaultValue,
		"Clock class - announced in master state. Always 255 for slave-only.\n"
	"	 Minimum, maximum and default values are controlled by presets.\n"
	"	 If set to 13 (application specific time source), announced \n"
	"	 time scale is always set to ARB. This setting controls the\n"
	"	 states a PTP port can be in. If below 128, port will only\n"
	"	 be in MASTER or PASSIVE states (master only). If above 127,\n"
	"	 port will be in MASTER or SLAVE states.",
	ptpPreset.clockClass.minValue,ptpPreset.clockClass.maxValue);

	/* ClockClass = 13 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE,
					rtOpts->timeProperties.ptpTimescale,FALSE,rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 14 triggers ARB */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==14,
					rtOpts->timeProperties.ptpTimescale,FALSE,rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 6 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==6,
					rtOpts->timeProperties.ptpTimescale,TRUE,rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 7 triggers PTP*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==7,
					rtOpts->timeProperties.ptpTimescale,TRUE,rtOpts->timeProperties.ptpTimescale);

	/* ClockClass = 255 triggers slaveOnly */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->clockQuality.clockClass==SLAVE_ONLY_CLOCK_CLASS,rtOpts->slaveOnly,TRUE,FALSE);
	/* ...and vice versa */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->slaveOnly==TRUE,rtOpts->clockQuality.clockClass,SLAVE_ONLY_CLOCK_CLASS,rtOpts->clockQuality.clockClass);

	CONFIG_MAP_INT_RANGE("ptpengine:priority1",rtOpts->priority1,rtOpts->priority1,
		"Priority 1 announced in master state,used for Best Master\n"
	"	 Clock selection.",0,248);

	CONFIG_MAP_INT_RANGE("ptpengine:priority2",rtOpts->priority2,rtOpts->priority2,
		"Priority 2 announced in master state, used for Best Master\n"
	"	 Clock selection.",0,248);

	CONFIG_MAP_INT_MIN( "ptpengine:max_listen",rtOpts->maxListen,rtOpts->maxListen,
		 "Number of consecutive resets to LISTENING before full network reset\n",1);

	/* 
	 * TODO: in unicast and hybrid mode, automativally override master delayreq interval with a default,
	 * rather than require setting it manually.
	 */

	/* hybrid mode -> should specify delayreq interval: override set in the bottom of this function */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(rtOpts->ipMode == IPMODE_HYBRID,
    		    "ptpengine:log_delayreq_interval",
		    "It is recommended to set the delay request interval (ptpengine:log_delayreq_interval) in hybrid mode"
	);

	/* unicast mode -> should specify delayreq interval if we can become a slave */
	CONFIG_KEY_CONDITIONAL_WARNING_NOTSET(rtOpts->ipMode == IPMODE_UNICAST &&
		    rtOpts->clockQuality.clockClass > 127,
		    "ptpengine:log_delayreq_interval",
		    "It is recommended to set the delay request interval (ptpengine:log_delayreq_interval) in unicast mode"
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

	CONFIG_KEY_TRIGGER("ptpengine:unicast_destinations",rtOpts->unicastDestinationsSet,TRUE,rtOpts->unicastDestinationsSet);

	CONFIG_MAP_CHARARRAY("ptpengine:unicast_destinations",rtOpts->unicastDestinations,rtOpts->unicastDestinations,
		"Specify unicast slave addresses for unicast master operation, or unicast\n"
	"	 master addresses for slave operation. Format is similar to an ACL: comma,\n"
	"        tab or space-separated IPv4 unicast addresses, one or more. For a slave,\n"
	"        when unicast negotiation is used, setting this is mandatory.");

	CONFIG_MAP_CHARARRAY("ptpengine:unicast_domains",rtOpts->unicastDomains,rtOpts->unicastDomains,
		"Specify PTP domain number for each configured unicast destination (ptpengine:unicast_destinations).\n"
	"	 This is only used by slave-only clocks using unicast destinations to allow for each master\n"
	"        to be in a separate domain, such as with Telecom Profile. The number of entries should match the number\n"
	"        of unicast destinations, otherwise unconfigured domains or domains set to 0 are set to domain configured in\n"
	"        ptpengine:domain. The format is a comma, tab or space-separated list of 8-bit unsigned integers (0 .. 255)");

	CONFIG_MAP_CHARARRAY("ptpengine:unicast_local_preference",rtOpts->unicastLocalPreference,rtOpts->unicastLocalPreference,
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

	CONFIG_KEY_TRIGGER("ptpengine:unicast_peer_destination",rtOpts->unicastPeerDestinationSet,TRUE,rtOpts->unicastPeerDestinationSet);

	CONFIG_MAP_CHARARRAY("ptpengine:unicast_peer_destination",rtOpts->unicastPeerDestination,rtOpts->unicastPeerDestination,
		"Specify peer unicast adress for P2P unicast. Mandatory when\n"
	"	 running unicast mode and P2P delay mode.");

	CONFIG_MAP_BOOLEAN("ptpengine:management_enable",rtOpts->managementEnabled,rtOpts->managementEnabled,
	"Enable handling of PTP management messages.");

	CONFIG_MAP_BOOLEAN("ptpengine:management_set_enable",rtOpts->managementSetEnable,rtOpts->managementSetEnable,
	"Accept SET and COMMAND management messages.");

	CONFIG_MAP_BOOLEAN("ptpengine:igmp_refresh",rtOpts->do_IGMP_refresh,rtOpts->do_IGMP_refresh,
	"Send explicit IGMP joins between engine resets and periodically\n"
	"	 in master state.");

	CONFIG_MAP_INT_RANGE("ptpengine:master_igmp_refresh_interval",rtOpts->masterRefreshInterval,rtOpts->masterRefreshInterval,
		"Periodic IGMP join interval (seconds) in master state when running\n"
		"	 IPv4 multicast: when set below 10 or when ptpengine:igmp_refresh\n"
		"	 is disabled, this setting has no effect.",0,255);

	CONFIG_MAP_INT_RANGE("ptpengine:multicast_ttl",rtOpts->ttl,rtOpts->ttl,
		"Multicast time to live for multicast PTP packets (ignored and set to 1\n"
	"	 for peer to peer messages).",1,64);

	CONFIG_MAP_INT_RANGE("ptpengine:ip_dscp",rtOpts->dscpValue,rtOpts->dscpValue,
		"DiffServ CodepPoint for packet prioritisation (decimal). When set to zero, \n"
	"	 this option is not used. Use 46 for Expedited Forwarding (0x2e).",0,63);

#ifdef PTPD_STATISTICS

	CONFIG_MAP_BOOLEAN("ptpengine:sync_stat_filter_enable",rtOpts->filterMSOpts.enabled,rtOpts->filterMSOpts.enabled,
		 "Enable statistical filter for Sync messages.");

	CONFIG_MAP_SELECTVALUE("ptpengine:sync_stat_filter_type",rtOpts->filterMSOpts.filterType,rtOpts->filterMSOpts.filterType,
		"Type of filter used for Sync message filtering",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN);

	CONFIG_MAP_INT_RANGE("ptpengine:sync_stat_filter_window",rtOpts->filterMSOpts.windowSize, rtOpts->filterMSOpts.windowSize,
		"Number of samples used for the Sync statistical filter",3,128);

	CONFIG_MAP_SELECTVALUE("ptpengine:sync_stat_filter_window_type",rtOpts->filterMSOpts.windowType, rtOpts->filterMSOpts.windowType,
		"Sample window type used for Sync message statistical filter. Delay Response outlier filter action.\n"
	"        Sliding window is continuous, interval passes every n-th sample only.",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL);

	CONFIG_MAP_BOOLEAN("ptpengine:delay_stat_filter_enable",rtOpts->filterSMOpts.enabled,rtOpts->filterSMOpts.enabled,
		 "Enable statistical filter for Delay messages.");

	CONFIG_MAP_SELECTVALUE("ptpengine:delay_stat_filter_type",rtOpts->filterSMOpts.filterType,rtOpts->filterSMOpts.filterType,
		"Type of filter used for Delay message statistical filter",
	"none", FILTER_NONE,
	"mean", FILTER_MEAN,
	"min", FILTER_MIN,
	"max", FILTER_MAX,
	"absmin", FILTER_ABSMIN,
	"absmax", FILTER_ABSMAX,
	"median", FILTER_MEDIAN);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_stat_filter_window",rtOpts->filterSMOpts.windowSize, rtOpts->filterSMOpts.windowSize,
		"Number of samples used for the Delay statistical filter",3,128);

	CONFIG_MAP_SELECTVALUE("ptpengine:delay_stat_filter_window_type",rtOpts->filterSMOpts.windowType, rtOpts->filterSMOpts.windowType,
		"Sample window type used for Delay message statistical filter\n"
	"        Sliding window is continuous, interval passes every n-th sample only",
	"sliding", WINDOW_SLIDING,
	"interval", WINDOW_INTERVAL);


	CONFIG_MAP_BOOLEAN("ptpengine:delay_outlier_filter_enable",rtOpts->oFilterSMConfig.enabled,rtOpts->oFilterSMConfig.enabled,
		 "Enable outlier filter for the Delay Response component in slave state");

	CONFIG_MAP_SELECTVALUE("ptpengine:delay_outlier_filter_action",rtOpts->oFilterSMConfig.discard,rtOpts->oFilterSMConfig.discard,
		"Delay Response outlier filter action. If set to 'filter', outliers are\n"
	"	 replaced with moving average.",
	"discard", TRUE,
	"filter", FALSE);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_capacity",rtOpts->oFilterSMConfig.capacity,rtOpts->oFilterSMConfig.capacity,
		"Number of samples in the Delay Response outlier filter buffer",10,STATCONTAINER_MAX_SAMPLES);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:delay_outlier_filter_threshold",rtOpts->oFilterSMConfig.threshold,rtOpts->oFilterSMConfig.threshold,
		"Delay Response outlier filter threshold (: multiplier for Peirce's maximum\n"
	"	 standard deviation. When set below 1.0, filter is tighter, when set above\n"
	"	 1.0, filter is looser than standard Peirce's test.\n"
	"        When autotune enabled, this is the starting threshold.", 0.001, 1000.0);

	CONFIG_MAP_BOOLEAN("ptpengine:delay_outlier_filter_always_filter",rtOpts->oFilterSMConfig.alwaysFilter,rtOpts->oFilterSMConfig.alwaysFilter,
		"Always run the Delay Response outlier filter, even if clock is being slewed at maximum rate");

	CONFIG_MAP_BOOLEAN("ptpengine:delay_outlier_filter_autotune_enable",rtOpts->oFilterSMConfig.autoTune,rtOpts->oFilterSMConfig.autoTune,
		"Enable automatic threshold control for Delay Response outlier filter.");

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_autotune_minpercent",rtOpts->oFilterSMConfig.minPercent,rtOpts->oFilterSMConfig.minPercent,
		"Delay Response outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",0,99);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_autotune_maxpercent",rtOpts->oFilterSMConfig.maxPercent,rtOpts->oFilterSMConfig.maxPercent,
		"Delay Response outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",1,100);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:delay_outlier_autotune_step",rtOpts->oFilterSMConfig.thresholdStep,rtOpts->oFilterSMConfig.thresholdStep,
		"The value the Delay Response outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.",0.01,10.0);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:delay_outlier_filter_autotune_minthreshold",rtOpts->oFilterSMConfig.minThreshold,rtOpts->oFilterSMConfig.minThreshold,
		"Minimum Delay Response filter threshold value used when auto-tuning", 0.01,10.0);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:delay_outlier_filter_autotune_maxthreshold",rtOpts->oFilterSMConfig.maxThreshold,rtOpts->oFilterSMConfig.maxThreshold,
		"Maximum Delay Response filter threshold value used when auto-tuning", 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterSMConfig.maxPercent <= rtOpts->oFilterSMConfig.minPercent,
					"ptpengine:delay_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterSMConfig.maxThreshold <= rtOpts->oFilterSMConfig.minThreshold,
					"ptpengine:delay_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:delay_outlier_filter_autotune_minthreshold\n");

	CONFIG_MAP_BOOLEAN("ptpengine:delay_outlier_filter_stepdetect_enable",rtOpts->oFilterSMConfig.stepDelay,rtOpts->oFilterSMConfig.stepDelay,
		"Enable Delay filter step detection (delaySM) to block when certain level exceeded");

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_stepdetect_threshold",rtOpts->oFilterSMConfig.stepThreshold,rtOpts->oFilterSMConfig.stepThreshold,
		"Delay Response step detection threshold. Step detection is performed\n"
	"	 only when delaySM is below this threshold (nanoseconds)", 50000, NANOSECONDS_MAX);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_stepdetect_level",rtOpts->oFilterSMConfig.stepLevel,rtOpts->oFilterSMConfig.stepLevel,
		"Delay Response step level. When step detection enabled and operational,\n"
	"	 delaySM above this level (nanosecond) is considered a clock step and updates are paused", 50000, NANOSECONDS_MAX);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_stepdetect_credit",rtOpts->oFilterSMConfig.delayCredit,rtOpts->oFilterSMConfig.delayCredit,
		"Initial credit (number of samples) the Delay step detection filter can block for\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",50,1000);

	CONFIG_MAP_INT_RANGE("ptpengine:delay_outlier_filter_stepdetect_credit_increment",rtOpts->oFilterSMConfig.creditIncrement,rtOpts->oFilterSMConfig.creditIncrement,
		"Amount of credit for the Delay step detection filter restored every full sample window",1,100);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:delay_outlier_weight",rtOpts->oFilterSMConfig.weight,rtOpts->oFilterSMConfig.weight,
		"Delay Response outlier weight: if an outlier is detected, determines\n"
	"	 the amount of its deviation from mean that is used to build the standard\n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", 0.01, 2.0);

    CONFIG_MAP_BOOLEAN("ptpengine:sync_outlier_filter_enable",rtOpts->oFilterMSConfig.enabled,rtOpts->oFilterMSConfig.enabled,
		"Enable outlier filter for the Sync component in slave state.");

    CONFIG_MAP_SELECTVALUE("ptpengine:sync_outlier_filter_action",rtOpts->oFilterMSConfig.discard,rtOpts->oFilterMSConfig.discard,
		"Sync outlier filter action. If set to 'filter', outliers are replaced\n"
	"	 with moving average.",
     "discard", TRUE,
     "filter", FALSE);

     CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_capacity",rtOpts->oFilterMSConfig.capacity,rtOpts->oFilterMSConfig.capacity,
    "Number of samples in the Sync outlier filter buffer.",10,STATCONTAINER_MAX_SAMPLES);

    CONFIG_MAP_DOUBLE_RANGE("ptpengine:sync_outlier_filter_threshold",rtOpts->oFilterMSConfig.threshold,rtOpts->oFilterMSConfig.threshold,
		"Sync outlier filter threshold: multiplier for the Peirce's maximum standard\n"
	"	 deviation. When set below 1.0, filter is tighter, when set above 1.0,\n"
	"	 filter is looser than standard Peirce's test.", 0.001, 1000.0);

	CONFIG_MAP_BOOLEAN("ptpengine:sync_outlier_filter_always_filter",rtOpts->oFilterMSConfig.alwaysFilter,rtOpts->oFilterMSConfig.alwaysFilter,
		"Always run the Sync outlier filter, even if clock is being slewed at maximum rate");

	CONFIG_MAP_BOOLEAN("ptpengine:sync_outlier_filter_autotune_enable",rtOpts->oFilterMSConfig.autoTune,rtOpts->oFilterMSConfig.autoTune,
		"Enable automatic threshold control for Sync outlier filter.");

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_autotune_minpercent",rtOpts->oFilterMSConfig.minPercent,rtOpts->oFilterMSConfig.minPercent,
		"Sync outlier filter autotune low watermark - minimum percentage\n"
	"	 of discarded samples in the update period before filter is tightened\n"
	"	 by the autotune step value.",0,99);

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_autotune_maxpercent",rtOpts->oFilterMSConfig.maxPercent,rtOpts->oFilterMSConfig.maxPercent,
		"Sync outlier filter autotune high watermark - maximum percentage\n"
	"	 of discarded samples in the update period before filter is loosened\n"
	"	 by the autotune step value.",1,100);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:sync_outlier_autotune_step",rtOpts->oFilterMSConfig.thresholdStep,rtOpts->oFilterMSConfig.thresholdStep,
		"Value the Sync outlier filter threshold is increased\n"
	"	 or decreased by when auto-tuning.",0.01,10.0);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:sync_outlier_filter_autotune_minthreshold",rtOpts->oFilterMSConfig.minThreshold,rtOpts->oFilterMSConfig.minThreshold,
		"Minimum Sync outlier filter threshold value used when auto-tuning", 0.01,10.0);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:sync_outlier_filter_autotune_maxthreshold",rtOpts->oFilterMSConfig.maxThreshold,rtOpts->oFilterMSConfig.maxThreshold,
		"Maximum Sync outlier filter threshold value used when auto-tuning", 0.01,10.0);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.maxPercent <= rtOpts->oFilterMSConfig.minPercent,
					"ptpengine:sync_outlier_filter_autotune_maxpercent value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minpercent\n");

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.maxThreshold <= rtOpts->oFilterMSConfig.minThreshold,
					"ptpengine:sync_outlier_filter_autotune_maxthreshold value has to be greater "
					"than ptpengine:sync_outlier_filter_autotune_minthreshold\n");

	CONFIG_MAP_BOOLEAN("ptpengine:sync_outlier_filter_stepdetect_enable",rtOpts->oFilterMSConfig.stepDelay,rtOpts->oFilterMSConfig.stepDelay,
		"Enable Sync filter step detection (delayMS) to block when certain level exceeded.");

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_stepdetect_threshold",rtOpts->oFilterMSConfig.stepThreshold,rtOpts->oFilterMSConfig.stepThreshold,
		"Sync step detection threshold. Step detection is performed\n"
	"	 only when delayMS is below this threshold (nanoseconds)", 100000, NANOSECONDS_MAX);

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_stepdetect_level",rtOpts->oFilterMSConfig.stepLevel,rtOpts->oFilterMSConfig.stepLevel,
		"Sync step level. When step detection enabled and operational,\n"
	"	 delayMS above this level (nanosecond) is considered a clock step and updates are paused", 100000, NANOSECONDS_MAX);

	CONFIG_CONDITIONAL_ASSERTION(rtOpts->oFilterMSConfig.stepThreshold <= (rtOpts->oFilterMSConfig.stepLevel + 100000),
					"ptpengine:sync_outlier_filter_stepdetect_threshold  has to be at least "
					"100 us (100000) greater than ptpengine:sync_outlier_filter_stepdetect_level\n");

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_stepdetect_credit",rtOpts->oFilterMSConfig.delayCredit,rtOpts->oFilterMSConfig.delayCredit,
		"Initial credit (number of samples) the Sync step detection filter can block for.\n"
	"	 When credit is exhausted, filter stops blocking. Credit is gradually restored",50,1000);

	CONFIG_MAP_INT_RANGE("ptpengine:sync_outlier_filter_stepdetect_credit_increment",rtOpts->oFilterMSConfig.creditIncrement,rtOpts->oFilterMSConfig.creditIncrement,
		"Amount of credit for the Sync step detection filter restored every full sample window",1,100);

	CONFIG_MAP_DOUBLE_RANGE("ptpengine:sync_outlier_weight",rtOpts->oFilterMSConfig.weight,rtOpts->oFilterMSConfig.weight,
		"Sync outlier weight: if an outlier is detected, this value determines the\n"
	"	 amount of its deviation from mean that is used to build the standard \n"
	"	 deviation statistics and influence further outlier detection.\n"
	"	 When set to 1.0, the outlier is used as is.", 0.01, 2.0);

        CONFIG_MAP_INT_RANGE("ptpengine:calibration_delay",rtOpts->calibrationDelay,rtOpts->calibrationDelay,
		"Delay between moving to slave state and enabling clock updates (seconds).\n"
	"	 This allows one-way delay to stabilise before starting clock updates.\n"
	"	 Activated when going into slave state and during slave's GM failover.\n"
	"	 0 - not used.",0,300);

#endif /* PTPD_STATISTICS */


        CONFIG_MAP_INT_RANGE("ptpengine:idle_timeout",rtOpts->idleTimeout,rtOpts->idleTimeout,
		"PTP idle timeout: if PTPd is in SLAVE state and there have been no clock\n"
	"	 updates for this amout of time, PTPd releases clock control.\n", 10,3600);

	CONFIG_MAP_BOOLEAN("ptpengine:panic_mode",rtOpts->enablePanicMode,rtOpts->enablePanicMode,
		"Enable panic mode: when offset from master is above 1 second, stop updating\n"
	"	 the clock for a period of time and then step the clock if offset remains\n"
	"	 above 1 second.");

    CONFIG_MAP_INT_RANGE("ptpengine:panic_mode_duration",rtOpts->panicModeDuration,rtOpts->panicModeDuration,
		"Duration (minutes) of the panic mode period (no clock updates) when offset\n"
	"	 above 1 second detected.",1,60);

	CONFIG_MAP_BOOLEAN("ptpengine:panic_mode_release_clock",rtOpts->panicModeReleaseClock,rtOpts->panicModeReleaseClock,
		"When entering panic mode, release clock control while panic mode lasts\n"
 	"	 if ntpengine:* configured, this will fail over to NTP,\n"
	"	 if not set, PTP will hold clock control during panic mode.");

    CONFIG_MAP_INT_RANGE("ptpengine:panic_mode_exit_threshold",rtOpts->panicModeExitThreshold,rtOpts->panicModeExitThreshold,
		"Do not exit panic mode until offset drops below this value (nanoseconds).\n"
	"	 0 = not used.",0,NANOSECONDS_MAX);

	CONFIG_MAP_BOOLEAN("ptpengine:pid_as_clock_identity",rtOpts->pidAsClockId,rtOpts->pidAsClockId,
	"Use PTPd's process ID as the middle part of the PTP clock ID - useful for running multiple instances.");

	CONFIG_MAP_BOOLEAN("ptpengine:ntp_failover",rtOpts->ntpOptions.enableFailover,rtOpts->ntpOptions.enableFailover,
		"Fail over to NTP when PTP time sync not available - requires\n"
	"	 ntpengine:enabled, but does not require the rest of NTP configuration:\n"
	"	 will warn instead of failing over if cannot control ntpd.");

	CONFIG_KEY_DEPENDENCY("ptpengine:ntp_failover", "ntpengine:enabled");

	CONFIG_MAP_INT_RANGE("ptpengine:ntp_failover_timeout",rtOpts->ntpOptions.failoverTimeout,
								rtOpts->ntpOptions.failoverTimeout,	
		"NTP failover timeout in seconds: time between PTP slave going into\n"
	"	 LISTENING state, and releasing clock control. 0 = fail over immediately.", 0, 1800);

	CONFIG_MAP_BOOLEAN("ptpengine:prefer_ntp",rtOpts->preferNTP,rtOpts->preferNTP,
		"Prefer NTP time synchronisation. Only use PTP when NTP not available,\n"
	"	 could be used when NTP runs with a local GPS receiver or another reference");


	CONFIG_MAP_BOOLEAN("ptpengine:panic_mode_ntp",rtOpts->panicModeReleaseClock,rtOpts->panicModeReleaseClock,
		"Legacy option from 2.3.0: same as ptpengine:panic_mode_release_clock");

	CONFIG_KEY_DEPENDENCY("ptpengine:panic_mode_ntp", "ntpengine:enabled");

	CONFIG_MAP_BOOLEAN("ptpengine:sigusr2_clears_counters",rtOpts->clearCounters,rtOpts->clearCounters,
		"Clear counters after dumping all counter values on SIGUSR2.");

	/* Defining the ACLs enables ACL matching */
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_permit",rtOpts->timingAclEnabled,TRUE,rtOpts->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:timing_acl_deny",rtOpts->timingAclEnabled,TRUE,rtOpts->timingAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_permit",rtOpts->managementAclEnabled,TRUE,rtOpts->managementAclEnabled);
	CONFIG_KEY_TRIGGER("ptpengine:management_acl_deny",rtOpts->managementAclEnabled,TRUE,rtOpts->managementAclEnabled);

	CONFIG_MAP_CHARARRAY("ptpengine:timing_acl_permit",rtOpts->timingAclPermitText, rtOpts->timingAclPermitText,
		"Permit access control list for timing packets. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");

	CONFIG_MAP_CHARARRAY("ptpengine:timing_acl_deny",rtOpts->timingAclDenyText, rtOpts->timingAclDenyText,
		"Deny access control list for timing packets. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");

	CONFIG_MAP_CHARARRAY("ptpengine:management_acl_permit",rtOpts->managementAclPermitText, rtOpts->managementAclPermitText,
		"Permit access control list for management messages. Format is a series of \n"
	"	 comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
	"	 where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
	"        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
	"        incoming messages. IP access lists are only supported when using the IP transport.");

	CONFIG_MAP_CHARARRAY("ptpengine:management_acl_deny",rtOpts->managementAclDenyText, rtOpts->managementAclDenyText,
		"Deny access control list for management messages. Format is a series of \n"
        "        comma, space or tab separated  network prefixes: IPv4 addresses or full CIDR notation a.b.c.d/x,\n"
        "        where a.b.c.d is the subnet and x is the decimal mask, or a.b.c.d/v.x.y.z where a.b.c.d is the\n"
        "        subnet and v.x.y.z is the 4-octet mask. The match is performed on the source IP address of the\n"
        "        incoming messages. IP access lists are only supported when using the IP transport.");


	CONFIG_MAP_SELECTVALUE("ptpengine:timing_acl_order",rtOpts->timingAclOrder,rtOpts->timingAclOrder,
		"Order in which permit and deny access lists are evaluated for timing\n"
	"	 packets, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	ACL_PERMIT_DENY,
				"deny-permit", 	ACL_DENY_PERMIT
				);

	CONFIG_MAP_SELECTVALUE("ptpengine:management_acl_order",rtOpts->managementAclOrder,rtOpts->managementAclOrder,
		"Order in which permit and deny access lists are evaluated for management\n"
	"	 messages, the evaluation process is the same as for Apache httpd.",
				"permit-deny", 	ACL_PERMIT_DENY,
				"deny-permit", 	ACL_DENY_PERMIT
				);


	/* Ethernet mode disables ACL processing*/
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3,rtOpts->timingAclEnabled,FALSE,rtOpts->timingAclEnabled);
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->transport == IEEE_802_3,rtOpts->managementAclEnabled,FALSE,rtOpts->managementAclEnabled);



/* ===== clock section ===== */

	CONFIG_MAP_BOOLEAN("clock:no_adjust",rtOpts->noAdjust,ptpPreset.noAdjust,
	"Do not adjust the clock");

	CONFIG_MAP_BOOLEAN("clock:no_reset",rtOpts->noResetClock,rtOpts->noResetClock,
	"Do not step the clock - only slew");

	CONFIG_MAP_BOOLEAN("clock:step_startup_force",rtOpts->stepForce,rtOpts->stepForce,
	"Force clock step on first sync after startup regardless of offset and clock:no_reset");

	CONFIG_MAP_BOOLEAN("clock:step_startup",rtOpts->stepOnce,rtOpts->stepOnce,
		"Step clock on startup if offset >= 1 second, ignoring\n"
	"        panic mode and clock:no_reset");


#ifdef HAVE_LINUX_RTC_H
	CONFIG_MAP_BOOLEAN("clock:set_rtc_on_step",rtOpts->setRtc,rtOpts->setRtc,
	"Attempt setting the RTC when stepping clock (Linux only - FreeBSD does \n"
	"        this for us. WARNING: this will always set the RTC to OS clock time,\n"
	"        regardless of time zones, so this assumes that RTC runs in UTC or \n"
	"        at least in the same timescale as PTP. true at least on most \n"
	"        single-boot x86 Linux systems.");
#endif /* HAVE_LINUX_RTC_H */

	CONFIG_MAP_SELECTVALUE("clock:drift_handling",rtOpts->drift_recovery_method,rtOpts->drift_recovery_method,
		"Observed drift handling method between servo restarts:\n"
	"	 reset: set to zero (not recommended)\n"
	"	 preserve: use kernel value,\n"
	"	 file: load/save to drift file on startup/shutdown, use kernel\n"
	"	 value inbetween. To specify drift file, use the clock:drift_file setting."
	,
				"reset", 	DRIFT_RESET,
				"preserve", 	DRIFT_KERNEL,
				"file", 	DRIFT_FILE
				);

	CONFIG_MAP_CHARARRAY("clock:drift_file",rtOpts->driftFile,rtOpts->driftFile,
	"Specify drift file");

	CONFIG_MAP_INT_RANGE("clock:leap_second_pause_period",rtOpts->leapSecondPausePeriod,
		rtOpts->leapSecondPausePeriod,
		"Time (seconds) before and after midnight that clock updates should pe suspended for\n"
	"	 during a leap second event. The total duration of the pause is twice\n"
	"        the configured duration",5,600);

	CONFIG_MAP_INT_RANGE("clock:leap_second_notice_period",rtOpts->leapSecondNoticePeriod,
		rtOpts->leapSecondNoticePeriod,
		"Time (seconds) before midnight that PTPd starts announcing the leap second\n"
	"	 if it's running as master",3600,86400);

	CONFIG_MAP_CHARARRAY("clock:leap_seconds_file",rtOpts->leapFile,rtOpts->leapFile,
	"Specify leap second file location - up to date version can be downloaded from \n"
	"        http://www.ietf.org/timezones/data/leap-seconds.list");

	CONFIG_MAP_SELECTVALUE("clock:leap_second_handling",rtOpts->leapSecondHandling,rtOpts->leapSecondHandling,
		"Behaviour during a leap second event:\n"
	"	 accept: inform the OS kernel of the event\n"
	"	 ignore: do nothing - ends up with a 1-second offset which is then slewed\n"
	"	 step: similar to ignore, but steps the clock immediately after the leap second event\n"
	"        smear: do not inform kernel, gradually introduce the leap second before the event\n"
	"               by modifying clock offset (see clock:leap_second_smear_period)",
				"accept", 	LEAP_ACCEPT,
				"ignore", 	LEAP_IGNORE,
				"step", 	LEAP_STEP,
				"smear",	LEAP_SMEAR
				);

	CONFIG_MAP_INT_RANGE("clock:leap_second_smear_period",rtOpts->leapSecondSmearPeriod,
		rtOpts->leapSecondSmearPeriod,
		"Time period (Seconds) over which the leap second is introduced before the event.\n"
	"	 Example: when set to 86400 (24 hours), an extra 11.5 microseconds is added every second"
	,3600,86400);


#ifdef HAVE_STRUCT_TIMEX_TICK
	/* This really is clock specific - different clocks may allow different ranges */
	CONFIG_MAP_INT_RANGE("clock:max_offset_ppm",rtOpts->servoMaxPpb,rtOpts->servoMaxPpb,
		"Maximum absolute frequency shift which can be applied to the clock servo\n"
	"	 when slewing the clock. Expressed in parts per million (1 ppm = shift of\n"
	"	 1 us per second. Values above 512 will use the tick duration correction\n"
	"	 to allow even faster slewing. Default maximum is 512 without using tick.",
	ADJ_FREQ_MAX/1000,ADJ_FREQ_MAX/500);
#endif /* HAVE_STRUCT_TIMEX_TICK */

	/*
	 * TimeProperties DS - in future when clock driver API is implemented,
	 * a slave PTP engine should inform a clock about this, and then that
	 * clock should pass this information to any master PTP engines, unless
	 * we override this. here. For now we just supply this to RtOpts.
	 */
	 

/* ===== servo section ===== */

	CONFIG_MAP_INT( "servo:delayfilter_stiffness",rtOpts->s,rtOpts->s,
	"One-way delay filter stiffness.");

	CONFIG_MAP_DOUBLE_MIN("servo:kp",rtOpts->servoKP,rtOpts->servoKP,
	"Clock servo PI controller proportional component gain (kP).",0.000001);
	CONFIG_MAP_DOUBLE_MIN("servo:ki",rtOpts->servoKI,rtOpts->servoKI,
	"Clock servo PI controller integral component gain (kI).",0.000001);

	CONFIG_MAP_SELECTVALUE("servo:dt_method",rtOpts->servoDtMethod,rtOpts->servoDtMethod,
		"How servo update interval (delta t) is calculated:\n"
	"	 none:     servo not corrected for update interval (dt always 1),\n"
	"	 constant: constant value (target servo update rate - sync interval for PTP,\n"
	"	 measured: servo measures how often it's updated and uses this interval.",
			"none", DT_NONE,
			"constant", DT_CONSTANT,
			"measured", DT_MEASURED
	);

	CONFIG_MAP_DOUBLE_RANGE("servo:dt_max",rtOpts->servoMaxdT,rtOpts->servoMaxdT,
		"Maximum servo update interval (delta t) when using measured servo update interval\n"
	"	 (servo:dt_method = measured), specified as sync interval multiplier.",1.5,100.0);

#ifdef PTPD_STATISTICS
	CONFIG_MAP_BOOLEAN("servo:stability_detection",rtOpts->servoStabilityDetection,
							rtOpts->servoStabilityDetection,
		 "Enable clock synchronisation servo stability detection\n"
	"	 (based on standard deviation of the observed drift value)\n"
	"	 - drift will be saved to drift file / cached when considered stable,\n"
	"	 also clock stability status will be logged.");

	CONFIG_MAP_DOUBLE_RANGE("servo:stability_threshold",rtOpts->servoStabilityThreshold,
							rtOpts->servoStabilityThreshold,
		"Specify the observed drift standard deviation threshold in parts per\n"
	"	 billion (ppb) - if stanard deviation is within the threshold, servo\n"
	"	 is considered stable.",
	1.0,10000.0);

	CONFIG_MAP_INT_RANGE("servo:stability_period",rtOpts->servoStabilityPeriod,
							rtOpts->servoStabilityPeriod,
		"Specify for how many statistics update intervals the observed drift\n"
	"	standard deviation has to stay within threshold to be considered stable.",
	1,100);

	CONFIG_MAP_INT_RANGE("servo:stability_timeout",rtOpts->servoStabilityTimeout,
							rtOpts->servoStabilityTimeout,
		"Specify after how many minutes without stabilisation servo is considered\n"
	"	 unstable. Assists with logging servo stability information and\n"
	"	 allows to preserve observed drift if servo cannot stabilise.\n",
	1,60);

#endif

	CONFIG_MAP_INT_RANGE("servo:max_delay",rtOpts->maxDelay,rtOpts->maxDelay,
		"Do accept master to slave delay (delayMS - from Sync message) or slave to master delay\n"
	"	 (delaySM - from Delay messages) if greater than this value (nanoseconds). 0 = not used."
	,0,NANOSECONDS_MAX);

	CONFIG_MAP_INT_MIN("servo:max_delay_max_rejected",rtOpts->maxDelayMaxRejected,rtOpts->maxDelayMaxRejected,
		"Maximum number of consecutive delay measurements exceeding maxDelay threshold,\n"
	"	 before slave is reset.", 0);

#ifdef PTPD_STATISTICS
	CONFIG_MAP_BOOLEAN("servo:max_delay_stable_only",rtOpts->maxDelayStableOnly,rtOpts->maxDelayStableOnly,
		"If servo:max_delay is set, perform the check only if clock servo has stabilised.\n");
#endif

	CONFIG_MAP_BOOLEAN("ptpengine:sync_sequence_checking",rtOpts->syncSequenceChecking,rtOpts->syncSequenceChecking,
		"When enabled, Sync messages will only be accepted if sequence ID is increasing."
	"        This is limited to 50 dropped messages.\n");

	CONFIG_MAP_INT_RANGE("ptpengine:clock_update_timeout",rtOpts->clockUpdateTimeout,rtOpts->clockUpdateTimeout,
		"If set to non-zero, timeout in seconds, after which the slave resets if no clock updates made. \n",
		0, 3600);

	CONFIG_MAP_INT_RANGE("servo:max_offset",rtOpts->maxOffset,rtOpts->maxOffset,
		"Do not reset the clock if offset from master is greater\n"
	"        than this value (nanoseconds). 0 = not used.",
	0,NANOSECONDS_MAX);

/* ===== global section ===== */

#ifdef PTPD_SNMP
	CONFIG_MAP_BOOLEAN("global:enable_snmp",rtOpts->snmp_enabled,rtOpts->snmp_enabled,
	"Enable SNMP agent (if compiled with PTPD_SNMP).");
#else
	if(!IS_QUIET() && CONFIG_ISTRUE("global:enable_snmp"))
	    INFO("SNMP support not enabled. Please compile with PTPD_SNMP to use global:enable_snmp\n");
#endif /* PTPD_SNMP */

	CONFIG_MAP_BOOLEAN("global:use_syslog",rtOpts->useSysLog,rtOpts->useSysLog,
		"Send log messages to syslog. Disabling this\n"
	"        sends all messages to stdout (or speficied log file).");

	CONFIG_MAP_CHARARRAY("global:lock_file",rtOpts->lockFile,rtOpts->lockFile,
	"Lock file location");

	CONFIG_MAP_BOOLEAN("global:auto_lockfile",rtOpts->autoLockFile,rtOpts->autoLockFile,
	"	 Use mode specific and interface specific lock file\n"
	"	 (overrides global:lock_file).");

	CONFIG_MAP_CHARARRAY("global:lock_directory",rtOpts->lockDirectory,rtOpts->lockDirectory,
		 "Lock file directory: used with automatic mode-specific lock files,\n"
	"	 also used when no lock file is specified. When lock file\n"
	"	 is specified, it's expected to be an absolute path.");

	CONFIG_MAP_BOOLEAN("global:ignore_lock",rtOpts->ignore_daemon_lock,rtOpts->ignore_daemon_lock,
	"Skip lock file checking and locking.");

	/* if quality file specified, enable quality recording  */
	CONFIG_KEY_TRIGGER("global:quality_file",rtOpts->recordLog.logEnabled,TRUE,FALSE);
	CONFIG_MAP_CHARARRAY("global:quality_file",rtOpts->recordLog.logPath,rtOpts->recordLog.logPath,
		"File used to record data about sync packets. Enables recording when set.");

	CONFIG_MAP_INT_MIN("global:quality_file_max_size",rtOpts->recordLog.maxSize,rtOpts->recordLog.maxSize,
		"Maximum sync packet record file size (in kB) - file will be truncated\n"
	"	if size exceeds the limit. 0 - no limit.", 0);

	CONFIG_MAP_INT_RANGE("global:quality_file_max_files",rtOpts->recordLog.maxFiles,rtOpts->recordLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", 0, 100);

	CONFIG_MAP_BOOLEAN("global:quality_file_truncate",rtOpts->recordLog.truncateOnReopen,rtOpts->recordLog.truncateOnReopen,
		"Truncate the sync packet record file every time it is (re) opened:\n"
	"	 startup and SIGHUP.");

	/* if status file specified, enable status logging*/
	CONFIG_KEY_TRIGGER("global:status_file",rtOpts->statusLog.logEnabled,TRUE,FALSE);
	CONFIG_MAP_CHARARRAY("global:status_file",rtOpts->statusLog.logPath,rtOpts->statusLog.logPath,
	"File used to log "PTPD_PROGNAME" status information.");
	/* status file can be disabled even if specified */
	CONFIG_MAP_BOOLEAN("global:log_status",rtOpts->statusLog.logEnabled, rtOpts->statusLog.logEnabled,
		"Enable / disable writing status information to file.");

	CONFIG_MAP_INT_RANGE("global:status_update_interval",rtOpts->statusFileUpdateInterval,rtOpts->statusFileUpdateInterval,
		"Status file update interval in seconds.",
	1,30);

#ifdef RUNTIME_DEBUG
	CONFIG_MAP_SELECTVALUE("global:debug_level",rtOpts->debug_level,rtOpts->debug_level,
	"Specify debug level (if compiled with RUNTIME_DEBUG).",
				"LOG_INFO", 	LOG_INFO,
				"LOG_DEBUG", 	LOG_DEBUG,
				"LOG_DEBUG1", 	LOG_DEBUG1,
				"LOG_DEBUG2", 	LOG_DEBUG2,
				"LOG_DEBUG3", 	LOG_DEBUG3,
				"LOG_DEBUGV", 	LOG_DEBUGV
				);
#else
	if (!IS_QUIET() && CONFIG_ISSET("global:debug_level"))
	    INFO("Runtime debug not enabled. Please compile with RUNTIME_DEBUG to use global:debug_level.\n");
#endif /* RUNTIME_DEBUG */

	/* if log file specified, enable file logging - otherwise disable */
	CONFIG_KEY_TRIGGER("global:log_file",rtOpts->eventLog.logEnabled,TRUE,FALSE);
	CONFIG_MAP_CHARARRAY("global:log_file",rtOpts->eventLog.logPath, rtOpts->eventLog.logPath,
		"Specify log file path (event log). Setting this enables logging to file.");

	CONFIG_MAP_INT_MIN("global:log_file_max_size",rtOpts->eventLog.maxSize,rtOpts->eventLog.maxSize,
		"Maximum log file size (in kB) - log file will be truncated if size exceeds\n"
	"	 the limit. 0 - no limit.", 0);

	CONFIG_MAP_INT_RANGE("global:log_file_max_files",rtOpts->eventLog.maxFiles,rtOpts->eventLog.maxFiles,
		"Enable log rotation of the sync packet record file up to n files.\n"
	"	 0 - do not rotate.\n", 0, 100);

	CONFIG_MAP_BOOLEAN("global:log_file_truncate",rtOpts->eventLog.truncateOnReopen,rtOpts->eventLog.truncateOnReopen,
		"Truncate the log file every time it is (re) opened: startup and SIGHUP.");

	CONFIG_MAP_SELECTVALUE("global:log_level",rtOpts->logLevel,rtOpts->logLevel,
		"Specify log level (only messages at this priority or higer will be logged).\n"
	"	 The minimal level is LOG_ERR. LOG_ALL enables debug output if compiled with\n"
	"	 RUNTIME_DEBUG.",
				"LOG_ERR", 	LOG_ERR,
				"LOG_WARNING", 	LOG_WARNING,
				"LOG_NOTICE", 	LOG_NOTICE,
				"LOG_INFO", 	LOG_INFO,
				"LOG_ALL",	LOG_ALL
				);

	/* if statistics file specified, enable statistics logging - otherwise disable  - log_statistics also controlled further below*/
	CONFIG_KEY_TRIGGER("global:statistics_file",rtOpts->statisticsLog.logEnabled,TRUE,FALSE);
	CONFIG_KEY_TRIGGER("global:statistics_file",rtOpts->logStatistics,TRUE,FALSE);
	CONFIG_MAP_CHARARRAY("global:statistics_file",rtOpts->statisticsLog.logPath,rtOpts->statisticsLog.logPath,
		"Specify statistics log file path. Setting this enables logging of \n"
	"	 statistics, but can be overriden with global:log_statistics.");

	CONFIG_MAP_INT_MIN("global:statistics_log_interval",rtOpts->statisticsLogInterval,rtOpts->statisticsLogInterval,
		 "Log timing statistics every n seconds for Sync and Delay messages\n"
	"	 (0 - log all).",0);

	CONFIG_MAP_INT_MIN("global:statistics_file_max_size",rtOpts->statisticsLog.maxSize,rtOpts->statisticsLog.maxSize,
		"Maximum statistics log file size (in kB) - log file will be truncated\n"
	"	 if size exceeds the limit. 0 - no limit.", 0);

	CONFIG_MAP_INT_RANGE("global:statistics_file_max_files",rtOpts->statisticsLog.maxFiles,rtOpts->statisticsLog.maxFiles,
		"Enable log rotation of the statistics file up to n files. 0 - do not rotate.", 0, 100);

	CONFIG_MAP_BOOLEAN("global:statistics_file_truncate",rtOpts->statisticsLog.truncateOnReopen,rtOpts->statisticsLog.truncateOnReopen,
		"Truncate the statistics file every time it is (re) opened: startup and SIGHUP.");

	CONFIG_MAP_BOOLEAN("global:dump_packets",rtOpts->displayPackets,rtOpts->displayPackets,
		"Dump the contents of every PTP packet");

	/* this also checks if the verbose_foreground flag is set correctly */
	CONFIG_MAP_BOOLEAN("global:verbose_foreground",rtOpts->nonDaemon,rtOpts->nonDaemon,
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

	CONFIG_MAP_BOOLEAN("global:foreground",rtOpts->nonDaemon,rtOpts->nonDaemon,
		"Run in foreground - ignored when global:verbose_foreground is set");

	if(CONFIG_ISTRUE("global:verbose_foreground")) {
		rtOpts->nonDaemon = TRUE;
	}

	/* If this is processed after verbose_foreground, we can still control logStatistics */
	CONFIG_MAP_BOOLEAN("global:log_statistics", rtOpts->logStatistics, rtOpts->logStatistics,
		"Log timing statistics for every PTP packet received\n");

	CONFIG_MAP_SELECTVALUE("global:statistics_timestamp_format", rtOpts->statisticsTimestamp, rtOpts->statisticsTimestamp,
		"Timestamp format used when logging timing statistics\n"
	"        (when global:log_statistics is enabled):\n"
	"        datetime - formatttted date and time: YYYY-MM-DD hh:mm:ss.uuuuuu\n"
	"        unix - Unix timestamp with nanoseconds: s.ns\n"
	"        both - Formatted date and time, followed by unix timestamp\n"
	"               (adds one extra field  to the log)\n",
		"datetime",	TIMESTAMP_DATETIME,
		"unix",		TIMESTAMP_UNIX,
		"both",		TIMESTAMP_BOTH
		);

	/* If statistics file is enabled but logStatistics isn't, disable logging to file */
	CONFIG_KEY_CONDITIONAL_TRIGGER(rtOpts->statisticsLog.logEnabled && !rtOpts->logStatistics,
					rtOpts->statisticsLog.logEnabled, FALSE, rtOpts->statisticsLog.logEnabled);

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H)
	CONFIG_MAP_INT_RANGE("global:cpuaffinity_cpucore",rtOpts->cpuNumber,rtOpts->cpuNumber,
		"Bind "PTPD_PROGNAME" process to a selected CPU core number.\n"
	"        0 = first CPU core, etc. -1 = do not bind to a single core.",
	-1,255);
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H */

	CONFIG_MAP_INT_RANGE("global:statistics_update_interval",rtOpts->statsUpdateInterval,
								rtOpts->statsUpdateInterval,
		"Clock synchronisation statistics update interval in seconds\n", 1, 60);

	CONFIG_MAP_BOOLEAN("global:periodic_updates", rtOpts->periodicUpdates, rtOpts->periodicUpdates,
		"Log a status update every time statistics are updated (global:statistics_update_interval).\n"
	"        The updates are logged even when ptpd is configured without statistics support");

#ifdef PTPD_STATISTICS

	CONFIG_CONDITIONAL_ASSERTION( rtOpts->servoStabilityDetection && (
				    (rtOpts->statsUpdateInterval * rtOpts->servoStabilityPeriod) / 60 >=
				    rtOpts->servoStabilityTimeout),
			"The configured servo stabilisation timeout has to be longer than\n"
		"	 servo stabilisation period");

#endif /* PTPD_STATISTICS */

	CONFIG_MAP_INT_RANGE("global:timingdomain_election_delay",rtOpts->electionDelay,rtOpts->electionDelay,
		" Delay (seconds) before releasing a time service (NTP or PTP)"
	"        and electing a new one to control a clock. 0 = elect immediately\n", 0, 3600);


/* ===== ntpengine section ===== */

	CONFIG_MAP_BOOLEAN("ntpengine:enabled",rtOpts->ntpOptions.enableEngine,rtOpts->ntpOptions.enableEngine,
	"Enable NTPd integration");

	CONFIG_MAP_BOOLEAN("ntpengine:control_enabled",rtOpts->ntpOptions.enableControl,rtOpts->ntpOptions.enableControl,
	"Enable control over local NTPd daemon");

	CONFIG_KEY_DEPENDENCY("ntpengine:control_enabled", "ntpengine:enabled");

	CONFIG_MAP_INT_RANGE("ntpengine:check_interval",rtOpts->ntpOptions.checkInterval,
								rtOpts->ntpOptions.checkInterval,
		"NTP control check interval in seconds\n", 5, 600);

	CONFIG_MAP_INT_RANGE("ntpengine:key_id",rtOpts->ntpOptions.keyId, rtOpts->ntpOptions.keyId,
		 "NTP key number - must be configured as a trusted control key in ntp.conf,\n"
	"	  and be non-zero for the ntpengine:control_enabled setting to take effect.\n", 0, 65535);

	CONFIG_MAP_CHARARRAY("ntpengine:key",rtOpts->ntpOptions.key, rtOpts->ntpOptions.key,
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
	    DBG("Automatic lock file name is: %s\n",rtOpts->lockFile);
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

	if(!IS_QUIET()) {

		findUnknownSettings(dict, target);

		if (parseResult)
			INFO("Configuration OK\n");
		    else
			ERROR("There are errors in the configuration - see previous messages\n");
	}

	if (!HELP_ON() && !IS_SHOWDEFAULT() && parseResult) {
		return target;
	}

	dictionary_del(target);
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
		ERROR("Could not load configuration file: %s\n",rtOpts->configFile);
		return FALSE;
	}

	/* make sure the %x% special keys are unset */
	HELP_END();

	dictionary_merge( dict, *target, 0, NULL);
	dictionary_del(dict);

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

	/* make sure the %x% special keys are unset */
	HELP_END();

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

	SET_SHOWDEFAULT();
	/* NULL will always be returned in this mode */
	parseConfig(dict, &rtOpts);
	END_SHOWDEFAULT();
	dictionary_del(dict);

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

	HELP_START();
	printf("\n============== Full list of "PTPD_PROGNAME" settings ========\n\n");

	/* NULL will always be returned in this mode */
	parseConfig(dict, &rtOpts);

	HELP_END();
	dictionary_del(dict);

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

	loadDefaultSettings(&rtOpts);
	dict = dictionary_new(0);

	HELP_ITEM(key);

	printf("\n");
	/* NULL will always be returned in this mode */
	parseConfig(dict, &rtOpts);
	
	if(!HELP_ITEM_FOUND())
	    printf("Unknown setting: %s\n\n", key);
	printf("Use -H or --long-help to show help for all settings.\n\n");
	HELP_END();
	
	dictionary_del(dict);
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
	int opt_index = 0;

	*ret = 0;

	/* there's NOTHING wrong with this */
	if(argc==1) {
			*ret = 1;
			goto short_help;
	}

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
	    {"unicast",		optional_argument, 0, 'U'},
	    {"unicast-negotiation",		optional_argument, 0, 'g'},
	    {"unicast-destinations",		required_argument, 0, 'u'},
	    {0,			0		 , 0, 0}
	};

	while ((c = getopt_long(argc, argv, "?c:kb:i:d:sgmGMWyUu:nf:S:r:DvCVHhe:Y:tOLEPAaR:l:p", long_options, &opt_index)) != -1) {
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
		case 't':
			WARN_DEPRECATED('t', 'n', "noadjust", "clock:no_adjust");
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
#endif
			"\n");

			return FALSE;
		/* run in foreground */
		case 'C':
			dictionary_set(dict,"global:foreground", "Y");
			break;
		/* verbose mode */
		case 'V':
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
			"Note 2: -U and -g options have been re-purposed in 2.3.1\n."
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
 * Iterate through every key in newDict and compare to oldDict,
 * return TRUE if both equal, otherwise FALSE;
 */
Boolean
compareConfig(dictionary* newDict, dictionary* oldDict)
{

    int i = 0;

    if( newDict == NULL || oldDict == NULL) return -1;

    for(i = 0; i < newDict->n; i++) {

        if(newDict->key[i] == NULL)
            continue;

	if(strcmp(newDict->val[i], dictionary_get(oldDict, newDict->key[i],"")) != 0 ) {
		DBG("Setting %s changed from '%s' to '%s'\n", newDict->key[i],dictionary_get(oldDict, newDict->key[i],""),
		    newDict->val[i]);
		return FALSE;
	}
    }

return TRUE;

}

/* Compare two configurations and set flags to mark components requiring restart */
int checkSubsystemRestart(dictionary* newConfig, dictionary* oldConfig)
{

	UInteger32 restartFlags = 0;



/* Settings not requiring component restarts are commented out - reduces number of macro calls */

/*
 * Log files are reopened and / or re-created on SIGHUP anyway,
 * so new log config is automatically applied - nothing is marked
 * for restart if logging configuration changed
 */

        COMPONENT_RESTART_REQUIRED("ptpengine:interface",     		PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:backup_interface",     	PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:preset",  		PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:ip_mode",       		PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_negotiation",  		PTPD_RESTART_PROTOCOL);
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_grant_duration",  		PTPD_RESTART_PROTOCOL);
//        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_negotiation_listening",  		PTPD_RESTART_NONE );
        COMPONENT_RESTART_REQUIRED("ptpengine:disable_bmca",  		PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:transport",     		PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:dot2as",     		PTPD_UPDATE_DATASETS );

#ifdef PTPD_PCAP
        COMPONENT_RESTART_REQUIRED("ptpengine:use_libpcap",   		PTPD_RESTART_NETWORK );
#endif
        COMPONENT_RESTART_REQUIRED("ptpengine:delay_mechanism",        	PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:domain",    		PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:port_number",    		PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:any_domain",    		PTPD_RESTART_PROTOCOL );
//        COMPONENT_RESTART_REQUIRED("ptpengine:inbound_latency",       PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:outbound_latency",      PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:offset_shift",      	PTPD_RESTART_NONE );

        COMPONENT_RESTART_REQUIRED("ptpengine:pid_as_clock_identity", 	PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:ptp_slaveonly",           PTPD_RESTART_PROTOCOL );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_announce_interval",   PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_announce_interval_max",   PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:announce_receipt_timeout",        PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_sync_interval",     	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_sync_interval_max",     	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:master_igmp_refresh_interval",     	PTPD_RESTART_PROTOCOL );
//        COMPONENT_RESTART_REQUIRED("ptpengine:log_delayreq_interval_initial",PTPD_RESTART_NONE );
#ifdef DBG_SIGUSR2_DUMP_COUNTERS
//        COMPONENT_RESTART_REQUIRED("ptpengine:sigusr2_clears_counters",      	PTPD_RESTART_NONE );
#endif /* DBG_SIGUSR2_DUMP_COUNTERS */

        COMPONENT_RESTART_REQUIRED("ptpengine:log_delayreq_interval",   PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_delayreq_interval_max",   PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_delayreq_override",   PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:log_delayreq_auto",   	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:foreignrecord_capacity", 	PTPD_RESTART_DAEMON );
        COMPONENT_RESTART_REQUIRED("ptpengine:ptp_allan_variance",    	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:ptp_clock_accuracy",    	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:utc_offset",        	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:utc_offset_valid",       	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:time_traceable",        	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:frequency_traceable",     PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:ptp_timescale",        	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:ptp_timesource",        	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:clock_class",       	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:priority1",         	PTPD_UPDATE_DATASETS );
        COMPONENT_RESTART_REQUIRED("ptpengine:priority2",         	PTPD_UPDATE_DATASETS );
//        COMPONENT_RESTART_REQUIRED("ptpengine:max_listen",         	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:always_respect_utc_offset", PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:prefer_utc_offset_valid", PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:require_utc_offset_valid", PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:announce_receipt_grace_period", PTPD_RESTART_NONE );
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_destinations",   	PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_domains",   	PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_local_preference",   	PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:unicast_peer_destination",   	PTPD_RESTART_NETWORK );
//        COMPONENT_RESTART_REQUIRED("ptpengine:management_enable",         	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:management_set_enable",         	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:igmp_refresh",         	PTPD_RESTART_NONE );
        COMPONENT_RESTART_REQUIRED("ptpengine:multicast_ttl",        		PTPD_RESTART_NETWORK );
        COMPONENT_RESTART_REQUIRED("ptpengine:ip_dscp",        		PTPD_RESTART_NETWORK );

#ifdef PTPD_SNMP
        COMPONENT_RESTART_REQUIRED("global:enable_snmp",       	PTPD_RESTART_DAEMON );
#endif /* PTPD_SNMP */

#ifdef PTPD_STATISTICS

	COMPONENT_RESTART_REQUIRED("ptpengine:sync_stat_filter_enable",		PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:sync_stat_filter_type",		PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:sync_stat_filter_window",		PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:sync_stat_filter_window_type",PTPD_RESTART_FILTERS );


	COMPONENT_RESTART_REQUIRED("ptpengine:delay_stat_filter_enable",	 PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:delay_stat_filter_type",		 PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:delay_stat_filter_window",	 PTPD_RESTART_FILTERS );
	COMPONENT_RESTART_REQUIRED("ptpengine:delay_stat_filter_window_type",PTPD_RESTART_FILTERS );

	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_enable",     PTPD_RESTART_FILTERS );
        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_always_filter",PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_action",    	PTPD_RESTART_NONE );
          COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_capacity",  	PTPD_RESTART_FILTERS );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_threshold",  PTPD_RESTART_NONE );
          COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_enable",  PTPD_RESTART_FILTERS );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_minpercent",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_maxpercent",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_step",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_minthreshold",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_autotune_maxthreshold",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_weight",  PTPD_RESTART_NONE );
	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_stepdetect_enable",PTPD_RESTART_FILTERS);
//	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_stepdetect_threshold", PTPD_RESTART_NONE);
//	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_stepdetect_level", PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_stepdetect_credit", PTPD_RESTART_FILTERS);
//	COMPONENT_RESTART_REQUIRED("ptpengine:delay_outlier_filter_stepdetect_credit_increment", PTPD_RESTART_NONE);


          COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_enable",      PTPD_RESTART_FILTERS );

//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_action",    	PTPD_RESTART_NONE );
          COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_capacity",  	PTPD_RESTART_FILTERS );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_threshold",  	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_weight",  	PTPD_RESTART_NONE );
          COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_enable",  PTPD_RESTART_FILTERS );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_minpercent",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_maxpercent",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_minthreshold",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_maxthreshold",  PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_autotune_step",  PTPD_RESTART_NONE );
	COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_stepdetect_enable",PTPD_RESTART_FILTERS);
//	COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_stepdetect_threshold", PTPD_RESTART_NONE);
//	COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_stepdetect_level", PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_stepdetect_credit", PTPD_RESTART_FILTERS);
//	COMPONENT_RESTART_REQUIRED("ptpengine:sync_outlier_filter_stepdetect_credit_increment", PTPD_RESTART_NONE);


//        COMPONENT_RESTART_REQUIRED("ptpengine:calibration_delay",  	PTPD_RESTART_NONE );

//        COMPONENT_RESTART_REQUIRED("servo:max_delay_stable_only",    	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:max_delay_max_rejected",    	PTPD_RESTART_NONE );

#endif /* PTPD_STATISTICS */

	COMPONENT_RESTART_REQUIRED("ptpengine:timing_acl_permit",		PTPD_RESTART_ACLS);
	COMPONENT_RESTART_REQUIRED("ptpengine:timing_acl_deny",			PTPD_RESTART_ACLS);
	COMPONENT_RESTART_REQUIRED("ptpengine:management_acl_permit",		PTPD_RESTART_ACLS);
	COMPONENT_RESTART_REQUIRED("ptpengine:management_acl_deny",		PTPD_RESTART_ACLS);
	COMPONENT_RESTART_REQUIRED("ptpengine:timing_acl_order",		PTPD_RESTART_ACLS);
	COMPONENT_RESTART_REQUIRED("ptpengine:management_acl_order",		PTPD_RESTART_ACLS);

//        COMPONENT_RESTART_REQUIRED("ptpengine:panic_mode",  	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:panic_mode_duration",  	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("ptpengine:panic_mode_exit_threshold",  	PTPD_RESTART_NONE );

//        COMPONENT_RESTART_REQUIRED("clock:no_adjust",    		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:no_reset",     		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:step_startup_force",     		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:step_startup",     		PTPD_RESTART_NONE );

#ifdef HAVE_LINUX_RTC_H
//        COMPONENT_RESTART_REQUIRED("clock:set_rtc_on_step",  		PTPD_RESTART_NONE );
#endif /* HAVE_LINUX_RTC_H */
//        COMPONENT_RESTART_REQUIRED("clock:drift_file",   		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:drift_file",   		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:drift_handling",       	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:leap_seconds_file",		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:leap_second_pause_period",	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:leap_second_notice_period",	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("clock:max_offset_ppm",       	PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:owdfilter_stiffness",         PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:kp",   			PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:ki",   			PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:dt_method",			PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:dt_max",			PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("servo:max_delay",    		PTPD_RESTART_NONE );

//        COMPONENT_RESTART_REQUIRED("servo:max_offset",   		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:use_syslog", 		PTPD_RESTART_LOGGING );

#ifdef PTPD_STATISTICS
//		COMPONENT_RESTART_REQUIRED("servo:stability_detection", PTPD_RESTART_NONE );
//		COMPONENT_RESTART_REQUIRED("servo:stability_threshold", PTPD_RESTART_NONE );
//		COMPONENT_RESTART_REQUIRED("servo:stability_period", PTPD_RESTART_NONE );
//		COMPONENT_RESTART_REQUIRED("servo:stability_timeout", PTPD_RESTART_NONE );
#endif


        COMPONENT_RESTART_REQUIRED("global:lock_file",   		PTPD_RESTART_DAEMON );
        COMPONENT_RESTART_REQUIRED("global:auto_lockfile",   		PTPD_RESTART_DAEMON );
        COMPONENT_RESTART_REQUIRED("global:lock_directory",   		PTPD_RESTART_DAEMON );
        COMPONENT_RESTART_REQUIRED("global:ignore_lock", 		PTPD_RESTART_DAEMON );
//        COMPONENT_RESTART_REQUIRED("global:quality_file",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:quality_file_max_size",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:quality_file_truncate",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:quality_file_max_files",		PTPD_RESTART_LOGGING );
        COMPONENT_RESTART_REQUIRED("global:log_file",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:log_file_max_size",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:log_file_truncate",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:log_file_file_max_files",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:status_update_interval",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:periodic_updates",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:status_file",			PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:log_level",		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:debug_level",		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:statistics_file",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:statistics_file_max_size",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:statistics_file_truncate",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:statistics_file_max_files",		PTPD_RESTART_LOGGING );
//        COMPONENT_RESTART_REQUIRED("global:statistics_log_interval",		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:log_statistics",			PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:statistics_timestamp_format",		PTPD_RESTART_NONE );
//        COMPONENT_RESTART_REQUIRED("global:dump_packets",		PTPD_RESTART_NONE );

//        COMPONENT_RESTART_REQUIRED("ptpengine:sync_sequence_checking",	PTPD_RESTART_NONE );
        COMPONENT_RESTART_REQUIRED("ptpengine:clock_update_timeout",		PTPD_RESTART_PROTOCOL );

#ifdef PTPD_STATISTICS
//		COMPONENT_RESTART_REQUIRED("global:statistics_update_interval", PTPD_RESTART_NONE );
#endif
        COMPONENT_RESTART_REQUIRED("global:foreground", 		PTPD_RESTART_DAEMON );
        COMPONENT_RESTART_REQUIRED("global:verbose_foreground",		PTPD_RESTART_DAEMON );

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H)
        COMPONENT_RESTART_REQUIRED("global:cpuaffinity_cpucore",	PTPD_CHANGE_CPUAFFINITY );
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H */

        COMPONENT_RESTART_REQUIRED("global:timingdomain_election_delay",	PTPD_RESTART_NONE );

	COMPONENT_RESTART_REQUIRED("ptpengine:panic_mode_release_clock",	PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ptpengine:idle_timeout",		PTPD_RESTART_NONE);

	COMPONENT_RESTART_REQUIRED("ntpengine:enabled",			PTPD_RESTART_NTPENGINE);
	COMPONENT_RESTART_REQUIRED("ptpengine:ntp_failover",		PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ptpengine:ntp_failover_timeout",	PTPD_RESTART_NTPCONFIG);
	COMPONENT_RESTART_REQUIRED("ptpengine:prefer_ntp",		PTPD_RESTART_NTPCONFIG);
	COMPONENT_RESTART_REQUIRED("ptpengine:panic_mode_ntp",		PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ntpengine:control_enabled",		PTPD_RESTART_NONE);
	COMPONENT_RESTART_REQUIRED("ntpengine:check_interval",		PTPD_RESTART_NTPCONFIG);

/* ========= Any additional logic goes here =========== */

	/* Set of possible PTP port states has changed */
	if(SETTING_CHANGED("ptpengine:slave_only") ||
		SETTING_CHANGED("ptpengine:clock_class")) {

		int clockClass_old = iniparser_getint(oldConfig,"ptpengine:ptp_clockclass",0);
		int clockClass_new = iniparser_getint(newConfig,"ptpengine:ptp_clockclass",0);

		/*
		 * We're changing from a mode where slave state is possible
		 * to a master only mode, or vice versa
		 */
		if ((clockClass_old < 128 && clockClass_new > 127) ||
		(clockClass_old > 127 && clockClass_new < 128)) {

			/* If we are in auto lockfile mode, trigger a check if other locks match */
			if( !SETTING_CHANGED("global:auto_lockfile") &&
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
