// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : linker.cpp
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

#include "linker.h"
#include "errors.h"

/* 
=============================================================================
Constructor
=============================================================================
*/
CLinker::CLinker( CParseCtx *pSpec )
{
    m_pSpec = pSpec;
    m_DebugLevel = 0;
    m_Mixed = 0;
    m_MapFile = 0;
}

/* 
=============================================================================
Add a define to the defines map
=============================================================================
*/
void CLinker::AddDefine(const char *name)
{
    char sMutable[1024];
    const char *var;
    const char *value;

    // Copy to a mutable string
    strncpy(sMutable, name, sizeof(sMutable));

    // Split out value from define (this is a command line operation)
    var = strtok(sMutable, "=");
    value = strtok(NULL, "=");

    // If no value given, default it to 1
    if (value == NULL)
        value = "1";

    // Add to our variable list
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(var, value));
}

/* 
=============================================================================
Add an include path to the list of paths.
=============================================================================
*/
void CLinker::AddLibPath(const char *name)
{
    int         len;
    char        path[1024];

    // Copy to a local string
    strncpy(path, name, sizeof(path));

    // Add '/' to end if it doesn't already exist
    len = strlen(path);
    if (path[len-1] != '/')
        strcat(path, "/");

    // Add to our list of include paths
    m_pSpec->m_LibPaths.push_back(path);
}

/* 
=============================================================================
Locate segments by spec
=============================================================================
*/
int CLinker::LocateSectionsBySpec(CSection *pSection, COperation *pOp)
{
    int         searchMode = 0;
    char        str[pOp->m_StrParam.length()+1];
    int         err = ERROR_NONE;
    int         match;
    char        mapStr[256];

    if (m_DebugLevel > 0)
        printf("Locating sections with %s\n", pOp->m_StrParam.c_str());
    if (strncmp(pOp->m_StrParam.c_str(), "*(", 2) == 0)
    {
        // Search mode 1 is match last part of string
        searchMode = 1;
        strcpy(str, &pOp->m_StrParam.c_str()[2]);
        if (str[strlen(str)-1] == ')')
            str[strlen(str)-1] = 0;

        // Test if the string end with wildcard
        if (str[strlen(str)-1] == '*')
        {
            // SearchMode 2 is strstr
            searchMode = 2;
            str[strlen(str)-1] = 0;
        }
    }
    else 
        strcpy(str, pOp->m_StrParam.c_str());

    // Loop for all files
    auto fit = m_FileList.begin();
    int  len = strlen(str);
    while (fit != m_FileList.end())
    {
        // Iterate through all sections of this file
        auto sit = (*fit)->m_FileSections.begin();
        while (sit != (*fit)->m_FileSections.end())
        {
            // Test if this section already located
            if (sit->second->m_LocateAddress != -1)
            {
                // Skip this section
                sit++;
                continue;
            }

            match = 0;
            if (searchMode == 0)
            {
                if (strcmp(str, sit->first.c_str()) == 0)
                    match = 1;
            }
            else if (searchMode == 1)
            {
                const char *pStr = sit->first.c_str();
                if (sit->first.length() >= len && strcmp(str, &pStr[strlen(pStr) - len]) == 0)
                    match = 1;
            }
            else if (searchMode == 2)
            {
                if (strstr(str, sit->first.c_str()) != NULL)
                    match = 1;
            }

            if (match)
            {
                int     offset = pSection->m_pMem->m_Address;

                // Locate the section at the current Memory address
                if (m_DebugLevel > 0)
                    printf("   Adding %s at 0x%04X\n", sit->first.c_str(), pSection->m_pMem->m_Address);

                // If the section has an AT specifier, then it will be located there
                if (pSection->m_pAtMem)
                    sit->second->m_LocateAddress = pSection->m_pAtMem->m_Address;
                else
                    sit->second->m_LocateAddress = pSection->m_pMem->m_Address;

                // Update all PUBLIC symbol addresses in this section and add
                // them to our known label map
                auto pit = sit->second->m_PublicLabels.begin();
                while (pit != sit->second->m_PublicLabels.end())
                {
                    // Update the label address
                    pit->second += offset;

                    // Add the label to our variable list
                    auto vit = m_pSpec->m_Variables.find(pit->first);
                    if (vit != m_pSpec->m_Variables.end())
                    {
                        printf("%s: Label %s already defined!\n", (*fit)->m_Filename.c_str(),
                                pit->first.c_str());
                        err = ERROR_DUPLICATE_SYMBOL;
                    }
                    else
                    {
                        char  sVal[16];
                        sprintf(sVal, "%d", pit->second);
                        m_pSpec->m_Variables.insert(std::pair<std::string,
                                std::string>(pit->first, sVal));
                    }

                    sprintf(mapStr, "0x%04X %s", pit->second, pit->first.c_str());

                    // Add this label to either the CODE or DATA label list
                    if (strchr(pSection->m_pMem->m_Access.c_str(), 'x') != NULL)
                        m_CodeMapSymbols.push_back(mapStr);
                    else
                        m_DataMapSymbols.push_back(mapStr);

                    // Next public label
                    pit++;
                }

                // Update all LOCAL symbol addresses in this section
                auto lit = sit->second->m_LocalLabels.begin();
                while (lit != sit->second->m_LocalLabels.end())
                {
                    // Update the label address
                    lit->second += offset;

                    sprintf(mapStr, "0x%04X %s", lit->second, lit->first.c_str());

                    // Add this label to either the CODE or DATA label list
                    if (strchr(pSection->m_pMem->m_Access.c_str(), 'x') != NULL)
                        m_CodeMapSymbols.push_back(mapStr);
                    else
                        m_DataMapSymbols.push_back(mapStr);

                    // Next public label
                    lit++;
                }
                    
                // Perform relocations of this section with this file
                auto sit2 = (*fit)->m_FileSections.begin();
                while (sit2 != (*fit)->m_FileSections.end())
                {
                    // Iterate through all relocations
                    auto rit = sit2->second->m_RelocationList.begin();
                    while (rit != sit2->second->m_RelocationList.end())
                    {
                        // Test if this relocation is relative to this file section
                        if ((*rit)->m_Section == sit->first)
                        {
                            // Add the location offset to the opcode
                            (*rit)->m_Opcode += offset;
                            
                            // Save the resulting value to the code
                            sit2->second->m_pCode[(*rit)->m_Offset] = (*rit)->m_Opcode;
                        }

                        // Next relocation
                        rit++;
                    }

                    // Next section within this file
                    sit2++;
                }

                // Advance the Memory address by the section's size
                pSection->m_pMem->m_Address += sit->second->m_LastCodeOffset;
                sit->second->m_pLocateMem = pSection->m_pMem;

                // If this section has an AT specifier, advance that address also 
                if (pSection->m_pAtMem)
                {
                    pSection->m_pAtMem->m_Address += sit->second->m_LastCodeOffset;
                    sit->second->m_pLocateMem = pSection->m_pAtMem;
                }
            }

            // Next section in the file
            sit++;
        }

        // Next file
        fit++;
    }

    return err;
}

/* 
=============================================================================
Locate segments
=============================================================================
*/
int CLinker::LocateSections(void)
{
    COperation *pOp;
    char        name[256];
    char        str[256];
    char        mapStr[256];
    int         address;
    int         err = ERROR_NONE;

    // Iterate over all sections in the SectionList
    auto it = m_pSpec->m_SectionList.begin();
    while (it != m_pSpec->m_SectionList.end())
    {
        if (m_DebugLevel > 0)
            printf("Locating items into section %s\n", (*it)->m_Name.c_str());

        auto opit = (*it)->m_Ops.begin();
        while (opit != (*it)->m_Ops.end())
        {
            // Get pointer to the operation
            pOp = *opit;

            switch (pOp->m_Type)
            {
                case OP_ASSIGN_VAR:
                    // Get the memory region where of the address
                    if ((*it)->m_pAtMem)
                        address = (*it)->m_pAtMem->m_Address;
                    else
                        address = (*it)->m_pMem->m_Address;

                    // Provide the address as a variable
                    sprintf(str, "%d", address);
                    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(
                                pOp->m_StrParam, str));

                    // Add the variable to the map symbols
                    sprintf(mapStr, "0x%04X %s", address, pOp->m_StrParam.c_str());
                    if (strchr((*it)->m_pMem->m_Access.c_str(), 'x') != NULL || (*it)->m_pAtMem)
                        m_CodeMapSymbols.push_back(mapStr);
                    else
                        m_DataMapSymbols.push_back(mapStr);

                    // Provide %hi and %lo also
                    sprintf(name, "%%lo(%s)", pOp->m_StrParam.c_str());
                    sprintf(str, "%d", address & 0xFF);
                    m_pSpec->m_Variables.insert(std::pair<std::string,
                            std::string>(name, str));
                    sprintf(name, "%%hi(%s)", pOp->m_StrParam.c_str());
                    sprintf(str, "%d", (address >> 8) & 0xFF);
                    m_pSpec->m_Variables.insert(std::pair<std::string,
                            std::string>(name, str));
                    break;

                case OP_LOAD_SECTION:
                    LocateSectionsBySpec(*it, pOp);
                    break;

                case OP_ASSIGN_PC:
                    break;
            }

            // Next operation
            opit++;
        }

        // Next segment
        it++;
    }

    // Test for any sections that weren't located
    auto fit = m_FileList.begin();
    while (fit != m_FileList.end())
    {
        // Iterate through all sections of this file
        auto sit = (*fit)->m_FileSections.begin();
        while (sit != (*fit)->m_FileSections.end())
        {
            if (sit->second->m_LocateAddress == -1)
            {
                printf("%s: Section %s not added to any memory section!\n",
                        sit->second->m_Filename.c_str(), sit->first.c_str());
                err = ERROR_SEGMENT_NOT_LOCATED;
            }

            // Next section
            sit++;
        }

        // Next file
        fit++;
    }
    return err;
}

/* 
=============================================================================
Resolve external symbols
=============================================================================
*/
int CLinker::ResolveExterns(void)
{
    int     err = ERROR_NONE;
    int     value;

    // Loop for all input files
    auto fit = m_FileList.begin();
    while (fit != m_FileList.end())
    {
        // Iterate through all sections of this file
        auto sit = (*fit)->m_FileSections.begin();
        while (sit != (*fit)->m_FileSections.end())
        {
            // Loop for all extern symbols in this section
            auto xit = sit->second->m_ExternsList.begin();
            while (xit != sit->second->m_ExternsList.end())
            {
                // Find this extern symbol in our variable map
                auto vit = m_pSpec->m_Variables.find((*xit)->m_Label);
                if (vit == m_pSpec->m_Variables.end())
                {
                    // TODO:  Search all libraries for this symbol

                    // Test if this symbol already reported as unresolved
                    auto rep = m_UnresolveReport.find((*xit)->m_Label);
                    if (rep == m_UnresolveReport.end())
                    {
                        printf("%s: Unresolved function %s\n", (*fit)->m_Filename.c_str(),
                                (*xit)->m_Label.c_str());

                        m_UnresolveReport.insert(std::pair<std::string, int>((*xit)->m_Label,1));
                    }
                    err = ERROR_UNDEFINED_SYMBOL;
                }
                else
                {
                    // Populate the section's code with the extern address
                    value = atoi(vit->second.c_str());
                    sit->second->m_pCode[(*xit)->m_Offset] |= value;
                    (*xit)->m_Resolved = 1;
                }

                // Next extern symbol
                xit++;
            }
            // Next file section
            sit++;
        }

        // Next input file
        fit++;
    }

    return err; 
}

/* 
=============================================================================
Perform the link operation
=============================================================================
*/
int CLinker::Assemble(void)
{
    int         err = ERROR_NONE;
    int         value;
    int         size;
    int         x;

    // Loop for all input files
    m_MaxCodeAddr = 0;
    m_MaxDataAddr = 0;
    auto fit = m_FileList.begin();
    while (fit != m_FileList.end())
    {
        // Iterate through all sections of this file
        auto sit = (*fit)->m_FileSections.begin();
        while (sit != (*fit)->m_FileSections.end())
        {
            // Test if the m_pLocateMem is executable
            if (strchr(sit->second->m_pLocateMem->m_Access.c_str(), 'x') != NULL)
            {
                // Add this section's code at it's m_LocateAddress location
                size = sit->second->m_LastCodeOffset;
                for (x = 0; x < size; x++)
                    m_Code[sit->second->m_LocateAddress + x] = sit->second->m_pCode[x];

                if (sit->second->m_LocateAddress + size > m_MaxCodeAddr)
                    m_MaxCodeAddr = sit->second->m_LocateAddress + size;
            }
            else
            {
                value = sit->second->m_LocateAddress + sit->second->m_LastCodeOffset;

                // Keep track of the Data size
                if (value > m_MaxDataAddr)
                    m_MaxDataAddr = value;
            }

            // Next file section
            sit++;
        }

        // Next input file
        fit++;
    }

    if (m_DebugLevel > 1)
    {
        for (x = 0; x < m_MaxCodeAddr; x++)
            printf("0x%04X:  0x%04X\n", x, m_Code[x]);
    }
    return 0;
}

/* 
=============================================================================
Generate Map File
=============================================================================
*/
int CLinker::GenerateMapFile(char *pOutFilename)
{
    char    str[strlen(pOutFilename)+5];
    char   *ptr;
    FILE   *fd;

    // Create the filename
    strcpy(str, pOutFilename);
    if ((ptr = strrchr(str, '.')) != NULL)
        *ptr = 0;
    strcat(str, ".map");

    // Open the file
    if ((fd = fopen(str, "w")) == NULL)
    {
        printf("Unable to open output file '%s'\n", str);
        return ERROR_CANT_OPEN_FILE;
    }

    fprintf(fd, "Code Space\n");
    fprintf(fd, "==========\n");
    // Sort the Code symbols
    m_CodeMapSymbols.sort();
    auto it = m_CodeMapSymbols.begin();
    while (it != m_CodeMapSymbols.end())
    {
        fprintf(fd, "%s\n", (*it).c_str());
        it++;
    }

    fprintf(fd, "\nData Space\n");
    fprintf(fd, "==========\n");
    m_DataMapSymbols.sort();
    it = m_DataMapSymbols.begin();
    while (it != m_DataMapSymbols.end())
    {
        fprintf(fd, "%s\n", (*it).c_str());
        it++;
    }

    fclose(fd);
    return ERROR_NONE;
}

/* 
=============================================================================
Generate Hex File
=============================================================================
*/
int CLinker::GenerateHexFile(char *pOutFilename)
{
    int         x;
    FILE       *fd;

    // Open the output file
    if ((fd = fopen(pOutFilename, "w")) == NULL)
    {
        printf("Unable to open output file '%s'\n", pOutFilename);
        return ERROR_CANT_OPEN_FILE;
    }

    // Write HEX data to the file
    for (x = 0; x < m_MaxCodeAddr; x++)
        fprintf(fd, "%04X\n", m_Code[x]);
    fclose(fd);

    printf("Code size:  %d\n", m_MaxCodeAddr);
    printf("Data size:  %d\n", m_MaxDataAddr);

    return ERROR_NONE;
}

/* 
=============================================================================
Generate Hex File
=============================================================================
*/
int CLinker::GenerateTestbenchFile(char *pOutFilename)
{
    int    x;
    int    bytesOnLine;
    FILE   *fd;
    char    str[strlen(pOutFilename)+5];
    char   *ptr;

    // Create the filename
    strcpy(str, pOutFilename);
    if ((ptr = strrchr(str, '.')) != NULL)
        *ptr = 0;
    strcat(str, ".hex");

    // Open the file
    if ((fd = fopen(str, "w")) == NULL)
    {
        printf("Unable to open output file '%s'\n", str);
        return ERROR_CANT_OPEN_FILE;
    }

    fprintf(fd, "@0\n");

    // Write HEX data to the file
    bytesOnLine = 0;
    for (x = 0; x < m_MaxCodeAddr; x++)
    {
        fprintf(fd, "%02x %02x ", m_Code[x] & 0xFF, m_Code[x] >> 8);
        bytesOnLine += 2;
        if (bytesOnLine == 32)
        {
            fprintf(fd, "\n");
            bytesOnLine = 0;
        }
    }
    if (bytesOnLine != 0)
        fprintf(fd, "\n");
    fclose(fd);

    return ERROR_NONE;
}

/* 
=============================================================================
Perform the link operation
=============================================================================
*/
int CLinker::Link(char *pOutFilename)
{
    int     err;

    // First locate all segments by walking through the operation list
    if ((err = LocateSections()) != ERROR_NONE)
        return err;

    if (m_DebugLevel > 0)
    {
        auto it = m_pSpec->m_Variables.begin();
        while (it != m_pSpec->m_Variables.end())
        {
            printf("%-20s%s\n", it->first.c_str(), it->second.c_str());
            it++;
        }
    }

    // Assign values to all labels base on locate addresses
    if ((err = ResolveExterns()) != ERROR_NONE)
        return err;

    // If no error, then assemble the program
    if ((err = Assemble()) != ERROR_NONE)
        return err;

    // Generate map file
    if (m_MapFile)
        if ((err = GenerateMapFile(pOutFilename)) != ERROR_NONE)
            return err;

    // Generate output hex file
    if ((err = GenerateHexFile(pOutFilename)) != ERROR_NONE)
        return err;

    // Generate output testbench file
    if ((err = GenerateTestbenchFile(pOutFilename)) != ERROR_NONE)
        return err;

    return ERROR_NONE;
}

// vim: sw=4 ts=4

