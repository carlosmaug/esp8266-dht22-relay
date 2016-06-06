/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/

/**
 * @file stdout.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @brief Stupid bit of code that does the bare minimum to make os_printf work. 
 *
 */

#include <esp8266.h>
#include <uart_hw.h>

static void ICACHE_FLASH_ATTR _stdout_uart_txd(char c) {
	// Wait until there is room in the FIFO
	while (((READ_PERI_REG(UART_STATUS(0))>>UART_TXFIFO_CNT_S)&UART_TXFIFO_CNT)>=126) ;
	// Send the character
	WRITE_PERI_REG(UART_FIFO(0), c);
}

static void ICACHE_FLASH_ATTR _stdout_putchar(char c) {
	// Convert \n -> \r\n
	if (c == '\n') _stdout_uart_txd('\r');

	_stdout_uart_txd(c);
}


void stdout_init() {
	// Enable TxD pin
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	
	// Set baud rate and other serial parameters to 115200,n,8,1
	uart_div_modify(0, UART_CLK_FREQ/BIT_RATE_115200);
	WRITE_PERI_REG(UART_CONF0(0), (STICK_PARITY_DIS)|(ONE_STOP_BIT << UART_STOP_BIT_NUM_S)| \
				(EIGHT_BITS << UART_BIT_NUM_S));

	// Reset tx & rx fifo
	SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);

	// Clear pending interrupts
	WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);

	// Install our own putchar handler
	os_install_putc1((void *)_stdout_putchar);
}
