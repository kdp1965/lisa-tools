// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : intelhex.h
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

#ifndef INTEL_HEX_H
#define INTEL_HEX_H

#include    <string>
#include    <stdint.h>

#define     INTELHEX_TYPE_EXT_ADDRESS       1
#define     INTELHEX_TYPE_DATA              2
#define     INTELHEX_TYPE_COUNT             3
#define     INTELHEX_TYPE_ENTRY             4

#define     INTELHEX_ERROR_NONE             0
#define     INTELHEX_ERROR_CHECKSUM         -1
#define     INTELHEX_ERROR_NOFILE           -2
#define     INTELHEX_ERROR_INVALID_FORMAT   -3
#define     INTELHEX_ERROR_BUFFER_TOO_SMALL -4

#define     INTELHEX_RECORD_DATA            0
#define     INTELHEX_RECORD_EOF             1
#define     INTELHEX_RECORD_SEGMENT_ADDR    2
#define     INTELHEX_RECORD_SEGMENT_ENTRY   3
#define     INTELHEX_RECORD_LINEAR_ADDR     4
#define     INTELHEX_RECORD_LINEAR_ENTRY    5

class CIntelHex
{
public:
    CIntelHex(std::string filename);

    /// Returns the lowest address reported in the S-Rec file
    virtual uint32_t    FindLowestAddress(void);

    /// Returns the entry location from the S-Rec file, or -1 if not found
    virtual uint32_t    FindEntryAddress(void);

    /// Reads all data from the file to the given buffer
    virtual int32_t     GetFileData(uint8_t* pData, uint32_t& len, uint32_t& startAddress,
                            uint32_t& entryAddress, bool& moreData, uint32_t& nextStartAddress);

    /// Starts a HEX output session
    virtual int32_t     HexOutStart(bool writeAll = false);

    /// Adds data to the Hex output
    virtual int32_t     HexOutAdd(const uint8_t* pData, uint32_t len, uint32_t firstAddress);

    /// Adds an end record the Hex output and closes the file
    virtual int32_t     HexOutEnd();

private:
    /// The filename to process
    std::string         m_Filename;

    /// String form of last error
    std::string         m_Error;

    /// Line number being parsed from input file
    uint32_t            m_Line;

    /// The current base address for segment or linear addressing
    uint32_t            m_BaseAddress;

    /// Indicates if all records (including all FFh) should be written to the output
    bool                m_writeAll;

    /// Parses a single line from the HEX file
    /// @return error number or zero if success
    virtual int32_t     ParseLine(const char * pLine, uint32_t& address, uint8_t* pData,
                            uint32_t& len, uint32_t& type);

    /// Writes a HexOut record the the m_outFd
    virtual int32_t     HexOutRecord(const uint8_t* pData, uint32_t len, char recordType, uint16_t addr);

    /// Current FILE descriptor for HexOut operations
    FILE*               m_outFd;
};

#endif  // INTEL_HEX_H

