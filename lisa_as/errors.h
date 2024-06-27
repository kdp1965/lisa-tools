// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : errors.h
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Error definitions used for the assembler framework
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#ifndef ERRORS_H
#define ERRORS_H

#define ERROR_NONE                          0
#define ERROR_INVALID_LOCATE                -1
#define ERROR_INVALID_SEGMENT_FORMAT        -2
#define ERROR_INVALID_SYNTAX                -3
#define ERROR_UNKONWN_SEGMENT_TYPE          -4
#define ERROR_FILE_NOT_FOUND                -5
#define ERROR_PARSER_ERROR                  -6
#define ERROR_CANT_OPEN_FILE                -7
#define ERROR_INVALID_ELSIF_SYNTAX          -8
#define ERROR_INVALID_VAR_SYNTAX            -9
#define ERROR_INVALID_INCLUDE_SYNTAX        -10
#define ERROR_INVALID_IFDEF_SYNTAX          -11
#define ERROR_INVALID_DATA_SYNTAX           -12
#define ERROR_INVALID_BASE_SYNTAX           -13
#define ERROR_INVALID_IFNDEF_SYNTAX         -14
#define ERROR_INVALID_CHECKSUM              -15
#define ERROR_INVALID_FILE_FORMAT           -16
#define ERROR_INVALID_IF_SYNTAX             -17
#define ERROR_INVALID_LABEL_SYNTAX          -18

#define ERROR_OUTPUT_PARAM_UNKNOWN          -19
#define ERROR_SEGMENT_NOT_LOCATED           -20
#define ERROR_VARIABLE_NOT_FOUND            -21
#define ERROR_OUT_OF_MEMORY                 -22
#define ERROR_RESOURCE_TOO_BIG              -23
#define ERROR_CANT_READ_FILE                -24
#define ERROR_SEGMENTS_OVERLAP              -25
#define ERROR_ELSE_WITHOUT_IF               -26
#define ERROR_ENDIF_WITHOUT_IF              -27
#define ERROR_INVALID_OPCODE_SYNTAX         -28
#define ERROR_INVALID_FILE_SYNTAX           -29
#define ERROR_INVALID_DEFINE_SYNTAX         -30
#define ERROR_INVALID_UNDEF_SYNTAX          -31
#define ERROR_LABEL_NOT_DEFINED             -32
#define ERROR_BRANCH_DISTANCE_TOO_BIG       -33

#endif  // ERRORS_H

