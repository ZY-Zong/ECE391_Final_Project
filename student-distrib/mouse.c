//
// Created by qig2 on 12/4/2019.
//
/**
 * This mouse driver is written based on OSdev document
 * See https://wiki.osdev.org/Mouse_Input for more information
 */
#include "mouse.h"
#include "lib.h"
#include "modex.h"
#include "vga/vga.h"
// Local constants
#define MOUSE_PORT_60 0x60
#define PORT_64 0x64
#define ACK 0xFA
#define RESET 0xFF
#define SVGA_WIDTH 1024
#define SVGA_HEIGHT 768
#define BLACK 0xFF000000
#define WHITE 0xFFFFFFFF
/**
 * The first byte of the packet received from mouse is in the following format
 * Y overflow	X overflow	Y sign bit	X sign bit	Always 1	Middle Btn	Right Btn	Left Btn
 * Bit 7            6           5          4           3            2           1          0
 */
#define Y_OVERFLOW 0x080
#define X_OVERFLOW 0x40
#define Y_SIGN 0x20
#define X_SIGN 0x10
#define VERIFY_ONE 0x08
#define MID_BUTTON 0x04
#define RIGHT_BUTTON 0x02
#define LEFT_BUTTON 0x01
#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 8

// Local helper functions
uint8_t read_from_60();
void write_byte_to_port(uint8_t byte, uint8_t port);
void send_command_to_60(uint8_t command);
static int16_t mouse_x = 0;
static int16_t mouse_y = 0;
static unsigned int origin_pixels[CURSOR_WIDTH][CURSOR_HEIGHT];


/**
 *
 * @param command - the command we need to send to the mouse
 */
void send_command_to_60(uint8_t command){
    // Sending a command or data byte to the mouse (to port 0x60) must be preceded by sending a 0xD4 byte to port 0x64.
    write_byte_to_port(0xD4, PORT_64);
    write_byte_to_port(command, MOUSE_PORT_60);
    // Wait for ACK
    while (1) {
        if (ACK == read_from_60()) {
            break;
        }
    }
}
uint8_t read_from_60() {
    // Bytes cannot be read from port 0x60 until bit 0 (value=1) of port 0x64 is set.
    while(1){
        if (1 == (inb(PORT_64) & 0x1)) {
            break;
        }
    }
    return inb(MOUSE_PORT_60);
}
void write_byte_to_port(uint8_t byte, uint8_t port) {
    while(1) {
        // All output to port 0x60 or 0x64 must be preceded by waiting for bit 1 (value=2) of port 0x64 to become clear.
        if (0 == (inb(PORT_64) & 0x2)) {
            break;
        }
    }
    outb(byte, port);
}

void mouse_init() {
    send_command_to_60(RESET);
    // Set Compaq Status/Enable IRQ12
    // Send the command byte 0x20 ("Get Compaq Status Byte") to the PS2 controller on port 0x64
    write_byte_to_port(0x20, PORT_64);
    // Very next byte returned should be the Status byte
    uint8_t compaq_status = read_from_60();
    compaq_status |= 0x2;
    compaq_status &= ~(0x20);
    write_byte_to_port(0x60, PORT_64);
    write_byte_to_port(compaq_status, MOUSE_PORT_60);
    // Aux Input Enable Command
//    write_byte_to_port(0xA8, PORT_64);
//    while (1) {
//        if (ACK == read_from_60()) {
//            break;
//        }
//    }
    // Enable Packet Streaming
    send_command_to_60(0xF4);
    // Initialize the buffer
    int i, j;
    for (i = 0; i < CURSOR_WIDTH; i++) {
        for (j = 0; j < CURSOR_HEIGHT; j++) {
            origin_pixels[i][j] = BLACK;
        }
    }
}
void mouse_interrupt_handler() {
    /**
     * Firstly, we need to check two things:
     * 1. Whether port 60 is ready for read (check bit 1 of port 64)
     * 2. Whether port 60 gives data for keyboard or mouse (check bit 5 of port 64)
     */
    if (0 == (inb(PORT_64) & 0x1)) {
        return;
    }
    if (0 == (inb(PORT_64) & 0x20)) {
        return;
    }
    uint8_t flags = inb(MOUSE_PORT_60);
    if (ACK == flags) {
        return;  // ignore the ACK
    } else {
        int32_t x_movement = read_from_60();
        int32_t y_movement = read_from_60();
        if (!(flags & VERIFY_ONE)) {
            printf("mouse packet not aligned!\n");
            return;
        }
        if ((flags & X_OVERFLOW) || (flags & Y_OVERFLOW)) {
            printf("mouse overflow occurred!\n");
            return;
        }
        // Button press
        if (flags & LEFT_BUTTON) {
            printf("left button pressed\n");
        }
        if (flags & MID_BUTTON) {
            printf("middle button pressed\n");
        }
        if (flags & RIGHT_BUTTON) {
            printf("right button pressed\n");
        }
        // Preserve the original color of pixels at old position of cursor
        int i, j;
        for (i = 0; i < CURSOR_WIDTH; i++) {
            for (j = 0; j < CURSOR_HEIGHT; j++) {
                vga_set_color_argb(origin_pixels[i][j] | 0xFF000000);
                vga_draw_pixel(mouse_x + i, mouse_y + j);
            }
        }
        // Update mouse location
        if (flags & X_SIGN) {
            x_movement |= 0xFFFFFF00;
        }
        if (mouse_x + x_movement < 0) {
            mouse_x = 0;
        } else if (mouse_x + x_movement > SVGA_WIDTH - 1 - CURSOR_WIDTH) {
            mouse_x = SVGA_WIDTH - 1- CURSOR_WIDTH;
        } else {
            mouse_x += x_movement;
        }
        //printf("X movement = %d mouse_x = %d\n", x_movement, mouse_x);
        if (flags & Y_SIGN) {
            y_movement |= 0xFFFFFF00;
        }
        // TODO: For y_movement, should negate the result, don't know why.
        if (mouse_y - y_movement < 0) {
            mouse_y = 0;
        } else if (mouse_y - y_movement > SVGA_HEIGHT - 1 - CURSOR_HEIGHT) {
            mouse_y = SVGA_HEIGHT - 1 - CURSOR_HEIGHT;
        } else {
            mouse_y -= y_movement;
        }
        //printf("Y movement is %d mouse_y = %d\n", y_movement, mouse_y);
        // Record the current pixels covered by the cursor
        for (i = 0; i < CURSOR_WIDTH; i++) {
            for (j = 0; j < CURSOR_HEIGHT; j++) {
                origin_pixels[i][j] = vga_get_pixel(mouse_x + i, mouse_y + j);
            }
        }

        vga_set_color_argb(WHITE);
        for (i = 0; i < CURSOR_WIDTH; i++) {
            for (j = 0; j < CURSOR_HEIGHT; j++) {
                vga_draw_pixel(mouse_x + i, mouse_y + j);
            }
        }
    }
}