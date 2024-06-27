// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : assembler.h
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

#ifndef BUILDER_H
#define BUILDER_H

#include <string>
#include <stdint.h>

#include "parsectx.h"

class CAssembler
{
    public:
        CAssembler();
        virtual             ~CAssembler();

        virtual int32_t     Assemble(char * pOutputFilename, CParseCtx *pSpec);


        /// Get output data
        void                GetData(const uint8_t *&pData, uint32_t& minAddr, uint32_t& maxAddr)
                                { pData = m_pData, minAddr = m_minAddr, maxAddr = m_maxAddr; }

        /// Holds the text for the last error encountered
        std::string         m_Error;

        /// Debug print level
        uint32_t            m_DebugLevel;

        bool                m_Mixed;
        int                 m_Width;

    private:
        // Define private member functions

        /// Iterates through all segments and assembles code
        int32_t             PerformCodeAssembly(ResourceSection_t* pSection, CResource* pRes);

        /// Iterates through all sections and reads in files and creates data
        int32_t             ReadAllSectionFiles(void);

        /// Iterates through all sections and reads in files and creates data
        int32_t             PopulateAllDataBlocks(void);

        /// Evaluates an expression which may contain a $(VARIABLE)
        int32_t             EvaluateExpression(std::string& sExpr, uint32_t& value, 
                                std::string& sFilename, uint32_t lineNo,
                                int& commaValue, bool isStxx = false);

        /// Evaluates a token
        int32_t             EvaluateToken(std::string& sExpr, uint32_t& value, 
                                std::string& sFilename, uint32_t lineNo);

        /// Replaces all occurances of variables withing sExpr and returns sSubst
        int32_t             SubstituteVariables(std::string& sExpr, std::string& sSubst, 
                                std::string& sFilename, uint32_t lineNo,
                                int& commaValue, bool isStxx);

        /// Validate all labels
        int32_t             AssembleLabels(void);

        /// Creates the output buffer for the image data & fills with fill character
        int32_t             CreateOutputBuffer(void);

        /// Creates the output buffer for the image data & fills with fill character
        int32_t             CreateOutputFile(const char *filename);

        /// Adds the specified data resource to the section
        int32_t             AddDataToSection(ResourceSection_t* pSection,
                                CResource* pRes);

        /// Adds the specified data resource to the section
        int32_t             AddDataAllocationToSection(ResourceSection_t* pSection,
                                CResource* pRes);

        /// Adds the specified align resource to the section
        int32_t             AddAlignToSection(ResourceSection_t* pSection,
                                CResource* pRes);

        /// Adds a variable to the spec's global variable space
        void                AddVariable(std::string sVarName, uint32_t value);

        /// Tests for any sections that overlap each other
        int32_t             TestOverlappingSections(void);

        char *              binstr(int len, uint32_t val, int sep[]);

        void                AddAsmVariables(void);
    private:
        // Define private member variables

        /// Points to the last imagespec section found
        ResourceSection_t*  m_LastSection;
        
        /// Points to the last active ResourceData_t object
        ResourceData_t*     m_LastResData;

        /// Pointer to the CParseCtx we are building from
        CParseCtx*          m_pSpec;

        /// Name of the target output file
        std::string         m_outputFile;
        std::string         m_basename;

        /// Pointer to the output data buffer
        uint8_t*            m_pData;

        /// Minimum address from input data
        uint32_t            m_minAddr;

        /// Maximum address from input data
        uint32_t            m_maxAddr;

        /// Temporary buffer for loading files
        uint8_t*            m_pTempData;

        FILE*               m_pOutFile;

        std::string         m_LastLabel;
        int                 m_LabelInst;

        int                 m_DefLen;
        int                 m_DefValue;
        char                m_BinStr[40];
};

#endif  // BUILDER_H

