#ifndef GPIO_H
#define GPIO_H

#include <string>
#include <stdint.h>

namespace PI {

class PollingGPIO
{
public:
    explicit PollingGPIO(int num);

    typedef std::string Edge;

    int WaitForEvent(Edge edge);


    ~PollingGPIO();
private:
    typedef enum {
        gp_export=0,	/* /sys/class/gpio/export */
        gp_unexport,	/* /sys/class/gpio/unexport */
        gp_direction,	/* /sys/class/gpio%d/direction */
        gp_edge,	/* /sys/class/gpio%d/edge */
        gp_value	/* /sys/class/gpio%d/value */
    } gpio_path_t;

    // Disallow default ctor
    PollingGPIO() =delete;
    PollingGPIO& operator =(PollingGPIO const&) =delete;

    void CleanUp();

    int _pin;
    int _fd;

    void gpio_close();
    int gpio_poll(int fd);
    int gpio_open_edge(int pin,const char *edge);
    std::string gpio_setpath(int pin, gpio_path_t type);
};

class UsualGPIO {
public:
    typedef enum {
        Input = 0,
        Output
    } Dir;

    explicit UsualGPIO(int gpio=4, Dir out =Input);

    // output methods
    void WriteLow(void);
    void WriteHigh(void);
    void Write(bool val);
    bool Read(void);
    ~UsualGPIO();
private:
    // no default ctor
    UsualGPIO() =delete;
    UsualGPIO& operator =(UsualGPIO const&) =delete;
    void gpio_config(int gpio,Dir output);
    void gpio_write(int gpio,int bit);
    int gpio_read(int gpio);
    static volatile u_int32_t* _ugpio;
    int _gpio;
    Dir _out;
};

}

#endif // GPIO_H
