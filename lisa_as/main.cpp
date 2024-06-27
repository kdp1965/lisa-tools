// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : main.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Main entry point for assembler framework.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#include "parser.h"
#include "assembler.h"
#include "errors.h"
#include "srec.h"
#include "intelhex.h"

void usage(const char *name)
{
    printf("\nusage:  %s [-DIgo] input_file [input_file]...\n", name);
    printf("\nOptions:\n");
    printf("   -I path         Add path to the include dirctory search list\n");
    printf("   -D name[=value] Define name in the define symbol table\n");
    printf("   -g level        Set the debug level\n");
    printf("   -o filename     Set the output filename\n\n");
}

/*
================================================================================
Main entry point for asm engine framework
================================================================================
*/
int main(int argc, char* argv[])
{
    CParseCtx       spec;
    CParser         parser(&spec);
    CAssembler      assembler;
    uint32_t        err;
    char*           pIn = NULL;
    char*           pOut = NULL;
    uint32_t        debugLevel = 0;
    bool            writeAll = false;
    bool            srecOutput = false;
    bool            binaryOutput = false;
    bool            mixed = false;
    int             width = 14;
    int             c;

    // Test if resource script provided
    if (argc < 2)
    {
        usage(argv[0]);
        exit(1);
    }

    // Parse options
    while ((c = getopt(argc, argv, "D:g:hI:mo:w:")) != -1)
    {
        switch (c)
        {
        case 'g':
            debugLevel = atoi(optarg);
            break;

        case 'D':
            parser.AddDefine(optarg);
            break;

        case 'I':
            parser.AddInclude(optarg);
            break;

        case 'o':
            // Indicate all data should be written
            pOut = optarg;
            break;

        case 'm':
            // Indicate all data should be written
            mixed = true;
            break;

        case 'w':
            width = atoi(optarg);
            break;

        case 'h':
            // Print the usage
            usage(argv[0]);
            return 0;

        case '?':
            if (optopt == 'g' || optopt == 'D' || optopt == 'I' || optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option '-%c'\n", optopt);
            else
                fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
            return 1;

        default:
            abort();
            break;
        }
    }

    if (pOut == NULL)
    {
       printf("Please specify output file with '-o filename' option\n");
       return 1;
    }

    parser.m_Width = width;
    assembler.m_Width = width;

    // The remaining arguments are input files.  Parse each one
    for (c = optind; c < argc; c++)
    {
       // Parse the resource script provided
       parser.m_DebugLevel = debugLevel;
       if ((err = parser.ParseFile(argv[c], &spec)) != ERROR_NONE)
           exit(err);

    }

    // Build the image
    assembler.m_DebugLevel = debugLevel;
    assembler.m_Mixed = mixed;
    err = assembler.Assemble(pOut, &spec);
    if (err != ERROR_NONE)
    {
        printf("Asm error = %d\n", err);
        return err;
    }

    return 0;
}
