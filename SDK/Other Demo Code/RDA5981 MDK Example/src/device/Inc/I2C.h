#ifndef __I2C_H
#define __I2C_H

#include "GPIO.h"

#if DEVICE_I2C_ASYNCH
#include "dma_api.h"
#endif

/**
 * @defgroup hal_I2CEvents I2C Events Macros
 *
 * @{
 */
#define I2C_EVENT_ERROR               (1 << 1)
#define I2C_EVENT_ERROR_NO_SLAVE      (1 << 2)
#define I2C_EVENT_TRANSFER_COMPLETE   (1 << 3)
#define I2C_EVENT_TRANSFER_EARLY_NACK (1 << 4)
#define I2C_EVENT_ALL                 (I2C_EVENT_ERROR |  I2C_EVENT_TRANSFER_COMPLETE | I2C_EVENT_ERROR_NO_SLAVE | I2C_EVENT_TRANSFER_EARLY_NACK)

/**@}*/


enum {
  I2C_ERROR_NO_SLAVE = -1,
  I2C_ERROR_BUS_BUSY = -2
};


typedef enum {
    I2C_0  = (int)RDA_I2C0_BASE
}I2CName;


typedef struct _i2c_t 
{
    RDA_I2C0_TypeDef *i2c;
}i2c_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hal_GeneralI2C I2C Configuration Functions
 * @{
 */

/** Initialize the I2C peripheral. It sets the default parameters for I2C
 *  peripheral, and configures its specifieds pins.
 *  
 *  @param obj  The I2C object
 *  @param sda  The sda pin
 *  @param scl  The scl pin
 */
void i2c_init(i2c_t *obj, PinName sda, PinName scl);

/** Configure the I2C frequency
 *
 *  @param obj The I2C object
 *  @param hz  Frequency in Hz
 */
void i2c_frequency(i2c_t *obj, int hz);

/** Send START command
 *
 *  @param obj The I2C object
 */
int  i2c_start(i2c_t *obj);

/** Send STOP command
 *
 *  @param obj The I2C object
 */
int  i2c_stop(i2c_t *obj);

/** Blocking reading data
 *
 *  @param obj     The I2C object
 *  @param address 7-bit address (last bit is 1)
 *  @param data    The buffer for receiving
 *  @param length  Number of bytes to read
 *  @param stop    Stop to be generated after the transfer is done
 *  @return Number of read bytes
 */
int i2c_read(i2c_t *obj, int address, char *data, int length, int stop);

/** Blocking sending data
 *
 *  @param obj     The I2C object
 *  @param address 7-bit address (last bit is 0)
 *  @param data    The buffer for sending
 *  @param length  Number of bytes to write
 *  @param stop    Stop to be generated after the transfer is done
 *  @return Number of written bytes
 */
int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop);

/** Reset I2C peripheral. TODO: The action here. Most of the implementation sends stop()
 *
 *  @param obj The I2C object
 */
void i2c_reset(i2c_t *obj);

/** Read one byte
 *
 *  @param obj The I2C object
 *  @param last Acknoledge
 *  @return The read byte
 */
int i2c_byte_read(i2c_t *obj, int last);

/** Write one byte
 *
 *  @param obj The I2C object
 *  @param data Byte to be written
 *  @return 0 if NAK was received, 1 if ACK was received, 2 for timeout.
 */
int i2c_byte_write(i2c_t *obj, int data);

/**@}*/

#if DEVICE_I2CSLAVE

/**
 * \defgroup SynchI2C Synchronous I2C Hardware Abstraction Layer for slave
 * @{
 */

/** Configure I2C as slave or master.
 *  @param obj The I2C object
 *  @return non-zero if a value is available
 */
void i2c_slave_mode(i2c_t *obj, int enable_slave);

/** Check to see if the I2C slave has been addressed.
 *  @param obj The I2C object
 *  @return The status - 1 - read addresses, 2 - write to all slaves,
 *         3 write addressed, 0 - the slave has not been addressed
 */
int  i2c_slave_receive(i2c_t *obj);

/** Configure I2C as slave or master.
 *  @param obj The I2C object
 *  @return non-zero if a value is available
 */
int  i2c_slave_read(i2c_t *obj, char *data, int length);

/** Configure I2C as slave or master.
 *  @param obj The I2C object
 *  @return non-zero if a value is available
 */
int  i2c_slave_write(i2c_t *obj, const char *data, int length);

/** Configure I2C address.
 *  @param obj     The I2C object
 *  @param idx     Currently not used
 *  @param address The address to be set
 *  @param mask    Currently not used
 */
void i2c_slave_address(i2c_t *obj, int idx, uint32_t address, uint32_t mask);

#endif


#ifdef __cplusplus
}
#endif

#endif

