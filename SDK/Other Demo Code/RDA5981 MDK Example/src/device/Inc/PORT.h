#ifndef __PORT_H
#define __PORT_H

#include "GPIO.h"


typedef struct _port_t 
{
    PortName port;
    uint32_t mask;
    __IO uint32_t *reg_out;
    __I  uint32_t *reg_in;
    __IO uint32_t *reg_dir;
}port_t;



#ifdef __cplusplus
extern "C" {
#endif



/** Get the pin name from the port's pin number
 *
 * @param port  The port name
 * @param pin_n The pin number within the specified port
 * @return The pin name for the port's pin number
 */
PinName port_pin(PortName port, int pin_n);

/** Initilize the port
 *
 * @param obj  The port object to initialize
 * @param port The port name
 * @param mask The bitmask to identify which bits in the port should be included (0 - ignore)
 * @param dir  The port direction
 */
void port_init(port_t *obj, PortName port, int mask, PinDirection dir);

/** Set the input port mode
 *
 * @param obj  The port object
 * @param mode THe port mode to be set
 */
void port_mode(port_t *obj, PinMode mode);

/** Set port direction (in/out)
 *
 * @param obj The port object
 * @param dir The port direction to be set
 */
void port_dir(port_t *obj, PinDirection dir);

/** Write value to the port
 *
 * @param obj   The port object
 * @param value The value to be set
 */
void port_write(port_t *obj, int value);

/** Read the current value on the port
 *
 * @param obj The port object
 * @return An integer with each bit corresponding to an associated port pin setting
 */
int port_read(port_t *obj);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
