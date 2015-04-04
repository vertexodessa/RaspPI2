#include "pi_gpio.h"
#include <assert.h>
#include <exception>
#include <signal.h>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <stdint.h>

namespace PI{
#if defined (OLD_BCM)
#define BCM2708_PERI_BASE       0x20000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)
#else
#define BCM2708_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)
#endif

#define BLOCK_SIZE (4*1024)

/* GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x)
   or SET_GPIO_ALT(x,y) */
#define INP_GPIO(g) *(_ugpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(_ugpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) \
    *(_ugpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(_ugpio+7)     /* sets   bits */
#define GPIO_CLR *(_ugpio+10)    /* clears bits */
#define GPIO_GET *(_ugpio+13)    /* gets   all GPIO input levels */

volatile u_int32_t* UsualGPIO::_ugpio = 0;

static int is_signaled = 0;	/* Exit program if signaled */
/*
 * Signal handler to quit the program :
 */
static void sigint_handler(int signo) {
    is_signaled = 1;		/* Signal to exit program */
}

// ------------------ Non-Polling GPIO methods ---------------------------

UsualGPIO::UsualGPIO(int gpio, Dir out)
{
    // TODO: make thread safe (singleton semantics)
    if ( !_ugpio ) {
#ifdef DEBUG
        std::cerr << "opening gpio\n";
#endif
        int fd = open("/dev/mem",O_RDWR|O_SYNC);  /* Needs root access */
        if ( fd < 0 ) {
            perror("Opening /dev/mem");
            exit(1);
        }

        void* map = mmap(
            NULL,             /* Any address */
            BLOCK_SIZE,       /* # of bytes */
            PROT_READ|PROT_WRITE,
            MAP_SHARED,       /* Shared */
            fd,               /* /dev/mem */
            GPIO_BASE         /* Offset to GPIO */
        );

        if ( map == MAP_FAILED ) {
            perror("mmap(/dev/mem)");
            exit(1);
        }

        close(fd);
        _ugpio = (volatile u_int32_t *)map;
    }
    _gpio = gpio;
    _out = out;

    if (_out == Output) {
        gpio_config(_gpio, Output);
    } else {
        gpio_config(_gpio, Input);
    }
}

void UsualGPIO::WriteLow()
{
    Write(0);
}

void UsualGPIO::WriteHigh()
{
    Write(1);
}

void UsualGPIO::Write(bool val)
{
    if ( !_out ) {
        gpio_config(_gpio, Output);
        _out = Output;
    }

    gpio_write(_gpio, val);
}

bool UsualGPIO::Read()
{
    if ( _out ) {
        gpio_config(_gpio, Input);
    }
    return gpio_read(_gpio);
}

UsualGPIO::~UsualGPIO()
{
    gpio_config(_gpio, Input);
    // TODO: if this is last GPIO, unmap _ugpio and assign NULL
    // maybe introduce a class for _ugpio and do this in DTOR
}

// ------------- private UsualGPIO section --------------------

/*********************************************************************
 * Perform initialization to access GPIO registers:
 * Sets up pointer ugpio.
 *********************************************************************/
void UsualGPIO::gpio_config(int gpio, Dir output)
{
#ifdef DEBUG
    std::cerr << __func__ << gpio << ": " << output << std::endl;
#endif
    INP_GPIO(gpio);
    if ( output ) {
        OUT_GPIO(gpio);
    }
}

/*********************************************************************
 * Write a bit to the GPIO pin
 *********************************************************************/
void UsualGPIO::gpio_write(int gpio,int bit) {
#ifdef DEBUG
    std::cerr << __func__ << gpio << ": " << bit << std::endl;
#endif

    unsigned sel = 1 << gpio;

    if ( bit ) {
        GPIO_SET = sel;
    } else  {
        GPIO_CLR = sel;
    }
}

/*********************************************************************
 * Read a bit from a GPIO pin
 *********************************************************************/
int UsualGPIO::gpio_read(int gpio) {
    unsigned sel = 1 << gpio;

    return (GPIO_GET) & sel ? 1 : 0;
}

// ------------------- private UsualGPIO section end -----------------

// ------------------ Polling GPIO methods ---------------------------
PollingGPIO::PollingGPIO(int num) : _fd{-1}
{
    _pin = num;
}

int PollingGPIO::WaitForEvent(PollingGPIO::Edge edge)
{
    // 2do: deinit, check if can be opened
    // 2do: error if edge is not in list

    if(_fd == -1) {
        signal(SIGINT,sigint_handler);		/* Trap on SIGINT */
        _fd = gpio_open_edge(_pin, edge.c_str());	/* GPIO input */
        // First is false positive
        gpio_poll(_fd);
    }

    std::cout << "Monitoring for GPIO input changes: pin" << _pin << std::endl;

    // 2do: check ret val
    int ret = gpio_poll(_fd);

    if (is_signaled) {
        std::cerr << "Ctrl-C pressed!\n";
        CleanUp();
        exit(0);
    }

    return ret;
}

PollingGPIO::~PollingGPIO()
{

}

// private PollingGPIO section
void PollingGPIO::CleanUp()
{
    close(_fd);
    gpio_close();
}

/*
 * Close (unexport) GPIO pin :
 */
void PollingGPIO::gpio_close() {
    FILE *f;

    /* Unexport :	/sys/class/gpio/unexport */
    std::string buf = gpio_setpath(_pin, gp_unexport);
    f = fopen(buf.c_str(), "w");
    assert(f);
    fprintf(f,"%d\n",_pin);
    fclose(f);
}

/*
 * This routine will block until the open GPIO pin has changed
 * value. This pin should be connected to the MCP23017 /INTA
 * pin.
 */
int PollingGPIO::gpio_poll(int fd) {
    struct pollfd polls;
    char buf[32];
    int rc, n;

    polls.fd = fd;			/* /sys/class/gpio17/value */
    polls.events = POLLPRI;		/* Exceptions */

    do	{
    rc = poll(&polls,1,-1);	/* Block */
     if (is_signaled)
         return 0;
    } while ( rc < 0 && errno == EINTR);

    assert(rc > 0);

    lseek(fd,0,SEEK_SET);
    n = read(fd,buf,sizeof buf);	/* Read value */
    assert(n>0);
    buf[n] = 0;

    rc = sscanf(buf,"%d",&n);
    assert(rc==1);
    return n;			/* Return value */
}

/*
 * Open /sys/class/gpio%d/value for edge detection :
 */
int PollingGPIO::gpio_open_edge(int pin,const char *edge) {
    std::string buf;
    FILE *f;
    int fd;

    /* Export pin : /sys/class/gpio/export */
    buf = gpio_setpath(pin, gp_export);
    f = fopen(buf.c_str(), "w");
    assert(f);
    fprintf(f,"%d\n",pin);
    fclose(f);

    /* Direction :	/sys/class/gpio%d/direction */
    buf = gpio_setpath(pin, gp_direction);
    f = fopen(buf.c_str(), "w");
    assert(f);
    fprintf(f,"in\n");
    fclose(f);

    /* Edge :	/sys/class/gpio%d/edge */
    buf = gpio_setpath(pin, gp_edge);
    f = fopen(buf.c_str(), "w");
    assert(f);
    fprintf(f,"%s\n", edge);
    fclose(f);

    /* Value :	/sys/class/gpio%d/value */
    buf = gpio_setpath(pin, gp_value);
    fd = open(buf.c_str(), O_RDWR);
    return fd;
}

/*
 * Internal : Create a pathname for type in buf.
 */
std::string PollingGPIO::gpio_setpath(int pin, gpio_path_t type) {
    const char *paths[] = {
    "export", "unexport", "gpio%d/direction",
    "gpio%d/edge", "gpio%d/value" };

    constexpr char BUFSIZE = 20;
    char cbuf[BUFSIZE];
    snprintf(cbuf, BUFSIZE, paths[type], pin);

    std::string buf;
    buf = std::string("/sys/class/gpio/") + cbuf;

    return buf;
}
// ------------- private PollingGPIO section end --------------------

}


