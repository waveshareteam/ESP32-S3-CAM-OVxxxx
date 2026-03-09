/*****************************************************************************
 * | File         :   i2c.h
 * | Author       :   Waveshare team
 * | Function     :   Hardware underlying interface
 * | Info         :
 * |                 I2C driver code for I2C communication.
 * ----------------
 * | This version :   V1.0
 * | Date         :   2024-11-26
 * | Info         :   Basic version
 *
 ******************************************************************************/

#ifndef __I2C_H
#define __I2C_H

#include <stdio.h>          // Standard input/output library
#include <string.h>         // String manipulation functions
#include <Wire.h>
#include "esp_log.h"        // ESP32 logging library for debugging

// Define the SDA (data) and SCL (clock) pins for I2C communication
#define EXAMPLE_I2C_MASTER_SDA GPIO_NUM_8  // SDA pin
#define EXAMPLE_I2C_MASTER_SCL GPIO_NUM_7  // SCL pin

/**
 * @brief Initialize the I2C master interface.
 * 
 * This function sets up the I2C master interface, including the SDA/SCL pins,
 * clock frequency, and device address.
 * 
 * @param Addr The I2C address of the device to initialize.
 * @return A handle to the I2C device if initialization is successful, NULL if failure.
 */
void DEV_I2C_Init();

/**
 * @brief Set a new I2C slave address for the device.
 * 
 * This function allows you to change the slave address of an I2C device during runtime.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Addr The new I2C address to set for the device.
 */
void DEV_I2C_Set_Slave_Addr( uint8_t Addr);

/**
 * @brief Write a single byte to the I2C device.
 * 
 * This function sends a command byte and a data byte to the I2C device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send to the device.
 * @param value The value byte to send to the device.
 */
void DEV_I2C_Write_Byte( uint8_t Cmd, uint8_t value);

/**
 * @brief Read a word (2 bytes) from the I2C device.
 * 
 * This function sends a command byte and reads two bytes from the I2C device.
 * It combines the two bytes into a 16-bit word.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send.
 * @return The 16-bit word read from the device.
 */
uint16_t DEV_I2C_Read_Word(uint8_t Cmd);

/**
 * @brief Write multiple bytes to the I2C device.
 * 
 * This function sends a block of data (multiple bytes) to the I2C device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param pdata A pointer to the data to write.
 * @param len The number of bytes to write.
 */
void DEV_I2C_Write_Nbyte(uint8_t *pdata, uint8_t len);

/**
 * @brief Read multiple bytes from the I2C device.
 * 
 * This function sends a command byte and reads multiple bytes from the I2C device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send.
 * @param pdata A pointer to the buffer to store the received data.
 * @param len The number of bytes to read.
 */
void DEV_I2C_Read_Nbyte(uint8_t Cmd, uint8_t *pdata, uint8_t len);

#endif
