//
// Created by liuzikai on 12/6/19.
//

#include "vga_draw.h"
#include "vga.h"

#include "text.h"

#define gr_readb(off)        (((volatile unsigned char *)GM)[(off)])
#define gr_readw(off)        (*(volatile unsigned short*)((GM)+(off)))
#define gr_readl(off)        (*(volatile unsigned long*)((GM)+(off)))
#define gr_writeb(v, off)    (GM[(off)] = (v))
#define gr_writew(v, off)    (*(unsigned short*)((GM)+(off)) = (v))
#define gr_writel(v, off)    (*(unsigned long*)((GM)+(off)) = (v))

#define alpha(color)    (((color) >> 24) & 0xFF)

static inline unsigned int channel_blend(unsigned int a, unsigned int b, unsigned int alpha) {
    unsigned int c1 = ((0x100 - alpha) * (a & 0xFF)) >> 8;
    unsigned int c2 = (alpha * (b & 0xFF)) >> 8;
    return ((c1 | c2) & 0xFF);
}

static inline vga_rgb rgb_blend(vga_rgb colora, vga_rgb colorb, unsigned int alpha) {
    unsigned int rb1 = ((0x100 - alpha) * (colora & 0xFF00FF)) >> 8;
    unsigned int rb2 = (alpha * (colorb & 0xFF00FF)) >> 8;
    unsigned int g1 = ((0x100 - alpha) * (colora & 0x00FF00)) >> 8;
    unsigned int g2 = (alpha * (colorb & 0x00FF00)) >> 8;
    return ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00);
}

static vga_argb curr_color = 0;

void vga_set_color_argb(vga_argb color) {
    curr_color = color;
}

/**
 * Draw a pixel using color set by vga_set_color_rgb() or vga_set_color_argb()
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 * @return 0 for success, other values for failure
 */
void vga_draw_pixel(int x, int y) {
    int i, j;
    unsigned long offset;
    vga_rgb c, t;

    c = curr_color;

    offset = (y + j) * vga_info.xbytes + (x + i) * 3;

    vga_set_page(offset >> 16);

    switch (offset & 0xFFFF) {
        case 0xFFFE:
            gr_writeb(channel_blend(gr_readb(0xFFFE), c, alpha(c)), 0xFFFE);
            gr_writeb(channel_blend(gr_readb(0xFFFF), c >> 8, alpha(c)), 0xFFFF);

            vga_set_page((offset >> 16) + 1);
            gr_writeb(channel_blend(gr_readb(0), c >> 16, alpha(c)), 0);
            break;
        case 0xFFFF:
            gr_writeb(channel_blend(gr_readb(0xFFFF), c, alpha(c)), 0xFFFF);

            vga_set_page((offset >> 16) + 1);
            gr_writeb(channel_blend(gr_readb(0), c >> 8, alpha(c)), 0);
            gr_writeb(channel_blend(gr_readb(1), c >> 16, alpha(c)), 1);
            break;
        default:
            offset &= 0xFFFF;
            t = gr_readl(offset);
            t = rgb_blend(t, c, alpha(c));
            gr_writew(t, offset);
            gr_writeb(t >> 16, offset + 2);
            break;
    }

}

vga_rgb vga_get_pixel(int x, int y) {
    unsigned long offset;
    vga_rgb pix = 0;

    offset = y * vga_info.xbytes + x * 3;
    vga_set_page(offset >> 16);
    switch (offset & 0xFFFF) {
        case 0xFFFE:
            pix = gr_readw(0xFFFE);
            vga_set_page((offset >> 16) + 1);
            return pix + (gr_readb(0) << 16);
        case 0xFFFF:
            pix = gr_readb(0xFFFF);
            vga_set_page((offset >> 16) + 1);
            return pix + (gr_readw(0) << 8);
        default:
            offset &= 0xFFFF;
            return gr_readw(offset) + (gr_readb(offset + 2) << 26);

    }
}

void vga_print_char(int x, int y, char ch, vga_argb fg, vga_argb bg) {
    int i, j;
    unsigned long offset;
    vga_rgb c, t;
    for (i = 0; i < FONT_WIDTH; i++) {
        for (j = 0; j < FONT_HEIGHT; j++) {

            c = ((font_data[ch][j] & (1 << (7 - i)) ? fg : bg));

            offset = (y + j) * vga_info.xbytes + (x + i) * 3;

            vga_set_page(offset >> 16);

            switch (offset & 0xFFFF) {
                case 0xFFFE:
                    gr_writeb(channel_blend(gr_readb(0xFFFE), c, alpha(c)), 0xFFFE);
                    gr_writeb(channel_blend(gr_readb(0xFFFF), c >> 8, alpha(c)), 0xFFFF);

                    vga_set_page((offset >> 16) + 1);
                    gr_writeb(channel_blend(gr_readb(0), c >> 16, alpha(c)), 0);
                    break;
                case 0xFFFF:
                    gr_writeb(channel_blend(gr_readb(0xFFFF), c, alpha(c)), 0xFFFF);

                    vga_set_page((offset >> 16) + 1);
                    gr_writeb(channel_blend(gr_readb(0), c >> 8, alpha(c)), 0);
                    gr_writeb(channel_blend(gr_readb(1), c >> 16, alpha(c)), 1);
                    break;
                default:
                    offset &= 0xFFFF;
                    t = gr_readl(offset);
                    t = rgb_blend(t, c, alpha(c));
                    gr_writew(t, offset);
                    gr_writeb(t >> 16, offset + 2);
                    break;
            }
        }
    }
}

void vga_print_char_array(int start_x, int start_y, char *array, int array_rows, int array_columns, vga_argb fg,
                          vga_argb bg) {
    int x, y, i, j;
    char ch;
    vga_rgb c, t;

    unsigned long offset = start_y * vga_info.xbytes + start_x * 3;
    int page = offset >> 16;

    offset &= 0xFFFF;
    vga_set_page(page);

    char* vm = (char*) (GM + offset);


    for (y = 0; y < array_rows; y++) {
        for (x = 0; x < array_columns; x++) {

            ch = array[y * array_columns + x];

            for (i = 0; i < FONT_WIDTH; i++) {
                for (j = 0; j < FONT_HEIGHT; j++) {

                    c = ((font_data[ch][j] & (1 << (7 - i)) ? fg : bg));

                    if (offset < 0xFFFD) {

                        // Write 4 bytes, last byte will get overlapped by the next write
                        *((unsigned int *) vm) = rgb_blend(*((unsigned int *) vm), c, alpha(c));

                        offset += 3;
                        vm += 3;

                    } else {

                        switch (offset) {
                            case 0xFFFD:

                                // Still write 4 bytes to last, last byte will overflow
                                *((unsigned int *) vm) = rgb_blend(*((unsigned int *) vm), c, alpha(c));

                                vga_set_page(++page);
                                offset = 0;
                                vm = GM;

                            case 0xFFFE:

                                // Still write 4 bytes to last, last 2 byte will overflow
                                *((unsigned int *) vm) = rgb_blend(*((unsigned int *) vm), c, alpha(c));

                                vga_set_page(++page);
                                offset = -2;
                                vm = GM - 2;

                                // First 2 bytes will underflow, actually write 2 bytes into video memory,
                                //  last byte will get overlapped by the next write
                                *((unsigned int *) vm) = rgb_blend(*((unsigned int *) vm), c, alpha(c));

                                gr_writeb(channel_blend(gr_readb(0xFFFE), c, alpha(c)), 0xFFFE);
                                gr_writeb(channel_blend(gr_readb(0xFFFF), c >> 8, alpha(c)), 0xFFFF);
//                            gr_writew(c, 0xFFFE);

                                vga_set_page((offset >> 16) + 1);
                                gr_writeb(channel_blend(gr_readb(0), c >> 16, alpha(c)), 0);
//                            gr_writeb(c >> 16, 0);
                                break;
                            case 0xFFFF:
                                gr_writeb(channel_blend(gr_readb(0xFFFF), c, alpha(c)), 0xFFFF);
//                            gr_writeb(c, 0xFFFF);

                                vga_set_page((offset >> 16) + 1);
                                gr_writeb(channel_blend(gr_readb(0), c >> 8, alpha(c)), 0);
                                gr_writeb(channel_blend(gr_readb(1), c >> 16, alpha(c)), 1);
//                            gr_writew(c >> 8, 0);
                                break;
                            default:

                                offset & 0xFFFF;
                                t = gr_readl(offset & 0xFFFF);
                                t = rgb_blend(t, c, alpha(c));
                                gr_writew(t, offset & 0xFFFF);
                                gr_writeb(t >> 16, (offset & 0xFFFF) + 2);

//                            gr_writew(c, (offset & 0xFFFF));
//                            gr_writeb(c >> 16, (offset & 0xFFFF) + 2);
                                break;
                        }
                    }

                    vga_set_page(offset >> 16);

                    switch (offset & 0xFFFF) {
                        case 0xFFFE:
                            gr_writeb(channel_blend(gr_readb(0xFFFE), c, alpha(c)), 0xFFFE);
                            gr_writeb(channel_blend(gr_readb(0xFFFF), c >> 8, alpha(c)), 0xFFFF);
//                            gr_writew(c, 0xFFFE);

                            vga_set_page((offset >> 16) + 1);
                            gr_writeb(channel_blend(gr_readb(0), c >> 16, alpha(c)), 0);
//                            gr_writeb(c >> 16, 0);
                            break;
                        case 0xFFFF:
                            gr_writeb(channel_blend(gr_readb(0xFFFF), c, alpha(c)), 0xFFFF);
//                            gr_writeb(c, 0xFFFF);

                            vga_set_page((offset >> 16) + 1);
                            gr_writeb(channel_blend(gr_readb(0), c >> 8, alpha(c)), 0);
                            gr_writeb(channel_blend(gr_readb(1), c >> 16, alpha(c)), 1);
//                            gr_writew(c >> 8, 0);
                            break;
                        default:

                            offset & 0xFFFF;
                            t = gr_readl(offset & 0xFFFF);
                            t = rgb_blend(t, c, alpha(c));
                            gr_writew(t, offset & 0xFFFF);
                            gr_writeb(t >> 16, (offset & 0xFFFF) + 2);

//                            gr_writew(c, (offset & 0xFFFF));
//                            gr_writeb(c >> 16, (offset & 0xFFFF) + 2);
                            break;
                    }



                }
            }
        }
    }

}