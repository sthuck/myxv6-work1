// Intel 8250 serial port (UART).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "keys.h"
#define COM1    0x3f8

static int uart;    // is there a uart?

void
uartinit(void)
{
  char *p;

  // Turn off the FIFO
  outb(COM1+2, 0);
  
  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM1+3, 0x80);    // Unlock divisor
  outb(COM1+0, 115200/9600);
  outb(COM1+1, 0);
  outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
  outb(COM1+4, 0);
  outb(COM1+1, 0x01);    // Enable receive interrupts.

  // If status is 0xFF, no serial port.
  if(inb(COM1+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM1+2);
  inb(COM1+0);
  picenable(IRQ_COM1);
  ioapicenable(IRQ_COM1, 0);
  
  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}

void
uartputc(int c)
{
  int i;

  if(!uart)
    return;
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
    microdelay(10);
  if (c==KEY_LF) {
    outb(COM1+0, 27);
    outb(COM1+0, 91);
    outb(COM1+0, 68);
    }
  else if (c==KEY_RT) {
    outb(COM1+0, 27);
    outb(COM1+0, 91);
    outb(COM1+0, 67);
  }
  else if (c==0x100) {
    outb(COM1+0, '\b');
    outb(COM1+0, 27);
    outb(COM1+0, 91);
    outb(COM1+0, 80);
  }
  else {
      outb(COM1+0, c);
    }
  
}

void
uartsetcolor(char c) {
  switch (c) {
    case 'A':
      uartputc(27);
      uartputc('[');
      uartputc('3');
      uartputc('7');
      uartputc(';');
      uartputc('4');
      uartputc('1');
      uartputc('m');
    break;
    case 'B':
      uartputc(27);
      uartputc('[');
      uartputc('3');
      uartputc('2');
      uartputc('m');
    break;
    case 'C':
      uartputc(27);
      uartputc('[');
      uartputc('3');
      uartputc('2');
      uartputc('m');
    break;
    case 'D':
      uartputc(27);
      uartputc('[');
      uartputc('3');
      uartputc('5');
      uartputc('m');
    break;
    case 'E':
      uartputc(27);
      uartputc('[');
      uartputc('4');
      uartputc('3');
      uartputc('m');
    break;
    default:
      uartputc(27);
      uartputc('[');
      uartputc('0');
      uartputc('m');
    break;
  }
}

static int
uartgetc(void)
{
  if(!uart)
    return -1;
  if(!(inb(COM1+5) & 0x01))
    return -1;
  return inb(COM1+0);
}

void
uartintr(void)
{
  consoleintr(uartgetc);
}
