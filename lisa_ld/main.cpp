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
#include "linker.h"
#include "errors.h"

void usage(const char *name)
{
    printf("\nusage:  %s [-DglLo] input_file [input_file]...\n", name);
    printf("\nOptions:\n");
    printf("   -L path         Add path to the library dirctory search list\n");
    printf("   -l name         Add library to be linked\n");
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
    CLinker         linker(&spec);
    CFile         * pFile;
    uint32_t        err;
    char*           pIn = NULL;
    char*           pOut = NULL;
    char*           pLinkerScript = NULL;
    uint32_t        debugLevel = 0;
    bool            writeAll = false;
    bool            srecOutput = false;
    bool            binaryOutput = false;
    bool            mixed = false;
    bool            mapFile = false;
    int             c;

    // Test if resource script provided
    if (argc < 2)
    {
        usage(argv[0]);
        exit(1);
    }

    // Parse options
    while ((c = getopt(argc, argv, "D:g:hl:L:mMo:T:")) != -1)
    {
        switch (c)
        {
        case 'g':
            debugLevel = atoi(optarg);
            break;

        case 'M':
            mapFile = true;
            break;

        case 'D':
            linker.AddDefine(optarg);
            break;

        case 'L':
            linker.AddLibPath(optarg);
            break;

        case 'T':
            pLinkerScript = optarg;
            break;

        case 'o':
            // Indicate all data should be written
            pOut = optarg;
            break;

        case 'm':
            // Indicate all data should be written
            mixed = true;
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

    if (pLinkerScript == NULL)
    {
        printf("Pleae specify Linker Script with '-T filename' option\n");
        return 1;
    }

    // Parse the linker script
    parser.m_DebugLevel = debugLevel;
    if ((err = parser.ParseLinkerScript(pLinkerScript, &spec)) != ERROR_NONE)
        exit(err);

    if (optind == argc)
    {
        printf("Expected one or more input files!\n");
        return 1;
    }

    // The remaining arguments are input files.  Parse each one
    for (c = optind; c < argc; c++)
    {
        // Create a new CFile class for this file
        pFile = new CFile(&spec);
        pFile->m_Filename = argv[c];

        // Parse the resource script provided
        pFile->m_DebugLevel = debugLevel;
        if ((err = pFile->LoadRelFile(argv[c])) != ERROR_NONE)
            exit(err);

        // Add this file to our list
        linker.m_FileList.push_back(pFile);
    }

    // Link the program
    linker.m_DebugLevel = debugLevel;
    linker.m_Mixed = mixed;
    linker.m_MapFile = mapFile;
    err = linker.Link(pOut);
    if (err != ERROR_NONE)
        return err;

    return 0;
}

// vim: sw=4 ts=4
