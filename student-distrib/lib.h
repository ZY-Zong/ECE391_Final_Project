/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#include "types.h"

// External variables that will be changed when switching terminals
extern int screen_x;
extern int screen_y;

#define VIDEO       0xA0000

/**
 * putc() may directly output to physical video memory when focus terminal is NULL_TERMINAL_ID.
 * But text must not write to SVGA video memory, or cached image in invisible part may get damaged.
 * Also, it can't write to original 0xB8 since it's the MMIO address of Cirrus 5446.
 *
 * To change this, remember to change page table in x86_desc.S and constants in vidmem.c.
 */
#define VIDEO_TEXT  0xBF000

#define TERMINAL_WIDTH_PIXEL     640
#define TERMINAL_HEIGHT_PIXEL    480

#define ATTRIB      0x7
#define FONT_WIDTH  8
#define FONT_HEIGHT 16

#define TERMINAL_TEXT_COLS    (TERMINAL_WIDTH_PIXEL / FONT_WIDTH)
#define TERMINAL_TEXT_ROWS    (TERMINAL_HEIGHT_PIXEL / FONT_HEIGHT)

/** System Calls within Kernel */

int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
//int32_t vidmap(uint8_t** screen_start);
//int32_t set_handler(int32_t signum, void* handler_address);
//int32_t sigreturn (void);

/** Debug Helper function */
// Use printf() if things to print are normal. Only use the following macros when info are optional.
#define DEBUG   1
#define DEBUG_VERBOSE   0
#if DEBUG
#if DEBUG_VERBOSE
#define DEBUG_PRINT(fmt, ...)    do { printf("%s:%d:%s(): " fmt "\n", \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define DEBUG_ERR(fmt, ...)      do { printf("[ERROR]" "%s:%d:%s(): " fmt "\n", \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define DEBUG_WARN(fmt, ...)     do { printf("[WARNING]" "%s:%d:%s(): " fmt "\n", \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#else
#define DEBUG_PRINT(fmt, ...)    do { printf(fmt "\n", ##__VA_ARGS__); } while (0)
#define DEBUG_ERR(fmt, ...)      do { printf("[ERROR] " fmt "\n", ##__VA_ARGS__); } while (0)
#define DEBUG_WARN(fmt, ...)     do { printf("[WARNING] " fmt "\n", ##__VA_ARGS__); } while (0)
#endif
#else
#define DEBUG_PRINT(fmt, ...)    do {} while (0)
#define DEBUG_ERR(fmt, ...)      do {} while (0)
#define DEBUG_WARN(fmt, ...)     do {} while (0)
#endif

int32_t printf(int8_t *format, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t *itoa(uint32_t value, int8_t* buf, int32_t radix);
int8_t *strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);
void reset_cursor();
void scroll_up();

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t*src);
int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
    : "=a"(val)
    : "d"(port)
    : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
    : "=a"(val)
    : "d"(port)
    : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
    : "=a"(val)
    : "d"(port)
    : "memory"
    );
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %l1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

#endif /* _LIB_H */
