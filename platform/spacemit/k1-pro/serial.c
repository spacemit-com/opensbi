/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Spacemit.
 */

#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>

#define SUART_REG_BASE		(0x10010000)
#define UART_SRC_CLK		(24000000)
#define UART_BAUD           (115200)      // Baud rate for UART

typedef struct serial_hw
{
	volatile unsigned int rbr;		/* 0 */
	volatile unsigned int ier;		/* 1 */
	volatile unsigned int fcr;		/* 2 */
	volatile unsigned int lcr;		/* 3 */
	volatile unsigned int mcr;		/* 4 */
	volatile unsigned int lsr;		/* 5 */
	volatile unsigned int msr;		/* 6 */
	volatile unsigned int sch;		/* 7 */
}serial_hw_t;

// UART Line Control Parameter
#define   PARITY       0           // Parity: 0,2 - no parity; 1 - odd parity; 3 - even parity
#define   STOP         0           // Number of Stop Bit: 0 - 1bit; 1 - 2(or 1.5)bits
#define   DLEN         3           // Data Length: 0 - 5bits; 1 - 6bits; 2 - 7bits; 3 - 8bits

#define thr rbr
#define dll rbr
#define dlh ier
#define iir fcr

void suart_serial_putc (char c)
{
	serial_hw_t *serial_ctrl_base  = (serial_hw_t *)SUART_REG_BASE;

	while((serial_ctrl_base->lsr & ( 1 << 6 )) == 0);
	serial_ctrl_base->thr = c;
}

int suart_serial_getc (void)
{
	serial_hw_t *serial_ctrl_base  = (serial_hw_t *)SUART_REG_BASE;

	if(serial_ctrl_base->lsr & 1)
	    return serial_ctrl_base->rbr;

	return -1;
}

int suart_serial_tstc (void)
{
	serial_hw_t *serial_ctrl_base  = (serial_hw_t *)SUART_REG_BASE;

	return serial_ctrl_base->lsr & 1;
}

static struct sbi_console_device suart_console = {
	.name = "suart",
	.console_putc = suart_serial_putc,
	.console_getc = suart_serial_getc
};

int suart_serial_init(void)
{
	u32 uart_clk;

	//uart init
	serial_hw_t *serial_ctrl_base  = (serial_hw_t *)SUART_REG_BASE;

	serial_ctrl_base->mcr = 0x3;
	uart_clk = (UART_SRC_CLK + 8 * UART_BAUD)/(16 * UART_BAUD);
	serial_ctrl_base->lcr |= 0x80;
	serial_ctrl_base->dlh = uart_clk>>8;
	serial_ctrl_base->dll = uart_clk&0xff;
	serial_ctrl_base->lcr &= ~0x80;
	serial_ctrl_base->lcr = ((PARITY&0x03)<<3) | ((STOP&0x01)<<2) | (DLEN&0x03);
	serial_ctrl_base->fcr = 0x7;

	sbi_console_set_device(&suart_console);

	return 0;
}

