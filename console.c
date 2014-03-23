// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "keys.h"

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;


static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    if (c != '%') {
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c) {
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";
      for (; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if (locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  cprintf("cpu%d: panic: ", cpu->id);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory
static int last_pos = -1;
static char mode=0; //esc mode
static short colormask=0x0700;
#define INPUT_BUF 128
#define MAX_HISTORY_LENGTH 20
static char history[MAX_HISTORY_LENGTH][INPUT_BUF+1];
static int last_history_ent = 0;
static int last_seen_hist = 0;
struct {
  struct spinlock lock;
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
  uint l;  //last index
} input;

static void
add_history(int start,int len) {
  if (len==0)
    return;
  if (len+start<=INPUT_BUF)
    safestrcpy(history[last_history_ent],&input.buf[start],len+1);
  else {
    int n1 = INPUT_BUF-start;
    strncpy(history[last_history_ent],&input.buf[start],n1);
    int n2 = len-n1+1;
    safestrcpy(&history[last_history_ent][n1],&input.buf[0],n2);
  }
  last_history_ent++;
  last_history_ent=last_history_ent%MAX_HISTORY_LENGTH;
}

void initvga(void)
{
  outb(CRTPORT, 14);
  last_pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  last_pos |= inb(CRTPORT + 1);
}

static void
setcursor(int delta)
{
  int pos;
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);
  crt[pos] = crt[pos] & 0x07FF;
  pos += delta;
  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = crt[pos] | 0x4F00;
}

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);
  crt[pos] = crt[pos] & 0x07FF;
  if (c == '\n') {
    pos = last_pos + 80 - last_pos % 80;
    last_pos += 80 - last_pos % 80;
  }  

  else if (c == BACKSPACE) {
    if (pos > 0) {
      if (pos == last_pos) {
        --pos;
        --last_pos;
        crt[pos] = 0x4F20;
      }
    else {
      int i=--pos;
      for (;i<last_pos;i++) 
        crt[i]=crt[i+1];
      last_pos--;
      }
    }
  }
  //"normal" chars
  else {                               
    if (pos == last_pos) {
      crt[pos++] = (c & 0xff) | colormask; // black on white
      last_pos++;
    } else {
      int i = last_pos++;
      for (; i > pos; i--) {
        crt[i] = crt[i - 1];    //make place in buffer to enter new 'c'
      }
      crt[pos++] = (c & 0xff) | colormask;
    }
  }

  if ((last_pos / 80) >= 27) { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 26 * 80);
    last_pos -= 80;
    pos -= 80;
    memset(crt + last_pos, 0, sizeof(crt[0]) * (27 * 80 - last_pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = crt[pos] | 0x4F00;
}

void
consputc(int c) {
    if (panicked) {
        cli();
        for (;;)
            ;
    }
    if (mode) {
        switch (c) {
        case 'A':
            colormask = 0x4F00; //red background
            uartsetcolor('A');
            break;
        case 'B':
            colormask = 0x0A00; //green high
            uartsetcolor('B');
            break;
        case 'C':
            colormask = 0x0200; //green low
            uartsetcolor('C');
            break;
        case 'D':
            colormask = 0x0500;
            uartsetcolor('D');
            break;
        case 'E':
            colormask = 0x0E00;
            uartsetcolor('E');
            break;
        default:
            colormask = 0x0700; //no color
            uartsetcolor('R');
            break;
        }
        mode = 0;
    }
    else {
      if (c==0x1b) {
      mode=1;
      return;
    }
        uartputc(c);
        cgaputc(c);
    }
}

static void
move_to_end_line(void) {
  int i = input.l-input.e;
  for (;i>0;i--) {
    setcursor(1);
    uartputc(KEY_RT);
  }
  input.e=input.l;
}

static void
move_to_begin_line(void) {
  int i = input.e-input.r;
  for (;i>0;i--) {
    setcursor(-1);
    uartputc(KEY_LF);
  }
  input.e=input.r;
}


static int
getHistory(int index) {
  if (history[index][0]) {
    int i = input.l-input.r;
    for (;i>0;i--)
      consputc(BACKSPACE);
    input.e=input.r;
    i=0;
    while (history[index][i]) {
      input.buf[input.e++ % INPUT_BUF]=history[index][i];
      consputc(history[index][i++]);
    }
    input.l=input.e;
    return 1;
  }
  return 0;
}
void
consoleintr(int (*getc)(void))
{
  int c,index;
  acquire(&input.lock);
  while ((c = getc()) >= 0) {
    switch (c) {
      case C('P'):  // Process listing.
        procdump();
        break;
      case C('U'):  // Kill line.
        move_to_end_line();
        while (input.l != input.w &&
               input.buf[(input.l - 1) % INPUT_BUF] != '\n') {
          input.l--;
          consputc(BACKSPACE);
        }
        input.e = input.l;
        break;
      case C('H'):
      case '\x7f':  // Backspace
        if (input.e != input.w) {
          if (input.l == input.e) { //cursor is at end
            input.e--;
            input.l--;
            consputc(BACKSPACE);
          } else {                  //cursor is in middle of line
            int i = --input.e;
            for (; i < input.l; i++)
              input.buf[i % INPUT_BUF] = input.buf[(i + 1) % INPUT_BUF];
            consputc(BACKSPACE);
            input.l--;
          }
        }
        break;
      case KEY_LF:
        if (input.e != input.w) {
          input.e--;
          uartputc(c);
          setcursor(-1);
        }
        break;
      case KEY_RT:
        if (input.e != input.l) {
          input.e++;
          uartputc(c);
          setcursor(1);
        }
        break;
      case KEY_UP:
        last_seen_hist--;
        index = (last_history_ent+last_seen_hist)%MAX_HISTORY_LENGTH;
        index = (index>=0) ? index : MAX_HISTORY_LENGTH+index;
        move_to_end_line();
        if (getHistory(index))
          ;
        else
          last_seen_hist++;
      break;
      case KEY_DN:
        last_seen_hist++;
        index = (last_history_ent+last_seen_hist)%MAX_HISTORY_LENGTH;
        index = (index>=0) ? index : MAX_HISTORY_LENGTH+index; 
        move_to_end_line();
        if (getHistory(index))
          ;
        else
          last_seen_hist--;
      break;
      case C('A'):
        move_to_begin_line();
        break;
      case C('E'):
        move_to_end_line();
        break;
      default:
        if (c != 0 && input.e-input.r < INPUT_BUF) {
          c = (c == '\r') ? '\n' : c;

          if (input.l == input.e || c == '\n') {        //cursor is at end or new line
            input.buf[input.l++ % INPUT_BUF] = c;
            input.e++;
          } else {
            int i = input.l++;
            for (; i > input.e; i--) {
              input.buf[i % INPUT_BUF] = input.buf[(i - 1) % INPUT_BUF]; //make place in buffer to enter new 'c'
            }
            input.buf[input.e++ % INPUT_BUF] = c;
          }
          consputc(c);
          if (c == '\n' || c == C('D') || input.l == input.r + INPUT_BUF) {
            last_seen_hist=0;
            add_history(input.r,input.l-input.r-1);
            input.w = input.l;
            input.e = input.l;
            wakeup(&input.r);
          }
        }
        break;
      }
  }
  release(&input.lock);
}


int
consoleread(struct inode * ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&input.lock);
  while (n > 0) {
    while (input.r == input.w) {
      if (proc->killed) {
        release(&input.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &input.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D')) { // EOF
      if (n < target) {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&input.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode * ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");
  initlock(&input.lock, "input");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  picenable(IRQ_KBD);
  ioapicenable(IRQ_KBD, 0);
}

