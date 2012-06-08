/**
 * @file MgmtMsgClient.cpp
 * Used as a glue for all of the client classes
 * 
 * @brief Main file for the PTPd Management Message Client
 */

#include <cstdlib>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Help.h"

using namespace std;

// TODO: 'verbose' flag handling
/* Flag set by ‘--verbose’. */
static int verbose_flag;

/*
 * 
 */
int main(int argc, char** argv) {
    int c;
    
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,    &verbose_flag, 1},
            {"brief",   no_argument,    &verbose_flag, 0},
            
            /* These options don't set a flag.
             * We distinguish them by their indices. */
            {"address", required_argument, 0, 'a'},
            {"message", required_argument, 0, 'm'},
            {"help",    no_argument,       0, 'h'},
            {"port",    required_argument, 0, 'p'},
            {0, 0, 0, 0}
        };
           
        /* getopt_long stores the option index here. */
        int option_index = 0;
     
        c = getopt_long (argc, argv, "a:m:hp:", long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
     
        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                
                if (optarg)
                    printf (" with arg %s\n", optarg);
                
               break;
     
            case 'h':
                printHelp(argv[0]);
                break;
     
            case 'a':
                printf ("option -a with value `%s'\n", optarg);
                break;
     
            case 'm':
                printf ("option -m with value `%s'\n", optarg);
                
                if (strcmp(optarg, "print") == 0)
                    printMgmtMsgsList();
                
                break;
     
            case 'p':
                printf ("option -p with value `%s'\n", optarg);
                break;
     
            case '?':
                /* getopt_long already printed an error message. */
                break;
     
            default:
                abort ();
        }
    }
    
    /* Instead of reporting ‘--verbose’
     * and ‘--brief’ as they are encountered,
     * we report the final status resulting from them. */
    if (verbose_flag)
        puts ("verbose flag is set");
     
    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    return 0;
}

