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
#define STATE_STRING                        1
#define STATE_DATA                          2
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

        int32_t             ParseFile(const char * pFilename, CParseCtx *pSpec = NULL);

        /// Debug print level
        uint32_t            m_DebugLevel;

        /// Adds a define from the command line
        void                AddDefine(const char *name); 

        void                AddInclude(const char *path);

        int                 m_Width;

    private:
        /// Parses a single line from a CParseCtx file
        virtual int32_t     ParseLine(const char* pLine, CParserFile* pFile);

        virtual int32_t     AppendString(char* sLine, CParserFile* pFile);

        /// Appends data to the last active data statement for multi-line data syntax
        virtual int32_t     AppendData(char* sLine, CParserFile* pFile);

        // Keyword handler functions.  New functions must be added to the
        // m_pKeywords array at the top of parser.cpp
        virtual bool        TestForInclude(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForSegment(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForDefine(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForUndef(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForIfdef(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForIfndef(char* sLine, CParserFile* pFile, int32_t& err);
        void                ProcessIfdef(char* sLine, CParserFile* pFile, int32_t& err, bool negate);
        virtual bool        TestForIf(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForElse(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForElsif(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForEndif(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForOrg(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForLabel(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForOpcode(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForData(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForExtern(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForPublic(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForLocal(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForDs(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForDb(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForDw(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForFile(char* sLine, CParserFile* pFile, int32_t& err);
        virtual bool        TestForLoc(char* sLine, CParserFile* pFile, int32_t& err);
        int                 directive_if(char* sExpr, CParserFile* pFile, int32_t& err, int instIsIf);
        bool                isConst(char *pStr);

        /// Evaluates an expression which may contain a $(VARIABLE)
        int32_t             EvaluateExpression(std::string& sExpr, uint32_t& value, 
                                std::string& sFilename, uint32_t lineNo);

        /// Replaces all occurances of variables withing sExpr and returns sSubst
        int32_t             SubstituteVariables(std::string& sExpr, std::string& sSubst, 
                                std::string& sFilename, uint32_t lineNo);

        void                Trim(std::string &str);

        /// Array of keyword handler function pointers
        static  CParserFuncPtr  m_pKeywords[22];

        /// Count of keyword handler function pointer in our array
        static  uint32_t    m_keywordCount;

        /// Holds the text for the last error encountered
        std::string         m_Error;

        /// Parser state in case it becomes stateful
        int32_t             m_ParseState;

        /// Points to the last imagespec section found
        ResourceSection_t*  m_LastSegment;
        
        /// Points to the last active ResourceData_t object
        ResourceData_t*     m_LastResData;

        /// Pointer to the CParseCtx we are parsing into
        CParseCtx*          m_pSpec;

        int                 m_Line;
        char                m_IfStat[100];
        int                 m_IfDepth;
        int                 m_LastIfElseLine;   // Line number of last #if, #ifdef, IF, or else
        int                 m_LastIfElseIsIf;   // True if last was #if or #ifdef
};

#endif  // PARSER_H

