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

DigitalOut cs_tx(PA_0);
DigitalOut cs_rx(PA_2);

typedef enum { INITIAL, CS_TX, CS_RX } current_cs; 
current_cs cs = INITIAL;

DigitalOut sdn(PA_1);

DigitalOut  led(PC_13);
DigitalOut  tx_spirit(PB_12);
DigitalIn   rx_spirit(PB_13);

//
// SPIRIT-1 SPI communucation block
//

void cs_low(void)
{
    if (cs == CS_TX)
        cs_tx = 0;
    else if (cs == CS_RX)
        cs_rx = 0;
    
    else {} // pass
}

void cs_high(void)
{
    if (cs == CS_TX)
        cs_tx = 1;
    else if (cs == CS_RX)
        cs_rx = 1;
    
    else {} // pass
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

void configure_common_registers(void)
{
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
    spirit_spi_write(0x0C, 0x01);
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
    spirit_spi_write(0x0B, 0x2D);
    ThisThread::sleep_for(200ms);

    // SET MOD1
    spirit_spi_write(0x1A, 0xA3);
    ThisThread::sleep_for(200ms);

    // Set MOD0
    spirit_spi_write(0x1B, 0x59);
    ThisThread::sleep_for(200ms);

    // Set FDEV0
    spirit_spi_write(0x1C, 0x12);
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

    // Set the RX filter
    spirit_spi_write(0x1D, 0x27);
    ThisThread::sleep_for(200ms);

    // Set the MAGIC register - AFC2
    spirit_spi_write(0x1E, 0x27);
    ThisThread::sleep_for(200ms);

    spirit_spi_write(0x4F, 0x40);
    ThisThread::sleep_for(200ms);

    // Disable the CSMA
    spirit_spi_write(0x51, 0x00);
    ThisThread::sleep_for(200ms);

    // Enable persistent TX and RX
    spirit_spi_write(0x52, 0x0B);
    ThisThread::sleep_for(200ms);

    // Set PA_POWER[8]
    spirit_spi_write(0x10, 0x01);
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

    printf("\r\n*** All registers configured ***");
}

void start_tx(void)
{
    // Go to READY
    spirit_spi_command(0x62);
    ThisThread::sleep_for(200ms);
    
    // Lock TX
    spirit_spi_command(0x66);
    ThisThread::sleep_for(200ms);

    // Start TX
    spirit_spi_command(0x60);
    ThisThread::sleep_for(200ms);
}

void start_rx(void)
{
    // Go to READY
    spirit_spi_command(0x62);
    ThisThread::sleep_for(200ms);

    // Lock RX
    spirit_spi_command(0x65);
    ThisThread::sleep_for(200ms);

    // Start RX
    spirit_spi_command(0x61);
    ThisThread::sleep_for(200ms);
}

void configure_freq_a(void)
{
    // Set SYNT3
    spirit_spi_write(0x08, 0x6C);
    ThisThread::sleep_for(200ms);

    // Set SYNT2
    spirit_spi_write(0x09, 0x1E);
    ThisThread::sleep_for(200ms);

    // Set SYNT1
    spirit_spi_write(0x0A, 0x15);
    ThisThread::sleep_for(200ms);

    // Set SYNT0
    spirit_spi_write(0x0B, 0xCD);
    ThisThread::sleep_for(200ms);
}

void configure_freq_b(void)
{
    // Set SYNT3
    spirit_spi_write(0x08, 0x6C);
    ThisThread::sleep_for(200ms);

    // Set SYNT2
    spirit_spi_write(0x09, 0x1E);
    ThisThread::sleep_for(200ms);

    // Set SYNT1
    spirit_spi_write(0x0A, 0x54);
    ThisThread::sleep_for(200ms);

    // Set SYNT0
    spirit_spi_write(0x0B, 0xB5);
    ThisThread::sleep_for(200ms);
}

void configure_tx(void)
{
    cs = CS_TX;
    configure_common_registers();

    //configure_freq_a();

    start_tx();
    printf("\r\n ...TX part configured...");

    uint8_t part_num    = spirit_spi_read(0xF0);
    uint8_t version_num = spirit_spi_read(0xF1);

    printf("\r\n SPI part_num (=1): %d", part_num);
    printf("\r\n SPI version_num (=48): %d", version_num);
}

void configure_rx(void)
{
    cs = CS_RX;
    configure_common_registers();

    //configure_freq_b();

    start_rx();
    printf("\r\n ...RX part configured...");

    uint8_t part_num    = spirit_spi_read(0xF0);
    uint8_t version_num = spirit_spi_read(0xF1);

    printf("\r\n SPI part_num (=1): %d", part_num); 
    printf("\r\n SPI version_num (=48): %d", version_num); 
}

int main()
{
    serial_init(&stdio_uart, PA_9, PA_10);
    stdio_uart_inited = 1; 
 
    cs = INITIAL;
    cs_tx  = 1;
    cs_rx = 1;
    sdn = 0;

    spi.frequency(1000000);
    spi.format(8, 0);
    
    printf("\r\n -------------------------------");
    configure_rx();
    configure_tx();
    printf("\r\n Ready!");
    printf("\r\n -------------------------------");

    for (int i = 0; i < 100; i++) {
        printf("\n.");
        ThisThread::sleep_for(500ms);
    }
    
    return 0;
}