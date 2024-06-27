// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : srec.h
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

#ifndef SREC_H
#define SREC_H

#include    <string>
#include    <stdint.h>

#define     SREC_TYPE_VENDOR_SPECIFIC   1
#define     SREC_TYPE_DATA              2
#define     SREC_TYPE_COUNT             3
#define     SREC_TYPE_ENTRY             4

#define     SREC_ERROR_NONE             0
#define     SREC_ERROR_CHECKSUM         -1
#define     SREC_ERROR_NOFILE           -2
#define     SREC_ERROR_INVALID_FORMAT   -3
#define     SREC_ERROR_BUFFER_TOO_SMALL -4

#define     SREC_RECORD_VENDOR_SPECIFIC '0'
#define     SREC_RECORD_DATA_16BIT      '1'
#define     SREC_RECORD_DATA_24BIT      '2'
#define     SREC_RECORD_DATA_32BIT      '3'
#define     SREC_RECORD_COUNT           '5'
#define     SREC_RECORD_ENTRY_32BIT     '7'
#define     SREC_RECORD_ENTRY_24BIT     '8'
#define     SREC_RECORD_ENTRY_16BIT     '9'

class CSrec
{
public:
    CSrec(std::string filename, uint32_t imageSize = 64*1024);

    /// Returns the lowest address reported in the S-Rec file
    virtual uint32_t    FindLowestAddress(void);

    /// Returns the entry location from the S-Rec file, or -1 if not found
    virtual uint32_t    FindEntryAddress(void);

    /// Reads all data from the file to the given buffer
    int32_t GetFileData(uint8_t* pData, uint32_t& len, uint32_t& startAddress,
                uint32_t& entryAddress, bool& moreData, uint32_t& nextStartAddress);

    /// Starts a HEX output session
    virtual int32_t     SrecOutStart(bool writeAll = false);

    /// Adds data to the Hex output
    virtual int32_t     SrecOutAdd(const uint8_t* pData, uint32_t len, uint32_t firstAddress);

    /// Adds an end record the Hex output and closes the file
    virtual int32_t     SrecOutEnd();

private:
    /// The filename to process
    std::string         m_Filename;

    /// The size of the output image
    uint32_t            m_ImageSize;

    /// String form of last error
    std::string         m_Error;

    /// Line number being parsed from input file
    uint32_t            m_Line;

    // Number of records wrttien to the output file
    uint32_t            m_RecordCount;

    /// Parses a single line from the S-Rec file
    /// @return error number or zero if success
    virtual int32_t     ParseLine(const char * pLine, uint32_t& address, uint8_t* pData, uint32_t& len, uint32_t& type);

    virtual uint8_t     ConvertHex(const char * pStr);

    // Output a single srec to the output file
    int32_t             SrecOutRecord(const uint8_t* pData, uint32_t len, char recordType, uint32_t addr);

    /// Indicates if all records (including all FFh) should be written to the output
    bool                m_writeAll;

    /// Current FILE descriptor for HexOut operations
    FILE*               m_outFd;
};

#endif  // SREC_H

