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
// Local constants
#define MOUSE_PORT_60 0x60
#define PORT_64 0x64
#define ACK 0xFA
#define RESET 0xFF
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

// Local helper functions
uint8_t read_from_60();
void write_byte_to_port(uint8_t byte, uint8_t port);
void send_command_to_60(uint8_t command);
static int16_t mouse_x = 0;
static int16_t mouse_y = 0;



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
}
void mouse_interrupt_handler() {
    // printf("test mouse driver");
    // TODO: why use try_read?
    uint8_t flags = read_from_60();
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
        if (flags & LEFT_BUTTON) {
            printf("left button pressed\n");
        }
        if (flags & MID_BUTTON) {
            printf("middle button pressed\n");
        }
        if (flags & RIGHT_BUTTON) {
            printf("right button pressed\n");
        }

        if (flags & X_SIGN) {
            x_movement |= 0xFFFFFF00;
        }
        if (mouse_x + x_movement < 0) {
            mouse_x = 0;
        } else if (mouse_x + x_movement > IMAGE_X_WIDTH - 1) {
            mouse_x = IMAGE_X_WIDTH - 1;
        } else {
            mouse_x += x_movement;
        }
        printf("X movement = %d mouse_x = %d\n", x_movement, mouse_x);
        if (flags & Y_SIGN) {
            y_movement |= 0xFFFFFF00;
        }
        // TODO: For y_movement, should negate the result, don't know why.
        if (mouse_y - y_movement < 0) {
            mouse_y = 0;
        } else if (mouse_y - y_movement > IMAGE_Y_DIM - 1) {
            mouse_y = IMAGE_Y_DIM - 1;
        } else {
            mouse_y -= y_movement;
        }
        printf("Y movement is %d mouse_y = %d\n", y_movement, mouse_y);
    }
}