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

#include "parser.h"
#include "file.h"

class CLinker
{
    public:
        CLinker( CParseCtx *pSpec );

        int             Link(char *pOutFilename);
        void            AddDefine(const char *name);
        void            AddLibPath(const char *name);

        CParseCtx     * m_pSpec;
        FileList_t      m_FileList;

    private:
        int             LocateSections(void);
        int             LocateSectionsBySpec(CSection *pSection, COperation *pOp);
        int             ResolveExterns(void);
        int             Assemble(void);
        int             GenerateMapFile(char *pOutFilename);
        int             GenerateHexFile(char *pOutFilename);
        int             GenerateTestbenchFile(char *pOutFilename);

    public:

        int             m_DebugLevel;
        int             m_Mixed;
        int             m_MapFile;
        uint16_t        m_Code[8192];
        int             m_MaxCodeAddr;
        int             m_MaxDataAddr;
        StrIntMap_t     m_UnresolveReport;
        StrList_t       m_CodeMapSymbols;
        StrList_t       m_DataMapSymbols;
};

#endif /* LINKER_H */

// vim: sw=4 ts=4
