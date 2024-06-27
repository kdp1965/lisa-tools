// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : parsectx.h
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Describes classes and structures used for parsing and assembling.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#ifndef PARSECTX_H
#define PARSECTX_H

#include <string>
#include <map>
#include <list>

#define     OPCODE_NOP      0x2880
#define     OPCODE_LDX      0x2860
#define     OPCODE_JAL      0x0000
#define     OPCODE_LDI      0x2000
#define     OPCODE_RETI     0x2100
#define     OPCODE_RET      0x2280
#define     OPCODE_RC       0x22C0
#define     OPCODE_RZ       0x2270
#define     OPCODE_CALL_IX  0x22A0
#define     OPCODE_JMP_IX   0x22A8
#define     OPCODE_XCHG     0x22B0
#define     OPCODE_XCHG_RA  0x22B0
#define     OPCODE_XCHG_A   0x22B1
#define     OPCODE_XCHG_SP  0x22B2
#define     OPCODE_SPIX     0x22B3
#define     OPCODE_ADC      0x2400
#define     OPCODE_ADS      0x2500
#define     OPCODE_ADX      0x2600
#define     OPCODE_SHL      0x2800
#define     OPCODE_SHR      0x2801
#define     OPCODE_LDC      0x2802
#define     OPCODE_TXA      0x2804
#define     OPCODE_TXAU     0x2806
#define     OPCODE_BTST     0x2810
#define     OPCODE_TAX      0x2840
#define     OPCODE_TAXU     0x2842
#define     OPCODE_AMODE    0x2850
#define     OPCODE_SRA      0x2858
#define     OPCODE_LRA      0x2859
#define     OPCODE_PUSH_A   0x2820
#define     OPCODE_POP_A    0x2830
#define     OPCODE_CPI      0x2900
#define     OPCODE_BNZ      0x2A00
#define     OPCODE_BZ       0x2C00
#define     OPCODE_BNC      0x2E00
#define     OPCODE_ADD      0x3000
#define     OPCODE_MUL      0x3100
#define     OPCODE_MULU     0x2100
#define     OPCODE_SUB      0x3200
#define     OPCODE_IF       0x3300
#define     OPCODE_IFTT     0x3308
#define     OPCODE_IFTE     0x3310
#define     OPCODE_AND      0x3400
#define     OPCODE_ANDI     0x3500
#define     OPCODE_OR       0x3600
#define     OPCODE_SWAPI    0x3700
#define     OPCODE_XOR      0x3800
#define     OPCODE_CMP      0x3A00
#define     OPCODE_SWAP     0x3B00
#define     OPCODE_LDAX     0x3C00
#define     OPCODE_LDA      0x3D00
#define     OPCODE_STAX     0x3E00
#define     OPCODE_STA      0x3F00
#define     OPCODE_DCX      0x2700

/// Structure for identifying resource files from a resource script
typedef struct ResourceString
{
    std::string     name;
    std::string     value;          // String value (not evaluated)
    std::string     spec_filename;
    int32_t         start;
    int32_t         end;
    uint32_t        spec_line;
} ResourceString_t;

/// String list used for ResourceData
typedef std::list<std::string> StrList_t;

#define TYPE_CONST    1
#define TYPE_LABEL    2
#define TYPE_OPCODE   3
#define TYPE_ORG      4
#define TYPE_DS       5
#define TYPE_DB       6
#define TYPE_DW       7

#define SIZE_LABEL    0x1000
#define SIZE_ABSOLUTE 0x1000
#define SIZE_PUBLIC   0x2000
#define SIZE_EXTERN   0x4000

#define OP_ASSIGN_VAR     1
#define OP_LOAD_SECTION   2
#define OP_ALIGN          3
#define OP_ASSIGN_PC      4

#define PARAM_KEEP        1

/// Definition of a generic resource
class COperation
{
public:
    COperation() { }

    int                 m_Type;
    std::string         m_StrParam;
    int                 m_IntParam;
};

class CLabel
{
public:
    CLabel() {}

    std::string   m_Name;
    std::string   m_Filename;
    std::string   m_Segment;
    int           m_Type;
    int           m_Line;
    int           m_Defined;
    int           m_Address;
};

class CMemory
{
public:
    CMemory() {}

    std::string   m_Name;
    std::string   m_Access;
    int           m_Origin;
    int           m_Length;
    int           m_Address;
};

/// Resource File list for saving all files in a section
typedef std::list<COperation *> OperationList_t;

/// String to string map for environment variables, etc.
typedef std::map<std::string, std::string> StrStrMap_t;

/// String to CLabel map for all known labels
typedef std::map<std::string, CLabel *> StrLabelMap_t;

/// The contents of a section as parsed from the resource script
class CSection
{
public:
    CSection()      { m_pMem = NULL; m_pAtMem = NULL; }
    std::string     m_Name;           // Name of the segment
    std::string     m_MemRegion;      // Name of the memory the section is in
    std::string     m_MemAtRegion;    // Name of the memory the section is loaded to
    CMemory        *m_pMem;           // Pointer to the memory block
    CMemory        *m_pAtMem;         // Pointer to the AT memory block
    OperationList_t m_Ops;            // Link operations to perform
};

/// List of all known sections
typedef std::list<CSection *> SectionList_t;

/// String to Memory map for saving label pointers
typedef std::map<std::string, CMemory *> StrMemoryMap_t;

/// Holds all parsed data from an image resource script
class CParseCtx
{
public:
    CParseCtx() {  m_BaseAddress = (uint32_t) -1,  
                    m_FileSize = 0, m_FillChar = 0x00; }

    /// Map of all variables in the specification
    StrStrMap_t         m_Variables;
    
    /// Array of library paths
    StrList_t           m_LibPaths;

    /// Map of all known memory regions
    StrMemoryMap_t      m_MemoryMap;

    /// List of sections parsed from the linker script
    SectionList_t       m_SectionList;

    /// The base address where this image is loaded in FLASH
    uint32_t            m_BaseAddress;

    /// Size of the generated output file
    uint32_t            m_FileSize;

    /// Fill character for the generated file
    uint8_t             m_FillChar;

    std::string         m_Filename;
    std::string         m_OutputArch;
    std::string         m_EntryLabel;
};

#endif  // PARSECTX_H

