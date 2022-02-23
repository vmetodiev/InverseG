//
// Compile with: gcc -Wall -o uart_demo_pthread uart_demo_pthread.c -lwiringPi -lpthread
//

//
//
// Make sure you have the following "isolcpus" lines inside the cmdline file
//
// $ cat /boot/cmdline.txt
// console=serial0,115200 console=tty1 root=PARTUUID=ba069350-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait isolcpus=2,3
//
//

#define _GNU_SOURCE

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

typedef struct uart_pair{
    int fd_a;
    int fd_b;
} uart_pair;

void *a_to_b_thread(void *param)
{
    uart_pair* serial_ports = (uart_pair *)param;
    int serial_port_a = serial_ports->fd_a;
    int serial_port_b = serial_ports->fd_b;

    uint8_t snd = 0;
    uint8_t rcv = 0;

    while(1)
    {       
        // A --> B
        write(serial_port_a, &snd, 1);
        int n = read(serial_port_b, &rcv, 1);
        if ( n > 0 )
        {
            printf("\r\nThread A-->B rcv: %u", rcv);
            snd++;
        }
    }
}

void *b_to_a_thread(void *param)
{
    uart_pair* serial_ports = (uart_pair *)param;
    int serial_port_a = serial_ports->fd_a;
    int serial_port_b = serial_ports->fd_b;

    uint8_t snd = 0;
    uint8_t rcv = 0;

    while(1)
    {       
        // B --> A
        write(serial_port_b, &snd, 1);
        int n = read(serial_port_a, &rcv, 1);
        if ( n > 0 )
        {
            printf("\r\n    Thread B-->A rcv: %u", rcv);
            snd++;
        }
    }

}

int main(void)
{
    int serial_port_a;
    int serial_port_b;
    
    // Initialise wiringPi setup
    if (wiringPiSetup () == -1)                                        
    {
        fprintf ( stderr, "Unable to start wiringPi: %s\n", strerror (errno) ) ;
        return 1 ;
    }

    // Open serial port A, using the wiringPI
    if ( (serial_port_a = serialOpen (COM_PORT_A_NAME, COM_PORT_RATE)) < 0 )
    {
        fprintf ( stderr, "Unable to open serial device: %s\n", strerror (errno) ) ;
        return 1 ;
    }


    // Open serial port B, using the wiringPI
    if ( (serial_port_b = serialOpen (COM_PORT_B_NAME, COM_PORT_RATE)) < 0 )
    {
        fprintf ( stderr, "Unable to open serial device: %s\n", strerror (errno) ) ;
        return 1 ;
    }

    uart_pair com_pairs;
    com_pairs.fd_a = serial_port_a;
    com_pairs.fd_b = serial_port_b;

    cpu_set_t cpuset;

    pthread_t thread1;
    pthread_t thread2;

    pthread_create( &thread1, 0, a_to_b_thread,  (void *)&com_pairs );
    pthread_create( &thread2, 0, b_to_a_thread,  (void *)&com_pairs );

    int s;
    CPU_SET(2, &cpuset);
    s = pthread_setaffinity_np(thread1, sizeof(cpuset), &cpuset);
    if (s != 0)
        fprintf( stderr, "FATAL: Could not set CPU core affinity for the a_to_b_thread..." );

    CPU_SET(3, &cpuset);
    s = pthread_setaffinity_np(thread2, sizeof(cpuset), &cpuset);
    if (s != 0)
        fprintf( stderr, "FATAL: Could not set CPU core affinity for the b_to_a_thread..." );

    pthread_join( thread1, NULL );
    pthread_join( thread2, NULL );

    //
    // Make this reachable for production code
    // 
    close(serial_port_a);
    close(serial_port_b);

    return 0;
}