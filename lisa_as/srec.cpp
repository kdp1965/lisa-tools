// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : srec.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Motorola S-Record file format read/write routines
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

#include    "srec.h"
#include    "errors.h"

/// ============================================================================

CSrec::CSrec(std::string filename, uint32_t imageSize) :
    m_Filename(filename), m_ImageSize(imageSize)
{
}

/// ============================================================================

uint32_t CSrec::FindLowestAddress(void)
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
            if ((type == SREC_TYPE_DATA) && (address < lowestAddress))
                lowestAddress = address;
        }
        else
            printf("%s\n", m_Error.c_str());
    }

    // Close the file and return the lowest address
    fclose(fd);
    return lowestAddress;
}

/// ============================================================================

uint32_t CSrec::FindEntryAddress(void)
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
            if (type == SREC_TYPE_ENTRY)
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

/// ============================================================================

int32_t CSrec::ParseLine(const char* pLine, uint32_t& address, uint8_t* pData, 
        uint32_t& len, uint32_t& type)
{
    uint8_t             checksum = 0;
    uint32_t            slen = strlen(pLine);
    uint32_t            reportedLength, addressBytes;
    bool                hasData = false;
    std::stringstream   err_str;

    m_Line++;

    // Test line for valid S-Record format
    if (pLine[0] != 'S')
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Non S-Record found";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Validate the line length
    if (slen < 10)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid line length";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Determine the type of s-record entry
    switch (pLine[1])
    {
    case SREC_RECORD_VENDOR_SPECIFIC:
        // Vendor specific data. Nothing to do
        type = SREC_TYPE_VENDOR_SPECIFIC;
        return ERROR_NONE;

    case SREC_RECORD_DATA_16BIT:
        // Data with 16-bit address
        type = SREC_TYPE_DATA;
        addressBytes = 2;
        hasData = true;
        break;

    case SREC_RECORD_DATA_24BIT:
        // Data with 24-bit address
        type = SREC_TYPE_DATA;
        addressBytes = 3;
        hasData = true;
        break;

    case SREC_RECORD_DATA_32BIT:
        // Data with 32-bit address
        type = SREC_TYPE_DATA;
        addressBytes = 4;
        hasData = true;
        break;

    case SREC_RECORD_COUNT:
        // Count of entries in S-Rec file
        type = SREC_TYPE_COUNT;
        addressBytes = 2;
        hasData = false;
        break;

    case SREC_RECORD_ENTRY_32BIT:
        // 32-bit Entry address 
        type = SREC_TYPE_ENTRY;
        addressBytes = 4;
        hasData = false;
        break;

    case SREC_RECORD_ENTRY_24BIT:
        // 24-bit Entry address 
        type = SREC_TYPE_ENTRY;
        addressBytes = 3;
        hasData = false;
        break;

    case SREC_RECORD_ENTRY_16BIT:
        // 16-bit Entry address 
        type = SREC_TYPE_ENTRY;
        addressBytes = 2;
        hasData = false;
        break;

    default:
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Get the reported length
    reportedLength = ConvertHex(&pLine[2]);
 
    // Validate the reported length against the actual length
    if (slen != reportedLength * 2 + 4)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid S-Record data length";
        m_Error = err_str.str();
        return ERROR_INVALID_FILE_FORMAT;
    }

    // Validate the checksum
    uint32_t c = 2;
    while (c < slen)
    {
        checksum += ConvertHex(&pLine[c]);
        c+= 2;
    }
    if (checksum != 0xFF)
    {
        err_str << m_Filename << ": Line " << m_Line <<
            ": Invalid S-Record checksum";
        m_Error = err_str.str();
        return ERROR_INVALID_CHECKSUM;
    }

    // Format checks out.  Get the address
    address = 0;
    uint32_t index = 4;
    for (c = 0; c < addressBytes; c++)
    {
        address = address * 256 + ConvertHex(&pLine[index]);
        index += 2;
    }

    // Now extract the data
    len = reportedLength - addressBytes - 1;
    for (c = 0; c < len; c++)
    {
        pData[c] = ConvertHex(&pLine[index]);
        index += 2;
    }

    return ERROR_NONE;
}

uint8_t CSrec::ConvertHex(const char* pStr)
{
    uint8_t val;

    // Check the line length byte - convert from HEX
    if (isdigit(pStr[0]))
        val = (pStr[0] - '0') * 16;
    else if (pStr[0] >= 'A' && pStr[0] <= 'F')
        val = (pStr[0] - 'A' + 10) * 16;
    else if (pStr[0] >= 'a' && pStr[0] <= 'f')
        val = (pStr[0] - 'a' + 10) * 16;
    if (isdigit(pStr[1]))
        val += pStr[1] - '0';
    else if (pStr[1] >= 'A' && pStr[1] <= 'F')
        val += pStr[1] - 'A' + 10;
    else if (pStr[1] >= 'a' && pStr[1] <= 'f')
        val += pStr[1] - 'a' + 10;

    return val;
}

/// ============================================================================

int32_t CSrec::GetFileData(uint8_t* pData, uint32_t& len, uint32_t& startAddress,
        uint32_t& entryAddress, bool& moreData, uint32_t& nextStartAddress)
{
    FILE*               fd;
    char                sLine[512];
    uint32_t            address;
    uint8_t             data[128];
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
            if (type == SREC_TYPE_ENTRY)
            {
                // Save the entry address.  This is saved relative to the
                // start address.
                entryAddress = address;
                entryAddressValid = true;
            }
            else if (type == SREC_TYPE_DATA)
            {
                // Skip S-Record entries that are less than the start
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

    // Test if entry address valid.  If not, assign it the same as startAddress
    if (!entryAddressValid)
        entryAddress = startAddress;

    // Calculate the length of the data
    len = highestAddress - startAddress;
    return ERROR_NONE;
}

// ============================================================================

int32_t CSrec::SrecOutStart(bool writeAll)
{
    // Try to open the output file
    m_outFd = fopen(m_Filename.c_str(), "w+");
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Initialize the base address
    m_RecordCount = 0;

    m_writeAll = writeAll;

    return ERROR_NONE;
}

// ============================================================================

int32_t CSrec::SrecOutAdd(const uint8_t* pData, uint32_t len, uint32_t firstAddress)
{
    uint32_t    currentAddr, c, recordSize, x;
    int32_t     err;
    char        recordType;

    // Validate the file is opened
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Determine the record type based on the image size
    if (m_ImageSize <= 64 * 1024)
        recordType = SREC_RECORD_DATA_16BIT;
    else if (m_ImageSize <= 16 * 1024 * 1024)
        recordType = SREC_RECORD_DATA_24BIT;
    else
        recordType = SREC_RECORD_DATA_32BIT;

    // Loop through all data and write records, max 32 bytes per record
    currentAddr = firstAddress;
    for (c = 0; c < len; )
    {
        // Default to recordSize of 32
        recordSize = 64;
        if (c + recordSize > len)
            recordSize = len - c;
        
        for (x = c; x < c + recordSize; x++)
        {
            if ((pData[x + firstAddress] != 0xFF) || m_writeAll)
            {
                // Write the next record of data
                if ((err = SrecOutRecord(&pData[c + firstAddress], recordSize, recordType, 
                    firstAddress + c)) != ERROR_NONE)
                {
                    return err;
                }

                break;
            }
        }


        // Update the current index into the data base on recordSize
        c += recordSize;
    }

    return ERROR_NONE;
}

// ============================================================================

int32_t CSrec::SrecOutEnd(void)
{
    int32_t     err;
    char        temp[2];
    char        recordType;

    // Validate the file is opened
    if (m_outFd == NULL)
        return ERROR_CANT_OPEN_FILE;

    // Determine the record type based on the image size
    if (m_ImageSize <= 64 * 1024)
        recordType = SREC_RECORD_ENTRY_16BIT;
    else if (m_ImageSize <= 16 * 1024 * 1024)
        recordType = SREC_RECORD_ENTRY_24BIT;
    else
        recordType = SREC_RECORD_ENTRY_32BIT;

    // Write an EOF record to the file
    if ((err = SrecOutRecord(NULL, 0, recordType, 0)) != ERROR_NONE)
        return err;

    // Close the file
    fclose(m_outFd);
    m_outFd = NULL;

    return 0;
}

// ============================================================================

int32_t CSrec::SrecOutRecord(const uint8_t* pData, uint32_t len, char recordType, uint32_t addr)
{
    uint8_t     checksum;
    uint32_t    c;

    // Increment the record count
    m_RecordCount++;

    // Write the record length, address and type fields to the file
    switch (recordType)
    {
    case SREC_RECORD_DATA_32BIT:
    case SREC_RECORD_ENTRY_32BIT:
        fprintf(m_outFd, "S%c%02x%08x", recordType, len+5, addr);
        checksum = len + 5;
        break;

    case SREC_RECORD_DATA_24BIT:
    case SREC_RECORD_ENTRY_24BIT:
        fprintf(m_outFd, "S%c%02x%06x", recordType, len+4, addr);
        checksum = len + 4;
        break;

    case SREC_RECORD_DATA_16BIT:
    case SREC_RECORD_ENTRY_16BIT:
    case SREC_RECORD_COUNT:
        fprintf(m_outFd, "S%c%02x%04x", recordType, len+3, addr);
        checksum = len + 3;
        break;
    }

    // Initialize the checksum
    checksum += (addr >> 8) + addr;
    if (recordType == SREC_RECORD_DATA_24BIT)
    {
        checksum += (addr >> 16);
    }
    else if (recordType == SREC_RECORD_DATA_32BIT)
    {
        checksum += (addr >> 16) + (addr >> 24);
    }

    // Loop through all data and write each byte to the file
    for (c = 0; c < len; c++)
    {
        // Write the next byte to the file
        fprintf(m_outFd, "%02x", pData[c]);

        // Update the checksum
        checksum += pData[c];
    }

    // Calcualtes 1's compliment of checksum
    checksum = (uint8_t) ~checksum;

    // Write the checksum to the file
    fprintf(m_outFd, "%02x\n", checksum);

    return ERROR_NONE;
}

