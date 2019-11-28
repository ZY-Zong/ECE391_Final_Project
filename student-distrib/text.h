//
// Created by gq290 on 11/28/2019.
//

#ifndef _TEXT_H
#define _TEXT_H
/*									tab:8
 *
 * text.h - font data and text to mode X conversion utility header file
 *
 * "Copyright (c) 2004-2009 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    2
 * Creation Date:   Thu Sep  9 22:08:16 2004
 * Filename:	    text.h
 * History:
 *	SL	1	Thu Sep  9 22:08:16 2004
 *		First written.
 *	SL	2	Sat Sep 12 13:40:11 2009
 *		Integrated original release back into main code base.
 */

/* The default VGA text mode font is 8x16 pixels. */
#define FONT_WIDTH   8
#define FONT_HEIGHT 16


#define STATUS_BAR_WIDTH 320 /*the width of status bar*/
#define SCROLL_WIDTH (STATUS_BAR_WIDTH / 4)
#define STATUS_BAR_HEIGHT 18  /* the height of status bar*/
#define STRING_LENGTH_MAX (STATUS_BAR_WIDTH / FONT_WIDTH) /*maximum length of the output string*/
#define OFF_PIXEL 0x0A /* The color of the off pixel in the text mode*/
#define ON_PIXEL 0x0C /* The color of the on pixel in the text mode*/

/* Standard VGA text font. */
extern unsigned char font_data[256][16];


/* This function transforms the input string of characters into
* VGA pixel buffer
* Input:
* in_string - a pointer to the original string
* plane_0 - a pointer to the output string stored in plane_0
* plane_1 - a pointer to the output string stored in plane_1
* plane_2 - a pointer to the output string stored in plane_2
* plane_3 - a pointer to the output string stored in plane_3
* Output: None.
* Side effects: modified plane_0, plane_1, plane_2, plane_3
*/

//void text_to_image(char * in_string, unsigned char* plane_0, unsigned char* plane_1 , unsigned char * plane_2, unsigned char* plane_3);
void text_to_image(char * in_string, unsigned char* array_2d);
#endif //_TEXT_H
