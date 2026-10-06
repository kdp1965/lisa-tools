// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : linker.h
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Definition of a parser framework class.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#ifndef LINKER_H
#define LINKER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "parser.h"
#include "file.h"
#include "parsectx.h"

class CLibFile;

class CLinker
{
    public:
        CLinker( CParseCtx *pSpec );

        int             Link(char *pOutFilename);
        void            AddDefine(const char *name);
        void            AddLibPath(const char *name);
        void            AddLibrary(const char *name);
        void            OpenLibraries(void);
        bool            ParseLibrary(std::ifstream &infile, CLibFile& libFile);

        CParseCtx     * m_pSpec;
        FileList_t      m_FileList;
        StrStrMap_t     m_LibList;

    private:
        int             LocateSections(void);
        int             LocateSectionsBySpec(CSection *pSection, COperation *pOp);
        int             ResolveExterns(int& LibSlicesAdded);
        int             ResolveLibCalls(int& LibSlicesAdded);
        int             AssignAddresses(void);
        int             Assemble(void);
        int             GenerateMapFile(char *pOutFilename);
        int             GenerateHexFile(char *pOutFilename);
        int             GenerateTestbenchFile(char *pOutFilename);
        int             GenerateListFile(char *pOutFilename);

    public:

        int             m_DebugLevel;
        int             m_Mixed;
        int             m_MapFile;
        int             m_ListFile;
        uint16_t        m_Code[32768];
        int             m_MaxCodeAddr;
        int             m_MaxDataAddr;
        StrIntMap_t     m_UnresolveReport;
        StrList_t       m_CodeMapSymbols;
        StrList_t       m_DataMapSymbols;
};

#endif /* LINKER_H */

// vim: sw=4 ts=4
