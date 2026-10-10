/* serial.h */

/* $Id: serial.h,v 1.8 2011/07/09 08:16:21 kpettit1 Exp $ */

/*
 * Copyright 2004 Stephen Hurd and Ken Pettit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif

#ifndef MT_SERIAL_H
#define MT_SERIAL_H

enum {
	SER_NO_ERROR,
	SER_IO_ERROR,
	SER_INVALID_PORT,
	SER_PORT_NOT_SELECTED,
	SER_TIMEOUT,
	SER_NO_DATA,
	SER_PORT_NOT_OPEN
};

enum {
	SER_MON_COM_READ,
	SER_MON_COM_WRITE,
	SER_MON_COM_PORT_CHANGE,
	SER_MON_COM_SIGNAL
};

enum {
	SER_MON_COM_CHANGE_ALL,
	SER_MON_COM_CHANGE_NAME,
	SER_MON_COM_CHANGE_STATE,
	SER_MON_COM_CHANGE_BAUD,
	SER_MON_COM_CHANGE_BITS,
	SER_MON_COM_CHANGE_STOP,
	SER_MON_COM_CHANGE_PARITY
};

#define SER_FLAG_RX_AVAIL	0x01
#define SER_FLAG_TX_EMPTY	0x02
#define SER_FLAG_OVERRUN	0x04
#define SER_FLAG_FRAME_ERR	0x08
#define SER_FLAG_PARITY_ERR	0x10
#define	SER_FLAG_CTS		0x20
#define SER_FLAG_DSR		0x40
#define SER_FLAG_RING		0x80

#define	SER_SIGNAL_CTS		0x01
#define	SER_SIGNAL_DSR		0x02
#define	SER_SIGNAL_DTR		0x04
#define	SER_SIGNAL_RTS		0x08

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ser_callback)(int pinLevel);
typedef void (*ser_monitor_cb)(int fMonType, unsigned char data);

typedef struct ser_params
{
	char			port_name[256];		// Host port name for emulation
	char			parity;				// Current parity setting
	int				baud_rate;			// Current baud rate
	int				bit_size;			// Bit size for theport
	int				stop_bits;			// Number of stop bits
	int				open_flag;			// Set to 1 when the port is open

	ser_callback	pCallback;			// Callback function for RX data
	ser_monitor_cb	pMonCallback;

	FILE*			pCmdFile;			// Command file for Simulated I/O

	// Host COM port control structures

#ifdef WIN32
	HANDLE			hComm;				// Handle to the serial port
	OVERLAPPED		osRead;				// Overlapped I/O structure for reading
	OVERLAPPED		osWrite;			// Overlapped I/O structure for writing
	DCB				dcb;				// Device control block
	HANDLE			hReadThread;		// Read COM thread
	HANDLE			hWriteThread;		// Write COM thread
	HANDLE			hThreadExitEvent;	// Event to trigger thread exiting
	HANDLE			hWriteEvent;		// Event to trigger a write operation
	HANDLE			hWriteMutex;
	HANDLE			hReadMutex;
	int				fReadThread;		// Flag indicating if Read Thread active
	int				fWriteThread;		// Flag indicating if Write Tread active
	int				fIntPending;
	int				dtrState;			// Current state of DTR
	int				rtsState;			// Current state of RTS
	char			rx_buf[128];		// Read buffer
	int				rxIn;				// Read buffer input location
	int				rxOut;				// Read buffer output location
	char			tx_buf[128];		// Write buffer
	int				txIn;				// Read buffer input location
	int				txOut;				// Read buffer output location
	int				tx_empty;			// Flag to indicate when TX is done
	char			flags;				// Current COMM port state flags

#else
	int				fd;					// fd of open serial port
	int				dtrState;			// Current state of DTR
	int				rtsState;			// Current state of RTS

#endif

} ser_params_t;

/* Configuration functions */
int ser_init(const char *pPortName, ser_params_t **ppContext);
int ser_deinit(ser_params_t *pContext);
int ser_get_port_list(ser_params_t *pContext, char* port_list, int max, int *count);
int ser_set_baud(ser_params_t *pContext, int baud_rate);
int ser_set_parity(ser_params_t *pContext, char parity);
int ser_set_bit_size(ser_params_t *pContext, int bit_size);
int ser_set_stop_bits(ser_params_t *pContext, int stop_bits);
int ser_set_callback(ser_params_t *pContext, ser_callback pCallback);
int ser_set_monitor_callback(ser_params_t *pContext, ser_monitor_cb pCallback);
int ser_set_port(ser_params_t *pContext, char* port);
int ser_get_port_settings(ser_params_t *pContext, char* port, int* open_state, int* baud, int* size, 
						  int* stop_bits, char* parity);
int ser_poll(ser_params_t *pContext);

int ser_open_port(ser_params_t* pContext);
int ser_close_port(ser_params_t* pContext);

/* Communication functions */
int ser_get_flags(ser_params_t *pContext, unsigned char *flags);
int ser_set_signals(ser_params_t *pContext, unsigned char flags);
int ser_get_signals(ser_params_t *pContext, unsigned char *signals);
int ser_read_byte(ser_params_t *pContext, char* data);
int ser_write_byte(ser_params_t *pContext, char data);

#ifdef __cplusplus
}
#endif


#endif
