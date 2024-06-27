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

#ifndef PARSER_H
#define PARSER_H

#include "parsectx.h"

#define STATE_IDLE                          0
#define STATE_MEMORY                        1
#define STATE_SECTIONS                      2
#define STATE_COMMENT                       3

#define		IF_STAT_ASSEMBLE				1
#define		IF_STAT_DONT_ASSEMBLE			2
#define		IF_STAT_NESTED_DONT_ASSEMBLE	3
#define		IF_STAT_EVAL_ERROR				4

#define     COND_EQ         1
#define     COND_LE         2
#define     COND_GE         3
#define     COND_NE         4
#define     COND_LT         5
#define     COND_GT         6
#define     COND_BINARY     7

class CParserFile
{
    public:
        CParserFile() { m_Line = 0; }

        std::string     m_Filename;
        uint32_t        m_Line;
};

class CParser;

typedef bool (CParser::*CParserFuncPtr)(char* pLine, CParserFile* pFile, int32_t& err);

class CParser
{
    public:
        CParser(CParseCtx* pSpec);

        int32_t             ParseLinkerScript(const char * pFilename, CParseCtx *pSpec = NULL);

        /// Debug print level
        uint32_t            m_DebugLevel;

    private:
        /// Parses a single line from a CParseCtx file
        virtual int32_t     ParseLine(const char* pLine, CParserFile* pFile);
        virtual int32_t     MemorySpecLine(char* pLine, CParserFile* pFile);
        virtual int32_t     SectionsLine(char* pLine, CParserFile* pFile);

        // Keyword handler functions.  New functions must be added to the
        // m_pKeywords array at the top of parser.cpp
        virtual bool        TestForOutputArch(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForEntry(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForMemory(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForSections(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForVariable(char* sLine, CParserFile* pFile, int32_t& err);
        bool                isConst(char *pStr);

        /// Evaluates an expression which may contain a TYPE(VARIABLE)
        int32_t             EvaluateExpression(std::string& sExpr, uint32_t& value, 
                                std::string& sFilename, uint32_t lineNo);

        /// Evaluates a token of an expresion which may contain a TYPE(VARIABLE)
        int32_t             EvaluateToken(std::string& sExpr, uint32_t& value, 
                                std::string& sFilename, uint32_t lineNo);

        /// Replaces all occurances of variables withing sExpr and returns sSubst
        int32_t             SubstituteVariables(std::string& sExpr, std::string& sSubst, 
                                std::string& sFilename, uint32_t lineNo);

        void                Trim(std::string &str);

        /// Array of keyword handler function pointers
        static  CParserFuncPtr  m_pKeywords[5];

        /// Count of keyword handler function pointer in our array
        static  uint32_t    m_keywordCount;

        /// Holds the text for the last error encountered
        std::string         m_Error;
        std::string         m_OutputArch;
        std::string         m_EntryLabel;

        /// Parser state in case it becomes stateful
        int32_t             m_ParseState;
        int32_t             m_ParsePushState;
        int32_t             m_ParseSectionState;

        /// Points to the last imagespec section found
        CSection          * m_ActiveSection;
        
        /// Pointer to the CParseCtx we are parsing into
        CParseCtx*          m_pSpec;

        int                 m_Line;
        int                 m_BraceLevel;
};

#endif  // PARSER_H

