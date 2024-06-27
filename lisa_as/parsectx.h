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

#define     OPCODE_NOP      0x281C
#define     OPCODE_NOTZ     0x281D
#define     OPCODE_BRK      0x281F
#define     OPCODE_LDX      0x2860
#define     OPCODE_JAL      0x0000
#define     OPCODE_LDI      0x2000
#define     OPCODE_RETI     0x2300
#define     OPCODE_RET      0x2280
#define     OPCODE_RC       0x22C0
#define     OPCODE_RZ       0x22E0
#define     OPCODE_RETS     0x22D0
#define     OPCODE_CALL_IX  0x22A0
#define     OPCODE_JMP_IX   0x22A8
#define     OPCODE_XCHG     0x22B0
#define     OPCODE_XCHG_RA  0x22B0
#define     OPCODE_XCHG_IA  0x22B1
#define     OPCODE_XCHG_SP  0x22B2
#define     OPCODE_SPIX     0x22B3
#define     OPCODE_ADC      0x2400
#define     OPCODE_ADS      0x2500
#define     OPCODE_ADX      0x2600
#define     OPCODE_SHL      0x2800
#define     OPCODE_SHR      0x2801
#define     OPCODE_LDC      0x2802
#define     OPCODE_LDZ      0x2818
#define     OPCODE_TXA      0x2804
#define     OPCODE_TXAU     0x2806
#define     OPCODE_BTST     0x2810
#define     OPCODE_TAX      0x2840
#define     OPCODE_TAXU     0x2842
#define     OPCODE_AMODE    0x2850
#define     OPCODE_SRA      0x2858
#define     OPCODE_LRA      0x2859
#define     OPCODE_PUSH_IX  0x285A
#define     OPCODE_POP_IX   0x285B
#define     OPCODE_PUSH_A   0x2820
#define     OPCODE_POP_A    0x2830
#define     OPCODE_CPI      0x2900
#define     OPCODE_BNZ      0x2A00
#define     OPCODE_BR       0x2C00
#define     OPCODE_BZ       0x2E00
#define     OPCODE_ADD      0x3000
#define     OPCODE_MUL      0x3100
#define     OPCODE_MULU     0x2100
#define     OPCODE_SUB      0x3200
#define     OPCODE_IF       0x2880
#define     OPCODE_IFTT     0x2888
#define     OPCODE_IFTE     0x2890
#define     OPCODE_AND      0x3400
#define     OPCODE_ANDI     0x3500
#define     OPCODE_OR       0x3600
#define     OPCODE_SWAPI    0x3700
#define     OPCODE_XOR      0x3800
#define     OPCODE_CMP      0x3A00
#define     OPCODE_SWAP     0x3B00
#define     OPCODE_LDAX     0x3C00
#define     OPCODE_LDAC     0x2870
#define     OPCODE_LDA      0x3D00
#define     OPCODE_STAX     0x3E00
#define     OPCODE_STA      0x3F00
#define     OPCODE_DCX      0x2700
#define     OPCODE_CPX      0x22B4
#define     OPCODE_CPX_RA   0x22B4
#define     OPCODE_CPX_SP   0x22B6
#define     OPCODE_INX      0x3900
#define     OPCODE_DIV      0x28C0
#define     OPCODE_REM      0x28C4
#define     OPCODE_LDDIV    0x285C
#define     OPCODE_SAVEC    0x285E
#define     OPCODE_RESTC    0x285F
#define     OPCODE_LDXX     0x3300
#define     OPCODE_STXX     0x3380
#define     OPCODE_SHL16    0x2808
#define     OPCODE_SHR16    0x280C
#define     OPCODE_ADDAX    0x2868
#define     OPCODE_ADDAXU   0x2869
#define     OPCODE_SUBAX    0x286A
#define     OPCODE_SUBAXU   0x286B
#define     OPCODE_DI       0xA078
#define     OPCODE_EI       0xA079

#define     OPCODE16_NOP      0xA070
#define     OPCODE16_NOTZ     0xA074
#define     OPCODE16_BRK      0xA07C
#define     OPCODE16_LDX      0xA180
#define     OPCODE16_JAL      0x0000
#define     OPCODE16_LDI      0x8000
#define     OPCODE16_RETI     0x8C00
#define     OPCODE16_RET      0x8A00
#define     OPCODE16_RC       0x8B00
#define     OPCODE16_RZ       0x8B80
#define     OPCODE16_RETS     0x8B40
#define     OPCODE16_CALL_IX  0x8A80
#define     OPCODE16_JMP_IX   0x8AA0
#define     OPCODE16_XCHG     0x8AC0
#define     OPCODE16_XCHG_RA  0x8AC0
#define     OPCODE16_XCHG_IA  0x8AC4
#define     OPCODE16_XCHG_SP  0x8AC8
#define     OPCODE16_SPIX     0x8ACC
#define     OPCODE16_ADC      0x9000
#define     OPCODE16_ADS      0x9400
#define     OPCODE16_ADX      0x9800
#define     OPCODE16_SHL      0xA000
#define     OPCODE16_SHR      0xA004
#define     OPCODE16_LDC      0xA008
#define     OPCODE16_LDZ      0xA060
#define     OPCODE16_TXA      0xA010
#define     OPCODE16_TXAU     0xA018
#define     OPCODE16_BTST     0xA040
#define     OPCODE16_TAX      0xA100
#define     OPCODE16_TAXU     0xA108
#define     OPCODE16_AMODE    0xA140
#define     OPCODE16_SRA      0xA160
#define     OPCODE16_LRA      0xA164
#define     OPCODE16_PUSH_IX  0xA168
#define     OPCODE16_POP_IX   0xA16C
#define     OPCODE16_PUSH_A   0xA080
#define     OPCODE16_POP_A    0xA0C0
#define     OPCODE16_CPI      0xA400
#define     OPCODE16_BNZ      0xA800
#define     OPCODE16_BR       0xB000
#define     OPCODE16_BZ       0xB800
#define     OPCODE16_ADD      0xC000
#define     OPCODE16_MUL      0xC400
#define     OPCODE16_MULU     0x8400
#define     OPCODE16_SUB      0xC800
#define     OPCODE16_IF       0xA200
#define     OPCODE16_IFTT     0xA208
#define     OPCODE16_IFTE     0xA210
#define     OPCODE16_AND      0xD000
#define     OPCODE16_ANDI     0xD400
#define     OPCODE16_OR       0xD800
#define     OPCODE16_SWAPI    0xDC00
#define     OPCODE16_XOR      0xE000
#define     OPCODE16_CMP      0xE800
#define     OPCODE16_SWAP     0xEC00
#define     OPCODE16_LDAX     0xF000
#define     OPCODE16_LDAC     0xA1C0
#define     OPCODE16_LDA      0xF400
#define     OPCODE16_STAX     0xF800
#define     OPCODE16_STA      0xFC00
#define     OPCODE16_DCX      0x9C00
#define     OPCODE16_CPX      0x8AD0
#define     OPCODE16_CPX_RA   0x8AD0
#define     OPCODE16_CPX_SP   0x8AD8
#define     OPCODE16_INX      0xE400
#define     OPCODE16_DIV      0xA300
#define     OPCODE16_REM      0xA310
#define     OPCODE16_LDDIV    0xA170
#define     OPCODE16_SAVEC    0xA178
#define     OPCODE16_RESTC    0xA17C
#define     OPCODE16_LDXX     0xCC00
#define     OPCODE16_STXX     0xCE00
#define     OPCODE16_SHL16    0xA020
#define     OPCODE16_SHR16    0xA030
#define     OPCODE16_ADDAX    0xA1A0
#define     OPCODE16_ADDAXU   0xA1A1
#define     OPCODE16_SUBAX    0xA1A2
#define     OPCODE16_SUBAXU   0xA1A3
#define     OPCODE16_TFA      0xA1E0
#define     OPCODE16_TFAU     0xA1E1
#define     OPCODE16_TAF      0xA1E2
#define     OPCODE16_TAFU     0xA1E3
#define     OPCODE16_FMUL     0xA1E4
#define     OPCODE16_FADD     0xA1E8
#define     OPCODE16_FNEG     0xA1EC
#define     OPCODE16_FSWAP    0xA1F0
#define     OPCODE16_FCMP     0xA1F4
#define     OPCODE16_FDIV     0xA1F8
#define     OPCODE16_ITOF     0xA320
#define     OPCODE16_FTOI     0xA321
#define     OPCODE16_DI       0xA078
#define     OPCODE16_EI       0xA079

typedef struct Opcode_s
{
    const char *name;
    int         args;
    int         value;
    int         size;
} Opcode_t;

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

/// Definition of a resource data set
typedef struct ResourceData
{
    std::string     name;
    uint32_t        elementSize;    // Number of bytes for each entry
    StrList_t       elements;       // List of elements (string form)
    std::string     spec_filename;
    uint32_t        spec_line;
    uint32_t        offset;         // Offset of start of data in section
    uint32_t        size;           // Size of data
} ResourceData_t;

/// Structure for defining instructions and labels
typedef struct Instruction_s
{
    int             type;           // Type of instruction 
    std::string     name;           // Name associated with instruction
    StrList_t       args;           // List of comma separated arguments
    std::string     filename;
    int32_t         value;
    int32_t         size;           // Size of this instruction
    int32_t         address;
    int32_t         line;
    int32_t         argc;           // Number of required args
} Instruction_t;

#define TYPE_CONST    1
#define TYPE_LABEL    2
#define TYPE_OPCODE   3
#define TYPE_ORG      4
#define TYPE_DS       5
#define TYPE_DB       6
#define TYPE_DW       7
#define TYPE_FILE     8
#define TYPE_LOC      9

#define SIZE_LABEL    0x1000
#define SIZE_ABSOLUTE 0x1000
#define SIZE_PUBLIC   0x2000
#define SIZE_EXTERN   0x4000
#define SIZE_LOCAL    0x8000

/// Definition of a generic resource
class CResource
{
public:
    CResource() { m_pFile = NULL, m_pData = NULL, 
                  m_pInst = NULL; m_align = 0; }

    ResourceString_t*   m_pFile;
    ResourceData_t*     m_pData;
    Instruction_t*      m_pInst;
    uint32_t            m_align;
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

/// Resource File list for saving all files in a section
typedef std::list<CResource *> ResourceList_t;

/// String to Resource map for saving label pointers
typedef std::map<std::string, CResource *> ResourceMap_t;

/// String to ResourceString map for saving environment variables
typedef std::map<std::string, ResourceString_t *> StrVarMap_t;

/// String to string map for environment variables, etc.
typedef std::map<std::string, std::string> StrStrMap_t;

/// String to CLabel map for all known labels
typedef std::map<std::string, CLabel *> StrLabelMap_t;

/// The contents of a section as parsed from the resource script
typedef struct ResourceSection
{
    uint32_t        address;        // Offset from beginning of file
    uint32_t        size;           // Size in bytes 
    std::string     type;           // Type of section (binary, elf, srec, etc.)
    std::string     name;           // Name of the segment
    StrVarMap_t     variables;
    ResourceList_t  resources;
    StrLabelMap_t   labels;         // Map to labels in segment
    std::string     filename;
    uint32_t        line;
    uint32_t        currentOffset;  // Current offset of resource being added
} ResourceSection_t;

/// Map's section names to the section data
typedef std::map<std::string, ResourceSection_t *> StrSectionMap_t;

/// Maps Section names to the Locate address
typedef std::map<std::string, uint32_t> StrAddressMap_t;

/// Holds all parsed data from an image resource script
class CParseCtx
{
public:
    CParseCtx() {  m_BaseAddress = (uint32_t) -1, m_Endian = ENDIAN_UNKNOWN, 
                    m_FileSize = 0, m_FillChar = 0xCD; }

    /// Map of all sections in the specification
    StrSectionMap_t     m_Segments;

    /// Map of all locates in the specification
    StrVarMap_t         m_Locates;

    /// Map of all variables in the specification
    StrStrMap_t         m_Variables;
    
    /// Array of include paths
    StrList_t           m_IncPaths;

    /// Map of labels to their resource pointer
    ResourceMap_t       m_Labels;

    /// Map of all known labels or referenced labels
    StrLabelMap_t       m_LabelMap;

    enum endian {
        ENDIAN_UNKNOWN = 0,
        ENDIAN_BIG,
        ENDIAN_LITTLE
    };

    /// The base address where this image is loaded in FLASH
    uint32_t            m_BaseAddress;

    /// The endianness of the target machine for this image
    enum endian         m_Endian;

    /// Size of the generated output file
    uint32_t            m_FileSize;

    /// Fill character for the generated file
    uint8_t             m_FillChar;

    std::string         m_Filename;
    std::string         m_ModuleName;
};

#endif  // PARSECTX_H

