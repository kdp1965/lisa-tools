// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : intelhex.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Intel HEX file format read/write routines
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sstream>

#include    "intelhex.h"
#include    "errors.h"

// ============================================================================

CIntelHex::CIntelHex(std::string filename) :
    m_Filename(filename)
{
    m_outFd = NULL;
    m_writeAll = false;
}

// ============================================================================

uint32_t CIntelHex::FindLowestAddress(void)
{
    FILE*       fd;
    char        sLine[512];
    uint32_t    address, lowestAddress;
    uint8_t     data[128];
    uint32_t    len, type;

    // Try to open the srec file
    if ((fd = fopen(m_Filename.c_str(), "r")) == NULL)
    {
        return (uint32_t) -1;
    }

    // Initialize losestAddress
    lowestAddress = (uint32_t) -1;

    // Initialize line number and parse each line
    m_Line = 0;
    while (fgets(sLine, sizeof sLine, fd) != NULL)
    {
        // Remove trailing \n if it exists
        len = strlen(sLine);
        if (sLine[len - 1] == '\n')
            sLine[len - 1] = '\0';

        // Set the max size of the data
        len = sizeof data;

        // Parse the line
        if (ParseLine(sLine, address, data, len, type) == ERROR_NONE)
        {
            if ((type == INTELHEX_TYPE_DATA) && (address < lowestAddress))
                lowestAddress = address;
        }
        else
            printf("%s\n", m_Error.c_str());
    }

    // Close the file and return the lowest address
    fclose(fd);
    return lowestAddress;
}

// ============================================================================

uint32_t CIntelHex::FindEntryAddress(void)
{
    FILE*       fd;
    char        sLine[512];
    uint32_t    address;
    uint8_t     data[128];
    uint32_t    len, type;

    // Try to open the srec file
    if ((fd = fopen(m_Filename.c_str(), "r")) == NULL)
    {
        return (uint32_t) -1;
    }

    // Initialize line number and parse each line
    m_Line = 0;
    while (fgets(sLine, sizeof sLine, fd) != NULL)
    {
        // Remove trailing \n if it exists
        len = strlen(sLine);
        if (sLine[len - 1] == '\n')
            sLine[len - 1] = '\0';

        // Set the max size of the data
        len = sizeof data;

        // Parse the line
        if (ParseLine(sLine, address, data, len, type) == ERROR_NONE)
        {
            if (type == INTELHEX_TYPE_ENTRY)
            {
                fclose(fd);
                return address;
            }
        }
        else
            printf("%s\n", m_Error.c_str());
    }

    // Close the file and return -1 - not found
    fclose(fd);
    return (uint32_t) -1;
}

// ============================================================================

int32_t CIntelHex::ParseLine(const char* pLine, uint32_t& address, uint8_t* pData, 
        uint32_t& len, uint32_t& type)
{
    uint8_t             checksum = 0;
    uint32_t            slen = strlen(pLine);
    uint32_t            reportedLength, addressBytes;
    uint32_t            recordType;
    uint32_t            recordAddress, value, chk, c;
    const char*         pHex;
    bool                hasData = false;
    std::stringstream   err_str;

    m_Line++;

    // Test line for valid S-Record format
    if (pLine[0] != ':')
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Non Intel Hex Record found";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Validate the line length
    if (slen < 11)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid line length";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Parse the data
    sscanf(pLine, ":%2x%4x%2x", &reportedLength, &recordAddress, &recordType);

    // Validate the reported length against the actual length
    if (slen != reportedLength * 2 + 11)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid HEX record data length";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Initialize the checksum
    checksum = reportedLength + (recordAddress >> 8) + (recordAddress & 0xFF) + recordType;

    // Point to data field within record
    pHex = &pLine[9];

    // Switch based on type of HEX record entry
    switch (recordType)
    {
    // Data entry
    case INTELHEX_RECORD_DATA:
        // Assign the address base on recordAddress and m_BaseAddress
        address = recordAddress + m_BaseAddress;

        // Read data from the record
        for (c = 0; c < reportedLength; c++)
        {
            // Get next byte from HEX record
            sscanf(pHex, "%2x", &value);
            pHex += 2;
            pData[c] = value;

            // Update the checksum
            checksum += value & 0xFF;
        }

        // Now read the checksum from the record and update
        sscanf(pHex, "%2x", &value);
        checksum = (checksum + value) & 0xFF;

        // Set the record type
        type = INTELHEX_TYPE_DATA;
        break;

    // End of file entry
    case INTELHEX_RECORD_EOF:
        // Nothing to do - just update the checksum
        sscanf(pHex, "%2x", &chk);
        checksum = (checksum + chk) & 0xFF;
        break;

    // Extended address entry
    case INTELHEX_RECORD_SEGMENT_ADDR:
        // The segment address is the 1st 4 bytes in the data field
        sscanf(pHex, "%4x%2x", &value, &chk);

        // The segment address is a base address to subsequent records, so
        // update the m_BaseAddress
        m_BaseAddress = (value << 4) & 0x000F0000;

        // Update the checksum
        checksum = (checksum + (value >> 8) + value + chk) & 0xFF;

        // Set the record type.
        type = INTELHEX_TYPE_EXT_ADDRESS;
        break;

    // Start segment address entry - entry location
    case INTELHEX_RECORD_SEGMENT_ENTRY:
        // The segmented start address is the 1st 4 bytes in the data field
        sscanf(pHex, "%8x%2x", &value, &chk);

        // Set the return address.  For segmented entry, it is only 20 bits
        address = value & 0x000FFFFF;

        // Update the checksum
        checksum += (value >> 24) + (value >> 16) + (value >> 8) + value + chk;
        checksum &= 0xFF;

        // Set the record type
        type = INTELHEX_TYPE_ENTRY;
        break;

    // Extended linear address entry
    case INTELHEX_RECORD_LINEAR_ADDR:
        // The linear address is the 1st 4 bytes in the data field
        sscanf(pHex, "%4x%2x", &value, &chk);

        // The segment address is a base address to subsequent records, so
        // update the m_BaseAddress
        m_BaseAddress = (value << 16);

        // Update the checksum
        checksum = (checksum + (value >> 8) + value + chk) & 0xFF;

        // Set the record type.
        type = INTELHEX_TYPE_EXT_ADDRESS;
        break;

    // Start linear address record
    case INTELHEX_RECORD_LINEAR_ENTRY:
        // The linear start address is the 1st 4 bytes in the data field
        sscanf(pHex, "%8x%2x", &value, &chk);

        // Set the return address.  For segmented entry, it is only 20 bits
        address = value;

        // Update the checksum
        checksum += (value >> 24) + (value >> 16) + (value >> 8) + value + chk;
        checksum &= 0xFF;

        // Set the record type
        type = INTELHEX_TYPE_ENTRY;
        break;

    default:
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Validate the checksum
    if (checksum != 0)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid HEX record checksum";
        m_Error = err_str.str();
        return ERROR_INVALID_CHECKSUM;
    }

    // Now set the return length
    len = reportedLength;

    return ERROR_NONE;
}

// ============================================================================

int32_t CIntelHex::GetFileData(uint8_t* pData, uint32_t& len, uint32_t& startAddress,
        uint32_t& entryAddress, bool& moreData, uint32_t& nextStartAddress)
{
    FILE*               fd;
    char                sLine[512];
    uint32_t            address;
    uint8_t             data[256];
    uint32_t            type;
    std::stringstream   err_str;
    uint32_t            maxLen = len;
    uint32_t            highestAddress;
    uint32_t            lowestAddress;
    bool                entryAddressValid = false;

    // Try to open the srec file
    if ((fd = fopen(m_Filename.c_str(), "r")) == NULL)
    {
        err_str << "Unable to open file " << m_Filename;
        m_Error = err_str.str();
        return -1;
    }

    // The lowest address is the starting address
    if ((lowestAddress = FindLowestAddress()) == (uint32_t) -1)
            return (int32_t) startAddress;

    // If we don't already have a starting address, then start at lowest
    if (startAddress == (uint32_t) -1)
        startAddress = lowestAddress;

    // Initialize line number and parse each line
    m_Line = 0;
    nextStartAddress = (uint32_t) -1;
    highestAddress = startAddress;
    moreData = false;

    // Start the base address at zero
    m_BaseAddress = 0;

    while (fgets(sLine, sizeof sLine, fd) != NULL)
    {
        // Remove trailing \n if it exists
        len = strlen(sLine);
        if (sLine[len - 1] == '\n')
            sLine[len - 1] = '\0';

        // Set the max size of the data
        len = sizeof data;

        // Parse the line
        if (ParseLine(sLine, address, data, len, type) == ERROR_NONE)
        {
            if (type == INTELHEX_TYPE_ENTRY)
            {
                // Save the entry address.  This is saved relative to the
                // start address.
                entryAddress = address;
                entryAddressValid = true;
            }
            else if (type == INTELHEX_TYPE_DATA)
            {
                // Skip HEX record entries that are less than the start
                if (address < startAddress)
                    continue;

                // Test if the data will fit in the output buffer
                if (address - startAddress + len > maxLen)
                {
                    // Data too large for buffer
                    moreData = true;
                    if (address < nextStartAddress)
                        nextStartAddress = address;
                    continue;
                }

                // Save the data in the output buffer
                memcpy(&pData[address-startAddress], data, len);
                if (address + len > highestAddress)
                    highestAddress = address + len;
            }
        }
        else
            printf("%s\n", m_Error.c_str());
    }

    // Close the file and return -1 - not found
    fclose(fd);

    // Test if entry address is provided.  If not, set it to the startAddress
    if (!entryAddressValid)
        entryAddress = startAddress;

    // Calculate the length of the data
    len = highestAddress - startAddress;
    return ERROR_NONE;
}

// ============================================================================

int32_t CIntelHex::HexOutStart(bool writeAll)
{
    // Try to open the output file
    m_outFd = fopen(m_Filename.c_str(), "w+");
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Initialize the base address
    m_BaseAddress = 0;

    m_writeAll = writeAll;

    return ERROR_NONE;
}

// ============================================================================

int32_t CIntelHex::HexOutAdd(const uint8_t* pData, uint32_t len, uint32_t firstAddress)
{
    uint8_t     temp[2];
    uint16_t    linearExt;
    uint32_t    currentAddr, c, recordSize, x;
    int32_t     err;
    uint32_t    fileBaseAddress = 0;

    // Validate the file is opened
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Test if firstAddress != current m_BaseAddress
    if ((firstAddress & 0xFFFF0000) != m_BaseAddress)
    {
        // Write a record to set the Linear extended address
        linearExt = firstAddress >> 16;
        temp[0] = linearExt >> 8;
        temp[1] = linearExt & 0xFF;

        // Write a Linear Address record to the file
        HexOutRecord(temp, 2, INTELHEX_RECORD_LINEAR_ADDR, 0);
        m_BaseAddress = firstAddress & 0xFFFF0000;
        fileBaseAddress = m_BaseAddress;
    }

    // Loop through all data and write records, max 32 bytes per record
    currentAddr = firstAddress;
    for (c = 0; c < len; )
    {
        // Default to recordSize of 32
        recordSize = 32;
        if (c + recordSize > len)
            recordSize = len - c;
        
        // Test if address + len causes a change in base address
        if ((((c + recordSize + firstAddress) & 0xFFFF0000) > m_BaseAddress) &&
            (((c + recordSize + firstAddress) & 0x0000FFFF) != 0))
        {
            // Restrict recordSize to fit within the 64K boundry of m_BaseAddress
            recordSize = m_BaseAddress - (c + firstAddress);
        }

        for (x = c; x < c + recordSize; x++)
        {
            if ((pData[x + firstAddress] != 0xFF) || m_writeAll)
            {
                // Test if we need to issue a linear extended address record
                if (fileBaseAddress != m_BaseAddress)
                {
                    // Write a record to set the Linear extended address
                    linearExt = m_BaseAddress >> 16;
                    temp[0] = linearExt >> 8;
                    temp[1] = linearExt & 0xFF;

                    // Write a Linear Address record to the file
                    if ((err = HexOutRecord(temp, 2, INTELHEX_RECORD_LINEAR_ADDR, 0)) != ERROR_NONE)
                        return err;
                    fileBaseAddress = m_BaseAddress;
                }

                // Write the next record of data
                if ((err = HexOutRecord(&pData[c + firstAddress], recordSize, INTELHEX_RECORD_DATA, 
                    (firstAddress + c) & 0xFFFF)) != ERROR_NONE)
                {
                    return err;
                }

                break;
            }
        }


        // Update the current index into the data base on recordSize
        c += recordSize;
        m_BaseAddress = (c + firstAddress) & 0xFFFF0000;
    }

    return ERROR_NONE;
}

// ============================================================================

int32_t CIntelHex::HexOutEnd(void)
{
    int32_t     err;

    // Validate the file is opened
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Write an EOF record to the file
    if ((err = HexOutRecord(NULL, 0, INTELHEX_RECORD_EOF, 0)) != ERROR_NONE)
        return err;

    // Close the file
    fclose(m_outFd);
    m_outFd = NULL;

    return 0;
}

// ============================================================================

int32_t CIntelHex::HexOutRecord(const uint8_t* pData, uint32_t len, char recordType, uint16_t addr)
{
    uint8_t     checksum;
    uint32_t    c;

    // Write the record length, address and type fields to the file
    fprintf(m_outFd, ":%02X%04X%02X", len, addr, recordType);

    // Initialize the checksum
    checksum = len + (addr >> 8) + addr + recordType;

    // Loop through all data and write each byte to the file
    for (c = 0; c < len; c++)
    {
        // Write the next byte to the file
        fprintf(m_outFd, "%02X", pData[c]);

        // Update the checksum
        checksum += pData[c];
    }

    // Calcualtes 2's compliment of checksum
    checksum = (uint8_t) -checksum;

    // Write the checksum to the file
    fprintf(m_outFd, "%02X\n", checksum);

    return ERROR_NONE;
}

