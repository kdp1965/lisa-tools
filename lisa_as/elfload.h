// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : elfload.h
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Implementaiton of an ELF file reader
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------
#ifndef ELFLOAD_H
#define ELFLOAD_H

#include <string>
#include <stdio.h>
#include <list>

#include "elf.h"

typedef std::list<Elf_Phdr> Elf_PhdrList_t;

class CElfLoad
{
public:
    CElfLoad(std::string& sFilename, uint32_t debugLevel = 0);
    ~CElfLoad();

    /// Reads the ELF headers from the file
    int32_t         ParseHeaders(void);

    /// Copies the ELF file's program data to the given buffer
    int32_t         GetProgramBytes(uint8_t* pAddr, uint32_t& length,
                        uint32_t& loadAddress, uint32_t& startAddress);

    /// Identifies the debug print level
    uint32_t        m_DebugLevel;

private:
    /// Performs an endian safe 32-bit read from the specified address
    uint32_t        ReadWord(const char* pAddr);

    /// Performs an endian safe 16-bit read from the specified address
    uint16_t        ReadHalf(const char* pAddr);

    // Define private member variables
private:
    /// Filename of the ELF file
    std::string     m_sFilename;

    /// Identifies if the Elf IDENT is valid
    bool            m_valid;

    /// Defines the endian-ness of the ELF file
    char            m_endian;

    /// Defines the machine type from ELF file
    uint16_t        m_machine;

    /// Defines the type of ELF file
    uint16_t        m_type;

    /// The program entry point
    uint32_t        m_entryAddress;

    /// The program load point
    uint32_t        m_loadAddress;

    /// Pointer to the file's data in-memory
    char*           m_pData;

    /// Size of file and m_pData buffer
    uint32_t        m_fileSize;

    /// Program header count
    uint16_t        m_phnum;

    /// Symbol header count
    uint16_t        m_shnum;

    /// Program header offset
    uint32_t        m_phoff;

    /// Symbol header offset
    uint32_t        m_shoff;

    /// List of Program headers
    Elf_PhdrList_t    m_phdrs;
};

#endif

