#include <p32xxxx.h>
#include <stdint.h>
#include <plib.h>
#include "config.h"
#include "oled.h"

static uint8_t spi_put(uint8_t byte) {

 uint8_t val;

    // clear out the SPI Buffer
    while (SPI1STATbits.SPIRBF == 1)
        val = SPI1BUF;

    // clear any old overflows
    SPI1STATbits.SPIROV = 1;

    // write register to read
    SPI1BUF = byte;

    // wait for the SITBE value to be set before reading
    int count = SPI1BRG;
    while ((count-- > 0) && (SPI1STATbits.SPIBUSY == 1)) ;

    count = SPI1BRG;
    while ((count-- > 0) && (SPI1STATbits.SPIRBF == 0));


}
#define SPION (1 << 15)



// fonts are 8x8, in column order.
extern const uint8_t rgbOledFont0[];

uint8_t gbuff[4][128];

void set_pixel(int row, int col)
{
    // example set pixel 9, thats page 1, bit 1
    int page = row / 8;
    int bit_in_row = row % 8;

    gbuff[page][col] |= 1 << bit_in_row;
}

void clear_pixel(int row, int col)
{
    // example set pixel 9, thats page 1, bit 1
    int page = row / 8;
    int bit_in_row = row % 8;

    gbuff[page][col] &= ~(1 << bit_in_row);
}

void scroll_text( )
{
    int i, j;
    for (i = 1; i < 4; i++)
    {
        for (j =0 ; j < 128; j++)
        {
            gbuff[i-1][j] = gbuff[i][j];
        }
    }
}

void set_text(int row, int col, char ch)
{
    if (row >= 4) return;
    if (row < 0) return;
    if (col < 0) return;
    if (col >= 128/8) return;
    if (ch < 0x20) return;
    if (ch > 0x7f) return;


    const uint8_t *fontptr = &rgbOledFont0[ (ch-0x20) * 8];

    char *dest = &gbuff[row][col*8];
    int i;
    for (i = 0; i < 8; i++)
    {
        *dest++ = *fontptr++;
    }
}

void set_string(int row, int col, char *msg)
{
    while (*msg != 0)
    {
        set_text(row, col++, *msg);
        msg++;
    }
}



void clear_buff( )
{
    int i,j;
    for (i = 0; i < 0; i < 4)
        for (j = 0; j < 128; j++)
            gbuff[i][j] = 0;
}

void set_buff( )
{
    int i,j;
    for (i = 0; i < 0; i < 4)
        for (j = 0; j < 128; j++)
            gbuff[i][j] = 0xff;
}

//#undef USE_DMA

#ifndef USE_DMA

void paint( )
{
    int page, column;

    for (page = 0; page < 4; page++)
    {
        // start sending data
        PORTClearBits(LED_DC);

        // set page address
        spi_put(0x22);
        spi_put(page);
        spi_put(0);
        spi_put(0x10);

        PORTSetBits(LED_DC);

        for (column = 0; column < 128; column++)
        {
            spi_put(gbuff[page][column]);
        }
    }
}
#endif

#ifdef USE_DMA

volatile int dma_idle = 1;
volatile int dma_repaint = 0;
volatile int dma_curr_page = 0;
char dma_cmd[4] = { 0x22, 0, 127, 0xB0 };

// the data just finished, setup the command
void __ISR(_DMA_1_VECTOR, ipl5) dma_handler_data( )
{

    mDMA1ClearIntFlag();
    DCH1INTCLR = 0xff;
    //LATDbits.LATD9 = 1;

    mSPI1ClearAllIntFlags( );

    dma_cmd[3] = (char) dma_curr_page;
    DmaChnDisable(1);

    // start sending command data
    DmaChnSetTxfer(2, (void *) dma_cmd, (void *) &SPI1BUF, 4, 1, 1);
    
//    //DmaChnStartTxfer(2, DMA_WAIT_NOT, 0);

    PORTClearBits(LED_DC);
    LATDbits.LATD9 = 0;

    DmaChnDisable(1);
    DmaChnEnable(2);
    DmaChnForceTxfer(2);
    //SPI1BUF = dma_cmd[0];


}

// the command just finished, setup the data
void __ISR(_DMA_2_VECTOR, ipl5) dma_handler_command( )
{
//    LATDbits.LATD9 = 1;
    LATDbits.LATD9 = 0;
    PORTSetBits(LED_DC);
    mDMA2ClearIntFlag();
    DCH2INTCLR = 0xff;

mSPI1ClearAllIntFlags( );
    
    
    
    

    DmaChnSetTxfer(1, (void *) &gbuff[dma_curr_page], (void *) &SPI1BUF, 128, 1, 1);

    dma_curr_page++;
    if (dma_curr_page == 4) {
        dma_curr_page = 0;
    }

    if ((dma_curr_page == 0) && (dma_repaint == 0)) {
        dma_idle = 1;
    }

    // start next block
    else {
    
        DmaChnDisable(2);
        DmaChnEnable(1);
          DmaChnForceTxfer(2);
        //SPI1BUF = gbuff[dma_curr_page][0];
        //DmaChnStartTxfer(1, DMA_WAIT_NOT, 0);
    }
    
}


void force_dma_start( )
{
    if (dma_idle == 0) return;

    dma_repaint = 0;
    dma_idle = 0;
    dma_curr_page = 0;
    dma_cmd[1] = (char) dma_curr_page;

    DmaChnSetTxfer(1, (void *) &dma_cmd, (void *) &SPI1BUF, 4, 1, 1);

    DmaChnDisable(2);
    DmaChnEnable(1);
    PORTClearBits(LED_DC);

    //LATDbits.LATD9 = 1;
    PORTSetBits(LED_DC);
    LATDbits.LATD9 = 0;

    
    DmaChnForceTxfer(1);
    //DmaChnStartTxfer(1, DMA_WAIT_NOT, 0);
    //SPI1BUF = dma_cmd[0];
    
}

void init_dma( )
{
    int chn = 1;

    DMACON = 1 << 15;

    mSPI1ClearAllIntFlags( );
    
    // enable channel 1 - the handler for the finished data / command starter
    DmaChnConfigure(1, DMA_CHN_PRI2, DMA_CONFIG_DEFAULT);
    DmaChnSetEventControl(1, DMA_EV_START_IRQ_EN|DMA_EV_START_IRQ(_SPI1_TX_IRQ));
    dma_cmd[1] = (char) dma_curr_page;
    DmaChnSetTxfer(1, (void *) &dma_cmd, (void *) &SPI1BUF, 4, 1, 1);
    DmaChnSetEvEnableFlags(chn, DMA_EV_BLOCK_DONE);
    mDMA1ClearIntFlag();
    DmaChnSetIntPriority(1, INT_PRIORITY_LEVEL_5, INT_SUB_PRIORITY_LEVEL_2);
    DmaChnIntDisable(1);

    DmaChnConfigure(2, DMA_CHN_PRI2, DMA_CONFIG_DEFAULT);
    DmaChnSetEventControl(2, DMA_EV_START_IRQ_EN|DMA_EV_START_IRQ(_SPI1_TX_IRQ));
    DmaChnSetTxfer(2, (void *) gbuff[0], (void *) &SPI1BUF, 128, 1, 1);
    DmaChnSetEvEnableFlags(2, DMA_EV_BLOCK_DONE);

    mDMA2ClearIntFlag();
    DmaChnSetIntPriority(2, INT_PRIORITY_LEVEL_5, INT_SUB_PRIORITY_LEVEL_3);
    DmaChnDisable(2);

    mDMA1IntEnable(1);
    mDMA2IntEnable(1);
    INTEnableSystemMultiVectoredInt();


    clear_buff();
    force_dma_start();
}


// start painting row 0
void paint( )
{
    if (dma_idle == 1) force_dma_start( );
    else dma_repaint = 1;
}


#endif


void init_oled( )
{
    
    PORTSetPinsDigitalOut(IOPORT_D, BIT_0 | BIT_6 | BIT_8 | BIT_9 | BIT_10 | BIT_13 );
    PORTSetPinsDigitalOut(IOPORT_E, BIT_8);

    PORTSetPinsDigitalOut(IOPORT_C, BIT_4);

    PORTSetBits(LED_DC);
    PORTSetBits(LED_VBAT);
    PORTSetBits(LED_VDD);
    PORTSetBits(LED_RESET);


    SPI1CON = 0;                    // reset the SPI bus
    uint8_t byte = SPI1BUF;         // read any data on it
    //SPI1BRG = 200;                   // set the delay = 1MHz
    SPI1BRG = 20;                   // set the delay = 1MHz
    SPI1STATCLR = 0x40;
    SPI1CON = 0xA120;

    SPI1CON |= SPION;

    
    PORTClearBits(LED_DC);      // clear data / cmd bit
    PORTClearBits(LED_VDD);     // clear VDD (turn it on, its complimentary)

    PORTClearBits(IOPORT_D, BIT_9);

    spi_put(0xAE);              // send AE - turn display off

    PORTClearBits(LED_RESET);   // drop reset

    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < (1 * CP0_PER_MSEC));

    PORTSetBits(LED_RESET);     // enable reset

    // set charge pump & pre-charge period (not documented in datasheets)
    spi_put(0x8D);
    spi_put(0x14);

    spi_put(0xD9);
    spi_put(0xF1);

    // turn on VCC and wait 100 ms
    PORTClearBits(LED_VBAT);
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < (100 * CP0_PER_MSEC));

    spi_put(0xA1);
    spi_put(0xC8);

    spi_put(0xda);
    spi_put(0x20);
    
    spi_put(0xaf);

    spi_put(0x20);
    spi_put(0xa2);
    
    //PORTSetBits(IOPORT_D, BIT_9);


    clear_buff( );
    
#ifdef USE_DMA
    init_dma();
#else
    paint( );
#endif

}