/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>

#include <uart-zynq.h>
#include <uart-zynq-hw.h>

#include <io.h>
#include <device.h>
#include <console.h>

/* FIXME zedboard uses UART1 only */
#define UART_BASE	UART1_PHYSBASE

/* internal routines */

static int __uart_zynq_enable(struct bus_device * bus, addr_t base)
{
	bus_write_fp bus_write32;

	if (bus != NULL) {
//		bus_write32 = bus->bus_ops
	}
	/* Enable TX and RX */
	write32(base + UART_OFFSET_CR, UART_CR_TX_EN | UART_CR_RX_EN);
}

static void __uart_zynq_disable(uint32_t base)
{
	/* Disable TX and RX */
	write32(base + UART_OFFSET_CR, UART_CR_TX_DIS | UART_CR_RX_DIS);
}

static void __uart_zynq_init(uint32_t base)
{
	/* Disable interrupts */
	write32(base + UART_OFFSET_IDR, UART_IXR_MASK);
	/* Disable TX and RX */
	__uart_zynq_disable(base);
	/* Reset TX and RX, Clear FIFO */
	write32(base + UART_OFFSET_CR, UART_CR_TXRST | UART_CR_RXRST);
	/* Clear Flags */
	write32(base + UART_OFFSET_ISR, UART_IXR_MASK);
	/* Mode Reset to Normal/8-N-1 */
	write32(base + UART_OFFSET_MR,
		UART_MR_CHMODE_NORM | UART_MR_CHARLEN_8_BIT |
		UART_MR_PARITY_NONE | UART_MR_STOPMODE_1_BIT);
	/* Trigger Reset */
	write32(base + UART_OFFSET_RXWM, UART_RXWM_RESET_VAL);
	write32(base + UART_OFFSET_TXWM, UART_TXWM_RESET_VAL);
	/* Disable RX timeout */
	write32(base + UART_OFFSET_RXTOUT, UART_RXTOUT_DISABLE);
	/* Reset baud rate generator and divider to genetate 115200 */
	write32(base + UART_OFFSET_BAUDGEN, 0x3E);
	write32(base + UART_OFFSET_BAUDDIV, 0x06);
	/* Set CR Value */
	write32(base + UART_OFFSET_CR,
		UART_CR_RX_DIS | UART_CR_TX_DIS | UART_CR_STOPBRK);
}

static unsigned char __uart_zynq_getchar(uint32_t base)
{
	while (read32(base + UART_OFFSET_SR) & UART_SR_RXEMPTY);
	return read8(base + UART_OFFSET_FIFO);
}

/* uart-zynq never fails to output a byte if it can block execution,
 * so this internal function returns nothing.
 * Note that all corresponding interface routines should return an int
 * to indicate success or failure.
 */
static int __uart_zynq_putchar(uint32_t base, unsigned char c)
{
	while (read32(base + UART_OFFSET_SR) & UART_SR_TXFULL);
	write8(base + UART_OFFSET_FIFO, c);
}

#ifdef RAW /* baremetal driver */

void uart_init(void)
{
	__uart_zynq_init(UART_BASE);
}

void uart_enable(void)
{
	__uart_zynq_enable(UART_BASE);
}

void uart_disable(void)
{
	__uart_zynq_disable(UART_BASE);
}

unsigned char uart_getchar(void)
{
	return __uart_zynq_getchar(UART_BASE);
}

int uart_putchar(unsigned char c)
{
	__uart_zynq_putchar(UART_BASE, c);
	return 0;
}

#else /* not RAW, or kernel driver */

#if PRIMARY_CONSOLE == uart_zynq

/* Meant to register to kernel, so this interface routine is static */
static int early_console_putchar(unsigned char c)
{
	__uart_zynq_putchar(UART_BASE, c);
	return 0;
}

int early_console_init(void)
{
	__uart_zynq_init(UART_BASE);
	__uart_zynq_enable(UART_BASE);
	set_console(early_console_putchar, DEFAULT_KPUTS);
	return 0;
}

#endif /* PRIMARY_CONSOLE == uart_zynq */

#endif /* RAW */

