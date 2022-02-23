//
// Compile with: gcc -Wall -o hw_tunnel hw_tunnel.c -lwiringPi -lpthread
//

//
//
// For best performance, make sure you have the following "isolcpus" lines inside the cmdline file
//
// $ cat /boot/cmdline.txt
// console=serial0,115200 console=tty1 root=PARTUUID=ba069350-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait isolcpus=2,3
//
//

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

#include <wiringPi.h>
#include <wiringSerial.h>

#define TUN_TAP_IFACE_NAME "inverseg0"
#define COM_PORT_A_NAME "/dev/ttyUSB0"
#define COM_PORT_RATE 115200
#define TUNNEL_PAIR_Q_SIZE_IN_BYTES 4096

// Globals
typedef struct tunnel_pair{
    int tuntapfd;
    int serialfd;
} tunnel_pair;

tunnel_pair tun_pair;

// Function prototypes
int tun_tap_iface_create(char *name, int type);
int serial_port_create(char *name, int baud);
void signal_handler(int signal);
void *read_tuntap_write_serial(void *param);
void *read_serial_write_tuntap(void *param);

// Functions implementations
int tun_tap_iface_create(char *name, int type)
{
    struct ifreq ifr;
    int fd;
    int ret;

    if (( fd = open("/dev/net/tun", O_RDWR) ) < 0)
    {    
        printf("error: open()\n");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = type;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (( ret = ioctl(fd, TUNSETIFF, (void *)&ifr) ) < 0) 
    {
        printf("error: ioctl()\n");
        close(fd);
        return ret;
    }

    return fd;
}

int serial_port_create(char *name, int baud)
{
    int fd;

    // Initialise wiringPi setup
    if (wiringPiSetup () == -1)                                        
    {
        fprintf ( stderr, "Unable to start wiringPi: %s\n", strerror (errno) ) ;
        return 1 ;
    }

    // Open serial port A
    if ( (fd = serialOpen (name, baud)) < 0 )
    {
        fprintf ( stderr, "Unable to open serial device: %s\n", strerror (errno) ) ;
        close(fd);
    }

    return fd;
}

void signal_handler(int signal)
{
    printf("\nTerminting...\n");

    printf("\nClosing the tunnel interface...");
    close(tun_pair.tuntapfd);

    printf("\nClosing the serial port...");
    close(tun_pair.serialfd);

    printf("\nDone!\n");
    exit(1);
}

void *read_tuntap_write_serial_thread(void *param)
{
    tunnel_pair* pair = (tunnel_pair *)param;
    int tuntapfd = pair->tuntapfd;
    int serialfd = pair->serialfd;

    char buffer[TUNNEL_PAIR_Q_SIZE_IN_BYTES] = { 0 };
    int len = 0;

    while(1)
    {
        len = read(tuntapfd, buffer, sizeof(buffer));
        if (len > 0)
        {
            // TBD: Use 'write' in a loop to guarantee that all bytes have been sent
            write(serialfd, buffer, len);
        }
    }
}

void *read_serial_write_tuntap_thread(void *param)
{
    tunnel_pair* pair = (tunnel_pair *)param;
    int tuntapfd = pair->tuntapfd;
    int serialfd = pair->serialfd;

    char buffer[TUNNEL_PAIR_Q_SIZE_IN_BYTES] = { 0 };
    int len = 0;

    while(1)
    {
        len = read(serialfd, buffer, sizeof(buffer));
        if (len > 0)
        {
            // TBD: Use 'write' in a loop to guarantee that all bytes have been sent
            write(tuntapfd, buffer, len);
        }
    }
}

int main(void)
{
    signal(SIGHUP,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGSTOP, signal_handler);
    signal(SIGINT,  signal_handler);

    // Attempt to create the tun/tap
    tun_pair.tuntapfd = tun_tap_iface_create(TUN_TAP_IFACE_NAME, IFF_TUN);

    if (tun_pair.tuntapfd > 0)
        printf("Interface %s successfully created!\n", TUN_TAP_IFACE_NAME);
    else
        printf("Error creating the %s interface: fd is %d\n", TUN_TAP_IFACE_NAME, tun_pair.tuntapfd);

    // Attempt to create the serial port
    tun_pair.serialfd = serial_port_create(COM_PORT_A_NAME, COM_PORT_RATE);

    if (tun_pair.serialfd > 0)
        printf("Serial port %s successfully created!\n", COM_PORT_A_NAME);
    else
        printf("Error creating the %s serial port: fd is %d\n", COM_PORT_A_NAME, tun_pair.serialfd);


    cpu_set_t cpuset;

    pthread_t thread1;
    pthread_t thread2;

    pthread_create( &thread1, 0, read_tuntap_write_serial_thread,  (void *)&tun_pair );
    pthread_create( &thread2, 0, read_serial_write_tuntap_thread,  (void *)&tun_pair );

    int s;
    CPU_SET(2, &cpuset);
    s = pthread_setaffinity_np(thread1, sizeof(cpuset), &cpuset);
    if (s != 0)
        fprintf( stderr, "FATAL: Could not set CPU core affinity for the read_tuntap_write_serial_thread..." );

    CPU_SET(3, &cpuset);
    s = pthread_setaffinity_np(thread2, sizeof(cpuset), &cpuset);
    if (s != 0)
        fprintf( stderr, "FATAL: Could not set CPU core affinity for the read_serial_write_tuntap_thread..." );

    pthread_join( thread1, NULL );
    pthread_join( thread2, NULL );

    /*
    //int rx_bytes = 0;
    //int tx_bytes = 0;

    while(1)
    {
        len = read(tun_pair.tuntapfd, buffer, sizeof(buffer));
        if (len > 0)
        {
            write(tun_pair.serialfd, buffer, len);
        }
    }
    */

    return 0;
}