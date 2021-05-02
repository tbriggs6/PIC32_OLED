/* 
 * File:   config.h
 * Author: tbriggs
 *
 * Created on November 13, 2014, 9:45 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H


#define LED_VDD   IOPORT_D,BIT_6
#define LED_VBAT  IOPORT_D,BIT_13
#define LED_RESET IOPORT_D,BIT_8
#define LED_DC    IOPORT_E,BIT_8



#define SYS_CLK (40000000L)
#define PB_CLK  (20000000L)

#define CP0_PER_USEC ((SYS_CLK/2) / 1000000)
#define CP0_PER_MSEC ((SYS_CLK/2) / 1000)

#define VIRT2PHY(x) \
    ((int)x<0?((int)x&0x1fffffffL):(unsigned int)((unsigned char*)x+(0x40000000L)))

#endif	/* CONFIG_H */

