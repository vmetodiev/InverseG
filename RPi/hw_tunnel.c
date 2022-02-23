/*
    Tun/Tap Interface Creation Example
*/

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

#define TUN_TAP_IFACE_NAME "inversg"

// Globals
int tuntapfd = -1;

// Function prototypes
int tun_tap_iface_create(char *name, int type);
void signal_handler(int signal);

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

void signal_handler(int signal)
{
    printf("\nTerminting...\n");
    close(tuntapfd);
    exit(1);
}

void main(void)
{
    signal(SIGHUP,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGSTOP, signal_handler);
    signal(SIGINT,  signal_handler);

    tuntapfd = tun_tap_iface_create(TUN_TAP_IFACE_NAME, IFF_TUN);
    char buffer[256] = { 0 };
    int len = 0;

    if (tuntapfd > 0)
        printf("Interface %s successfully created!\n", TUN_TAP_IFACE_NAME);
    else
        printf("Error creating the %s interface: fd is %d\n", TUN_TAP_IFACE_NAME, tuntapfd);

    while(1)
    {
        len = read(tuntapfd, buffer, sizeof(buffer));
        printf("\nReceived %d bytes...", len);
    }

    close(tuntapfd);
}