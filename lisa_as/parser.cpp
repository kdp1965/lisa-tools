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

#include "parser.h"
#include "errors.h"

// Define our opcodes
Opcode_t gOpcodes[] = 
{
//    Name      args   value           size
    { "ldx",       1, OPCODE_LDX,        2 | SIZE_LABEL | SIZE_ABSOLUTE },
    { "jal",       1, OPCODE_JAL,        1 | SIZE_LABEL | SIZE_ABSOLUTE },
    { "ldi",       1, OPCODE_LDI,        1 },
    { "reti",      1, OPCODE_RETI,       1 },
    { "ret",       0, OPCODE_RET,        1 },
    { "rets",      0, OPCODE_RETS,       1 },
    { "rc",        0, OPCODE_RC,         1 },
    { "rz",        0, OPCODE_RZ,         1 },
    { "call_ix",   0, OPCODE_CALL_IX,    1 },
    { "jmp_ix",    0, OPCODE_JMP_IX,     1 },
    { "push_ix",   0, OPCODE_PUSH_IX,    1 },
    { "pop_ix",    0, OPCODE_POP_IX,     1 },
    { "xchg",      1, OPCODE_XCHG,       1 },
    { "xchg_ra",   0, OPCODE_XCHG_RA,    1 },
    { "xchg_ia",   0, OPCODE_XCHG_IA,    1 },
    { "xchg_sp",   0, OPCODE_XCHG_SP,    1 },
    { "spix",      0, OPCODE_SPIX,       1 },
    { "adc",       1, OPCODE_ADC,        1 },
    { "adx",       1, OPCODE_ADX,        1 },
    { "addax",     0, OPCODE_ADDAX,      1 },
    { "addaxu",    0, OPCODE_ADDAXU,     1 },
    { "subax",     0, OPCODE_SUBAX,      1 },
    { "subaxu",    0, OPCODE_SUBAXU,     1 },
    { "ads",       1, OPCODE_ADS,        1 },
    { "shl",       0, OPCODE_SHL,        1 },
    { "shr",       0, OPCODE_SHR,        1 },
    { "shl16",     0, OPCODE_SHL16,      1 },
    { "shr16",     0, OPCODE_SHR16,      1 },
    { "ldc",       1, OPCODE_LDC,        1 },
    { "ldz",       1, OPCODE_LDZ,        1 },
    { "txa",       0, OPCODE_TXA,        1 },
    { "txau",      0, OPCODE_TXAU,       1 },
    { "btst",      1, OPCODE_BTST,       1 },
    { "tax",       0, OPCODE_TAX,        1 },
    { "taxu",      0, OPCODE_TAXU,       1 },
    { "amode",     1, OPCODE_AMODE,      1 },
    { "sra",       0, OPCODE_SRA,        1 },
    { "lra",       0, OPCODE_LRA,        1 },
    { "cpi",       1, OPCODE_CPI,        1 },
    { "push_a",    0, OPCODE_PUSH_A,     1 },
    { "pop_a",     0, OPCODE_POP_A,      1 },
    { "br",        1, OPCODE_BR,         1 | SIZE_LABEL },
    { "bnz",       1, OPCODE_BNZ,        1 | SIZE_LABEL },
    { "bz",        1, OPCODE_BZ,         1 | SIZE_LABEL },
    { "add",       1, OPCODE_ADD,        1 },
    { "mul",       1, OPCODE_MUL,        1 },
    { "mulu",      1, OPCODE_MULU,       1 },
    { "sub",       1, OPCODE_SUB,        1 },
    { "if",        1, OPCODE_IF,         1 },
    { "iftt",      1, OPCODE_IFTT,       1 },
    { "ifte",      1, OPCODE_IFTE,       1 },
    { "and",       1, OPCODE_AND,        1 },
    { "andi",      1, OPCODE_ANDI,       1 },
    { "or",        1, OPCODE_OR,         1 },
    { "swapi",     1, OPCODE_SWAPI,      1 },
    { "xor",       1, OPCODE_XOR,        1 },
    { "cmp",       1, OPCODE_CMP,        1 },
    { "swap",      1, OPCODE_SWAP,       1 },
    { "lda",       1, OPCODE_LDA,        1 },
    { "ldax",      1, OPCODE_LDAX,       1 },
    { "stax",      1, OPCODE_STAX,       1 },
    { "ldxx",      1, OPCODE_LDXX,       1 },
    { "stxx",      1, OPCODE_STXX,       1 },
    { "sta",       1, OPCODE_STA,        1 },
    { "nop",       0, OPCODE_NOP,        1 },
    { "notz",      0, OPCODE_NOTZ,       1 },
    { "dcx",       1, OPCODE_DCX,        1 },
    { "inx",       1, OPCODE_INX,        1 },
    { "cpx",       1, OPCODE_CPX,        1 },
    { "div",       1, OPCODE_DIV,        1 },
    { "rem",       1, OPCODE_REM,        1 },
    { "lddiv",     1, OPCODE_LDDIV,      1 },
    { "savec",     0, OPCODE_SAVEC,      1 },
    { "restc",     0, OPCODE_RESTC,      1 },
    { "taf",       0, OPCODE16_TAF,      1 },
    { "tafu",      0, OPCODE16_TAFU,     1 },
    { "tfa",       0, OPCODE16_TFA,      1 },
    { "tfau",      0, OPCODE16_TFAU,     1 },
    { "fmul",      1, OPCODE16_FMUL,     1 },
    { "fdiv",      1, OPCODE16_FDIV,     1 },
    { "fadd",      1, OPCODE16_FADD,     1 },
    { "fswap",     1, OPCODE16_FSWAP,    1 },
    { "fcmp",      1, OPCODE16_FCMP,     1 },
    { "itof",      0, OPCODE16_ITOF,     1 },
    { "ftoi",      0, OPCODE16_FTOI,     1 },
    { "di",        0, OPCODE16_DI,       1 },
    { "ei",        0, OPCODE16_EI,       1 }
};
int gOpcodeCount = sizeof(gOpcodes) / sizeof(Opcode_t);

// Define our opcodes
Opcode_t gOpcodes16[] = 
{
//    Name      args   value           size
    { "ldx",       1, OPCODE16_LDX,        2 | SIZE_LABEL | SIZE_ABSOLUTE },
    { "jal",       1, OPCODE16_JAL,        1 | SIZE_LABEL | SIZE_ABSOLUTE },
    { "ldi",       1, OPCODE16_LDI,        1 },
    { "reti",      1, OPCODE16_RETI,       1 },
    { "ret",       0, OPCODE16_RET,        1 },
    { "rets",      0, OPCODE16_RETS,       1 },
    { "rc",        0, OPCODE16_RC,         1 },
    { "rz",        0, OPCODE16_RZ,         1 },
    { "call_ix",   0, OPCODE16_CALL_IX,    1 },
    { "jmp_ix",    0, OPCODE16_JMP_IX,     1 },
    { "push_ix",   0, OPCODE16_PUSH_IX,    1 },
    { "pop_ix",    0, OPCODE16_POP_IX,     1 },
    { "xchg",      1, OPCODE16_XCHG,       1 },
    { "xchg_ra",   0, OPCODE16_XCHG_RA,    1 },
    { "xchg_ia",   0, OPCODE16_XCHG_IA,    1 },
    { "xchg_sp",   0, OPCODE16_XCHG_SP,    1 },
    { "spix",      0, OPCODE16_SPIX,       1 },
    { "adc",       1, OPCODE16_ADC,        1 },
    { "adx",       1, OPCODE16_ADX,        1 },
    { "addax",     0, OPCODE16_ADDAX,      1 },
    { "addaxu",    0, OPCODE16_ADDAXU,     1 },
    { "subax",     0, OPCODE16_SUBAX,      1 },
    { "subaxu",    0, OPCODE16_SUBAXU,     1 },
    { "ads",       1, OPCODE16_ADS,        1 },
    { "shl",       0, OPCODE16_SHL,        1 },
    { "shr",       0, OPCODE16_SHR,        1 },
    { "shl16",     0, OPCODE16_SHL16,      1 },
    { "shr16",     0, OPCODE16_SHR16,      1 },
    { "ldc",       1, OPCODE16_LDC,        1 },
    { "ldz",       1, OPCODE16_LDZ,        1 },
    { "txa",       0, OPCODE16_TXA,        1 },
    { "txau",      0, OPCODE16_TXAU,       1 },
    { "btst",      1, OPCODE16_BTST,       1 },
    { "tax",       0, OPCODE16_TAX,        1 },
    { "taxu",      0, OPCODE16_TAXU,       1 },
    { "amode",     1, OPCODE16_AMODE,      1 },
    { "sra",       0, OPCODE16_SRA,        1 },
    { "lra",       0, OPCODE16_LRA,        1 },
    { "cpi",       1, OPCODE16_CPI,        1 },
    { "push_a",    0, OPCODE16_PUSH_A,     1 },
    { "pop_a",     0, OPCODE16_POP_A,      1 },
    { "br",        1, OPCODE16_BR,         1 | SIZE_LABEL },
    { "bnz",       1, OPCODE16_BNZ,        1 | SIZE_LABEL },
    { "bz",        1, OPCODE16_BZ,         1 | SIZE_LABEL },
    { "add",       1, OPCODE16_ADD,        1 },
    { "mul",       1, OPCODE16_MUL,        1 },
    { "mulu",      1, OPCODE16_MULU,       1 },
    { "sub",       1, OPCODE16_SUB,        1 },
    { "if",        1, OPCODE16_IF,         1 },
    { "iftt",      1, OPCODE16_IFTT,       1 },
    { "ifte",      1, OPCODE16_IFTE,       1 },
    { "and",       1, OPCODE16_AND,        1 },
    { "andi",      1, OPCODE16_ANDI,       1 },
    { "or",        1, OPCODE16_OR,         1 },
    { "swapi",     1, OPCODE16_SWAPI,      1 },
    { "xor",       1, OPCODE16_XOR,        1 },
    { "cmp",       1, OPCODE16_CMP,        1 },
    { "swap",      1, OPCODE16_SWAP,       1 },
    { "lda",       1, OPCODE16_LDA,        1 },
    { "ldax",      1, OPCODE16_LDAX,       1 },
    { "stax",      1, OPCODE16_STAX,       1 },
    { "ldxx",      1, OPCODE16_LDXX,       1 },
    { "stxx",      1, OPCODE16_STXX,       1 },
    { "sta",       1, OPCODE16_STA,        1 },
    { "nop",       0, OPCODE16_NOP,        1 },
    { "notz",      0, OPCODE16_NOTZ,       1 },
    { "dcx",       1, OPCODE16_DCX,        1 },
    { "inx",       1, OPCODE16_INX,        1 },
    { "cpx",       1, OPCODE16_CPX,        1 },
    { "div",       1, OPCODE16_DIV,        1 },
    { "rem",       1, OPCODE16_REM,        1 },
    { "lddiv",     0, OPCODE16_LDDIV,      1 },
    { "savec",     0, OPCODE16_SAVEC,      1 },
    { "restc",     0, OPCODE16_RESTC,      1 },
    { "taf",       0, OPCODE16_TAF,        1 },
    { "tafu",      0, OPCODE16_TAFU,       1 },
    { "tfa",       0, OPCODE16_TFA,        1 },
    { "tfau",      0, OPCODE16_TFAU,       1 },
    { "fmul",      1, OPCODE16_FMUL,       1 },
    { "fdiv",      1, OPCODE16_FDIV,     1 },
    { "fadd",      1, OPCODE16_FADD,       1 },
    { "fswap",     1, OPCODE16_FSWAP,      1 },
    { "fcmp",      1, OPCODE16_FCMP,       1 },
    { "itof",      0, OPCODE16_ITOF,       1 },
    { "ftoi",      0, OPCODE16_FTOI,       1 },
    { "di",        0, OPCODE16_DI,         1 },
    { "ei",        0, OPCODE16_EI,         1 }
};
int gOpcodeCount16 = sizeof(gOpcodes16) / sizeof(Opcode_t);

// Define array of pointers to keyword handlers
CParserFuncPtr CParser::m_pKeywords[] = { 
    &CParser::TestForInclude, 
    &CParser::TestForSegment, 
    &CParser::TestForDefine, 
    &CParser::TestForUndef,
    &CParser::TestForIfdef,   
    &CParser::TestForIfndef,
    &CParser::TestForIf,
    &CParser::TestForElse,
    &CParser::TestForElsif,
    &CParser::TestForEndif,
    &CParser::TestForLabel,
    &CParser::TestForData,
    &CParser::TestForExtern,
    &CParser::TestForPublic,
    &CParser::TestForLocal,
    &CParser::TestForOrg,
    &CParser::TestForDs,
    &CParser::TestForDb,
    &CParser::TestForDw,
    &CParser::TestForFile,
    &CParser::TestForLoc,

    // Tests for registered opcodes
    &CParser::TestForOpcode
};
uint32_t CParser::m_keywordCount = sizeof(CParser::m_pKeywords) / sizeof (CParserFuncPtr);

#define     iswhite(a)  (((a) == ' ') || ((a) == '\t'))

// =============================================================================

CParser::CParser(CParseCtx *pSpec) : m_pSpec(pSpec)
{
    m_ParseState = 0;
    m_LastSegment = NULL;
    m_LastResData = NULL;
    m_IfDepth = 0;
    m_IfStat[0] = IF_STAT_ASSEMBLE;

    // Create a default 'config' segment
#if 0
    ResourceSection *segment = new ResourceSection_t;
    std::string segmentName = "code";
    segment->line = 0;
    segment->address = 0;
    segment->filename = "cmd_line";
    segment->type = "code";
    segment->name = segmentName;
    m_pSpec->m_Segments.insert(std::pair<std::string, ResourceSection_t*>(segmentName, segment));
#endif
    m_LastSegment = NULL;

    // Setup locate address of 0 for 'config' segment
    ResourceString_t* pVar = new ResourceString_t;
    std::string sectionName = "config";
    pVar->value = "0";
    pVar->spec_filename = "cmd_line";
    pVar->spec_line = 0;
    m_pSpec->m_Locates.insert(std::pair<std::string,ResourceString_t *>(sectionName, pVar));
 }

/* 
=============================================================================
Data, files and opcodes can be separated into segments and located / 
relocated.
=============================================================================
*/
bool CParser::TestForSegment(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sSegment = "segment";
    const   char*       sSection = "section";
    const   char*       sSegmentCap = "SEGMENT";
    const   char*       sType = "type";
    const   char*       sTypeCap = "TYPE";
    const   char*       sSrec = "srec";
    const   char*       sElf = "elf";
    const   char*       sHex = "hex";
    const   char*       sBinary = "binary";
    const   char*       sCode = "code";
    const   char*       sTypes[] = { sSrec, sElf, sHex, sBinary, sCode };
    const   uint32_t    numTypes = sizeof (sTypes) / sizeof(char *);
    uint32_t            c;
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         segmentName;

    // Test for leading '.'
    if (*sLine == '.')
        sLine++;
    // Test for segment keyword
    if ((strncmp(sLine, sSegment, 7) == 0 || strncmp(sLine, sSegmentCap, 7) == 0 ||
          strncmp(sLine, sSection, 7) == 0) &&
            iswhite(sLine[7]))
    {
        // Skip the segment keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Search for segment name
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if ((sToken == NULL) || (strncmp(sToken, "//", 2) == 0))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected segment name after 'segment'";
            m_Error = err_str.str();
            err = ERROR_INVALID_SEGMENT_FORMAT;
            return true;
        }
        
        // Save the segment name
        segmentName = sToken;

        // Test if the segment already exists
        auto pSeg = m_pSpec->m_Segments.find(segmentName);
        if (pSeg != m_pSpec->m_Segments.end())
        {
            m_LastSegment = pSeg->second;
            return true;
        }

        // Save the segment and it's type to our array
        ResourceSection *segment = new ResourceSection_t;
        segment->line = pFile->m_Line;
        segment->address = 0;
        segment->filename = pFile->m_Filename;
        segment->type = sToken;
        segment->name = segmentName;
        m_pSpec->m_Segments.insert(std::pair<std::string, ResourceSection_t*>(segmentName, segment));
        m_LastSegment = segment;

        err = ERROR_NONE;
        return true;
    }
    
    // Segment keyword not found
    return false;
}

/* 
=============================================================================
Handle include keywords in the assembled source
=============================================================================
*/
bool CParser::TestForInclude(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sInclude = "include";
    const   char*       sInclude2 = "#include";
    const   char*       sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::stringstream   path;

    // Test for Include keyword in line
    if ((strncmp(sLine, sInclude2, 8) == 0) || (strncmp(sLine, sInclude, 7) == 0))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the include keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the file name 
        if (sNextToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected filename after 'include'";
            m_Error = err_str.str();
            err = ERROR_INVALID_INCLUDE_SYNTAX;
            return true;
        }

        // Skip whitespace from sNextToken
        while ((*sNextToken == ' ') || (*sNextToken == '\t'))
            sNextToken++;
        sToken = sNextToken + 1;
        while ((*sToken != '\0') && (*sToken != '"'))
            sToken++;

        // Test for quote
        if ((*sNextToken != '"') || (*sToken != '"'))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected quotes around filename";
            m_Error = err_str.str();
            err = ERROR_INVALID_INCLUDE_SYNTAX;
            return true;
        }

        sToken = strtok_r(NULL, "\"", &sNextToken);

        // Include path parsed.  Try to open the file
        FILE* fd;
        StrList_t::iterator lit = m_pSpec->m_IncPaths.begin();
        
        // First try in current directory
        if ((fd = fopen(sToken, "r")) != NULL)
        {
            fclose(fd);
        }
        if (fd == NULL)
        {
            for (; lit != m_pSpec->m_IncPaths.end(); lit++)
            {
                // Build path 
                path << *lit << sToken;

                if ((fd = fopen(path.str().c_str(), "r")) != NULL)
                {
                    fclose(fd);
                    sToken = path.str().c_str();
                    break;
                }
            }

            if (lit == m_pSpec->m_IncPaths.end())
            {
                err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                   ": Unable to open include file " << sToken;
                m_Error = err_str.str();
                err = ERROR_INVALID_INCLUDE_SYNTAX;
                return true;
            }
        }

        // Parse this file
        err = ParseFile(sToken);
        return true;
    }

    // Not include keyword
    return false;
}

// =============================================================================

int32_t CParser::EvaluateExpression(std::string& sExpr, uint32_t& value,
        std::string& sFilename, uint32_t lineNo)
{
    int32_t                 err = ERROR_NONE;
    std::string             sSubst;
    StrStrMap_t::iterator   varIter;
    std::string             varValue;

    // Perform variable substitution
    if ((err = SubstituteVariables(sExpr, sSubst, sFilename, lineNo)) != ERROR_NONE)
        return err;

    // Do a lookup in case it is a define
    varValue = sSubst;
    if ((varIter = m_pSpec->m_Variables.find(sSubst)) != m_pSpec->m_Variables.end())
    {
        // Variable found
        varValue = varIter->second;
    }
   
    // Evaluate.  We may want this to support equations later
    if (strncmp(varValue.c_str(), "0x", 2) == 0)
       sscanf(varValue.c_str() + 2, "%x", &value);
    else if (strncmp(varValue.c_str(), "b'", 2) == 0 ||
             strncmp(varValue.c_str(), "B'", 2) == 0)
    {
        // Convert binary value
        const char *ptr = varValue.c_str()+2;
        value = 0;
        while (*ptr != '\'' && *ptr != '\0')
        {
           if (*ptr == '_')
              ptr++;
           else if (*ptr == '0')
           {
              value = value << 1;
              ptr++;
           }
           else if (*ptr == '1')
           {
              value = (value << 1) | 1;
              ptr++;
           }
           else
              break;
        }
    }
    else
        sscanf(varValue.c_str(), "%i", &value);
    return err;
}

// =============================================================================

int32_t CParser::SubstituteVariables(std::string& sExpr, std::string& sSubst,
        std::string& sFilename, uint32_t lineNo)
{
    int32_t                 err = ERROR_NONE;
    size_t                  found;
    const char*             pStr;
    uint32_t                varEnd, c;
    StrStrMap_t::iterator   varIter;

    // First copy expression to subst
    sSubst = sExpr;
   
    // Search for '$(' in the expression
    while ((found = sSubst.find("$(")) != std::string::npos)
    {
        // Find end of variable to perform lookup
        pStr = sSubst.c_str();
        for (c = found + 2; pStr[c] != '\0' && pStr[c] != ')'; c++)
            ;

        // Test if end of variable found
        if (pStr[c] == ')')
        {
            // Get variable name
            std::string varName = sSubst.substr(found+2, c-found-2);
            std::string varValue;
            bool varFound = false;
            char*   pEnvVar;

            // Lookup variable in CParseCtx's variable map
            if ((varIter = m_pSpec->m_Variables.find(varName)) == m_pSpec->m_Variables.end())
            {
                // Variable not found, search environemnt
                if ((pEnvVar = getenv(varName.c_str())) != NULL)
                {
                    varFound = true;
                    varValue = pEnvVar;
                }
                else
                {
                    printf("%s: Line %d: Variable %s not defined\n", sFilename.c_str(), lineNo, varName.c_str());
                    err = ERROR_VARIABLE_NOT_FOUND;
                    break;
                }
            }
            else
            {
                // Variable found
                varFound = true;
                varValue = varIter->second;
            }

            // Test if varibable found local or envrironment space
            if (varFound)
            {
                // Perform the substitution
                sSubst.replace(found, c-found+1, varValue);

                if (m_DebugLevel >= 5)
                {
                    printf("%s: Line %d: Subst text = '%s'\n", sFilename.c_str(),
                            lineNo, sSubst.c_str());
                }
            }
            else
            {
                break;
            }
        }
    }

    return err;
}

void CParser::Trim(std::string &str)
{
   size_t   endpos = str.find_last_not_of(" \t");
   if (std::string::npos != endpos)
      str = str.substr(0, endpos+1);
   size_t   startpos = str.find_first_not_of(" \t");
   if (std::string::npos != startpos)
      str = str.substr(startpos);
}

/*
============================================================================
This function is called when an if or elsif directive is encountered
============================================================================
*/
#if 1
int CParser::directive_if(char* sExpr, CParserFile* pFile, int32_t& err, int instIsIf)
{
    uint32_t      valuel, valuer;
    std::stringstream   err_str;
    std::string   errVar;

    m_LastIfElseLine = m_Line;
    m_LastIfElseIsIf = 1;

    // First push results from previous IF/ELSE/ENDIF operation so
    // we generate "nested if" conditions.  Initialize condition to
    // EVAL_ERROR in case the condition does not evaluate
    if (instIsIf)
    {
        // Process if instrution
        if ((m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE) ||
            (m_IfStat[m_IfDepth] == IF_STAT_NESTED_DONT_ASSEMBLE) ||
            (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR))
        {
            m_IfStat[++m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
        }
        else
            m_IfStat[++m_IfDepth] = IF_STAT_EVAL_ERROR;

        if (m_IfDepth >= sizeof(m_IfStat))
        {
            m_IfDepth--;
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Too many nested 'if' statements";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }
    }
    else
    {
        if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
            m_IfStat[m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
        else
            if (m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE)
                m_IfStat[m_IfDepth] = IF_STAT_EVAL_ERROR;
    }

    // Determine if both equations can be evaluated
    if (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR)
    {
        char * pEqLeft;
        char * pEqRight;
        char * pCond;
        int    cond;
        std::string sEqLeft;
        std::string sEqRight;

        pEqLeft = sExpr;
        pEqRight = sExpr;

        while (*pEqRight != '=' && *pEqRight != '<' && *pEqRight != '>' &&
                *pEqRight != '\0')
            pEqRight++;

        pCond = pEqRight;
        while (*pEqRight == '=' || *pEqRight == '<' || *pEqRight == '>' ||
              *pEqRight == ' ' || *pEqRight == '\t')
            pEqRight++;

        if (strncmp(pCond, "==", 2) == 0)
            cond = COND_EQ;
        else if (strncmp(pCond, "<=", 2) == 0)
            cond = COND_LE;
        else if (strncmp(pCond, ">=", 2) == 0)
            cond = COND_GE;
        else if (strncmp(pCond, "!=", 2) == 0)
            cond = COND_NE;
        else if (*pCond == '<')
            cond = COND_LT;
        else if (*pCond == '>')
            cond = COND_GT;
        else
            cond = COND_BINARY;
        if (pCond)
           *pCond = '\0';

        if (*pEqRight == '\0')
           pEqRight = NULL;
        if (pEqLeft)
            sEqLeft = pEqLeft;
        if (pEqRight)
            sEqRight = pEqRight;
        Trim(sEqLeft);
        Trim(sEqRight);

        if (EvaluateExpression(sEqLeft, valuel, pFile->m_Filename, pFile->m_Line) == ERROR_NONE)
        {
            // Check if condition contains 2 equations or not
            if (pEqRight != 0)
            {
                if (EvaluateExpression(sEqRight, valuer, pFile->m_Filename, pFile->m_Line) == ERROR_NONE)
                {
                    m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;

                    // Both equations evaluate, check condition
                    switch (cond)
                    {
                    case COND_EQ:
                        if (valuel == valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;

                    case COND_NE:   
                        if (valuel != valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;

                    case COND_GE:   
                        if (valuel >= valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;

                    case COND_LE:   
                        if (valuel <= valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;

                    case COND_GT:   
                        if (valuel > valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;

                    case COND_LT:   
                        if (valuel < valuer)
                            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                        break;
                    }
                }
            }
            else
            {
                // Check bit 0 of the evaluated expression from equation 1
                if (((int) valuel) & 0x01)
                    m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
                else
                    m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
            }
        }
    }       
    if (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR || errVar != "")
    {
        err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
            ": Symbol " << errVar << " undefined";
        m_Error = err_str.str();
        err = ERROR_INVALID_DEFINE_SYNTAX;
    }
    return true;
}
#endif


/* 
=============================================================================
Handle 'define' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForDefine(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "define";
    const   char*       sKey2 = "#define";
    char*               sToken;
    char*               sNextToken;
    int                 defined, len;
    std::stringstream   err_str;
    std::string         varName;

    // Test for endian keyword
    if ((((strncmp(sLine, sKey, 6) == 0) && iswhite(sLine[6])) ||
         ((strncmp(sLine, sKey2, 7) == 0)) && iswhite(sLine[7])))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the define keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the token name
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected TOKEN";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }
        varName = sToken;

        // Get the value
        sToken = sNextToken;
        while (*sToken == ' ' || *sToken == '\t')
           sToken++;

        if (*sToken == '\0')
        {
           sToken = (char *) "1";
        }

        len = strlen(sToken);
        
        while (sToken[len-1] == ' ' || sToken[len-1] == '\t')
            sToken[(len--) - 1] = '\0';

        // Test if define already defined
        defined = m_pSpec->m_Variables.find(varName) != m_pSpec->m_Variables.end();
        if (defined)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Token " << varName << " already defined.";
            m_Error = err_str.str();
            err = ERROR_INVALID_VAR_SYNTAX;
            return true;
        }

        // Add define to the list of defines
        m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, sToken));
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'define' keyword
    return false;
}

/* 
=============================================================================
Handle 'undef' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForUndef(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sUndef = "undef";
    const   char*       sUndef2 = "#undef";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;

    // Test for undef keyword
    if ((strncmp(sLine, sUndef, 5) == 0) || (strncmp(sLine, sUndef2, 6) == 0))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the undef keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the token name
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected 'big' or 'little' specification";
            m_Error = err_str.str();
            err = ERROR_INVALID_UNDEF_SYNTAX;
            return true;
        }

        // TODO:  Find the token in the token list 

        // TODO:  Remove the token from the list
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'undef' keyword
    return false;
}

/* 
=============================================================================
Process ifdef and ifndef
=============================================================================
*/
void CParser::ProcessIfdef(char* sToken, CParserFile* pFile, int32_t& err,
                           bool negate)
{
    bool                    defined;
    StrVarMap_t::iterator   defineIter;
    std::stringstream       err_str;
    std::string             def;

    err = ERROR_NONE;

    // First push results from previous IF/ELSE/ENDIF operation so
    // we generate "nested if" conditions.  Initialize condition to
    // EVAL_ERROR in case the condition does not evaluate
    if ((m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE) ||
        (m_IfStat[m_IfDepth] == IF_STAT_NESTED_DONT_ASSEMBLE) ||
        (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR))
    {
        m_IfStat[++m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
    }
    else
        m_IfStat[++m_IfDepth] = IF_STAT_EVAL_ERROR;

    // Ensure our #ifdef stack depth hasn't overflown
    if (m_IfDepth >= sizeof(m_IfStat))
    {
        m_IfDepth--;
        err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
            ": Too many nested if directives";
        m_Error = err_str.str();
        err = ERROR_INVALID_IFDEF_SYNTAX;
    }
    else if (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR)
    {
        m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
        def = sToken;

        // Lookup the symbol
        defined = m_pSpec->m_Variables.find(def) != m_pSpec->m_Variables.end();
        if ((defined && !negate) || (!defined && negate))
            m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
    }
}

/* 
=============================================================================
Handle 'ifdef' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForIfdef(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "ifdef";
    const   char*       sKey2 = "#ifdef";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    uint32_t            fillChar;

    // Test for env keyword
    if ((strncmp(sLine, sKey, 5) == 0) || (strncmp(sLine, sKey2, 6) == 0))
    {
        // Skip the ifdef keyword
        sToken = strtok_r(sLine, " \t=", &sNextToken);

        // Parse the ifdef token
        sToken = strtok_r(NULL, " \t=", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected TOKEN";
            m_Error = err_str.str();
            err = ERROR_INVALID_IFDEF_SYNTAX;
            return true;
        }

        // Process if the ifdef
        ProcessIfdef(sToken, pFile, err, false);
        return true;
    }

    // Not ifdef keyword
    return false;
}

/* 
=============================================================================
Handle 'ifndef' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForIfndef(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "ifndef";
    const   char*       sKey2 = "#ifndef";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;

    // Test for ifndef keyword
    if ((strncmp(sLine, sKey, 6) == 0) || (strncmp(sLine, sKey2, 7) == 0))
    {
        // Skip the keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the token name
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected TOKEN";
            m_Error = err_str.str();
            err = ERROR_INVALID_IFNDEF_SYNTAX;
            return true;
        }

        // Process if the ifdef
        ProcessIfdef(sToken, pFile, err, true);
        return true;
    }

    // Did not detect 'ifndef' keyword
    return false;
}

/* 
=============================================================================
Handle 'if' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForIf(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sIf = "#if";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;
    uint32_t            alignment;

    // Test for 'if' keyword
    if (strncmp(sLine, sIf, 3) == 0)
    {
        // Skip the if keyword
        sToken = strtok_r(sLine, " \t=", &sNextToken);

        // Get the expression
        sToken = strtok_r(NULL, " \t", &sNextToken);

        // Validate we have an expression
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected expression after 'if'";
            m_Error = err_str.str();
            err = ERROR_INVALID_IF_SYNTAX;
            return true;
        }

        // Evaluate the expression(s)
        directive_if(sToken, pFile, err, 1);

        err = ERROR_NONE;
        return true;
    }

    // Not 'if' keyword
    return false;
}

/* 
=============================================================================
Handle 'endif' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForEndif(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "endif";
    const   char*       sKey2 = "#endif";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;
    uint32_t            alignment;

    // Test for 'endif' keyword
    if ((strncmp(sLine, sKey, 5) == 0) || (strncmp(sLine, sKey2, 6) == 0))
    {
        m_LastIfElseLine = pFile->m_Line;
        m_LastIfElseIsIf = 0;

        // First insure 'endif' has a matching if
        if (m_IfDepth == 0)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                " ENDIF without a matching IF";
            m_Error = err_str.str();
            err = ERROR_ELSE_WITHOUT_IF;
            return true;
        }
        
        // Pop If from stack
        m_IfDepth--;

        err = ERROR_NONE;
        return true;
    }

    // Not Endif keyword
    return false;
}

/* 
=============================================================================
Handle 'else' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForElse(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sElse = "else";
    const   char*       sElse2 = "#else";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;
    uint32_t            alignment;

    // Test for 'else' keyword
    if ((strncmp(sLine, sElse, 4) == 0) || (strncmp(sLine, sElse2, 5) == 0))
    {
        m_LastIfElseLine = m_Line;
        m_LastIfElseIsIf = 0;

        // Process ELSE statement during parsing.  
        // First insure else has a matching if
        if (m_IfDepth == 0)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                " ELSE without a matching IF";
            m_Error = err_str.str();
            err = ERROR_ELSE_WITHOUT_IF;
            return true;
        }
        
        // Now check if the active IF statement is not a NESTED_DONT_ASSEMBLE.
        // If it isn't then change the state of the  assembly
        if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
            m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
        else
            if (m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE)
                m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;

        err = ERROR_NONE;
        return true;
    }

    // Not 'else' keyword
    return false;
}

/* 
=============================================================================
Handle 'elsif' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForElsif(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "elsif";
    const   char*       sKey2 = "#elsif";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         varName;
    uint32_t            alignment;

    // Test for elsif keyword
    if ((strncmp(sLine, sKey, 5) == 0) || (strncmp(sLine, sKey2, 6) == 0))
    {
        m_LastIfElseLine = m_Line;
        m_LastIfElseIsIf = 0;

        // First insure elsif has a matching if
        if (m_IfDepth == 0)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                " ELSIF without an opening IF";
            m_Error = err_str.str();
            err = ERROR_ELSE_WITHOUT_IF;
            return true;
        }
        
        // Call the common 'if' handling logic
        directive_if(sToken, pFile, err, 0);

        err = ERROR_NONE;
        return true;
    }

    // Not 'elsif' keyword
    return false;
}

/* 
=============================================================================
Handle detection of labels in the code
=============================================================================
*/
bool CParser::TestForLabel(char* sLine, CParserFile* pFile, int32_t& err)
{
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         labelName;
    int                 defined;

    // Test for a ':' symbol
    if (strchr(sLine, ':') != NULL)
    {
        // Process the label only if in an assemble state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Get the label name
        sToken = strtok_r(sLine, " \t:", &sNextToken);

        // Validate a label was given (not just a line with ':' on it)
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected a label name";
            m_Error = err_str.str();
            err = ERROR_INVALID_LABEL_SYNTAX;
            return true;
        }

        // Find the token in the label list 
        labelName = sToken;
        defined = m_pSpec->m_Labels.find(labelName) != m_pSpec->m_Labels.end();

        // Test if the label already exists
        if (defined)
        {
            // Dupliate label!
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Label '" << labelName << "' already defined";
            m_Error = err_str.str();
            err = ERROR_INVALID_LABEL_SYNTAX;
            return true;
        }

        // Validate there is an active segment
        if (m_LastSegment == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Labels only valid in a segment.  Please define a segment.";
            m_Error = err_str.str();
            err = ERROR_INVALID_LABEL_SYNTAX;
            return true;
        }

        // Create a resource for the label
        Instruction_t *pInst = new Instruction_t;
        CResource* pRes = new CResource;

        // Populate with the label information
        pRes->m_pInst = pInst;
        pInst->type = TYPE_LABEL;
        pInst->name = labelName;
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by the assembler
        pInst->address = m_LastSegment->address;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        // Now add it to the map of labels
        m_pSpec->m_Labels.insert(std::pair<std::string, CResource *>(labelName, pRes));

        // Test if the label already exists in our LabelMap
        auto labelIter = m_pSpec->m_LabelMap.find(labelName);
        if (labelIter == m_pSpec->m_LabelMap.end())
        {
            CLabel *pLabel = new CLabel();
            pLabel->m_Name = labelName;
            pLabel->m_Segment = m_LastSegment->name;
            pLabel->m_Defined = 1;
            pLabel->m_Type = 0;
            pLabel->m_Filename = pFile->m_Filename;
            pLabel->m_Line = pFile->m_Line;
            m_pSpec->m_LabelMap.insert(std::pair<std::string, CLabel *>(labelName, pLabel));
            m_LastSegment->labels.insert(std::pair<std::string, CLabel *>(labelName, pLabel));
        }
        else
        {
            // Mark the label as defined
            labelIter->second->m_Defined = 1;
            labelIter->second->m_Segment = m_LastSegment->name;
            labelIter->second->m_Filename = pFile->m_Filename;
            labelIter->second->m_Line = pFile->m_Line;
        }
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect a label
    return false;
}

/* 
=============================================================================
Handle '.org' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForOrg(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".org";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 4) == 0) && iswhite(sLine[4]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .org keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to '.org'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the def
        Instruction_t *pInst = new Instruction_t;
        pInst->args.push_back(sToken);

        // Test for 2nd argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken != NULL)
        {
           pInst->args.push_back(sToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_ORG;
        pInst->size = 0;
        pInst->name = "org";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'org' keyword
    return false;
}

/* 
=============================================================================
Handle '.public' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForPublic(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".public";
    char*               sToken;
    char*               sNextToken;
    int                 defined;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 7) == 0) && iswhite(sLine[7]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .public keyword
        sToken = strtok_r(sLine, " \t,", &sNextToken);
        sToken = strtok_r(NULL, " \t,", &sNextToken);

        // Get the first argument
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to '.org'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);

           // See if we already know about this label
           auto iter = m_pSpec->m_LabelMap.find(sToken);
           if (iter == m_pSpec->m_LabelMap.end())
           {
             // Create a new label and add it to the segment and global
             CLabel *pNewLabel = new CLabel();
             pNewLabel->m_Name = sToken;
             pNewLabel->m_Type = SIZE_PUBLIC | SIZE_ABSOLUTE;
             pNewLabel->m_Line = 0;
             pNewLabel->m_Defined = 0;
             m_pSpec->m_LabelMap.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
             m_LastSegment->labels.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
           }
           else
           {
             // Mark the existing label as PUBLIC
             iter->second->m_Type |= SIZE_PUBLIC;
           }

           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = SIZE_PUBLIC;
        pInst->size = 0;
        pInst->name = "public";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'public' keyword
    return false;
}

/* 
=============================================================================
Handle '.local' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForLocal(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".local";
    char*               sToken;
    char*               sNextToken;
    int                 defined;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 6) == 0) && iswhite(sLine[6]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .public keyword
        sToken = strtok_r(sLine, " \t,", &sNextToken);
        sToken = strtok_r(NULL, " \t,", &sNextToken);

        // Get the first argument
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to '.org'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);

           // See if we already know about this label
           auto iter = m_pSpec->m_LabelMap.find(sToken);
           if (iter == m_pSpec->m_LabelMap.end())
           {
             // Create a new label and add it to the segment and global
             CLabel *pNewLabel = new CLabel();
             pNewLabel->m_Name = sToken;
             pNewLabel->m_Type = SIZE_LOCAL;
             pNewLabel->m_Line = 0;
             pNewLabel->m_Defined = 0;
             m_pSpec->m_LabelMap.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
             m_LastSegment->labels.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
           }
           else
           {
             // Mark the existing label as PUBLIC
             iter->second->m_Type |= SIZE_LOCAL;
           }

           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = SIZE_LOCAL;
        pInst->size = 0;
        pInst->name = "local";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'public' keyword
    return false;
}

/* 
=============================================================================
Handle 'ds' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForDs(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "ds";
    const   char*       sKey2 = ".ds";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 2) == 0 && iswhite(sLine[2])) ||
         (strncmp(sLine, sKey2, 3) == 0 && iswhite(sLine[3])))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the ds keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to 'ds'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);
           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_DS;
        pInst->size = 0;
        pInst->name = "ds";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'ds' keyword
    return false;
}

/* 
=============================================================================
Handle 'db' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForDb(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "db";
    const   char*       sKey2 = ".db";
    char*               sToken;
    char*               ptr;
    int                 inQuote;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 2) == 0 && iswhite(sLine[2])) ||
         (strncmp(sLine, sKey2, 3) == 0 && iswhite(sLine[3])))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the db keyword
        sToken = &sLine[3];
        while (*sToken && iswhite(*sToken))
            sToken++;

        // Validate we have at least one arg
        if (!*sToken)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to 'db'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Parse all arguments
        while (*sToken)
        {
            // Prepare to find end of token
            ptr = sToken;

            // Test if argument is a quoted string
            if (*sToken == '"')
            {
                // Skip the opening quote
                ptr++;

                // Parse the string
                while (*ptr && *ptr != '"')
                {
                    // Test for escaped quote
                    if (*ptr == '\\' && *(ptr+1))
                        ptr++;
                    ptr++;
                }

                // If we ended with quote, advance to terminate
                if (*ptr == '"')
                    ptr++;
            }
            else
            {
                // Separate arguments by whitespace or comma
                while (*ptr && *ptr != ',' && !iswhite(*ptr))
                    ptr++;
            }

            // Terminate the arg and add it
            if (*ptr)
            {
                *ptr++ = 0;
                pInst->args.push_back(sToken);
            }
            else
                pInst->args.push_back(sToken);

            // Advance sToken
            sToken = ptr;
            while (iswhite(*sToken))
                sToken++;
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_DB;
        pInst->size = 0;
        pInst->name = "db";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'db' keyword
    return false;
}

/* 
=============================================================================
Handle 'dw' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForDw(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = "dw";
    const   char*       sKey2 = ".dw";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 2) == 0 && iswhite(sLine[2])) ||
         (strncmp(sLine, sKey2, 3) == 0 && iswhite(sLine[3])))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the dw keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to 'ds'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);
           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_DW;
        pInst->size = 0;
        pInst->name = "dw";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'dw' keyword
    return false;
}

/* 
=============================================================================
Handle '.extern' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForExtern(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".extern";
    char*               sToken;
    char*               sNextToken;
    int                 defined;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 7) == 0) && iswhite(sLine[7]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .org keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected argument to '.org'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);

           // See if we already know about this label
           auto iter = m_pSpec->m_LabelMap.find(sToken);
           if (iter == m_pSpec->m_LabelMap.end())
           {
             // Create a new label and add it to the segment and global
             CLabel *pNewLabel = new CLabel();
             pNewLabel->m_Name = sToken;
             pNewLabel->m_Type = SIZE_EXTERN | SIZE_ABSOLUTE;
             pNewLabel->m_Line = 0;
             pNewLabel->m_Defined = 0;
             m_pSpec->m_LabelMap.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
             m_LastSegment->labels.insert(std::pair<std::string, CLabel *>(sToken, pNewLabel));
           }
           else
           {
             // Mark the existing label as EXTERN
             iter->second->m_Type |= SIZE_EXTERN;
           }
           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = SIZE_EXTERN;
        pInst->size = 0;
        pInst->name = "public";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect 'public' keyword
    return false;
}

/* 
=============================================================================
Handle '.file' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForFile(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".file";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 5) == 0) && iswhite(sLine[5]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .file keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected arguments to '.file'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);
           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_FILE;
        pInst->size = 0;
        pInst->name = "file";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect '.file' keyword
    return false;
}

/* 
=============================================================================
Handle '.loc' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForLoc(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sKey = ".loc";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for endian keyword
    if ((strncmp(sLine, sKey, 4) == 0) && iswhite(sLine[4]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the .file keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the first argument
        sToken = strtok_r(NULL, " \t,", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected arguments to '.loc'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DEFINE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the .public
        Instruction_t *pInst = new Instruction_t;

        // Test for arguments.  These will be labels we expect to find in the segment
        while (sToken != NULL)
        {
           pInst->args.push_back(sToken);
           sToken = strtok_r(NULL, " \t,", &sNextToken);
        }

        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_LOC;
        pInst->size = 0;
        pInst->name = "loc";
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect '.loc' keyword
    return false;
}

/* 
=============================================================================
Test if provided string is a constant
=============================================================================
*/
bool CParser::isConst(char *pBuf)
{
    int     x;
    bool    isConst = 1;

    // Test for 0x hex value
    if (strncmp(pBuf, "0x", 2) == 0)
    {
        // Test for all hex characters
        for (x = 2; x < strlen(pBuf); x++)
        {
            if (!isxdigit(pBuf[x]))
            {
                isConst = 0;
                break;
            }
        }
    }
    else
    {
        // Test for all decimal characters
        for (x = 2; x < strlen(pBuf); x++)
        {
            if (!isdigit(pBuf[x]))
            {
                isConst = 0;
                break;
            }
        }
    }

    return isConst;
}

/* 
=============================================================================
Handle detection of opcodes in the code
=============================================================================
*/
bool CParser::TestForOpcode(char* sLine, CParserFile* pFile, int32_t& err)
{
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         sOpcode;
    std::string         sArg;
    int                 x, y, len;
    char                buf[512];
    char*               pBuf;

    // Ensure this isn't a variable declaration
    if (strchr(sLine, '=') == NULL)
    {
        // Do opcode assembly only if in an assemble state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Get the opcode name
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Validate opcode name from the strtok
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected opcode name.";
            m_Error = err_str.str();
            err = ERROR_INVALID_OPCODE_SYNTAX;
            return true;
        }
        sOpcode = sToken;

        // Find the opcode in our table
        for (x = 0; x < gOpcodeCount; x++)
        {
            if (strcmp(gOpcodes[x].name, sToken) == 0)
            {
                // Opcode found!  break and process it
                break;
            }
        }

        // Test if opcode found or not
        if (x == gOpcodeCount)
        {
            // Opcode not found
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Unknown opcode '" << sToken << "'";
            m_Error = err_str.str();
            err = ERROR_INVALID_OPCODE_SYNTAX;
            return true;
        }

        // Create an Instruction object for the opcode
        Instruction_t *pInst = new Instruction_t;

        // Get all opcode arguments from the input string
        for (y = 0; y < gOpcodes[x].args; y++)
        {
            // Get the next argument
            sToken = strtok_r(NULL, ",\n", &sNextToken);

            // Test if an argument found
            if (sToken == NULL)
            {
                // Not enough arguments provided
                err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                    ": Expected " << gOpcodes[x].args << " arguments for opcode '" <<
                    sOpcode << "'";
                m_Error = err_str.str();
                err = ERROR_INVALID_OPCODE_SYNTAX;
                return true;
            }
            
            // Copy to a large static buffer for space trimming
            strncpy(buf, sToken, sizeof(buf));
            pBuf = buf;
            while (*pBuf == ' ')
               pBuf++;
            len = strlen(pBuf);
            while (pBuf[len-1] == ' ')
               pBuf[(len--) - 1] = '\0';

            // Add the argument to the instruction arg list
            pInst->args.push_back(pBuf);

            if (y == 0 && (gOpcodes[x].size & SIZE_LABEL) == SIZE_LABEL)
            {
                if (!isConst(pBuf))
                {
                    // This is a label argument.  Find label in our list
                    std::string labelName = pBuf;
                    auto labelIter = m_pSpec->m_LabelMap.find(labelName);
                    if (labelIter == m_pSpec->m_LabelMap.end())
                    {
                        CLabel *pLabel = new CLabel();
                        pLabel->m_Name = labelName;
                        pLabel->m_Defined = 0;
                        pLabel->m_Type = gOpcodes[x].size & SIZE_ABSOLUTE;
                        pLabel->m_Line = 0;
                        m_pSpec->m_LabelMap.insert(std::pair<std::string, CLabel *>(labelName, pLabel));
                        m_LastSegment->labels.insert(std::pair<std::string, CLabel *>(labelName, pLabel));
                    }
                    else
                    {
                        // Mark the label as ABSOLUTE if defined by the opcode
                        labelIter->second->m_Type |= (gOpcodes[x].size & SIZE_ABSOLUTE);
                    }
                }
            }
        }

        // Test for additional decription argument
        if (sNextToken != NULL)
        {
            // Skip whitespace and ','
            while (*sNextToken == ',' || *sNextToken == ' ')
                sNextToken++;

            if (*sNextToken != '\0')
            {
                pInst->args.push_back(sNextToken);
            }
        }
        
        // Create a resource for the instruction object
        CResource* pRes = new CResource;
        pRes->m_pInst = pInst;

        // Populate with our opcode data
        pInst->type = TYPE_OPCODE;
        if (m_Width == 16)
            pInst->value = gOpcodes16[x].value;
        else
            pInst->value = gOpcodes[x].value;
        pInst->size = gOpcodes[x].size;
        pInst->name = sOpcode;
        pInst->argc = gOpcodes[x].args;
        pInst->filename = pFile->m_Filename;
        pInst->line = pFile->m_Line;
        m_LastSegment->address += gOpcodes[x].size;

        // Address will be calculated by assembler
        pInst->address = 0;

        // Add resource to the parse context
        m_LastSegment->resources.push_back(pRes);
        
        err = ERROR_NONE;
        return true;
    }

    // Did not detect an opcode
    return false;
}

/* 
=============================================================================
Handle 'data' keyword in the assembled source
=============================================================================
*/
bool CParser::TestForData(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sData = "data";
    const   char*       sDataCap = "DATA";
    const   char*       s32 = "uint32_t";
    const   char*       s16 = "uint16_t";
    const   char*       s8 =  "uint8_t";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    std::string         dataName;
    uint32_t            dataBytes;

    // Test for data keyword
    if (((strncmp(sLine, sData, 4) == 0) || (strncmp(sLine, sDataCap, 4) == 0)) &&
        iswhite(sLine[4]))
    {
        // Include the file only if we are in an IF_ASSEMBLE state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
        {
            err = ERROR_NONE;
            return true;
        }

        // Skip the data keyword
        sToken = strtok_r(sLine, " \t", &sNextToken);

        // Get the name of the data
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected name after 'data'";
            m_Error = err_str.str();
            err = ERROR_INVALID_DATA_SYNTAX;
            return true;
        }

        // Save the name 
        dataName = sToken;

        // Get the data type
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if (sToken == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected 'uint32_t', 'uint16_t', or 'uint8_t' specification";
            m_Error = err_str.str();
            err = ERROR_INVALID_DATA_SYNTAX;
            return true;
        }

        // Test data type
        if (strcmp(sToken, s32) == 0)
            dataBytes = 4;
        else if (strcmp(sToken, s16) == 0)
            dataBytes = 2;
        else if (strcmp(sToken, s8) == 0)
            dataBytes = 1;
        else
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected 'uint32_t', 'uint16_t', or 'uint8_t' specification";
            m_Error = err_str.str();
            err = ERROR_INVALID_DATA_SYNTAX;
            return true;
        }

        // Search for '{'
        sToken = strtok_r(NULL, " \t", &sNextToken);
        if ((sToken == NULL) || (strncmp(sToken, "{", 1) != 0))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected '{' to specify data";
            m_Error = err_str.str();
            err = ERROR_INVALID_DATA_SYNTAX;
            return true;
        }

        // Test if the token has more than just '{', such as '{13,' 
        if (strlen(sToken) > 1)
        {
            // Restore token state and skip the '{' for next strtok_r
            sToken[strlen(sToken)] = ' ';
            sNextToken = sToken + 1;
        }

        // Validate there is an active segment
        if (m_LastSegment == NULL)
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Data keyword only valid in a segment";
            m_Error = err_str.str();
            err = ERROR_INVALID_DATA_SYNTAX;
            return true;
        }

        // Create a new resource to hold the data
        ResourceData_t* pResData = new ResourceData_t;
        CResource* pRes = new CResource;
        pRes->m_pData = pResData;
        pResData->name = dataName;
        pResData->elementSize = dataBytes;
        pResData->spec_filename = pFile->m_Filename;
        pResData->spec_line = pFile->m_Line;

        // Add resource to the spec
        m_LastSegment->resources.push_back(pRes);
        m_LastResData = pResData;

        // Change to DATA state in case we don't see a '}'
        m_ParseState = STATE_DATA;

        // Now read all data on this line
        AppendData(sNextToken, pFile);

        err = ERROR_NONE;
        return true;
    }

    // Not a data keyword
    return false;
}

// =============================================================================

int32_t CParser::AppendString(char* sLine, CParserFile* pFile)
{
    return 0;
}

// =============================================================================

int32_t CParser::AppendData(char* sLine, CParserFile* pFile)
{
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;
    char*               pTokStr = sLine;
    int                 array_state = 0, array_count, i;
    char*               sArrayValue;
    
    // Validate m_LastResData is valid
    if (m_LastResData == NULL)
        return ERROR_INVALID_DATA_SYNTAX;

    // Now read all data on this line
    while ((sToken = strtok_r(pTokStr, " \t,", &sNextToken)) != NULL)
    {
        // NULL out pTokStr to allow parsing the rest of the line
        pTokStr = NULL;

        // Test for '[' array specifier
        if (sToken[0] == '[')
        {
            array_state = 1;
            sToken++;
            if (sToken[0] == '\0')
                continue;
        }

        // Test if processing an array
        if (array_state > 0)
        {
            switch (array_state)
            {
            case 1:   // Get array value
                if (sToken[strlen(sToken)-1] == ']')
                {
                    // Invalid array syntax ... no array size given
                    return ERROR_INVALID_DATA_SYNTAX;
                }
                sArrayValue = sToken;
                array_state = 2;
                continue;

            case 2:   // Get array count
                if ((sToken[strlen(sToken)-1] == '}') && (sToken[strlen(sToken)-2] == ']'))
                {
                    array_state = 0;
                    m_ParseState = STATE_IDLE;
                } else if (sToken[strlen(sToken)-1] == ']')
                {
                    // Token includes termination ']'.  Zero it out
                    sToken[strlen(sToken)-1] = '\0';
                    array_state = 0;
                }

                // Get the array count and add sArrayValue to the resource
                sscanf(sToken, "%i", &array_count);
                for (i = 0; i < array_count; i++)
                {
                    m_LastResData->elements.push_back(sArrayValue);
                }
                if (m_ParseState == STATE_IDLE)
                {
                    *sToken = '\0';
                    break;
                }

                if (array_state != 0)
                    array_state = 3;
                continue;

            case 3:   // Get array closure
                if (sToken[0] == ']')
                {
                    array_state = 0;
                    sToken++;
                }
                else
                    return ERROR_INVALID_DATA_SYNTAX;

                if (sToken[0] == '\0')
                    continue;
                else if (sToken[0] == '}')
                {
                    m_ParseState = STATE_IDLE;
                    *sToken = '\0';
                }
                break;
            }
        }

        // Test for '}'
        if (sToken[strlen(sToken)-1] == '}')
        {
            // End of data set found
            m_ParseState = STATE_IDLE;
            sToken[strlen(sToken)-1] = '\0';
        }

        // Save this entry in the ResourceData elements list
        if (strlen(sToken) > 0)
        {  
            m_LastResData->elements.push_back(sToken);
        }

        if (m_ParseState == STATE_IDLE)
        {
            m_LastResData = NULL;
            break;
        }
    }

    return ERROR_NONE;
}

/* 
=============================================================================
Add an include path to the list of paths.
=============================================================================
*/
void CParser::AddInclude(const char *name)
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
    m_pSpec->m_IncPaths.push_back(path);
}

/* 
=============================================================================
Add a define to the defines map
=============================================================================
*/
void CParser::AddDefine(const char *name)
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

// =============================================================================

int32_t CParser::ParseLine(const char* sLine, CParserFile* pFile)
{
    char                    sMutable[512];
    char*                   sToken, *sNextToken;
    std::stringstream       err_str;
    int32_t                 err, c;

    // Copy const line to a mutable string
    strncpy(sMutable, sLine, sizeof sMutable);

    switch (m_ParseState)
    {
    case STATE_IDLE:
        // Remove leading whitespace
        while (*sLine == ' ' || *sLine == '\t')
            sLine++;

        // Test for blank line
        if (strlen(sLine) == 0)
        {
            if (m_ParseState == STATE_STRING)
                m_ParseState = STATE_IDLE;
            return ERROR_NONE;
        }

        // Test all keyword handlers for known keywords
        for (c = 0; c < m_keywordCount; c++)
        {
            char *ptr = sMutable;
            while (iswhite(*ptr))
               ptr++;
            if ((this->*m_pKeywords[c])(ptr, pFile, err))
                return err;
        }

        // Test if we are in a non-assemble state
        if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
           return ERROR_NONE;

        // Unknown keyword
        sToken = strtok_r(sMutable, " \t", &sNextToken);
        if (sToken != NULL)
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << ": Unknown keyword '" 
                << sToken << "'";
        else
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << ": Syntax error";
        m_Error = err_str.str();
        return ERROR_INVALID_SYNTAX;

    case STATE_STRING:
        // We are processing a multi-line string.  Get next portion of string
        if (sMutable[0] > ' ')
            sToken = strtok_r(sMutable, " \t", &sNextToken);
        else
            sToken = sMutable;

        // Append the string to the last string accessed
        return AppendString(sToken, pFile);

    case STATE_DATA:
        // We are processing a multi-line data statement.  Get next data
        if (!iswhite(sMutable[0]))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expecting '}' at end of data statement";
            printf("%s\n", err_str.str().c_str());
            m_ParseState = STATE_IDLE;
            
            // Re-parse this line in STATE_IDLE mode
            ParseLine(sLine, pFile);       

            // But return an error
            return ERROR_INVALID_DATA_SYNTAX;
        }

        // Whitespace on next line.  Continue reading data into m_LastResData
        return AppendData(sMutable, pFile);

    case STATE_COMMENT:
        return ERROR_NONE;

    default:
        // Parser error!
        return ERROR_PARSER_ERROR;
    }

    return ERROR_PARSER_ERROR;
}

/* 
=============================================================================
Parse an input file.  This may be called recursively in the case an 
'include' directive is encountered.
=============================================================================
*/
int32_t CParser::ParseFile(const char *pFilename, CParseCtx* pSpec)
{
    CParserFile file;
    char        sLine[512];
    FILE*       fd;
    int32_t     err, lastErr;
    bool        parseFailed;
    char*       pComment;
    bool        comment_block = false;

    // Try to open the file
    if ((fd = fopen(pFilename, "r")) == NULL)
    {
        printf("Error opening file %s\n", pFilename);
        return ERROR_CANT_OPEN_FILE;
    }

    // Initialize the CParserFile
    file.m_Filename = pFilename;
    file.m_Line = 0;

    // Save the assemply spec
    if (pSpec != NULL)
        m_pSpec = pSpec;
    m_pSpec->m_Filename = pFilename;

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
        if ((pComment = strstr(sLine, "//")) != NULL)
            *pComment = '\0';

        // Search for 'c' style comment or continuation of same
        while (((pComment = strstr(sLine, "/*")) != NULL) || (m_ParseState == STATE_COMMENT))
        {
            // Test for lines between open and close comment
            if (pComment == NULL)
            {
                // Test for close comment
                pComment = strstr(sLine, "*/");
                if (pComment == NULL)
                {
                    *sLine = '\0';
                    break;
                }
                else
                   pComment = sLine;
            }

            // Change comment bytes to spaces (whitespace)
            while (*pComment && strncmp(pComment, "*/", 2) != 0)
            {
                // Change anything in the comment to whitespace
                *pComment++ = ' ';
            }

            // Test if end of comment found above
            if (strncmp(pComment, "*/", 2) == 0)
            {
                // Remove the end comment
                *pComment++ = ' ';
                *pComment++ = ' ';
                m_ParseState = STATE_IDLE;
                comment_block = false;
            }
            else
            {
                comment_block = true;
            }
        }

        // Parse the line
        if ((err = ParseLine(sLine, &file)) != ERROR_NONE)
        {
            parseFailed = true;
            lastErr = err;
            printf("%s\n", m_Error.c_str());
            m_Error = "";
        }

        // Test if we need to transition to comment block mode
        if (comment_block)
        {
            m_ParseState = STATE_COMMENT;
        }
    }

    // Close the file
    fclose(fd);

    return lastErr;
}

// vim: sw=4 ts=4
