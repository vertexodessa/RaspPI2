/*********************************************************************
 * evinput.c : Event driven GPIO input
 *
 * ./evinput gpio#
 *********************************************************************/

#include "pi_gpio.h"
#include "pi_utils.h"
#include <iostream>
#include <time.h>


/*
 * Main program :
 */


int main(int argc,char **argv) {
    using namespace PI;
    UsualGPIO p21(21, UsualGPIO::Input);
    UsualGPIO p26(26, UsualGPIO::Output);

    while(1) {
        std::cerr << measure_dist(&p26, &p21) << std::endl;
    }

    return 0;
}

/*********************************************************************
 * End evinput.c - by Warren Gay
 * Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
 * This source code is placed into the public domain.
*/
