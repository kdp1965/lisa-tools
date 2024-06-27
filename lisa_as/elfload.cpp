// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : elfload.cpp
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

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "elfload.h"
#include "errors.h"

// =============================================================================

CElfLoad::CElfLoad(std::string& sFilename, uint32_t debugLevel)
{
    // Initialize variables
    m_sFilename = sFilename;
    m_pData = NULL;
    m_endian = 0;
    m_machine = 0;
    m_type = 0;
    m_DebugLevel = debugLevel;
    m_entryAddress = (uint32_t) -1;
    m_loadAddress = (uint32_t) -1;

    // Initialize the m_valid to false to indicate we don't know if the
    // file header(s) is valid yet
    m_valid = false;
}

CElfLoad::~CElfLoad(void)
{
    // Delete buffer if it exists
    if (m_pData != NULL)
        delete[] m_pData;
}

// =============================================================================

int32_t CElfLoad::ParseHeaders(void)
{
    FILE*       fd;
    uint16_t  e_phentsize, e_shentsize;
    uint32_t    c;
    Elf_Phdr  phdr;

    // Test if file is already opened
    if (m_pData == NULL)
    {
        // Try to open the file
        if ((fd = fopen(m_sFilename.c_str(), "r")) == NULL)
        {
            m_valid = false;
            return ERROR_CANT_OPEN_FILE;
        }

        // Determine the file size
        fseek(fd, 0, SEEK_END);

        // Get the size of the ELF file
        m_fileSize = ftell(fd);

        // Allocate space
        m_pData = new char[m_fileSize];

        // Rewind and read the data
        rewind(fd);
        fread(m_pData, 1, m_fileSize, fd);
        fclose(fd);
    }

    // Read the Ehdr
    if ((m_pData[0] != 0x7F) || (m_pData[1] != 'E') ||
        (m_pData[2] != 'L') || (m_pData[3] != 'F'))
    {
        m_valid = false;
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Read the class.  Currently we only process 32-bit objects
    if (m_pData[4] != 1)
    {
        m_valid = false;
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Get the endianness
    m_endian = m_pData[5];

    // Get the object file type (we only deal with 2 (executable)
    m_type = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_type)]);
    if (m_type != 2)
    {
        m_valid = false;
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Get the machine type
    m_machine = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_machine)]);

    // Get the entry point
    m_entryAddress = ReadWord(&m_pData[offsetof(Elf_Ehdr, e_entry)]);

    // Get the number of program sections and offset
    m_phnum = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_phnum)]);
    m_phoff = ReadWord(&m_pData[offsetof(Elf_Ehdr, e_phoff)]);
    e_phentsize = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_phentsize)]);

    // Get the number of symbol sections
    m_shnum = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_shnum)]);
    m_shoff = ReadWord(&m_pData[offsetof(Elf_Ehdr, e_shoff)]);
    e_shentsize = ReadHalf(&m_pData[offsetof(Elf_Ehdr, e_shentsize)]);

    // Print debug info if requested
    if (m_DebugLevel >= 4)
    {
        printf("Elf: IDENT   = %c%c%c\n", m_pData[1], m_pData[2], m_pData[3]);
        printf("     DATA    = %s endian\n", m_endian == ELFDATA2LSB ? "Little" : "Big");
        printf("     MACHINE = %d\n", m_machine);
        printf("     ENTRY   = 0x%0X\n", m_entryAddress);
        printf("     PHNUM   = %d\n", m_phnum);
        printf("     SHNUM   = %d\n", m_shnum);
        printf("     PHOFF   = %d\n", m_phoff);
        printf("     SHOFF   = %d\n", m_shoff);
    }

    // Parse through all program headers
    for (c = 0; c < m_phnum; c++)
    {
        uint32_t   base = c * e_phentsize + m_phoff;

        phdr.p_type = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_type)]);
        phdr.p_filesz = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_filesz)]);
        phdr.p_memsz = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_memsz)]);
        phdr.p_flags = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_flags)]);
        phdr.p_align = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_align)]);
        phdr.p_offset = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_offset)]);
        phdr.p_vaddr = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_vaddr)]);
        phdr.p_paddr = ReadWord(&m_pData[base + offsetof(Elf_Phdr, p_paddr)]);
        if (m_loadAddress == (uint32_t) -1)
            m_loadAddress = phdr.p_vaddr;

        // Print debug stuff if requested
        if (m_DebugLevel >= 4)
        {
            printf("     PH%d:\n", c);
            printf("        p_type: 0x%0X\n", phdr.p_type);
            printf("        p_offset: 0x%0X\n", phdr.p_offset);
            printf("        p_vaddr: 0x%0X\n", phdr.p_vaddr);
            printf("        p_paddr: 0x%0X\n", phdr.p_paddr);
            printf("        p_filesz: 0x%0X\n", phdr.p_filesz);
            printf("        p_memsz: 0x%0X\n", phdr.p_memsz);
            printf("        p_flags: 0x%0X\n", phdr.p_flags);
            printf("        p_align: 0x%0X\n", phdr.p_align);
        }

        // Add this section to our list of Program headers
        m_phdrs.push_back(phdr);
    }

    // This is a hack for now and matches what WinMon is doing (yuk!).  It
    // hardcodes the string tables at the last index and the 3rd from last
    // instead of searching for them.  This is BAD as it assumes a particular
    // format on the ELF file.
    uint32_t sh_st, st, stmp;
    char*   pShStrTable;
    char*   pStrTable;
    stmp = m_shoff + sizeof(Elf_Shdr) * (m_shnum-3);
    pShStrTable = &m_pData[ReadWord(&m_pData[stmp + offsetof(Elf_Shdr, sh_offset)])];
    stmp = m_shoff + sizeof(Elf_Shdr) * (m_shnum-1);
    pStrTable = &m_pData[ReadWord(&m_pData[stmp + offsetof(Elf_Shdr, sh_offset)])];

    // Parse through all Symbol headers
    for (c = 0; c < m_shnum; c++)
    {
        uint32_t   base = c * e_shentsize + m_shoff;
        Elf_Shdr  shdr;

        shdr.sh_name = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_name)]);
        shdr.sh_type = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_type)]);
        shdr.sh_flags = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_flags)]);
        shdr.sh_addr = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_addr)]);
        shdr.sh_offset = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_offset)]);
        shdr.sh_size = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_size)]);
        shdr.sh_link = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_link)]);
        shdr.sh_info = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_info)]);
        shdr.sh_addralign = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_addralign)]);
        shdr.sh_entsize = ReadWord(&m_pData[base + offsetof(Elf_Shdr, sh_entsize)]);

        // Print debug info if requested
        if ((m_DebugLevel >= 4) && (shdr.sh_type == 2))
        {
            printf("     SH%d:\n", c);
            printf("        sh_name: 0x%0X\n", shdr.sh_name);
            printf("        sh_type: 0x%0X\n", shdr.sh_type);
            printf("        sh_flags: 0x%0X\n", shdr.sh_flags);
            printf("        sh_addr: 0x%0X\n", shdr.sh_addr);
            printf("        sh_offset: 0x%0X\n", shdr.sh_offset);
            printf("        sh_size: 0x%0X\n", shdr.sh_size);
            printf("        sh_link: 0x%0X\n", shdr.sh_link);
            printf("        sh_info: 0x%0X\n", shdr.sh_info);
            printf("        sh_addralign: 0x%0X\n", shdr.sh_addralign);
            printf("        sh_entsize: 0x%0X\n", shdr.sh_entsize);
        }

        // Parse through Symbol tables
        if (shdr.sh_type == 2)
        {
            uint32_t   symbase;
            Elf_Sym   sym;
            int j, count;
            
            symbase = shdr.sh_offset;
            count = shdr.sh_size / shdr.sh_entsize;
            for (j = 0; j < count; j++, symbase += shdr.sh_entsize)
            {
                char* pName = &pStrTable[ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_name)])];
                int len = strlen(pName);

                sym.st_name = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_name)]);
                sym.st_value = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_value)]);
                sym.st_size = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_size)]);
                sym.st_info = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_info)]);
                sym.st_other = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_other)]);
                sym.st_shndx = ReadWord(&m_pData[symbase + offsetof(Elf_Sym, st_shndx)]);

                // Test for object or function symbols
                if (len > 0 && ((sym.st_info & 0xF) == 1 ||
                    (sym.st_info & 0xF) == 2))
                {
                    // Add this symbol name and value to our map
                }
            }
        }
    }

    // Mark the header data as valid
    m_valid = true;
    return ERROR_NONE;
}

// =============================================================================

uint32_t CElfLoad::ReadWord(const char *pAddr)
{
    uint32_t  ret;

    // Validate the headers are valid
    if (pAddr >= m_pData + m_fileSize)
        return (uint32_t) -1;

    if (m_endian == ELFDATA2LSB)
    {
        ret = ((uint8_t) pAddr[0] & 0xFF) | 
            (((uint32_t) pAddr[1] & 0xFF) << 8) |
            (((uint32_t) pAddr[2] & 0xFF) << 16) |
            (((uint32_t) pAddr[3] & 0xFF) << 24);
    }
    else
    {
        ret = (pAddr[3] & 0xFF) | 
            (((uint32_t) pAddr[2] & 0xFF) << 8) |
            (((uint32_t) pAddr[1] & 0xFF) << 16) |
            (((uint32_t) pAddr[0] & 0xFF) << 24);
    }

    return ret;
}

// =============================================================================

uint16_t CElfLoad::ReadHalf(const char *pAddr)
{
    uint16_t  ret;

    // Validate the headers are valid
    if (pAddr >= m_pData + m_fileSize)
        return (uint16_t) -1;

    if (m_endian == ELFDATA2LSB)
        ret = ((uint16_t) pAddr[0] & 0xFF) | (((uint32_t) pAddr[1] & 0xFF) << 8);
    else
        ret = ((uint16_t) pAddr[1] & 0xFF) | (((uint32_t) pAddr[0] & 0xFF) << 8);

    return ret;
}

// =============================================================================

int32_t CElfLoad::GetProgramBytes(uint8_t* pAddr, uint32_t& length,
    uint32_t& loadAddress, uint32_t& entryAddress)
{
    int32_t     err;
    uint32_t    maxLen = length;

    // Test if the data is valid
    if (!m_valid)
    {
        if ((err = ParseHeaders()) != ERROR_NONE)
            return err;
    }

    // Loop through all Phdr entries and copy data to the output buffer
    length = 0;
    Elf_PhdrList_t::iterator it;
    for (it = m_phdrs.begin(); it != m_phdrs.end(); it++)
    {
        if ((*it).p_type == 1)
        {
            if (length + (*it).p_filesz > maxLen)
            {
                // File too large 
                length += (*it).p_filesz;
                return ERROR_RESOURCE_TOO_BIG;
            }

            // Copy the data to the buffer
            memcpy(&pAddr[length], &m_pData[(*it).p_offset], (*it).p_filesz);

            // Increment the length
            length += (*it).p_filesz;
        }   
    }

    // Return the load and entry addresses
    loadAddress = m_loadAddress;
    entryAddress = m_entryAddress;

    return ERROR_NONE;
}

