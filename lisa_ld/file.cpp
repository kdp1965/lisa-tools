// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : parser.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Parser framework for assembler.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "file.h"
#include "errors.h"

#define     iswhite(a)  (((a) == ' ') || ((a) == '\t'))

/* 
=============================================================================
Constructor
=============================================================================
*/
CFile::CFile(CParseCtx* pSpec)
{
    m_pSpec = pSpec;
    m_ActiveSection = NULL;
}

/* 
=============================================================================
Parse arguments from the line
=============================================================================
*/
int CFile::ParseArgs(char *sLine, CParserFile *pFile)
{
    char*               sToken;
    char*               sNextToken;
    CFileSection      * pSection;

    m_Argc = 0;
    // Parse out the full request
    sToken = strtok_r(sLine, " \t", &sNextToken);
    if (strlen(sToken) > 1)
    {
        printf("%s: Line %d: Expected single character command specifier, got %s\n",
                pFile->m_Filename.c_str(), pFile->m_Line, sToken);
        return ERROR_INVALID_SYNTAX;
    }
    m_Args[m_Argc++] = sToken;

    sToken = strtok_r(NULL, " \t", &sNextToken);
    while (sToken && m_Argc < 8)
    {
        m_Args[m_Argc++] = sToken;
        sToken = strtok_r(NULL, " \t", &sNextToken);
    }

    return ERROR_NONE;
}

/* 
=============================================================================
Parse section line from file
=============================================================================
*/
int CFile::ParseSection(CParserFile *pFile)
{
    CFileSection      * pSection;

    // Parse out the full request
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected section name after 's'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Create a new section and make it active
    pSection = new CFileSection();
    pSection->m_Filename = pFile->m_Filename;
    pSection->m_Name = m_Args[1];
    m_ActiveSection = pSection;
    m_FileSections.insert(std::pair<std::string, CFileSection *>(m_Args[1], pSection));
    return ERROR_NONE;
}

/* 
=============================================================================
Parse public line from file
=============================================================================
*/
int CFile::ParsePublic(CParserFile *pFile)
{
    std::string         name;
    int                 address;

    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Public label declaration must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the label name
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected label name after 'p'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }
    name = m_Args[1];

    // Get the label address
    if (m_Argc < 3)
    {
        printf("%s: Line %d: Expected label address after name\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }
    address = strtol(m_Args[2].c_str(), NULL, 0);

    m_ActiveSection->m_PublicLabels.insert(std::pair<std::string, int>(name, address));
    return ERROR_NONE;
}

/* 
=============================================================================
Parse local line from file
=============================================================================
*/
int CFile::ParseLocal(CParserFile *pFile)
{
    std::string         name;
    int                 address;

    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Local label declaration must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the label name
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected label name after 'l'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }
    name = m_Args[1];

    // Get the label address
    if (m_Argc < 3)
    {
        printf("%s: Line %d: Expected label address after name\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }
    address = strtol(m_Args[2].c_str(), NULL, 0);

    m_ActiveSection->m_LocalLabels.insert(std::pair<std::string, int>(name, address));
    return ERROR_NONE;
}

/* 
=============================================================================
Parse address line from file
=============================================================================
*/
int CFile::ParseAddress(CParserFile *pFile)
{
    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Address specification must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the address
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected address after 'a'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }
    m_ActiveSection->m_Address = strtol(m_Args[1].c_str(), NULL, 0);

    return ERROR_NONE;
}

/* 
=============================================================================
Parse instruction line from file
=============================================================================
*/
int CFile::ParseInstruction(CParserFile *pFile)
{
    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Instructions must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the address
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected instruction opcode after 'i'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Keep track of the first valid instruction address / offset
    if (m_ActiveSection->m_Address < m_ActiveSection->m_FirstCodeOffset)
         m_ActiveSection->m_FirstCodeOffset = m_ActiveSection->m_Address;

    m_ActiveSection->m_pCode[m_ActiveSection->m_Address++] =
        strtol(m_Args[1].c_str(), NULL, 0);

    // Keep track of the last valid instruction address / offset
    if (m_ActiveSection->m_Address > m_ActiveSection->m_LastCodeOffset)
         m_ActiveSection->m_LastCodeOffset = m_ActiveSection->m_Address;

    return ERROR_NONE;
}

/* 
=============================================================================
Parse extern line from file
=============================================================================
*/
int CFile::ParseExtern(CParserFile *pFile)
{
    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Externs must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the address
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected opcode after 'e'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the label reference
    if (m_Argc < 3)
    {
        printf("%s: Line %d: Expected label after the opcode\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Keep track of the first valid instruction address / offset
    if (m_ActiveSection->m_Address < m_ActiveSection->m_FirstCodeOffset)
         m_ActiveSection->m_FirstCodeOffset = m_ActiveSection->m_Address;
    
    // Create a new Relocation object and add it to the externs list
    CRelocation *pRel = new CRelocation;
    pRel->m_Type = REL_TYPE_EXTERN;
    pRel->m_Label = m_Args[2];
    pRel->m_Opcode = strtol(m_Args[1].c_str(), NULL, 0);
    pRel->m_Offset = m_ActiveSection->m_Address;
    m_ActiveSection->m_pCode[m_ActiveSection->m_Address++] = pRel->m_Opcode;
    m_ActiveSection->m_ExternsList.push_back(pRel);

    // Keep track of the last valid instruction address / offset
    if (m_ActiveSection->m_Address > m_ActiveSection->m_LastCodeOffset)
         m_ActiveSection->m_LastCodeOffset = m_ActiveSection->m_Address;

    return ERROR_NONE;
}

/* 
=============================================================================
Parse a relocation line from file
=============================================================================
*/
int CFile::ParseRelocation(CParserFile *pFile)
{
    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Externs must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Get the address
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected opcode after '%s'\n",
                pFile->m_Filename.c_str(), pFile->m_Line, m_Args[0].c_str());
        return ERROR_INVALID_SYNTAX;
    }

    // Get the label reference
    if (m_Argc < 3)
    {
        printf("%s: Line %d: Expected label after the opcode\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Keep track of the first valid instruction address / offset
    if (m_ActiveSection->m_Address < m_ActiveSection->m_FirstCodeOffset)
         m_ActiveSection->m_FirstCodeOffset = m_ActiveSection->m_Address;

    // Create a new Relocation object and add it to the externs list
    CRelocation *pRel = new CRelocation;
    pRel->m_Type = m_Args[0][0] == 'R' ? REL_TYPE_SYMBOL : REL_TYPE_FUNCTION;
    pRel->m_Section = m_Args[2];
    pRel->m_Opcode = strtol(m_Args[1].c_str(), NULL, 0);
    pRel->m_Offset = m_ActiveSection->m_Address;
    m_ActiveSection->m_pCode[m_ActiveSection->m_Address++] = pRel->m_Opcode;
    m_ActiveSection->m_RelocationList.push_back(pRel);

    // Keep track of the last valid instruction address / offset
    if (m_ActiveSection->m_Address > m_ActiveSection->m_LastCodeOffset)
         m_ActiveSection->m_LastCodeOffset = m_ActiveSection->m_Address;

    return ERROR_NONE;
}

/* 
=============================================================================
Parse a ParseUninitializedAlloc line from file
=============================================================================
*/
int CFile::ParseUninitializedAlloc(CParserFile *pFile)
{
    // Validate we have an active section
    if (m_ActiveSection == NULL)
    {
        printf("%s: Line %d: Externs must be in a section\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Validate a size was provided
    if (m_Argc < 2)
    {
        printf("%s: Line %d: Expected storage size qualifier after 'u'\n",
                pFile->m_Filename.c_str(), pFile->m_Line);
        return ERROR_INVALID_SYNTAX;
    }

    // Keep track of the first valid instruction address / offset
    if (m_ActiveSection->m_Address < m_ActiveSection->m_FirstCodeOffset)
         m_ActiveSection->m_FirstCodeOffset = m_ActiveSection->m_Address;

    // Simply advance the section's address the specified amount
    m_ActiveSection->m_Address += strtol(m_Args[1].c_str(), NULL, 0);

    // Keep track of the last valid instruction address / offset
    if (m_ActiveSection->m_Address > m_ActiveSection->m_LastCodeOffset)
         m_ActiveSection->m_LastCodeOffset = m_ActiveSection->m_Address;

    return ERROR_NONE;
}

/* 
=============================================================================
Parse next line from the file
=============================================================================
*/
int CFile::ParseLine(const char *pLine, CParserFile *pFile)
{
    char    sMutable[512];
    int     ret = ERROR_NONE;

    // Remove leading spaces
    while (iswhite(*pLine))
        pLine++;
    if (strlen(pLine) == 0)
        return ERROR_NONE;

    // Copy string to mutable
    strncpy(sMutable, pLine, sizeof sMutable);

    // Parse the args
    if ((ret = ParseArgs(sMutable, pFile)) != ERROR_NONE)
        return ret;

    // Process based on command specifier
    switch (m_Args[0][0])
    {
        // Section specifier
        case 's':
            ret = ParseSection(pFile);
            break;

        // Public label specifier
        case 'p':
            ret = ParsePublic(pFile);
            break;

        // Local label specifier
        case 'l':
            ret = ParseLocal(pFile);
            break;

        // Address specifier
        case 'a':
            ret = ParseAddress(pFile);
            break;

        // Instruction specifier
        case 'i':
            ret = ParseInstruction(pFile);
            break;

        // Extern label specifier
        case 'e':
            ret = ParseExtern(pFile);
            break;

        // Relocation specifier
        case 'r':
        case 'R':
            ret = ParseRelocation(pFile);
            break;

        // Uninitialized data specifier
        case 'u':
            ret = ParseUninitializedAlloc(pFile);
            break;

        default:
            printf("%s: Line %d: Unknown relocation request %c\n",
                    pFile->m_Filename.c_str(), pFile->m_Line, *sMutable);
            ret = ERROR_INVALID_SYNTAX;
            break;
    }

    return ret;
}

/* 
=============================================================================
Load a relocatable file
=============================================================================
*/
int CFile::LoadRelFile(const char *pFilename)
{
    CParserFile     file;
    char            sLine[512];
    FILE*           fd;
    int32_t         err, lastErr;
    bool            parseFailed;
    char*           pComment;
    int             c;
    int             numOnLine;

    // Try to open the file
    if ((fd = fopen(pFilename, "r")) == NULL)
    {
        printf("Error opening file %s\n", pFilename);
        return ERROR_CANT_OPEN_FILE;
    }

    // Initialize the CFileSection and CParserFile
    file.m_Filename = pFilename;
    file.m_Line = 0;

    // Read all lines from the input file and parse each
    parseFailed = false;
    lastErr = ERROR_NONE;
    while (fgets(sLine, sizeof sLine, fd) != NULL)
    {
        // Increment the line number for this file
        file.m_Line++;

        // Remove trailing \n if it exists
        int len = strlen(sLine);
        if (sLine[len - 1] == '\n')
            sLine[len - 1] = '\0';

        // Search for a comment in the line "//" and remove it
        if ((pComment = strstr(sLine, "#")) != NULL)
            *pComment = '\0';

        // Remove trailing whitespace
        while (iswhite(sLine[strlen(sLine)-1]))
            sLine[strlen(sLine)-1] = 0;

        // Test for empty line
        if (strlen(sLine) == 0)
            continue;

        // Parse the line
        if ((err = ParseLine(sLine, &file)) != ERROR_NONE)
        {
            parseFailed = true;
            lastErr = err;
        }
    }

    // Close the file
    fclose(fd);

    if (m_DebugLevel > 1)
    {
        auto it = m_FileSections.begin();
        while (it != m_FileSections.end())
        {
            printf("Section: %s\n", it->second->m_Name.c_str());

            // Print all public labels
            auto pit = it->second->m_PublicLabels.begin();
            while (pit != it->second->m_PublicLabels.end())
            {
                printf("  PUBLIC: %s\n", pit->first.c_str());
                pit++;
            }

            // Print all extern references
            auto eit = it->second->m_ExternsList.begin();
            while (eit != it->second->m_ExternsList.end())
            {
                printf("  EXTERN: %s\n", (*eit)->m_Label.c_str());
                eit++;
            }

            // Print all relocation references
            auto rit = it->second->m_RelocationList.begin();
            while (rit != it->second->m_RelocationList.end())
            {
                printf("  RELOC:  0x%04X  %s\n", (*rit)->m_Opcode, (*rit)->m_Label.c_str());
                rit++;
            }

            // Print all opcode values
            if (it->second->m_FirstCodeOffset != 0xFFFFFF)
            {
                printf("  CODE:  0x%04X - 0x%04X\n", it->second->m_FirstCodeOffset,
                        it->second->m_LastCodeOffset);
                numOnLine = 0;
                printf("    ");
                for (c = it->second->m_FirstCodeOffset; c < it->second->m_LastCodeOffset; c++)
                {
                    printf("0x%04X  ", it->second->m_pCode[c]);
                    numOnLine++;
                    if (numOnLine == 8)
                    {
                        printf("\n    ");
                        numOnLine = 0;
                    }
                }
                printf("\n");
            }
            else if (it->second->m_Address != 0)
            {
                printf("  DATA: 0x0000 - 0x%04X\n", it->second->m_Address);
            }

            it++;
        }
    }

    return lastErr;
}

// vim: sw=4 ts=4
