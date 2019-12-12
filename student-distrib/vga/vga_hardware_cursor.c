//
// Created by gq290 on 12/11/2019.
//

#include "vga_hardware_cursor.h"
#include "../lib.h"
#include "vga_draw.h"
unsigned char _64x64_planes_0_1[1024];
unsigned char _32x32_planes_0_1[256];
char _32x32_hw_cursor[1024] =
        {
    //01234567890123456789012345678901
    "00000000000000000000000000000000"
    "20000000000000000000000000000000"
    "22000000000000000000000000000000"
    "22200000000000000000000000000000"
    "22220000000000000000000000000000"
    "22122000000000000000000000000000"
    "22112200000000000000000000000000"
    "22111220000000000000000000000000"
    "22111122000000000000000000000000"
    "22111112200000000000000000000000"
    "22111111220000000000000000000000"
    "22111111122000000000000000000000"
    "22111111112200000000000000000000"
    "22111111111220000000000000000000"
    "22111111111122000000000000000000"
    "22111111111112200000000000000000" // 16
    "22111111111111220000000000000000"
    "22111111111111122000000000000000"
    "22111111111111112200000000000000"
    "22111111111111111220000000000000"
    "22111111112200000000000000000000"
    "22111111112200000000000000000000"
    "22111111111220000000000000000000"
    "22111222111220000000000000000000"
    "22112200221122000000000000000000"
    "22112200221122000000000000000000"
    "22122000022112200000000000000000"
    "21120000022112200000000000000000"
    "22200000002211220000000000000000"
    "22000000002222000000000000000000"
    "20000000002200000000000000000000"
    "00000000000000000000000000000000" //32
        };
//"22222222222222222222222222222222"
//"22222222222222222222222222222222"
//"22000000000000000000000000000022"
//"22000000000000000000000000000022"
//"22003333333333333333333333330022"
//"22003333333333333333333333330022"
//"22003300000000000000000000330022"
//"22003300000000000000000000330022"
//"22003300222222222222222200330022"
//"22003300222222222222222200330022"
//"22003300220000000000002200330022"
//"22003300220000000000002200330022"
//"22003300220033333333002200330022"
//"22003300220033333333002200330022"
//"22003300220033111133002200330022"
//"22003300220033111133002200330022"
//"22003300220033111133002200330022"
//"22003300220033111133002200330022"
//"22003300220033333333002200330022"
//"22003300220033333333002200330022"
//"22003300220000000000002200330022"
//"22003300220000000000002200330022"
//"22003300222222222222222200330022"
//"22003300222222222222222200330022"
//"22003300000000000000000000330022"
//"22003300000000000000000000330022"
//"22003333333333333333333333330022"
//"22003333333333333333333333330022"
//"22000000000000000000000000000022"
//"22000000000000000000000000000022"
//"22222222222222222222222222222222"
//"22222222222222222222222222222222"
void outportb(port, data) {
    outb(data, port);
}
uint8_t inportb (port) {
    return inb(port);
}
void outport(port, data) {
    outw(data, port);
}
// **************************************************************************
// INITIALIZE SINGLE PAGE 16K GRANULARITY MODE.
// **************************************************************************
void SP_16K_Init( void )
{
    unsigned char grb;
    outportb( 0x3ce, 0x0b );
    grb = inportb( 0x3cf );
    grb |= 0x20;
    grb &= ~0x01;
    outportb( 0x3cf, grb );
};

// SP_16K_Init()
// GRB[5] <-- 1
// GRB[0] <-- 0
// **************************************************************************
// SET GR9 OFFSET REGISTER IN SINGLE PAGE 16K GRANULARITY MODE
// **************************************************************************
unsigned int SP_16K_Set_Offset_Reg( unsigned long address )
{
    unsigned char gr9_value;
    unsigned int ret_val;
    ret_val = (unsigned int) (address & 0x0000ffffUL);
    address &= 0xffff0000UL;
    address >>= 16;
    gr9_value = (unsigned char) address;
    gr9_value <<= 2;
    outportb( 0x3ce, 0x09 );
    outportb( 0x3cf, gr9_value );
    return ret_val;
};

// SP_16K_Set()
// **************************************************************************
// INITIALIZE CURSOR PLANES 0 AND 1.
// **************************************************************************
void Cursor_Plane_Init( char* map, unsigned char* plane, int number_of_bytes )
{
    unsigned char mask;
    int bit, byte;
    int i = 0;
    unsigned char* plane_0 = &plane[0];
    unsigned char* plane_1 = &plane[number_of_bytes];
    for ( byte = 0; byte < number_of_bytes; byte++ )
    {
        plane_0[byte] = 0;
        plane_1[byte] = 0;
        mask = 0x80;
        for ( bit = 0; bit < 8; bit++, mask >>= 1, i++ )
        {
            if
                    ( map[i] == '1' )
            {
                plane_0[byte] |= mask;
            }
            else if ( map[i] == '2' )
            {
                plane_1[byte] |= mask;
            }
            else if ( map[i] == '3' )
            {
                plane_0[byte] |= mask;
                plane_1[byte] |= mask;
            }
        }
    };
}
// Cursor_Plane_Init()
// **************************************************************************
// SET X, Y COORDINATE OF HARDWARE CURSOR.
// **************************************************************************
void Cursor_Set_XY( int x, int y )
{
    x <<= 5;
    x |= 0x0010;
    outport( 0x3c4, x );
    y <<= 5;
    y |= 0x0011;
    outport( 0x3c4, y );
};

// Cursor_Set_XY()
// **************************************************************************
// SET EXTENDED DAC COLORS FOR HARDWARE CURSOR.
// **************************************************************************
void Cursor_Set_Color( unsigned long background, unsigned long foreground )
{
    unsigned char sr12;
    unsigned char b_red, b_green, b_blue;
    unsigned char f_red, f_green, f_blue;
    b_red = (unsigned char) (background & 0x000000ffUL);
    b_green = (unsigned char) ((background & 0x0000ff00UL) >> 8);
    b_blue = (unsigned char) ((background & 0x00ff0000UL)  >> 16);
    f_red = (unsigned char)(foreground & 0x000000ffUL);
    f_green = (unsigned char)((foreground & 0x0000ff00UL)>> 8);
    f_blue = (unsigned char)((foreground & 0x00ff0000UL)>> 16);

// SR12[1] : ENABLE ACCESS TO DAC
    outportb( 0x3c4, 0x12 );
    sr12 = inportb( 0x3c5 ) | 0x82;
    outportb( 0x3c5, sr12 );

    outportb(0x3c8,0x00 );
    outportb(0x3c9,b_red );
    outportb(0x3c9,b_green );
    outportb(0x3c9,b_blue );

// WRITE FOREGROUND
    outportb( 0x3c8, 0x0f );
    outportb( 0x3c9, f_red );
    outportb( 0x3c9, f_green );
    outportb( 0x3c9, f_blue );
    // SR12[1] : DISABLE ACCESS TO DAC EXTENDED COLORS
    outportb( 0x3c4, 0x12 );
// SR12
    sr12 = inportb( 0x3c5 ) & ~0x82; // READ AND SET SR12[1]
    outportb( 0x3c5, sr12 );
}

// WRITE NEW VALUE
// Cursor_Set_Color()
// **************************************************************************
// ENABLE 64 X 64 HARDWARE CURSOR.
// **************************************************************************

// _64x64_Patterns_Load()
// **************************************************************************
// ENABLE 32 X 32 HARDWARE CURSOR.
// **************************************************************************
void _32x32_Cursor_Enable( void )
{
    unsigned char sr12;
    outportb( 0x3c4, 0x12 );
    sr12 = inportb( 0x3c5 ) | 0x01;
    sr12 &= ~0x04;
    outportb( 0x3c5, sr12 );
};
// _32x32_Cursor_Enable()
// **************************************************************************
// DISABLE 32 X 32 HARDWARE CURSOR.
// **************************************************************************
void _32x32_Cursor_Disable( void )
{
    unsigned char sr12;
    outportb( 0x3c4, 0x12 );
    sr12 = inportb( 0x3c5 );
    sr12 &= ~0x01;
    outportb( 0x3c5, sr12 );
};
// _32x32_Cursor_Disable()
// **************************************************************************
// LOAD 32X32 HARDWARE CURSOR PATTERN 0 IN 8 BPP MODE.
// LOAD 8 BYTES FROM PLANE 0 TO TO DISPLAY MEMORY,
// FOLLOWED BY 8 BYTES FROM PLANE 1 UNTIL THE PATTERN IS COPIED.
// **************************************************************************
void _32x32_Patterns_Load( unsigned char * pattern )
{
//unsigned int di, l;
//unsigned long pattern_address = 0x200000UL - 0x4000UL; // 2 MB DISPLAY MEM
//unsigned char far* mem_ptr
//= (unsigned char far*) MK_FP( 0xa000, 0 );
//unsigned char far* plane_ptr = pattern;
//di = SP_16K_Set_Offset_Reg( pattern_address );
//for ( l = 0; l < 256; l++ ) mem_ptr[di++] = *plane_ptr++;
    int i;
    for (i = 0; i < 256; i++) {
        vga_set_byte(4177920 + i, pattern[i]);
    }
}
// _32x32_Patterns_Load()
// **************************************************************************
// SET PREVIOUSLY LOADED PATTERN FOR 32 X 32 HARDWARE CURSOR.

// **************************************************************************
void _32x32_Cursor_Set_Pattern( int pattern_number )
{
    unsigned char sr13 = (unsigned char) pattern_number;
    outportb( 0x3c4, 0x13 );
    outportb( 0x3c5, sr13 );
};

// _32x32_Cursor_Set_Pattern()
// **************************************************************************
// INITIALIZE, LOAD, AND DISPLAY 32 X 32 AND 64 X 64 HARDWARE CURSORS.
// CHANGE COLORS AND LOCATION.
// **************************************************************************
void hardware_cursor_init()
{
// SET MODE 5F -- 640X480X8 GRAPHICS MODE
    //Cursor_Plane_Init( _64x64_hw_cursor, _64x64_planes_0_1, 512 );
    Cursor_Plane_Init( _32x32_hw_cursor, _32x32_planes_0_1, 128 );
    SP_16K_Init();
// INITIALIZE GRAPHICS MEMORY MODE
// INITIALIZE AND DISPLAY 64 X 64 HARDWARE CURSOR
//    _64x64_Patterns_Load( _64x64_planes_0_1 );
//    Cursor_Set_XY( 0, 0 );
//    _64x64_Cursor_Enable();
// CHANGE COLOR OF 64 X 64 HARDWARE CURSOR
//    Cursor_Set_Color( 0x0000003fUL, 0x00003f00UL );
//    Cursor_Set_Color( 0x00003f00UL, 0x003f0000UL );
//    Cursor_Set_Color( 0x0000003fUL, 0x003f0000UL );
// INITIALIZE AND DISPLAY 32 X 32 HARDWARE CURSOR
    _32x32_Patterns_Load( _32x32_planes_0_1 );
    _32x32_Cursor_Enable();
// CHANGE LOCATION OF 32 X 32 HARDWARE CURSOR
};
