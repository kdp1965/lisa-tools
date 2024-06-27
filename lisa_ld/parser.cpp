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

// Define array of pointers to keyword handlers
CParserFuncPtr CParser::m_pKeywords[] = { 
    &CParser::TestForOutputArch, 
    &CParser::TestForEntry, 
    &CParser::TestForSections, 
    &CParser::TestForMemory,

    // Must be last in the list to prevent syntax errors
    &CParser::TestForVariable
};
uint32_t CParser::m_keywordCount = sizeof(CParser::m_pKeywords) / sizeof (CParserFuncPtr);

#define     iswhite(a)  (((a) == ' ') || ((a) == '\t'))

// =============================================================================

CParser::CParser(CParseCtx *pSpec) : m_pSpec(pSpec)
{
    m_ParseState  = 0;
    m_BraceLevel  = 0;
    m_ActiveSection = NULL;
}

/* 
=============================================================================
Handle variables in the assembled source
=============================================================================
*/
bool CParser::TestForVariable(char* sLine, CParserFile* pFile, int32_t& err)
{
    char*               sToken;
    char*               sNextToken;
    std::string         varName;
    std::string         valString;;
    std::stringstream   err_str;
    bool                conditional = false;
    uint32_t            valuel;
    char                tempName[256];
    char                temp[16];
    
    // Test for an '=' on the line to indicate a variable assignment
    if (strchr(sLine, '=') == NULL)
        return false;

    // Test for "?=" assignment
    if (strstr(sLine, "?=") != NULL)
        conditional = true;

    // Parse the variable name from the line
    sToken = strtok_r(sLine, " \t?=", &sNextToken);
    varName = sToken;

    // Now parse the value
    if (sNextToken == NULL)
    {
        err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
            ": Expected variable value after '='";
        m_Error = err_str.str();
        err = ERROR_INVALID_VAR_SYNTAX;
        return true;
    }

    // Skip the '=' and any leading whitespace
    while (*sNextToken == '?' || *sNextToken == '=' || *sNextToken == ' ' || *sNextToken == '\t')
        sNextToken++;

    // Remove trailing whitespace or semicolon from value
    sToken = sNextToken;
    sNextToken += strlen(sToken) - 1;
    while ((*sNextToken == ' ') || (*sNextToken == '\t') || (*sNextToken == ';'))
    {
        *sNextToken = '\0';
        sNextToken--;
    }

    // Test if variable already exists
    if (m_pSpec->m_Variables.find(varName) != m_pSpec->m_Variables.end())
    {
        if (conditional)
        {
            err = ERROR_NONE;
            return true;
        }
        err_str << pFile->m_Filename << ": Line " << pFile->m_Line << ": variable " <<
            varName << " already defined";
        m_Error = err_str.str();
        err = ERROR_INVALID_VAR_SYNTAX;
        return true;
    }

    // Test if a conditional assignment and variable exists in environment
    if (conditional && (getenv(varName.c_str()) != NULL))
    {
        // Don't assign.  It was conditional and the var exists in the environment
        err = ERROR_NONE;
        return true;
    }

    valString = sToken;
    if (EvaluateExpression(valString, valuel, pFile->m_Filename, pFile->m_Line) != ERROR_NONE)
    {
        err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
            ": Unable to evaluate expression";
        m_Error = err_str.str();
        err = ERROR_INVALID_VAR_SYNTAX;
        return true;
    }

    // Assign the variable
    sprintf(temp, "%d", valuel);
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, temp));

    sprintf(tempName, "%%lo(%s)", varName.c_str());
    sprintf(temp, "%d", valuel & 0xFF);
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(tempName, temp));
    sprintf(tempName, "%%hi(%s)", varName.c_str());
    sprintf(temp, "%d", (valuel >> 8) & 0xFF);
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(tempName, temp));

    // Provide %hi(variable) and %lo(variable) also

    err = ERROR_NONE;
    return true;
}

/* 
=============================================================================
Find Output Arch directive
=============================================================================
*/
bool CParser::TestForOutputArch(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sName = "output_arch";
    const   char*       sNameCap = "OUTPUT_ARCH";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for OUTPUT_ARCH keyword
    if ((strncmp(sLine, sName, 11) == 0) || (strncmp(sLine, sNameCap, 11) == 0))
    {
        // Skip the OUTPUT_ARCH keyword
        sToken = strtok_r(sLine, " \t()", &sNextToken);

        // Search for OUTPUT_ARCH name
        sToken = strtok_r(NULL, " \t()", &sNextToken);
        if ((sToken == NULL) || (strncmp(sToken, "//", 2) == 0))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected ARCH name after 'OUTPUT_ARCH'";
            m_Error = err_str.str();
            err = ERROR_INVALID_SEGMENT_FORMAT;
            return true;
        }
        
        // Save the segment name
        m_pSpec->m_OutputArch = sToken;

        err = ERROR_NONE;
        return true;
    }
    
    // Segment keyword not found
    return false;
}

/* 
=============================================================================
Find Entry directive
=============================================================================
*/
bool CParser::TestForEntry(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sName = "entry";
    const   char*       sNameCap = "ENTRY";
    char*               sToken;
    char*               sNextToken;
    std::stringstream   err_str;

    // Test for OUTPUT_ARCH keyword
    if ((strncmp(sLine, sName, 5) == 0) || (strncmp(sLine, sNameCap, 5) == 0))
    {
        // Skip the OUTPUT_ARCH keyword
        sToken = strtok_r(sLine, " \t()", &sNextToken);

        // Search for OUTPUT_ARCH name
        sToken = strtok_r(NULL, " \t()", &sNextToken);
        if ((sToken == NULL) || (strncmp(sToken, "//", 2) == 0))
        {
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << 
                ": Expected ARCH name after 'OUTPUT_ARCH'";
            m_Error = err_str.str();
            err = ERROR_INVALID_SEGMENT_FORMAT;
            return true;
        }
        
        // Save the segment name
        m_EntryLabel = sToken;

        return true;
    }
    
    // Segment keyword not found
    return false;
}

/* 
=============================================================================
Test for Memory directive
=============================================================================
*/
bool CParser::TestForMemory(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sName = "memory";
    const   char*       sNameCap = "MEMORY";

    // Test for OUTPUT_ARCH keyword
    if ((strncmp(sLine, sName, 6) == 0) || (strncmp(sLine, sNameCap, 6) == 0))
    {
        // Set the parse state
        m_ParseState = STATE_MEMORY;
        err = ERROR_NONE;
        return true;
    }
    
    // Segment keyword not found
    return false;
}

/* 
=============================================================================
Test for Sections directive
=============================================================================
*/
bool CParser::TestForSections(char* sLine, CParserFile* pFile, int32_t& err)
{
    const   char*       sName = "sections";
    const   char*       sNameCap = "SECTIONS";

    // Test for OUTPUT_ARCH keyword
    if ((strncmp(sLine, sName, 8) == 0) || (strncmp(sLine, sNameCap, 8) == 0))
    {
        // Set the parse state
        m_ParseState = STATE_SECTIONS;
        m_ParseSectionState = STATE_IDLE;
        err = ERROR_NONE;
        return true;
    }
    
    // Segment keyword not found
    return false;
}

/* 
=============================================================================
Evaluate an expression that can contain addition and subtraction
=============================================================================
*/
int32_t CParser::EvaluateExpression(std::string& sExpr, uint32_t& value,
        std::string& sFilename, uint32_t lineNo)
{
    int32_t                 err = ERROR_NONE;
    uint32_t                tempVal;
    std::string             sSubst;
    std::string             varValue;
    char                    opStack[20];
    int                     opStackIdx = 0;
    int32_t                 valStack[20];
    int                     valStackIdx = 0;
    int                     len, x, parenLevel;

    // Perform variable substitution
    if ((err = SubstituteVariables(sExpr, sSubst, sFilename, lineNo)) != ERROR_NONE)
        return err;

    // Ensure + or - have spaces around them
    for (x = 0; x < sSubst.length(); x++)
    {
        if (sSubst[x] == '+')
        {
            sSubst.replace(x, 1, " + ");
            x += 2;
        }
        else if (sSubst[x] == '-')
        {
            sSubst.replace(x, 1, " - ");
            x += 2;
        }
    }

    // Setup our evaluation stacks
    opStack[0] = '=';
    opStackIdx = 1;
    valStackIdx = 0;

    // Parse the equation
    len = sSubst.length();
    for (x = 0; x <= len && err == ERROR_NONE; )
    {
        switch (sSubst[x])
        {
            case '\0':
            case ' ':
                // Test if we have something to evaluate
                if (strlen(varValue.c_str()) > 0)
                {
                    // Evaluate the token
                    err = EvaluateToken(varValue, tempVal, sFilename, lineNo);
                    if (err == ERROR_NONE && opStackIdx > 0)
                    {
                        // Test for = operation
                        if (opStack[opStackIdx-1] == '=')
                        {
                            valStack[valStackIdx++] = tempVal;
                            opStackIdx--;
                        }

                        // Test for & operation ... higher precedence
                        else if (opStack[opStackIdx-1] == '&')
                        {
                            valStack[valStackIdx-1] &= tempVal;
                            opStackIdx--;
                        }

                        // Must be + / - operation ... push value to stack
                        // to test for higher precidence & operation following
                        else
                        {
                            valStack[valStackIdx++] = tempVal;
                        }
                    }
                }

                varValue = "";

                // Advance to next character
                x++;

                break;
           
            case '-':
                opStack[opStackIdx++] = '-';
                x++;
                break;

            case '+':
                opStack[opStackIdx++] = '+';
                x++;
                break;

            case '(':
                parenLevel = 1;
                x++;
                while (x <= len && parenLevel > 0)
                {
                   if (sSubst[x] == '(')
                   {
                      parenLevel++;
                      varValue += '(';
                   }
                   else if (sSubst[x] == ')')
                   {
                      parenLevel--;
                      if (parenLevel > 0)
                         varValue += ')';
                   }
                   else
                   {
                     varValue += sSubst[x];
                   }
                   x++;
                }

                // Test for balanced parenthesis
                if (parenLevel > 0)
                {
                    printf("%s: Line %d: Unbalanced parenthesis\n", sFilename.c_str(), lineNo);
                    err = ERROR_SEGMENT_NOT_LOCATED;
                }
                
                err = EvaluateExpression(varValue, tempVal, sFilename, lineNo);
                varValue = "";
                if (err == ERROR_NONE)
                {
                    // Test for = operation
                    if (opStack[opStackIdx-1] == '=')
                    {
                        valStack[valStackIdx++] = tempVal;
                        opStackIdx--;
                    }
                    // Test for & operation ... higher precedence
                    else if (opStack[opStackIdx-1] == '&')
                    {
                        valStack[valStackIdx-1] &= tempVal;
                        opStackIdx--;
                    }

                    // Must be | operation ... push value to stack
                    // to test for higher precidence & operation following
                    else
                    {
                        valStack[valStackIdx++] = tempVal;
                    }
                }
                break;

            default:
                varValue += sSubst[x];
                x++;
                break;
        }
    }

    // Unwind anything on the operation stack
    while (opStackIdx)
    {
       switch (opStack[--opStackIdx])
       {
            // Test for = operation
            case '+':
                tempVal = valStack[--valStackIdx];
                valStack[valStackIdx-1] += tempVal;
                break;

            case '-':
                tempVal = valStack[--valStackIdx];
                valStack[valStackIdx-1] -= tempVal;
                break;
       }
    }

    // Test for invalid equation
    if (valStackIdx != 1)
    {
        printf("%s: Line %d: Invalid equation\n", sFilename.c_str(), lineNo);
        err = ERROR_SEGMENT_NOT_LOCATED;
    }
    else
        value = valStack[0];

    return err;
}

/* 
=============================================================================
Evaluate a token
=============================================================================
*/
int32_t CParser::EvaluateToken(std::string& sExpr, uint32_t& value,
        std::string& sFilename, uint32_t lineNo)
{
    int32_t                 err = ERROR_NONE;
    StrStrMap_t::iterator   varIter;
    std::string             varValue;

    // Do a lookup in case it is a define
    varValue = sExpr;
    if ((varIter = m_pSpec->m_Variables.find(sExpr)) != m_pSpec->m_Variables.end())
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
    while ((found = sSubst.find("(")) != std::string::npos)
    {
        found -= 6;
        // Find end of variable to perform lookup
        pStr = sSubst.c_str();
        for (c = found; pStr[c] != '\0' && pStr[c] != ')'; c++)
            ;

        // Test if end of variable found
        if (pStr[c] == ')')
        {
            // Get variable name
            std::string varName = sSubst.substr(found, c-found+1);
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
Parse a line from the MEMORY section
=============================================================================
*/
int32_t CParser::MemorySpecLine(char* sLine, CParserFile* pFile)
{
    char*               sToken;
    char*               sNextToken;
    std::string         name;
    std::string         access;
    uint32_t            origin;
    uint32_t            length;
    char                units;
    int                 lineNo = pFile->m_Line;

    // Get the memory region name
    sToken = strtok_r(sLine, " \t(", &sNextToken);
    if (sToken == NULL)
    {
        printf("%s: Line %d: Expected memory region name\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    name = sToken;

    // Parse the access modes
    sToken = strtok_r(NULL, " \t():", &sNextToken);
    if (sToken == NULL)
    {
        printf("%s: Line %d: Expected memory access modes\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    access = sToken;

    // Parse the ORIGIN
    sToken = strtok_r(NULL, " \t():=", &sNextToken);
    if (sToken == NULL || strcmp(sToken, "ORIGIN") != 0)
    {
        printf("%s: Line %d: Expected memory ORIGIN\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    sToken = strtok_r(NULL, " \t():=", &sNextToken);
    if (sToken == NULL)
    {
        printf("%s: Line %d: Expected memory ORIGIN\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    origin = strtol(sToken, NULL, 0);

    // Parse the LENGTH
    sToken = strtok_r(NULL, " \t():=", &sNextToken);
    if (sToken == NULL || strcmp(sToken, "LENGTH") != 0)
    {
        printf("%s: Line %d: Expected memory LENGTH\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    sToken = strtok_r(NULL, " \t():=", &sNextToken);
    if (sToken == NULL)
    {
        printf("%s: Line %d: Expected memory LENGTH\n", pFile->m_Filename.c_str(), lineNo);
        return ERROR_INVALID_SYNTAX;
    }
    length = atoi(sToken);
    units = sToken[strlen(sToken)-1];
    if (units == 'K' || units == 'k')
        length *= 1024;
    else if (units == 'M' || units == 'm')
        length *= 1024 * 1024;

    // Create a new memory region and add it to our map
    CMemory *pMem = new CMemory;
    pMem->m_Name = name;
    pMem->m_Access = access;
    pMem->m_Origin = origin;
    pMem->m_Length = length;
    pMem->m_Address = origin;
    m_pSpec->m_MemoryMap.insert(std::pair<std::string, CMemory *>(name, pMem));

    // Add ORIGIN and LENGTH as variables
    char    varName[64];
    char    varValue[16];

    sprintf(varName, "ORIGIN(%s)", name.c_str());
    sprintf(varValue, "%d", origin);
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, varValue));
    sprintf(varName, "%s_origin", name.c_str());
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, varValue));

    sprintf(varName, "LENGTH(%s)", name.c_str());
    sprintf(varValue, "%d", length);
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, varValue));
    sprintf(varName, "%s_length", name.c_str());
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(varName, varValue));

    return ERROR_NONE;
}

/*
=============================================================================
Parse a line from the SECTIONS section
=============================================================================
*/
int32_t CParser::SectionsLine(char* sLine, CParserFile* pFile)
{
    std::string     name;
    char           *sToken;
    char           *sNextToken;
    char           *pColon;
    int             type = 0;
    int             intParam = 0;

    // Keep track of brace level
    if (*sLine == '{')
    {
        m_BraceLevel++;
        if (m_BraceLevel == 2 && m_ActiveSection == NULL)
        {
            printf("%s: Line %d: Expecting section 'name :' before open brace\n",
                    pFile->m_Filename.c_str(), pFile->m_Line);
            return ERROR_INVALID_SYNTAX;
        }

        m_ParseSectionState = STATE_SECTIONS;
    }
    else if (*sLine == '}')
    {
        m_BraceLevel--;
        if (m_BraceLevel == 1)
        {
            // Go back to STATE_IDLE for the section
            m_ParseSectionState = STATE_IDLE;

            // Get the '}' token
            sToken = strtok_r(sLine, " \t", &sNextToken);

            // The closing section brace should specify the memory region
            sToken = strtok_r(NULL, " \t", &sNextToken);
            if (!sToken || strcmp(sToken, ">") != 0)
            {
                printf("%s: Line %d: Expecting '>' memory specification\n",
                        pFile->m_Filename.c_str(), pFile->m_Line);
                return ERROR_INVALID_SYNTAX;
            }

            // Get the memory region name
            sToken = strtok_r(NULL, " \t", &sNextToken);
            if (!sToken)
            {
                printf("%s: Line %d: Expecting memory region name after '>'\n",
                        pFile->m_Filename.c_str(), pFile->m_Line);
                return ERROR_INVALID_SYNTAX;
            }

            // Find memory region in our m_MemoryMap
            auto it = m_pSpec->m_MemoryMap.find(sToken);
            if (it == m_pSpec->m_MemoryMap.end())
            {
                printf("%s: Line %d: Unknown memory region %s\n",
                        pFile->m_Filename.c_str(), pFile->m_Line, sToken);
                return ERROR_UNKNOWN_MEMORY_REGION;
            }

            // Assign the memory region
            m_ActiveSection->m_MemRegion = sToken;
            m_ActiveSection->m_pMem = it->second;
            
            // Test for AT keyword 
            sToken = strtok_r(NULL, " \t", &sNextToken);
            if (sToken)
            {
                // Test if token is "AT"
                if (strcmp(sToken, "AT") != 0)
                {
                    printf("%s: Line %d: Unknown directive %s\n",
                            pFile->m_Filename.c_str(), pFile->m_Line, sToken);
                    return ERROR_UNKNOWN_DIRECTIVE;
                }

                // Get the '>' token
                sToken = strtok_r(NULL, " \t", &sNextToken);
                if (!sToken || strcmp(sToken, ">") != 0)
                {
                    printf("%s: Line %d: Expecting '>' following 'AT'\n",
                            pFile->m_Filename.c_str(), pFile->m_Line);
                    return ERROR_INVALID_SYNTAX;
                }

                // Get the AT memory region
                sToken = strtok_r(NULL, " \t", &sNextToken);
                if (!sToken)
                {
                    printf("%s: Line %d: Expecting memory region name after '>'\n",
                            pFile->m_Filename.c_str(), pFile->m_Line);
                    return ERROR_INVALID_SYNTAX;
                }

                it = m_pSpec->m_MemoryMap.find(sToken);
                if (it == m_pSpec->m_MemoryMap.end())
                {
                    printf("%s: Line %d: Unknown memory region %s\n",
                            pFile->m_Filename.c_str(), pFile->m_Line, sToken);
                    return ERROR_UNKNOWN_MEMORY_REGION;
                }

                // Assign the memory region
                m_ActiveSection->m_MemAtRegion = sToken;
                m_ActiveSection->m_pAtMem = it->second;
            }
        }
    }
    else if (m_BraceLevel == 1)
    {
        // First line before open brace should contain section name and ':'
        pColon = strchr(sLine, ':');
        sToken = strtok_r(sLine, " \t:", &sNextToken);
        if (!pColon || !sToken)
        {
            printf("%s: Line %d: Expecting section 'name :' syntax\n",
                    pFile->m_Filename.c_str(), pFile->m_Line);
            return ERROR_INVALID_SYNTAX;
        }

        // Make sure this section doesn't already exist
        auto it = m_pSpec->m_SectionList.begin();
        while (it != m_pSpec->m_SectionList.end())
        {
            if ((*it)->m_Name == sToken)
            {
                printf("%s: Line %d: Section %s already declared.\n",
                        pFile->m_Filename.c_str(), pFile->m_Line, sToken);
                return ERROR_DUPLICATE_SECTION;
            }

            // Advance to next section
            it++;
        }

        // Create a new section 
        CSection *pSection = new CSection();
        pSection->m_Name = sToken;
        m_pSpec->m_SectionList.push_back(pSection);
        m_ActiveSection = pSection;
    }
    else
    {
        // Section operation specification.  First test for label assignment
        if (strchr(sLine, '=') != NULL)
        {
            // Remove terminating ';'
            if (sLine[strlen(sLine)-1] == ';')
                sLine[strlen(sLine)-1] = 0;

            // Parse line manually
            sToken = sLine;
            while (*sToken && *sToken != '=' && !iswhite(*sToken))
                sToken++;

            // If white space, NULL terminate and advance to '='
            if (iswhite(*sToken))
            {
                *sToken++ = 0;
                while (iswhite(*sToken))
                    sToken++;
            }

            // Validate a single value on left of =
            if (*sToken != '=')
            {
                *sToken = 0;
                printf("%s: Line %d: Expected '=' after lvalue %s\n",
                        pFile->m_Filename.c_str(), pFile->m_Line, sLine);
                return ERROR_INVALID_SYNTAX;
            }

            // NULL terminate
            if (*sToken)
                *sToken++ = 0;

            // Test if assigning PC to a variable or value to PC
            if (strcmp(sLine, ".") == 0)
            {
                // Setup for assignment of value to PC
                type = OP_ASSIGN_PC;
            }
            else
            {
                // Setup for assignment of PC to var
                type = OP_ASSIGN_VAR;
                name = sLine;
            }

            // Get next non-white token after the =
            while (*sToken && iswhite(*sToken))
                sToken++;
            if (!*sToken)
            {
                printf("%s: Line %d: Expected rvalue after '='\n",
                        pFile->m_Filename.c_str(), pFile->m_Line);
                return ERROR_INVALID_SYNTAX;
            }

            // If this is OP_ASSIGN_VAR, then we expect the argument to be '.'
            if (type == OP_ASSIGN_VAR)
            {
                if (strcmp(sToken, ".") != 0)
                {
                    printf("%s: Line %d: Expected '.' after '=', got %s\n",
                            pFile->m_Filename.c_str(), pFile->m_Line, sToken);
                    return ERROR_INVALID_SYNTAX;
                }
            }
            else
            {
                name = sToken;
            }

            // Create the new op
            COperation *pOp = new COperation();
            pOp->m_Type = type;
            pOp->m_StrParam = name;
            pOp->m_IntParam = 0;
            m_ActiveSection->m_Ops.push_back(pOp);
        }
        else
        {
            // Test for KEEP( modifier
            if (strncmp(sLine, "KEEP(", 5) == 0 && sLine[strlen(sLine)-1] == ')')
            {
                // Record the KEEP state and remove from line
                intParam = PARAM_KEEP;
                sLine += 5;
                while (*sLine && iswhite(*sLine))
                    sLine++;

                // Remove the trailing ')'
                sLine[strlen(sLine)-1] = 0;
            }

            // Validate we have a load seciton
            if (!*sLine)
            {
                printf("%s: Line %d: Expected section load specification\n",
                        pFile->m_Filename.c_str(), pFile->m_Line);
                return ERROR_INVALID_SYNTAX;
            }

            // Create the new op
            COperation *pOp = new COperation();
            pOp->m_Type = OP_LOAD_SECTION;
            pOp->m_StrParam = sLine;
            pOp->m_IntParam = intParam;
            m_ActiveSection->m_Ops.push_back(pOp);
        }
    }
    return ERROR_NONE;
}

/*
=============================================================================
Parse next line of the linker script
=============================================================================
*/
int32_t CParser::ParseLine(const char* sLine, CParserFile* pFile)
{
    char                    sMutable[512];
    char*                   sToken, *sNextToken;
    std::stringstream       err_str;
    int32_t                 err, c;

    // Remove leading whitespace
    while (*sLine == ' ' || *sLine == '\t')
        sLine++;

    // Copy const line to a mutable string
    strncpy(sMutable, sLine, sizeof sMutable);

    // Test for blank line
    if (strlen(sLine) == 0)
    {
        return ERROR_NONE;
    }

    switch (m_ParseState)
    {
    case STATE_IDLE:
        // Test all keyword handlers for known keywords
        for (c = 0; c < m_keywordCount; c++)
        {
            char *ptr = sMutable;
            while (iswhite(*ptr))
               ptr++;
            if ((this->*m_pKeywords[c])(ptr, pFile, err))
                return err;
        }

        // Unknown keyword
        sToken = strtok_r(sMutable, " \t", &sNextToken);
        if (sToken != NULL)
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << ": Unknown keyword '" 
                << sToken << "'";
        else
            err_str << pFile->m_Filename << ": Line " << pFile->m_Line << ": Syntax error";
        m_Error = err_str.str();
        return ERROR_INVALID_SYNTAX;

    case STATE_MEMORY:
        // Test for end of MEMORY section
        if (strcmp(sLine, "}") == 0 && m_BraceLevel == 1)
        {
            m_BraceLevel = 0;
            m_ParseState = STATE_IDLE;
            return ERROR_NONE;
        }
        else if (strcmp(sLine, "{") == 0 && m_BraceLevel == 0)
        {
            m_BraceLevel = 1;
            return ERROR_NONE;
        }

        // We are processing a multi-line MEMORY specification.
        return MemorySpecLine(sMutable, pFile);

    case STATE_SECTIONS:
        // Test for end of SECTIONS section
        if (strcmp(sLine, "}") == 0 && m_BraceLevel == 1)
        {
            m_BraceLevel = 0;
            m_ParseState = STATE_IDLE;
            m_ActiveSection = NULL;
            return ERROR_NONE;
        }
        else if (strcmp(sLine, "{") == 0 && m_BraceLevel == 0)
        {
            m_BraceLevel = 1;
            return ERROR_NONE;
        }

        // Whitespace on next line.  Continue reading data into m_LastResData
        return SectionsLine(sMutable, pFile);

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
int32_t CParser::ParseLinkerScript(const char *pFilename, CParseCtx* pSpec)
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

        // Remove trailing whitespace
        while (iswhite(sLine[strlen(sLine)-1]))
            sLine[strlen(sLine)-1] = 0;

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
                if (m_ParseState == STATE_COMMENT)
                    m_ParseState = m_ParsePushState;
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
            if (m_ParseState != STATE_COMMENT)
                m_ParsePushState = m_ParseState;
            m_ParseState = STATE_COMMENT;
        }
    }

    // Close the file
    fclose(fd);

    // Print the section info
    if (m_DebugLevel > 1)
    {
        auto it = m_pSpec->m_SectionList.begin();
        while (it != m_pSpec->m_SectionList.end())
        {
            printf("Section: %s\n", (*it)->m_Name.c_str());
            auto opit = (*it)->m_Ops.begin();
            while (opit != (*it)->m_Ops.end())
            {
                switch ((*opit)->m_Type)
                {
                    case OP_ASSIGN_VAR:
                        printf("   %s = .\n", (*opit)->m_StrParam.c_str());
                        break;
        
                    case OP_ASSIGN_PC:
                        printf("   . = %s\n", (*opit)->m_StrParam.c_str());
                        break;
        
                    case OP_LOAD_SECTION:
                        printf("   LOAD %s%s\n", (*opit)->m_StrParam.c_str(),
                                (*opit)->m_IntParam == 1 ? "(K)" : "");
                        break;
                }
        
                opit++;
            }
        
            it++;
        }
    }

    return lastErr;
}

// vim: sw=4 ts=4
