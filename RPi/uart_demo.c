//
// Compile with: gcc -Wall -o uart_demo uart_demo.c -lwiringPi
//

#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

#include <wiringPi.h>
#include <wiringSerial.h>

#define COM_PORT_A_NAME "/dev/ttyUSB0"
#define COM_PORT_B_NAME "/dev/ttyUSB1"

#define COM_PORT_RATE 115200

int main(void)
{
    int serial_port_a;
    int serial_port_b;
    
    // Initialise wiringPi setup
    if (wiringPiSetup () == -1)                                        
    {
        fprintf ( stdout, "Unable to start wiringPi: %s\n", strerror (errno) ) ;
        return 1 ;
    }

    // Open serial port A
    if ( (serial_port_a = serialOpen (COM_PORT_A_NAME, COM_PORT_RATE)) < 0 )
    {
        fprintf ( stderr, "Unable to open serial device: %s\n", strerror (errno) ) ;
        return 1 ;
    }


    // Open serial port B
    if ( (serial_port_b = serialOpen (COM_PORT_B_NAME, COM_PORT_RATE)) < 0 )
    {
        fprintf ( stderr, "Unable to open serial device: %s\n", strerror (errno) ) ;
        return 1 ;
    }

    uint8_t snd = 0;
    uint8_t rcv = 0;

    //
    // From now on, we will work with the serial FD natively, without wiringPi
    //
    while(1)
    {       
        // A --> B
        write(serial_port_a, &snd, 1);
        int n = read(serial_port_b, &rcv, 1);
        if ( n > 0 )
        {
            printf("\r\nrcv: %u", rcv);
            snd++;
        }
    }

    //
    // Make this reachable for production code
    // 
    close(serial_port_a);
    close(serial_port_b);

    return 0;
}