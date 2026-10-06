/*
================================================================================
cmod_a7.h:     Header file for the test C program for lisa core
================================================================================
*/
enum
{
  SFR_PORTA             = 0,
  SFR_PORTB,
  SFR_PORTC,
  SFR_PORTC_DIR,

  SFR_TIMER1_PRE_DIVL   = 0x08,
  SFR_TIMER1_PRE_DIVH,
  SFR_TIMER1_DIVL,
  SFR_TIMER1_DIVH,
  SFR_TIMER1_CTRL,
  SFR_TIMER1_TICK,

  SFR_UART_RXTX         = 0x10,
  SFR_UART_STATUS,
  SFR_UART2_RXTX,
  SFR_UART2_STATUS,
  SFR_UART2_BRG,   

  SFR_TIMER2_PRE_DIVL   = 0x18,
  SFR_TIMER2_PRE_DIVH,
  SFR_TIMER2_DIVL,
  SFR_TIMER2_DIVH,
  SFR_TIMER2_CTRL,
  SFR_TIMER2_TICK,

  SFR_I2C_DIV_LSB       = 0x20,
  SFR_I2C_DIV_MSB,
  SFR_I2C_CTRL,
  SFR_I2C_RX,
  SFR_I2C_STATUS,
  SFR_I2C_TX,
  SFR_I2C_CMD
};

sfr unsigned char PORTA             = SFR_PORTA;
sfr unsigned char PORTB             = SFR_PORTB;
sfr unsigned char PORTC             = SFR_PORTC;
sfr unsigned char PORTC_DIR         = SFR_PORTC_DIR;

sfr unsigned char TIMER1_PRE_DIVL   = SFR_TIMER1_PRE_DIVL;
sfr unsigned char TIMER1_PRE_DIVH   = SFR_TIMER1_PRE_DIVH;
sfr unsigned char TIMER1_DIVL       = SFR_TIMER1_DIVL;
sfr unsigned char TIMER1_DIVH       = SFR_TIMER1_DIVH;
sfr unsigned char TIMER1_CTRL       = SFR_TIMER1_CTRL;
sfr unsigned char TIMER1_TICK       = SFR_TIMER1_TICK;

sfr unsigned char UART_RXTX         = SFR_UART_RXTX;
sfr unsigned char UART_STATUS       = SFR_UART_STATUS;
sfr unsigned char UART2_RXTX        = SFR_UART2_RXTX;
sfr unsigned char UART2_STATUS      = SFR_UART2_STATUS;
sfr unsigned char UART2_BRG         = SFR_UART2_BRG;

sfr unsigned char I2C_DIV_LSB       = SFR_I2C_DIV_LSB;
sfr unsigned char I2C_DIV_MSB       = SFR_I2C_DIV_MSB;
sfr unsigned char I2C_CTRL          = SFR_I2C_CTRL;
sfr unsigned char I2C_RX            = SFR_I2C_RX;
sfr unsigned char I2C_STATUS        = SFR_I2C_STATUS;
sfr unsigned char I2C_TX            = SFR_I2C_TX;
sfr unsigned char I2C_CMD           = SFR_I2C_CMD;

sfr unsigned char TIMER2_PRE_DIVL   = SFR_TIMER2_PRE_DIVL;
sfr unsigned char TIMER2_PRE_DIVH   = SFR_TIMER2_PRE_DIVH;
sfr unsigned char TIMER2_DIVL       = SFR_TIMER2_DIVL;
sfr unsigned char TIMER2_DIVH       = SFR_TIMER2_DIVH;
sfr unsigned char TIMER2_CTRL       = SFR_TIMER2_CTRL;
sfr unsigned char TIMER2_TICK       = SFR_TIMER2_TICK;


#define CLK_SPEED_HZ        22000000
#define TIMER_PRE_DIV_RATE  1000
#define UART_BAUD_RATE      115200

#define I2C_CMD_START      0x80
#define I2C_CMD_STOP       0x40
#define I2C_CMD_READ       0x20
#define I2C_CMD_WRITE      0x10
#define I2C_CMD_ACK        0x04
#define I2C_CMD_IACK       0x01

#define I2C_STATUS_RXACK   0x80
#define I2C_STATUS_BUSY    0x40
#define I2C_STATUS_AL      0x40     // Arbitration lost
#define I2C_STATUS_TIP     0x02     // Transfer in progress
#define I2C_STATUS_IRQ     0x01

/*
================================================================================
Constants for the Timer peripherals
================================================================================
*/
#define TIMER_ENABLE             0x01
#define TIMER_ROLLOVER           0x80

/*
================================================================================
Constants for the UART peripherals
================================================================================
*/
#define UART_RX_AVAIL            0x01
#define UART_TX_EMPTY            0x02
#define UART_AUTOBAUD_DISABLE    0x04

void enable_int(void);
void disable_int(void);
