// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : assembler.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Performs assembly operations after the parse phase.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <algorithm>

#include "assembler.h"
#include "errors.h"
#include "srec.h"
#include "elfload.h"
#include "elf.h"
#include "intelhex.h"

using namespace std;

// =============================================================================

CAssembler::CAssembler()
{
    m_LastSection = NULL;
    m_LastResData = NULL;
    m_minAddr = 0xFFFFFFFF;
    m_maxAddr = 0;
    m_pData = NULL;
    m_pTempData = NULL;
    m_Mixed = false;
}

// =============================================================================

CAssembler::~CAssembler()
{
    // Destroy output buffer
    if (m_pData != NULL)
        delete[] m_pData;

    if (m_pTempData != NULL)
        delete[] m_pTempData;
}

// =============================================================================

int32_t CAssembler::PerformCodeAssembly(ResourceSection_t* pSection, CResource* pRes)
{
    Instruction_t*          pInst = pRes->m_pInst;
    int32_t                 ret = ERROR_NONE;
    StrList_t::iterator     it;
    StrLabelMap_t::iterator labelIt;
    std::string             sarg[8];
    std::string             externLabel;
    int                     isExtern;
    int                     isLabel;
    int                     diff;
    int                     divRemOffset = 0;
    int                     commaValue;
    const char             *pStr;

    // Up to 3 arguments
    uint32_t                arg[3];
    uint32_t                op1;
    int                     argc;
    uint16_t                op_reti;

    if (m_Width == 16)
        op_reti = OPCODE16_RETI;
    else
        op_reti = OPCODE_RETI;
    
    arg[0] = 0;
    arg[1] = 0;
    arg[2] = 0;
    isExtern = 0;
    isLabel  = 0;

    switch (pInst->type)
    {
        case SIZE_PUBLIC:
            // Validate the public symbol was defined
            labelIt = m_pSpec->m_LabelMap.find(pInst->args.front());
            if (labelIt == m_pSpec->m_LabelMap.end())
            {
                printf("%s: Line %d: PUBLIC Label %s not defined\n", 
                    pInst->filename.c_str(), pInst->line, pInst->args.front().c_str());
                return ERROR_LABEL_NOT_DEFINED;
            }

            // Test if the label was defined
            if (labelIt->second->m_Defined == 0)
            {
                printf("%s: Line %d: PUBLIC Label %s not defined\n", 
                    pInst->filename.c_str(), pInst->line, pInst->args.front().c_str());
                return ERROR_LABEL_NOT_DEFINED;
            }

            // Tell the linker of our public symbol
            fprintf(m_pOutFile, "p %s 0x%04X\n", pInst->args.front().c_str(), labelIt->second->m_Address);
            break;

        case SIZE_LOCAL:
            // Validate the local symbol was defined
            labelIt = m_pSpec->m_LabelMap.find(pInst->args.front());
            if (labelIt == m_pSpec->m_LabelMap.end())
            {
                printf("%s: Line %d: LOCAL Label %s not defined\n", 
                    pInst->filename.c_str(), pInst->line, pInst->args.front().c_str());
                return ERROR_LABEL_NOT_DEFINED;
            }

            // Test if the label was defined
            if (labelIt->second->m_Defined == 0)
            {
                printf("%s: Line %d: LOCAL Label %s not defined\n", 
                    pInst->filename.c_str(), pInst->line, pInst->args.front().c_str());
                return ERROR_LABEL_NOT_DEFINED;
            }

            // Tell the linker of our public symbol
            fprintf(m_pOutFile, "l %s 0x%04X\n", pInst->args.front().c_str(), labelIt->second->m_Address);
            break;

        case TYPE_LABEL:
            // Upate the label address
            m_LastLabel = pInst->name;
            m_LabelInst = 0;
            pInst->address = pSection->address; 
            fprintf(m_pOutFile, "# %s (0x%04X):\n", pInst->name.c_str(), pInst->address);
            break;

        case TYPE_OPCODE:
            // Update the address of this opcode
            pInst->address = pSection->address; 

            // Resolve arguments
            it = pInst->args.begin();
            argc = 0;
            while (it != pInst->args.end())
            {
                // Save arg for debug output
                sarg[argc] = *it;

                // Evaluate each argument
                if ((*it)[0] == '%')
                {
                    isLabel = 1;
                    isExtern = 1;
                    externLabel = *it;
                }
                else
                {
                    bool isStxx = pInst->value == OPCODE_STXX || pInst->value == OPCODE_LDXX ||
                                  pInst->value == OPCODE16_STXX || pInst->value == OPCODE16_LDXX;
                    commaValue = -1;
                    if ((ret = EvaluateExpression(*it, arg[argc], pInst->filename,
                        pInst->line, commaValue, isStxx)) != ERROR_NONE)
                    {
                        printf("%s: Line %d: Argument %s has no value\n", pInst->filename.c_str(),
                                pInst->line, (*it).c_str());
                        ret = ERROR_VARIABLE_NOT_FOUND;
                    }

                    // Test if argument is an extern label
                    labelIt = m_pSpec->m_LabelMap.find(*it);
                    if (labelIt != m_pSpec->m_LabelMap.end())
                    {
                        isLabel = 1;
                        if (labelIt->second->m_Type & SIZE_EXTERN)
                        {
                            isExtern = 1;
                            externLabel = *it;
                        }
                    }
                }

                it++;
                argc++;
            }
            if (ret != ERROR_NONE)
                return ret;

            // Set the base opcode register value
            op1 = pInst->value;

            // Add modiications to the base register as needed
            switch (pInst->value)
            {
                case OPCODE_BNZ:
                case OPCODE_BR:
                case OPCODE_BZ:
                    // These are relative jumps
                    diff = arg[0] - pInst->address;

                    // Validate the jump distance
                    if (diff > 255 || diff < -256)
                    {
                        printf("%s: Line %d: Branch distance (%d) too big\n",
                                pInst->filename.c_str(),
                                pInst->line, diff);
                        ret = ERROR_BRANCH_DISTANCE_TOO_BIG;
                        break;
                    }

                    op1 |= diff & 0x1FF;
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    break; 

                case OPCODE16_BNZ:
                case OPCODE16_BR:
                case OPCODE16_BZ:
                    // These are relative jumps
                    diff = arg[0] - pInst->address;

                    // Validate the jump distance
                    if (diff > 1023 || diff < -1204)
                    {
                        printf("%s: Line %d: Branch distance (%d) too big\n",
                                pInst->filename.c_str(),
                                pInst->line, diff);
                        ret = ERROR_BRANCH_DISTANCE_TOO_BIG;
                        break;
                    }

                    op1 |= diff & 0x7FF;
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    break; 

                case OPCODE_JAL:
                    if (m_Width == 16)
                        op1 |= (arg[0] & 0x7FFF);
                    else
                        op1 |= (arg[0] & 0x1FFF);
                    if (isExtern)
                    {
                        fprintf(m_pOutFile, "e 0x%04X %s  # %s   %-8s\n", op1,
                          externLabel.c_str(), pInst->name.c_str(),
                          externLabel.c_str());
                    }
                    else
                        if (isLabel)
                            fprintf(m_pOutFile, "r 0x%04X %s%s # %-8s%s\n", op1,
                              m_pSpec->m_ModuleName.c_str(),
                              labelIt->second->m_Segment.c_str(),
                              pInst->name.c_str(), sarg[0].c_str());
                        else
                            fprintf(m_pOutFile, "r 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                              sarg[0].c_str());
                    break;

                case OPCODE16_LDX:
                case OPCODE_LDX:
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    if (isExtern)
                        fprintf(m_pOutFile, "e 0x0000 %s\n", externLabel.c_str());
                    else if (isLabel)
                        fprintf(m_pOutFile, "R 0x%04X %s%s\n", arg[0],
                              m_pSpec->m_ModuleName.c_str(),
                              labelIt->second->m_Segment.c_str());
                    else
                        fprintf(m_pOutFile, "i 0x%04X\n", arg[0]);
                    break;

                case OPCODE16_LDDIV:
                case OPCODE_LDDIV:
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    fprintf(m_pOutFile, "i 0x%04X\n", arg[0]);
                    break;

                case OPCODE16_DIV:
                case OPCODE_DIV:
                case OPCODE16_REM:
                case OPCODE_REM:
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1 | arg[0], pInst->name.c_str(),
                          sarg[0].c_str());
                    if ((arg[0] & 1) == 0)
                        fprintf(m_pOutFile, "i 0x%04X\n", commaValue);
                    break;

                case OPCODE16_ADS:
                case OPCODE16_ADX:
                    op1 |= arg[0] & 0x3FF;
                    fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    break;

                case OPCODE16_LDI:
                case OPCODE_LDI:
                    if (isExtern)
                    {
                        fprintf(m_pOutFile, "e 0x%04X %s  # %s   %-8s\n", op1,
                          externLabel.c_str(), pInst->name.c_str(),
                          externLabel.c_str());
                        break;
                    }

                    // Else fall through to default

                default:
                    if (m_Width == 16)
                        op1 |= arg[0] & 0x3FF;
                    else
                        op1 |= arg[0] & 0xFF;
                    if (pInst->argc == 1)
                      fprintf(m_pOutFile, "i 0x%04X  # %-8s%s\n", op1, pInst->name.c_str(),
                          sarg[0].c_str());
                    else
                      fprintf(m_pOutFile, "i 0x%04X  # %-8s\n", op1, pInst->name.c_str());
                    break;
            }

            // Test if mixed assembly / source mode requested
            if (m_Mixed)
            {
                if (pInst->size == 1)
                   fprintf(m_pOutFile, "# %s\n", pInst->name.c_str());
                else if (pInst->size == 2)
                   fprintf(m_pOutFile, "# %s   %s, %s\n", pInst->name.c_str(), sarg[0].c_str(),
                         sarg[1].c_str());
                else if (pInst->size == 3)
                   fprintf(m_pOutFile, "# %s   %s, %s, %s\n", pInst->name.c_str(), sarg[0].c_str(),
                         sarg[1].c_str(), sarg[2].c_str());
            }

            
            m_LabelInst++;

            // Update the section address
            pSection->address += pInst->size & 0xF;
            break;

        case TYPE_ORG:
            // Evaluate the org
            it = pInst->args.begin();

            // Evaluate the 1st argument
            if ((ret = EvaluateExpression(*it, arg[0], pInst->filename, commaValue,
                pInst->line)) != ERROR_NONE)
            {
                printf("%s: Line %d: Argument %s has no value\n", pInst->filename.c_str(),
                        pInst->line, (*it).c_str());
                ret = ERROR_VARIABLE_NOT_FOUND;
                break;
            }

            // Now asign the org address
            pSection->address = arg[0];
            fprintf(m_pOutFile, "a 0x%04X  # .org\n", pSection->address);
            break;

        case TYPE_DS:
            // Evaluate the ds
            it = pInst->args.begin();

            // Evaluate the 1st argument
            if ((ret = EvaluateExpression(*it, arg[0], pInst->filename, commaValue,
                pInst->line)) != ERROR_NONE)
            {
                printf("%s: Line %d: Argument %s has no value\n", pInst->filename.c_str(),
                        pInst->line, (*it).c_str());
                ret = ERROR_VARIABLE_NOT_FOUND;
                break;
            }

            // Add space to section
            pSection->address += arg[0];
            fprintf(m_pOutFile, "u %d\n", arg[0]);
            break;
            
        case TYPE_DB:
            // Evaluate the DS
            it = pInst->args.begin();

            // Evaluate all arguments
            while (it != pInst->args.end())
            {
                // Test for quoted string
                if ((*it).c_str()[0] == '"')
                {
                    pStr = &(*it).c_str()[1];
                    fprintf(m_pOutFile, "# DB %s\n", (*it).c_str());
                    while (*pStr && *pStr != '"')
                    {
                        // Test for escape sequences \r \t \n
                        if (*pStr == '\\')
                        {
                            pStr++;
                            if (*pStr == 't')
                                fprintf(m_pOutFile, "i 0x%04X  # reti \\%c\n", op_reti | 0x9, *pStr);
                            if (*pStr == 'n')
                                fprintf(m_pOutFile, "i 0x%04X  # reti \\%c\n", op_reti | 0xA, *pStr);
                            if (*pStr == 'r')
                                fprintf(m_pOutFile, "i 0x%04X  # reti \\%c\n", op_reti | 0xD, *pStr);
                        }
                        else
                            fprintf(m_pOutFile, "i 0x%04X  # reti %c\n", op_reti | *pStr, *pStr);
                        pStr++;
                        pSection->address++;
                    }
                }
                else
                {
                    // Evaluate the argument
                    if ((ret = EvaluateExpression(*it, arg[0], pInst->filename, commaValue,
                        pInst->line)) != ERROR_NONE)
                    {
                        printf("%s: Line %d: Argument %s has no value\n", pInst->filename.c_str(),
                                pInst->line, (*it).c_str());
                        ret = ERROR_VARIABLE_NOT_FOUND;
                        break;
                    }

                    // Add the argument as an RETI + value
                    fprintf(m_pOutFile, "i 0x%04X  # reti %s\n", op_reti | arg[0], (*it).c_str());
                    pSection->address++;
                }

                it++;
            }
            break;
            
        case TYPE_DW:
            // Evaluate the DS
            it = pInst->args.begin();

            // Evaluate all arguments
            while (it != pInst->args.end())
            {
                // Evaluate the argument
                if ((ret = EvaluateExpression(*it, arg[0], pInst->filename, commaValue,
                    pInst->line)) != ERROR_NONE)
                {
                    printf("%s: Line %d: Argument %s has no value\n", pInst->filename.c_str(),
                            pInst->line, (*it).c_str());
                    ret = ERROR_VARIABLE_NOT_FOUND;
                    break;
                }

                // Add the argument as an RETI + value
                fprintf(m_pOutFile, "i 0x%04X  # reti %s (LSB)\n", op_reti | arg[0] & 0xFF,
                        (*it).c_str());
                fprintf(m_pOutFile, "i 0x%04X  # %*s (MSB)\n", op_reti | (arg[0] >> 8) & 0xFF,
                        (int) (*it).length()+5, " ");
                pSection->address += 2;

                it++;
            }
            break;
            
        default:
            // Unhandled opcode!
            break;
    }

    return ret;
}

// =============================================================================

int32_t CAssembler::ReadAllSectionFiles(void)
{
    int32_t                     ret = ERROR_NONE;
    int32_t                     err;
    uint32_t                    value;
    StrSectionMap_t::iterator   it = m_pSpec->m_Segments.begin();
    StrVarMap_t::iterator       locateIter;

    // Loop through all sections
    for (; it != m_pSpec->m_Segments.end(); ++it)
    {
        // Get the sectionName and ResourceSection_t from the map
        string sectionName = it->first;
        ResourceSection_t* pSection = it->second;

        // Initialize the section data
        pSection->size = 0;
        pSection->address = 0;
        pSection->currentOffset = 0;

        // Write section name to the output file
        fprintf(m_pOutFile, "s %s%s\n", m_pSpec->m_ModuleName.c_str(), sectionName.c_str());

        // Loop through all of the section's data and add it to the output
        ResourceList_t::iterator lit = pSection->resources.begin();
        for (; lit != pSection->resources.end(); lit++)
        {
            // Test if this resource is a file resource
            if ((*lit)->m_pData != NULL)
                err = AddDataAllocationToSection(pSection, *lit);
            else if ((*lit)->m_pInst != NULL)
                err = PerformCodeAssembly(pSection, *lit);
            else if ((*lit)->m_align != 0)
                err = AddAlignToSection(pSection, *lit);   

            // Save the error code if not ERROR_NONE for the return
            if (err != ERROR_NONE)
                ret = err;
        }

        // Add automatic section variables
        AddVariable(sectionName + ".START", pSection->address + m_pSpec->m_BaseAddress);
        AddVariable(sectionName + ".END", pSection->address + m_pSpec->m_BaseAddress + pSection->size - 1);
        AddVariable(sectionName + ".NEXT", pSection->address + pSection->size);
        AddVariable(sectionName + ".SIZE", pSection->size);
        AddVariable(sectionName + ".OFFSET", pSection->address);
    }

    return ret;
}

// =============================================================================

int32_t CAssembler::PopulateAllDataBlocks(void)
{
    int32_t                     ret = ERROR_NONE;
    int32_t                     err = ERROR_NONE;
    StrSectionMap_t::iterator   it = m_pSpec->m_Segments.begin();

    // Loop through all sections
    for (; it != m_pSpec->m_Segments.end(); ++it)
    {
        // Get the sectionName and ResourceSection_t from the map
        string sectionName = it->first;
        ResourceSection_t* pSection = it->second;

        // Loop through all of the section's data and add it to the output
        ResourceList_t::iterator lit = pSection->resources.begin();
        for (; lit != pSection->resources.end(); lit++)
        {
            // Test if this resource is a data resource
            if ((*lit)->m_pData != NULL)
            {
                err = AddDataToSection(pSection, *lit);

                // Save the error code if not ERROR_NONE for the return
                if (err != ERROR_NONE)
                    ret = err;
            }
        }
    }

    return ret;
}

// =============================================================================

int32_t CAssembler::EvaluateExpression(std::string& sExpr, uint32_t& value,
        std::string& sFilename, uint32_t lineNo, int& commaValue, bool isStxx)
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
    int                     neg;

    // Perform variable substitution
    if ((err = SubstituteVariables(sExpr, sSubst, sFilename, lineNo,
                                   commaValue, isStxx)) != ERROR_NONE)
        return err;

    // Setup our evaluation stacks
    opStack[0] = '=';
    opStackIdx = 1;
    valStackIdx = 0;
    neg = 1;

    // Parse the equation
    len = strlen(sSubst.c_str());
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

                        // Must be | operation ... push value to stack
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
                neg = -1;
                x++;
                break;

            case '|':
                opStack[opStackIdx++] = '|';
                x++;
                break;

            case '&':
                opStack[opStackIdx++] = '&';
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
                
                err = EvaluateExpression(varValue, tempVal, sFilename, lineNo, commaValue);
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
            case '=':
                valStack[valStackIdx++] = tempVal;
                break;

            case '&':
                valStack[valStackIdx-1] &= tempVal;
                break;

            case '|':
                tempVal = valStack[--valStackIdx];
                valStack[valStackIdx-1] |= tempVal;
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
        value = valStack[0] * neg;

    return err;
}

// =============================================================================

int32_t CAssembler::EvaluateToken(std::string& sExpr, uint32_t& value,
        std::string& sFilename, uint32_t lineNo)
{
    int32_t                 err = ERROR_NONE;
    StrStrMap_t::iterator   varIter;
    ResourceMap_t::iterator labelIter;
    StrLabelMap_t::iterator labelMapIter;
    std::string             varValue;
    CResource              *pRes;
    int                     commaValue;


    // Do a lookup in case it is a define
    varValue = sExpr;
    if ((varIter = m_pSpec->m_Variables.find(sExpr)) != m_pSpec->m_Variables.end())
    {
        // Variable found.  Evaluate it as an expression
        varValue = varIter->second;
        err = EvaluateExpression(varValue, value, sFilename, lineNo, commaValue);
        return err;
    }

    // Look in the labels for the token
    if ((labelIter = m_pSpec->m_Labels.find(sExpr)) != m_pSpec->m_Labels.end())
    {
        // Label found.  Use the label address as the value
        pRes = labelIter->second;
        value = pRes->m_pInst->address; 
        return err;
    }

    // Look in LabelMap in case it is an extern
    if ((labelMapIter = m_pSpec->m_LabelMap.find(sExpr)) != m_pSpec->m_LabelMap.end())
    {
        // Label found.  Test if it is extern
        if (labelMapIter->second->m_Type & SIZE_EXTERN)
        {
          value = 0;
          return err;
        }
    }
   
    // Evaluate.  
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
    else if (strncmp(varValue.c_str(), "h'", 2) == 0 || 
             strncmp(varValue.c_str(), "H'", 2) == 0)
    {
        sscanf(&varValue.c_str()[2], "%i", &value);
    }
    else
    {
        // Validate only numeric values present
        int  x, len;
        len = strlen(varValue.c_str());
        for (x = 0; x < len; x++)
        {
            char ch = varValue.c_str()[x];
            if (ch < '0' || ch > '9')
            {
                printf("%s: Line %d: Variable %s not defined2\n", 
                        sFilename.c_str(), lineNo, varValue.c_str());
                return ERROR_VARIABLE_NOT_FOUND;
            }
        }
        sscanf(varValue.c_str(), "%i", &value);
    }

    return err;
}

// =============================================================================

int32_t CAssembler::SubstituteVariables(std::string& sExpr, std::string& sSubst,
        std::string& sFilename, uint32_t lineNo, int& commaValue,  bool isStxx)
{
    int32_t                 err = ERROR_NONE;
    size_t                  found;
    const char*             pStr;
    uint32_t                varEnd, c;
    StrStrMap_t::iterator   varIter;

    // First copy expression to subst
    sSubst = sExpr;
  
    // Search for comma
    found = sSubst.find("@");
    if (found != std::string::npos)
    {
        // We have an @ argument
        commaValue = atoi(&sSubst.c_str()[found+1]);
        sSubst.replace(found, std::string::npos, "");
    }
     
    // Search for (ix) in the expression
    found = sSubst.find("(ix)");
    if (found != std::string::npos)
    {
        sSubst.replace(found, 4, "");
    }

    // Search for (sp) in the expression
    found = sSubst.find("(sp)");
    if (found != std::string::npos)
    {
        if (isStxx)
            sSubst.replace(found, 4, "");
        else
        {
            if (m_Width == 16)
                sSubst.replace(found, 4, " | 512");
            else
                sSubst.replace(found, 4, " | 128");
        }
    }

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
                    printf("%s: Line %d: Variable %s not defined3\n", sFilename.c_str(), lineNo, varName.c_str());
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

// =============================================================================

int32_t CAssembler::AssembleLabels(void)
{
    uint32_t                    c;
    int32_t                     ret;
    uint32_t                    tempVal;
    int                         err;
    StrLabelMap_t::iterator     labelIt;
    StrList_t::iterator         sit;
    StrSectionMap_t::iterator   it = m_pSpec->m_Segments.begin();

    // Loop through all sections
    for (; it != m_pSpec->m_Segments.end(); ++it)
    {
        // Get the sectionName and ResourceSection_t from the map
        string sectionName = it->first;
        ResourceSection_t* pSection = it->second;

        // Initialize the section data
        pSection->size = 0;
        pSection->address = 0;
        pSection->currentOffset = 0;

        // Loop through all of the section's data and add it to the output
        ResourceList_t::iterator lit = pSection->resources.begin();
        for (; lit != pSection->resources.end(); lit++)
        {
            if ((*lit)->m_pInst != NULL)
            {
                Instruction_t*  pInst = (*lit)->m_pInst;
                switch (pInst->type)
                {
                    case TYPE_LABEL:
                        // Find the label 
                        labelIt = m_pSpec->m_LabelMap.find(pInst->name);
                        if (labelIt == m_pSpec->m_LabelMap.end())
                        {
                          printf("Error looking up label %s\n", pInst->name.c_str());
                          break;
                        }
                        // Assign the label an address
                        labelIt->second->m_Address = pSection->address;
                        pInst->address = pSection->address;
                        break;

                    case TYPE_OPCODE:
                        pSection->address += pInst->size & 0xF;
                        break;

                    case TYPE_ORG:
                        // Validate an arg given
                        if (pInst->args.size() == 0)
                        {
                          printf("%s: Line %d: Expecting argument for .org\n",
                              pInst->filename.c_str(), pInst->line);
                          break; 
                        }
                        err = EvaluateToken(pInst->args.front(), tempVal, pInst->filename, pInst->line);
                        if (err == ERROR_NONE)
                          pSection->address = tempVal;
                        else
                          printf("%s: Line %d: Unable to evaluate %s\n",
                              pInst->filename.c_str(), pInst->line,
                              pInst->args.front().c_str());
                        break;

                     case TYPE_DS:
                        // Validate an arg given
                        if (pInst->args.size() == 0)
                        {
                          printf("%s: Line %d: Expecting argument for ds directive\n",
                              pInst->filename.c_str(), pInst->line);
                          break; 
                        }
                        err = EvaluateToken(pInst->args.front(), tempVal, pInst->filename, pInst->line);
                        if (err == ERROR_NONE)
                          pSection->address += tempVal;
                        else
                          printf("%s: Line %d: Unable to evaluate %s\n",
                              pInst->filename.c_str(), pInst->line,
                              pInst->args.front().c_str());
                        break;

                     case TYPE_DB:
                        // Validate an arg given
                        if (pInst->args.size() == 0)
                        {
                          printf("%s: Line %d: Expecting argument for db directive\n",
                              pInst->filename.c_str(), pInst->line);
                          break; 
                        }
                        sit = pInst->args.begin();
                        while (sit != pInst->args.end())
                        {
                            // Test for quoted string
                            if ((*sit)[0] == '"')
                            {
                                // Decode any escaped characters in the string
                                // and calculate the length
                                for (c = 1; c < (*sit).length()-1; c++)
                                {
                                    // Test for '\' character
                                    if ((*sit)[c] == '\\')
                                        c++;
                                    pSection->address++;
                                }
                            }
                            else
                                pSection->address++;

                            // Advance to next arg
                            sit++;
                        }
                        break;
                }
            }

            // Save the error code if not ERROR_NONE for the return
            if (err != ERROR_NONE)
                ret = err;
        }
    }

#if 0
    auto labelIter = m_pSpec->m_LabelMap.begin();
    while (labelIter != m_pSpec->m_LabelMap.end())
    {
      printf("Label: %s, Type 0x%05X, Addr %d\n", labelIter->first.c_str(),
          labelIter->second->m_Type, labelIter->second->m_Address);

      // Next label
      labelIter++;
    }
#endif

    return ERROR_NONE;
}

// =============================================================================

int32_t CAssembler::CreateOutputBuffer(void)
{
    uint32_t    c;
    uint8_t*    pPtr;

    // Allocate the output buffer
    m_pData = new uint8_t[m_pSpec->m_FileSize];
    m_pTempData = new uint8_t[m_pSpec->m_FileSize];
    pPtr = m_pData;
    if (m_pData == NULL)
    {
        printf("%s: Unable to allocate memory\n", m_pSpec->m_Filename.c_str());
        return ERROR_OUT_OF_MEMORY;
    }

    // Now fill the buffer with the Fill Char
    for (c = 0; c < m_pSpec->m_FileSize; c++)
        *pPtr++  = m_pSpec->m_FillChar;

    return ERROR_NONE;
}

// =============================================================================

int32_t CAssembler::CreateOutputFile(const char *filename)
{
    uint32_t    c;
    char      * pStr;
    char      * pExt;

    // Allocate the output buffer
    m_pOutFile = fopen(filename, "w+");
    if (m_pOutFile == NULL)
    {
       printf("%s: Unable to open file for writing\n", filename);
       return ERROR_CANT_OPEN_FILE;
    }

    char    str[strlen(filename)+1];
    strcpy(str, filename);
    pStr = basename(str);
    pExt = strrchr(pStr, '.');
    if (pExt)
        *pExt = 0;
    m_pSpec->m_ModuleName = pStr; 

    return ERROR_NONE;
}

// =============================================================================

int32_t CAssembler::AddDataToSection(ResourceSection_t* pSection, CResource* pRes)
{
    ResourceData_t*     pData = pRes->m_pData;
    uint32_t            offset;
    uint32_t            value;
    int32_t             err, ret = ERROR_NONE;
    int                 commaValue;
    StrList_t::iterator it;
    
    // Start at beginning offset of data section
    offset = pSection->address + pData->offset;

    // Loop for all elements
    for (it = pData->elements.begin(); it != pData->elements.end(); it++)
    {
        // Evaluate the expression
        err = EvaluateExpression(*it, value, pData->spec_filename, pData->spec_line, commaValue);
        if (err != ERROR_NONE)
        {
            printf("Error evaluating expression '%s'\n", (*it).c_str());
            // Save the error as the return value and keep parsing to find more errors
            ret = err;
            continue;
        }

        if (m_DebugLevel >=4 )
        {
            printf("Adding 0x%0X to %s at 0x%0X\n", value, pData->name.c_str(),
                    offset);
        }

        // Add the data to the output
        switch (pData->elementSize)
        {
        case 1:         // Process uint8_t element
            *((uint8_t *) &m_pData[offset]) = (uint8_t) (value & 0xFF);
            offset++;
            break;

        case 2:         // Process uint16_t element
            if (m_pSpec->m_Endian == CParseCtx::ENDIAN_BIG)
            {
                *((uint8_t *) &m_pData[offset]) = (uint8_t) (value >> 8);
                *((uint8_t *) &m_pData[offset+1]) = (uint8_t) (value & 0xFF);
            }
            else
            {
                *((uint8_t *) &m_pData[offset]) = (uint8_t) (value & 0xFF);
                *((uint8_t *) &m_pData[offset+1]) = (uint8_t) (value >> 8);
            }
            offset += 2;
            break;

        case 4:         // Process uint32_t element
            if (m_pSpec->m_Endian == CParseCtx::ENDIAN_BIG)
            {
                *((uint8_t *) &m_pData[offset]) = (uint8_t) (value >> 24);
                *((uint8_t *) &m_pData[offset+1]) = (uint8_t) ((value >> 16) & 0xFF);
                *((uint8_t *) &m_pData[offset+2]) = (uint8_t) ((value >> 8) & 0xFF);
                *((uint8_t *) &m_pData[offset+3]) = (uint8_t) (value & 0xFF);
            }
            else
            {
                *((uint8_t *) &m_pData[offset]) = (uint8_t) (value & 0xFF);
                *((uint8_t *) &m_pData[offset+1]) = (uint8_t) ((value >> 8) & 0xFF);
                *((uint8_t *) &m_pData[offset+2]) = (uint8_t) ((value >> 16) & 0xFF);
                *((uint8_t *) &m_pData[offset+3]) = (uint8_t) (value >> 24);
            }
            offset += 4;
            break;

        default:
            printf("Invalid element size %d\n", pData->elementSize);
            ret = ERROR_PARSER_ERROR;
            break;
        }
    }

    return ERROR_NONE;
}

// =============================================================================

int32_t CAssembler::AddDataAllocationToSection(ResourceSection_t* pSection, CResource* pRes)
{
    ResourceData_t* pData = pRes->m_pData;

    // Print debug info
    if (m_DebugLevel >= 2)
        printf("Adding data %s at address 0x%0X\n", pData->name.c_str(), pSection->currentOffset +
            pSection->address + m_pSpec->m_BaseAddress);
    
    // Save the current section offset as our offset
    pData->offset = pSection->currentOffset;
    pData->size = pData->elementSize * pData->elements.size();

    // Validate data section will fit in remaining image space
    if (pSection->currentOffset + pData->size > m_pSpec->m_FileSize) 
    {
        // Image output size too small
        printf("%s: Line %d: Image file too small for data section %s\n",
                pData->spec_filename.c_str(), pData->spec_line, pData->name.c_str());
        return ERROR_RESOURCE_TOO_BIG;
    }

    // Update the section size and offset
    pSection->currentOffset += pData->size;
    pSection->size += pData->size;

    // Create START, END, SIZE and OFFSET variables for this data section
    AddVariable(pData->name + ".START", pData->offset + pSection->address + m_pSpec->m_BaseAddress);
    AddVariable(pData->name + ".END", pData->offset + pSection->address + m_pSpec->m_BaseAddress + pData->size - 1);
    AddVariable(pData->name + ".NEXT", pData->offset + pSection->address + pData->size);
    AddVariable(pData->name + ".SIZE", pData->size);
    AddVariable(pData->name + ".OFFSET", pSection->address + pData->offset);

    if (pData->offset + pSection->address < m_minAddr)
        m_minAddr = pData->offset + pSection->address;
    if (pData->offset + pSection->address + pData->size - 1 > m_maxAddr)
        m_maxAddr = pData->offset + pSection->address + pData->size - 1;

    return ERROR_NONE;
}

// =============================================================================

int32_t CAssembler::AddAlignToSection(ResourceSection_t* pSection, CResource* pRes)
{
    // Validate the alignment isn't zero
    if (pRes->m_align == 0)
        return ERROR_NONE;

    // Calculate the delta to add to perform alignment
    uint32_t roundUp = (pSection->currentOffset + pSection->address + pRes->m_align - 1) / 
        pRes->m_align;
    roundUp *= pRes->m_align;
    uint32_t delta = roundUp - (pSection->currentOffset + pSection->address);

    // Update the section size and offset
    pSection->currentOffset += delta;
    pSection->size += delta;

    // Create START variable for this data section
    if (m_DebugLevel >= 2)
        printf("Aligning to address 0x%0X\n", pSection->currentOffset + pSection->address +
                m_pSpec->m_BaseAddress);

    return ERROR_NONE;
}

// =============================================================================

#define GLOBAL_SYMBOL_TABLE_SIZE  (128*1024)
char GlobalSymbolTable[GLOBAL_SYMBOL_TABLE_SIZE];

// =============================================================================

int32_t CAssembler::TestOverlappingSections(void)
{
    int32_t                     ret = ERROR_NONE;
    int32_t                     err;
    uint32_t                    value;
    StrSectionMap_t::iterator   it = m_pSpec->m_Segments.begin();
    std::string                 s1, s2;

    // Iterate through all sections
    for (; it != m_pSpec->m_Segments.end(); ++it)
    {
        StrSectionMap_t::iterator it2 = m_pSpec->m_Segments.begin();
        ResourceSection_t* pSection1 = it->second;
        s1 = it->first;

        // Iterate through all remaining sections
        for (it2 = m_pSpec->m_Segments.begin(); it2 != m_pSpec->m_Segments.end(); ++it2)
        {
            // Get the section to compare
            ResourceSection_t* pSection2 = it2->second;
            s2 = it2->first;

            // Peform test only if it2's address is greater than it1's
            if (pSection2->address <= pSection1->address)
            {
                continue;
            }

            // Test if the sections overlap
            if (pSection1->address + pSection1->size > pSection2->address)
            {
                // Sections overlap
                printf("Sections %s and %s overlap by %d bytes!\n", s1.c_str(), 
                    s2.c_str(), pSection1->address + pSection1->size - pSection2->address);
                ret = ERROR_SEGMENTS_OVERLAP;
            }
        }
    }

    return ERROR_NONE;
}

// =============================================================================

void CAssembler::AddVariable(std::string sVarName, uint32_t value)
{
    std::stringstream   sTempName, sTempVal;

    sTempName << sVarName;
    sTempVal << value;
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(sTempName.str(), sTempVal.str()));
    if (m_DebugLevel >= 3)
        printf("Adding variable %s=0x%0X\n", sVarName.c_str(), value);

    sTempName << ".HEX";
    sTempVal.str("");
    sTempVal << "0x" << hex << value;
    m_pSpec->m_Variables.insert(std::pair<std::string, std::string>(sTempName.str(), sTempVal.str()));
    if (m_DebugLevel >= 3)
        printf("Adding variable %s=0x%0X\n", sTempName.str().c_str(), value);
}

// =============================================================================

void CAssembler::AddAsmVariables(void)
{
    // Add IF, IFTT, IFTE conditions
    AddVariable("eq",   0);
    AddVariable("z",    0);
    AddVariable("ne",   1);
    AddVariable("nz",   1);
    AddVariable("nc",   2);
    AddVariable("~z",   2);
    AddVariable("c",    3);
    AddVariable("gt",   4);
    AddVariable("lt",   5);
    AddVariable("ge",   6);
    AddVariable("gte",  6);
    AddVariable("le",   7);
    AddVariable("lte",  7);
    
    // Signed comparisions
    AddVariable("sgt",  0x24);
    AddVariable("slt",  0x25);
    AddVariable("sge",  0x26);
    AddVariable("sgte", 0x26);
    AddVariable("sle",  0x27);
    AddVariable("slte", 0x27);
    
    AddVariable("ra", 0);
    AddVariable("sp", 8);
    
    // Variables for div and rem
    AddVariable("ii", 0);
    AddVariable("ic", 1);
    AddVariable("ci", 2);
    AddVariable("cc", 3);

    // Variables for floating point
    AddVariable("f0", 0);
    AddVariable("f1", 1);
    AddVariable("f2", 2);
    AddVariable("f3", 3);

    AddVariable("notz", 2);     // Can be used with ldz opcode
}

// =============================================================================

int32_t CAssembler::Assemble(char * pOutputFilename, CParseCtx *pSpec)
{
    int32_t     err;

    // Save the image spec and output filename provided
    m_pSpec = pSpec;
    m_outputFile = pOutputFilename;

    // Validate all labels
    if ((err = AssembleLabels()) != ERROR_NONE)
        return err;

    // Add assembly variables
    AddAsmVariables();

    // Create output buffer and fill with fill char
    if ((err = CreateOutputBuffer()) != ERROR_NONE)
        return err;

    // Create output file 
    if ((err = CreateOutputFile(pOutputFilename)) != ERROR_NONE)
    {
        printf("Error creating output file\n");
        return err;
    }

    // Locate all section files and data
    if ((err = ReadAllSectionFiles()) != ERROR_NONE)
    {
        printf("Error reading sections\n");
        return err;
    }

    // Populate 'data' blocks in all sections
    if ((err = PopulateAllDataBlocks()) != ERROR_NONE)
    {
        printf("Error populating data blocks\n");
        return err;
    }
    
    // Test for overlapping sections
    if ((err = TestOverlappingSections()) != ERROR_NONE)
    {
        printf("Error testing overlapping sections\n");
        return err;
    }

    return ERROR_NONE;
}

// vim: sw=4 ts=4
