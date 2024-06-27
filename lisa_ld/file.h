// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : parser.h
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

#ifndef FILE_H
#define FILE_H

#include "parser.h"

#define REL_TYPE_EXTERN     1
#define REL_TYPE_FUNCTION   2
#define REL_TYPE_SYMBOL     3

class CRelocation
{
    public:
        CRelocation() { m_Offset = 0; m_Opcode = 0; m_Resolved = 0; }

        int             m_Type;
        int             m_Offset;
        int             m_Opcode;
        int             m_Resolved;
        std::string     m_Section;
        std::string     m_Label;
};

typedef std::map<std::string, int> StrIntMap_t;
typedef std::list<CRelocation *> RelocationList_t;

class CFileSection
{
    public:
        CFileSection() { m_Address = 0; m_Line = 0; m_LocateAddress = -1;
                         m_pCode = new uint16_t[8192];
                         m_FirstCodeOffset = 0xFFFFFF;
                         m_LastCodeOffset = 0; }
        ~CFileSection() { delete[] m_pCode; }

        std::string         m_Filename;
        std::string         m_Name;
        int                 m_Address;
        int                 m_Line;
        StrIntMap_t         m_PublicLabels;
        StrIntMap_t         m_LocalLabels;
        RelocationList_t    m_RelocationList;
        RelocationList_t    m_ExternsList;
        CMemory            *m_pLocateMem;       // Locate memory region

        uint16_t           *m_pCode;
        int                 m_LocateAddress;
        int                 m_FirstCodeOffset;
        int                 m_LastCodeOffset;
};

typedef std::map<std::string, CFileSection *> FileSectionMap_t;

class CFile
{
    public:
        CFile(CParseCtx* pSpec);

        int                 LoadRelFile(const char *pFilename);

        int                 m_DebugLevel;
        std::string         m_Filename;
        FileSectionMap_t    m_FileSections;

    private:
        /// Parses a single line from a CParseCtx file
        int                 ParseLine(const char* pLine, CParserFile* pFile);
        int                 ParseArgs(char *sLine, CParserFile* pFile);
        int                 ParseSection(CParserFile* pFile);
        int                 ParsePublic(CParserFile* pFile);
        int                 ParseLocal(CParserFile* pFile);
        int                 ParseAddress(CParserFile* pFile);
        int                 ParseInstruction(CParserFile* pFile);
        int                 ParseExtern(CParserFile* pFile);
        int                 ParseRelocation(CParserFile* pFile);
        int                 ParseUninitializedAlloc(CParserFile* pFile);


    private:
        CFileSection       *m_ActiveSection;
        std::string         m_Args[8];
        int                 m_Argc;

        /// Pointer to the CParseCtx we are parsing into
        CParseCtx*          m_pSpec;
};

typedef std::list<CFile *> FileList_t;

#endif /* FILE_H */

// vim: sw=4 ts=4

