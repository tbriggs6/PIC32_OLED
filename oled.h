/* 
 * File:   oled.h
 * Author: tbriggs
 *
 * Created on November 13, 2014, 9:46 AM
 */

#ifndef OLED_H
#define	OLED_H

#define USE_DMA 1

void init_oled( );
void set_pixel(int row, int col);
void set_text(int row, int col, char ch);
void clear_buff( );
void set_buff( );
void paint( );
void scroll_text( );

#endif	/* OLED_H */

