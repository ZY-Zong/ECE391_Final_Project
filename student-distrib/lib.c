/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "modex.h"
#include "vga/vga.h"
#include "gui/gui.h"
// Constants for different sizes of screen
#define MODE_SVGA
// #define MODE_X

#ifdef MODE_SVGA
#define CUR_TERMINAL_WIDTH 640
#define CUR_TERMINAL_HEIGHT 480
#endif

#ifdef MODE_X
#define CUR_TERMINAL_WIDTH 640
#define CUR_TERMINAL_HEIGHT 480
#endif


#define MAX_TERMINAL_WIDTH 640
#define MAX_TERMINAL_HEIGHT 480
#define SVGA_WIDTH 1024
#define SVGA_HEIGHT 768
#define ATTRIB      0x7
#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#define NUM_COLS    (CUR_TERMINAL_WIDTH / FONT_WIDTH)
#define NUM_ROWS    (CUR_TERMINAL_HEIGHT / FONT_HEIGHT)
#define MAX_COLS    (MAX_TERMINAL_WIDTH / FONT_WIDTH)
#define MAX_ROWS    (MAX_TERMINAL_HEIGHT / FONT_HEIGHT)
#define BLACK 0xFF000000
#define WHITE 0xFFFFFFFF

/*
 * macro used to target a specific video plane or planes when writing
 * to video memory in mode X; bits 8-11 in the mask_hi_bits enable writes
 * to planes 0-3, respectively
 */
#define SET_WRITE_MASK(mask_hi_bits)                                    \
do {                                                                    \
    asm volatile ("                                                     \
	movw $0x03C4,%%dx    	/* set write mask                    */;\
	movb $0x02,%b0                                                 ;\
	outw %w0,(%%dx)                                                 \
    " : : "a" ((mask_hi_bits)) : "edx", "memory");                      \
} while (0)

int screen_x;
int screen_y;
static char *video_mem = (char *) VIDEO;
// Always keep the screen_char buffer its largest size (takes 2.4kB)
static uint8_t screen_char[MAX_COLS * MAX_ROWS];

/**
 * Reset input point to the upper left corner of the screen
 * @input: None
 * @output: None
 * @return: None
 */
void reset_cursor() {
    screen_x = 0;
    screen_y = 0;
}

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
#ifdef MODE_X
void clear(void) {
    int i, j;
    for (i = 0; i < 4; i++) {  // Loop over four planes
        SET_WRITE_MASK(1 << (8 + i));
        for (j = 0; j < IMAGE_X_WIDTH * IMAGE_Y_DIM; j++) {
            *(uint8_t *)(video_mem + j) = OFF_PIXEL;
        }
    }
    for (i = 0; i < MAX_COLS; i++) {
        for (j = 0; j < MAX_ROWS; j++) {
            screen_char[j * MAX_COLS + i] = 0;
        }
    }
}
#endif

#ifdef MODE_SVGA

void clear(void) {
    int i, j;
    vga_set_color_argb(BLACK);
    for (i = 0; i < CUR_TERMINAL_WIDTH; i++) {
        for (j = 0; j < CUR_TERMINAL_HEIGHT; j++) {
            vga_draw_pixel(i, j);
        }
    }
    // TODO: Preserve the last line
}

#endif

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t *buf = format;

    /* Stack pointer for the other parameters */
    int32_t *esp = (void *) &format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%': {
                int32_t alternate = 0;
                buf++;

                format_char_switch:
                /* Conversion specifiers */
                switch (*buf) {
                    /* Print a literal '%' character */
                    case '%':
                        putc('%');
                        break;

                        /* Use alternate formatting */
                    case '#':
                        alternate = 1;
                        buf++;
                        /* Yes, I know gotos are bad.  This is the
                         * most elegant and general way to do this,
                         * IMHO. */
                        goto format_char_switch;

                        /* Print a number in hexadecimal form */
                    case 'x': {
                        int8_t conv_buf[64];
                        if (alternate == 0) {
                            itoa(*((uint32_t *) esp), conv_buf, 16);
                            puts(conv_buf);
                        } else {
                            int32_t starting_index;
                            int32_t i;
                            itoa(*((uint32_t *) esp), &conv_buf[8], 16);
                            i = starting_index = strlen(&conv_buf[8]);
                            while (i < 8) {
                                conv_buf[i] = '0';
                                i++;
                            }
                            puts(&conv_buf[starting_index]);
                        }
                        esp++;
                    }
                        break;

                        /* Print a number in unsigned int form */
                    case 'u': {
                        int8_t conv_buf[36];
                        itoa(*((uint32_t *) esp), conv_buf, 10);
                        puts(conv_buf);
                        esp++;
                    }
                        break;

                        /* Print a number in signed int form */
                    case 'd': {
                        int8_t conv_buf[36];
                        int32_t value = *((int32_t *) esp);
                        if (value < 0) {
                            conv_buf[0] = '-';
                            itoa(-value, &conv_buf[1], 10);
                        } else {
                            itoa(value, conv_buf, 10);
                        }
                        puts(conv_buf);
                        esp++;
                    }
                        break;

                        /* Print a single character */
                    case 'c':
                        putc((uint8_t) *((int32_t *) esp));
                        esp++;
                        break;

                        /* Print a NULL-terminated string */
                    case 's':
                        puts(*((int8_t **) esp));
                        esp++;
                        break;

                    default:
                        break;
                }

            }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t *s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
#ifdef MODE_X
void putc(uint8_t c) {
    if(c == '\n' || c == '\r') {
        if (screen_x < NUM_COLS - 1) {
            int i;
            for (i = screen_x; i < NUM_COLS; i++) {
                screen_char[screen_y * MAX_COLS + i] = 0;
            }
        }
        screen_x = 0;
        screen_y++;
        if (NUM_ROWS == screen_y) {
            scroll_up();
        }
    } else if ('\b' == c) {
        // If user types backspace
        if (0 == screen_x) {
            if (0 == screen_y) { // At the top left corner of the screen
                return;
            } else { // Originally at the start of a new line, now at the end of last line
                screen_x = NUM_COLS - 1;
                screen_y--;
            }
        } else { // Normal cases for backspace
            screen_x--;
        }
        int i, j;
        for (i = 0; i < 4; i++) {  // Loop over four planes
            SET_WRITE_MASK(1 << (8 + i));
            for (j = 0; j < FONT_HEIGHT; j++) {
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (screen_y * FONT_HEIGHT + j) + screen_x * 2))) = font_data[' '][j] & (1 << (7 - i))? ON_PIXEL : OFF_PIXEL;
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (screen_y * FONT_HEIGHT + j) + screen_x * 2 + 1))) = font_data[' '][j] & (1 << (3 - i))? ON_PIXEL : OFF_PIXEL;
            }
        }
        screen_char[screen_y * MAX_COLS + screen_x] = 0;

        // Don't increase screen_x since next time we need to start from the same location for a new character
    } else {
        // Normal cases for a character
        int i, j;
        for (i = 0; i < 4; i++) {  // Loop over four planes
            SET_WRITE_MASK(1 << (8 + i));
            for (j = 0; j < FONT_HEIGHT; j++) {
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (screen_y * FONT_HEIGHT + j) + screen_x * 2))) = font_data[c][j] & (1 << (7 - i))? ON_PIXEL : OFF_PIXEL;
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (screen_y * FONT_HEIGHT + j) + screen_x * 2 + 1))) = font_data[c][j] & (1 << (3 - i))? ON_PIXEL : OFF_PIXEL;
            }
        }
        screen_char[screen_y * MAX_COLS + screen_x] = c;
        screen_x++;
        if (NUM_COLS == screen_x) {
            // We need a new line
            screen_x %= NUM_COLS;
            screen_y++;
            if (NUM_ROWS == screen_y) {
                scroll_up();
            }
        }
    }
}
#endif

#ifdef MODE_SVGA

void putc(uint8_t c) {
    if (c == '\n' || c == '\r') {
        if (screen_x < NUM_COLS - 1) {
            int i;
            for (i = screen_x; i < NUM_COLS; i++) {
                screen_char[screen_y * MAX_COLS + i] = 0;
            }
        }
        screen_x = 0;
        screen_y++;
        if (NUM_ROWS == screen_y) {
            scroll_up();
        }
    } else if ('\b' == c) {
        // If user types backspace
        if (0 == screen_x) {
            if (0 == screen_y) { // At the top left corner of the screen
                return;
            } else { // Originally at the start of a new line, now at the end of last line
                screen_x = NUM_COLS - 1;
                screen_y--;
            }
        } else { // Normal cases for backspace
            screen_x--;
        }

        vga_print_char(screen_x * FONT_WIDTH, screen_y * FONT_HEIGHT, ' ', WHITE, BLACK);
//        gui_print_char(' ', screen_x * FONT_WIDTH, screen_y * FONT_HEIGHT);

        screen_char[screen_y * MAX_COLS + screen_x] = 0;

        // Don't increase screen_x since next time we need to start from the same location for a new character
    } else {
        // Normal cases for a character

        vga_print_char(screen_x * FONT_WIDTH, screen_y * FONT_HEIGHT, c, WHITE, BLACK);
//        gui_print_char(c, screen_x * FONT_WIDTH, screen_y * FONT_HEIGHT);

        screen_char[screen_y * MAX_COLS + screen_x] = c;
        screen_x++;
        if (NUM_COLS == screen_x) {
            // We need a new line
            screen_x %= NUM_COLS;
            screen_y++;
            if (NUM_ROWS == screen_y) {
                scroll_up();
            }
        }
    }
}

#endif
/**
 * scroll_up
 * This function is called whenever the cursor moves to NUM_ROWS row (which should not happen).
 * Then we move the screen one line up so that we can continuously type words.
 * Side Effect: Discard the top most line of the screen.
 */
#ifdef MODE_X
void scroll_up() {
    int x,y;
    int i, j;
    screen_x = 0;
    screen_y = 0;
    for (y = 1; y < NUM_ROWS; y++) {
        for (x = 0; x < NUM_COLS; x++) {
            for (i = 0; i < 4; i++) {  // Loop over four planes
                SET_WRITE_MASK(1 << (8 + i));
                for (j = 0; j < FONT_HEIGHT; j++) {
                    *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * ((y - 1) * FONT_HEIGHT + j) + x * 2))) = font_data[screen_char[y * MAX_COLS + x]][j] & (1 << (7 - i))? ON_PIXEL : OFF_PIXEL;
                    *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * ((y - 1) * FONT_HEIGHT + j) + x * 2 + 1))) = font_data[screen_char[y * MAX_COLS + x]][j] & (1 << (3 - i))? ON_PIXEL : OFF_PIXEL;
                }
            }
            screen_char[(y - 1) * MAX_COLS + x] = screen_char[y * MAX_COLS + x];
        }
    }
    // Clean up the last line
    y = NUM_ROWS - 1;
    for (x = 0; x < NUM_COLS; x++) {
        for (i = 0; i < 4; i++) {  // Loop over four planes
            SET_WRITE_MASK(1 << (8 + i));
            for (j = 0; j < FONT_HEIGHT; j++) {
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (y * FONT_HEIGHT + j) + x * 2))) = font_data[' '][j] & (1 << (7 - i))? ON_PIXEL : OFF_PIXEL;
                *(uint8_t *)(video_mem + ((IMAGE_X_WIDTH * (y * FONT_HEIGHT + j) + x * 2 + 1))) = font_data[' '][j] & (1 << (3 - i))? ON_PIXEL : OFF_PIXEL;
            }
        }

    }
    // Reset the cursor to the column 0, row (NUM_ROWS - 1)
    screen_y = NUM_ROWS - 1;
    screen_x = 0;
}
#endif

#ifdef MODE_SVGA

void scroll_up() {
    int x, y;

//    for (y = 1; y < NUM_ROWS; y++) {
//        for (x = 0; x < NUM_COLS; x++) {
//            screen_char[(y - 1) * MAX_COLS + x] = screen_char[y * MAX_COLS + x];
//        }
//    }
    vga_screen_copy(0, FONT_HEIGHT, 0, 0, CUR_TERMINAL_WIDTH, CUR_TERMINAL_WIDTH - FONT_HEIGHT);
//    vga_print_char_array(0, 0, (char *) screen_char, NUM_ROWS - 1, NUM_COLS, WHITE, BLACK);

    // Clean up the last line
    y = NUM_ROWS - 1;
    for (x = 0; x < NUM_COLS; x++) {
        vga_print_char(x * FONT_WIDTH, y * FONT_HEIGHT, ' ', WHITE, BLACK);
//        gui_print_char(' ', x * FONT_WIDTH, y * FONT_HEIGHT);
        screen_char[y * MAX_COLS + x] = 0x0;
    }
    // Reset the cursor to the column 0, row (NUM_ROWS - 1)
    screen_y = NUM_ROWS - 1;
    screen_x = 0;
}

#endif

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t *itoa(uint32_t value, int8_t *buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t *strrev(int8_t *s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t *s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void *memset(void *s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
    :
    : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
    : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void *memset_word(void *s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
    :
    : "a"(c), "D"(s), "c"(n)
    : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void *memset_dword(void *s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
    :
    : "a"(c), "D"(s), "c"(n)
    : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void *memcpy(void *dest, const void *src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
    :
    : "S"(src), "D"(dest), "c"(n)
    : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void *memmove(void *dest, const void *src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
    :
    : "D"(dest), "S"(src), "c"(n)
    : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t *s1, const int8_t *s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t *strcpy(int8_t *dest, const int8_t *src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t *strncpy(int8_t *dest, const int8_t *src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

/**
 * C linkage to halt() system call
 * @param status    Exit code of current process
 * @return This function should never return
 */
int32_t halt(uint8_t status) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x01), "b" (status)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to execute() system call
 * @param command    Command to be executed
 * @return Terminate status of the program (0-255 if program terminate by calling halt(), 256 if exception occurs)
 * @note New program given in command will run immediately, and this function will return after its terminate
 */
int32_t execute(const uint8_t *command) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x02), "b" (command)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to read() system call
 * @param fd        File descriptor
 * @param buf       Buffer to store output
 * @param nbytes    Maximal number of bytes to write
 * @return 0 on success, -1 on failure
 */
int32_t read(int32_t fd, void *buf, int32_t nbytes) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x03), "b" (fd), "c" (buf), "d" (nbytes)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to write() system call
 * @param fd        File descriptor
 * @param buf       Buffer of content to write
 * @param nbytes    Number of bytes to write
 * @return 0 on success, -1 on failure
 */
int32_t write(int32_t fd, const void *buf, int32_t nbytes) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x04), "b" (fd), "c" (buf), "d" (nbytes)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to open() system call
 * @param filename    String of filename to open
 * @return 0 on success, -1 on failure
 */
int32_t open(const uint8_t *filename) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x05), "b" (filename)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to close() system call
 * @param fd    File descriptor
 * @return 0 on success, -1 on failure
 */
int32_t close(int32_t fd) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x06), "b" (fd)
    : "memory", "cc");
    return ret;
}

/**
 * C linkage to getargs() system call
 * @param buf       String buffer to accept args
 * @param nbytes    Maximal number of bytes to write to buf
 * @return 0 on success, -1 on no argument or argument string can't fit in nbytes
 */
int32_t getargs(uint8_t *buf, int32_t nbytes) {
    long ret;
    asm volatile ("INT $0x80"
    : "=a" (ret)
    : "a" (0x07), "b" (buf), "c" (nbytes)
    : "memory", "cc");
    return ret;
}
