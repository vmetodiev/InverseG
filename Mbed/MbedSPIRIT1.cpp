/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include <cstdint>
#include <cstdio>

#define SPI_WRITE_OP    0x00
#define SPI_READ_OP     0x01
#define SPI_COMMAND_OP  0x80

#define SPI_DUMMY_BYTE  0x00

// Function prototypes
static uint16_t spirit_spi_write(uint8_t address, uint8_t data);
static uint8_t spirit_spi_read(uint8_t address);
static uint16_t spirit_spi_command(uint8_t command);

// Declarations needed to change the parameters of stdio UART 
extern serial_t     stdio_uart; 
extern int          stdio_uart_inited; 

SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk
DigitalOut cs(PA_0);
DigitalOut sdn(PA_1);

DigitalOut  led(PC_13);
DigitalOut  tx_spirit(PB_12);
DigitalIn   rx_spirit(PB_13);

//
// SPIRIT-1 SPI Communucation block
//

void cs_low(void)
{
    cs = 0;
}

void cs_high(void)
{
    cs = 1;
}

static uint16_t spirit_spi_write(uint8_t address, uint8_t data)
{
    uint16_t status = 0x00;
    
    cs_low();
    uint8_t upper_byte = spi.write(SPI_WRITE_OP);
    uint8_t lower_byte = spi.write(address);
    spi.write(data);
    cs_high();

    status |= (upper_byte << 8);
    status |= lower_byte;

    return  status;
}

static uint8_t spirit_spi_read(uint8_t address)
{
    uint8_t read_value = 0x00;
    
    cs_low();
    uint8_t upper_byte = spi.write(SPI_READ_OP);
    uint8_t lower_byte = spi.write(address);
    read_value = spi.write(SPI_DUMMY_BYTE);
    cs_high();

    return read_value;
}

static uint16_t spirit_spi_command(uint8_t command)
{
    uint16_t status = 0x00;

    cs_low();
    uint8_t upper_byte = spi.write(SPI_COMMAND_OP);
    uint8_t lower_byte = spi.write(command);
    cs_high();

    status |= (upper_byte << 8);
    status |= lower_byte;

    return  status;
}

int main()
{
    serial_init(&stdio_uart, PA_9, PA_10);
    stdio_uart_inited = 1; 
 
    cs  = 1;
    sdn = 0;

    spi.frequency(1000000);
    spi.format(8, 0);
    
    char str[8] = { '\0' };
    printf("\r\n ********************************");
    
    bool read_op = false;

    //
    // Execute the initial register configuration
    //
    printf("\r\n*** Executing registers configuration ***");

    // Enter STANDBY, check the XO_RCO_TEST, disable PD_CLKDIV
    spirit_spi_write(0xB4, 0x29);
    ThisThread::sleep_for(200ms);

    // Check SYNTH_CONFIG[0]
    spirit_spi_write(0x9F, 0x20);
    ThisThread::sleep_for(200ms);

    // SYNTH_CONFIG[1] (REFDIV and VCO_L_SEL)
    spirit_spi_write(0x9E, 0x5D);
    ThisThread::sleep_for(200ms);

    // RCO and VCO automatic calibration RCO_CALIBRATION
    spirit_spi_write(0x50, 0x06);
    ThisThread::sleep_for(200ms);

    // Check the 24_26MHz_SELECT bit in the ANA_FUNC_CONF register
    spirit_spi_write(0x01, 0xC0);
    ThisThread::sleep_for(200ms);

    // Set FC_OFFSET (=0d)
    spirit_spi_write(0x0F, 0x00);
    ThisThread::sleep_for(200ms);
    spirit_spi_write(0x0E, 0x00);
    ThisThread::sleep_for(200ms);

    // Set CHSPACE (=16d)
    spirit_spi_write(0x0C, 0x10);
    ThisThread::sleep_for(200ms);

    // Set CHNUM (=0d)
    spirit_spi_write(0x6C, 0x00);
    ThisThread::sleep_for(200ms);

    // Set SYNT3
    spirit_spi_write(0x08, 0x6C);
    ThisThread::sleep_for(200ms);

    // Set SYNT2
    spirit_spi_write(0x09, 0x1E);
    ThisThread::sleep_for(200ms);

    // Set SYNT1
    spirit_spi_write(0x0A, 0x35);
    ThisThread::sleep_for(200ms);

    // Set SYNT0
    spirit_spi_write(0x0B, 0x45);
    ThisThread::sleep_for(200ms);

    // SET MOD1
    spirit_spi_write(0x1A, 0x48);
    ThisThread::sleep_for(200ms);

    // Set MOD0
    spirit_spi_write(0x1B, 0x5E);
    ThisThread::sleep_for(200ms);

    // Set TXSOURCE  as "Direct through GPIO" inside PCKTCTRL1
    spirit_spi_write(0x33, 0x08);
    ThisThread::sleep_for(200ms);

    // Set RX_MODE as "Direct through GPIO" inside PCKTCTRL3
    spirit_spi_write(0x31, 0x27);
    ThisThread::sleep_for(200ms);

    // Set GPIO_2 as TX pin inside GPIO2_CONF
    spirit_spi_write(0x03, 0x11);
    ThisThread::sleep_for(200ms);

    // Set GPIO_3 as RX pin inside GPIO3_CONF
    spirit_spi_write(0x02, 0x43);
    ThisThread::sleep_for(200ms);

    // Set the RX filter (0x26 --> 12.115kHz ; 0x27 --> 6.057kHz)
    spirit_spi_write(0x1E, 0x27);
    ThisThread::sleep_for(200ms);

    // Set PN9 inside PCKTCTRL1
    /*
    spirit_spi_write(0x33, 0x0C);
    ThisThread::sleep_for(200ms);
    
    // Set PA_POWER[8]
    spirit_spi_write(0x10, 0x21);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[7]
    spirit_spi_write(0x11, 0x0E);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[6]
    spirit_spi_write(0x12, 0x1A);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[5]
    spirit_spi_write(0x13, 0x25);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[4]
    spirit_spi_write(0x14, 0x35);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[3]
    spirit_spi_write(0x15, 0x40);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[2]
    spirit_spi_write(0x16, 0x4E);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[1]
    spirit_spi_write(0x17, 0x00);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[0]
    spirit_spi_write(0x18, 0x07);
    ThisThread::sleep_for(200ms);
    */

    printf("\r\n*** All registers configured ***");

    /*   
    // // // Tx logic

    // Go to READY
    spirit_spi_command(0x62);
    ThisThread::sleep_for(200ms);
    
    // Lock TX
    spirit_spi_command(0x66);
    ThisThread::sleep_for(200ms);

    // Start TX
    spirit_spi_command(0x60);
    ThisThread::sleep_for(200ms);

    while(1)
    {
        led = !led;
        tx_spirit = !tx_spirit;
        ThisThread::sleep_for(1s);
    }
    */     

    // // // Rx logic

    // Go to READY
    spirit_spi_command(0x62);
    ThisThread::sleep_for(200ms);

    // Lock RX
    spirit_spi_command(0x65);
    ThisThread::sleep_for(200ms);

    // Start RX
    spirit_spi_command(0x61);
    ThisThread::sleep_for(200ms);

    while(1)
    {
        led = rx_spirit;
    }
    

    /*
    while(1) 
    {  
        // Get the SPI operation (C/R/w)
        printf("\r\n ---- SPI OP (C/R/W): ");    scanf("%7s", str);
        printf("\n ---> OP: %s", str);
        
        if (str[0] == 'C') {    // COMMAND mode 
            printf("\r\n ---- COMMAND: 0x");    scanf("%7s", str);
            uint8_t command = (uint8_t)strtol(str, NULL, 16);
            spirit_spi_command(command);
            printf("\n ---> SPI Command Execution Finished");
        }

        else {                 // Register READ/WRITE mode
            if (str[0] == 'R')
                read_op = true;
            else
                read_op = false;

            str[0] = '\0';

            printf("\r\n ---- ADDRESS: 0x");    scanf("%7s", str);
            uint8_t reg_addr = (uint8_t)strtol(str, NULL, 16);
            printf("\n ---> SPI ADDRESS: %d", reg_addr);
            str[0] = '\0';
            
            if (read_op) {         // In case of read operation
                uint8_t reg_read_value = spirit_spi_read(reg_addr);
                printf("\n ---> READ VALUE: %d", reg_read_value);
            }
            else {                 // In case of write operation
                printf("\r\n ---- VALUE: 0x");    scanf("%7s", str);
                uint8_t reg_value = (uint8_t)strtol(str, NULL, 16);
                spirit_spi_write(reg_addr, reg_value);
                printf("\n ---> SPI Write Finished");
            }
        }

        // Always perform this simple test
        uint8_t part_num    = spirit_spi_read(0xF0);
        uint8_t version_num = spirit_spi_read(0xF1);

        printf("\r\n SPI part_num (=1): %d", part_num); 
        printf("\r\n SPI version_num (=48): %d", version_num); 
        
        printf("\r\n ******************************** \r\n");
        
        ThisThread::sleep_for(1s);
    }
    */

    return 0;
}
