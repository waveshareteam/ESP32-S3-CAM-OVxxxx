/*****************************************************************************
 * | File         :   i2c.c
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

#include "i2c.h"  // Include I2C driver header for I2C functions
static const char *TAG = "i2c";  // Define a tag for logging

uint8_t Driver_addr;

/**
 * @brief Initialize the I2C master interface.
 * 
 * This function configures the I2C master bus and adds a device with the specified address.
 * The I2C clock source, frequency, SCL/SDA pins, and other settings are configured here.
 * 
 * @param Addr The I2C address of the device to be initialized.
 * @return The device handle if initialization is successful, NULL otherwise.
 */
void DEV_I2C_Init()
{
    Wire.begin( EXAMPLE_I2C_MASTER_SDA, EXAMPLE_I2C_MASTER_SCL,100000); 
}

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr); 
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    return -1;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return 0;
}




bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);       
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if ( Wire.endTransmission(true))
  {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return -1;
  }
  return 0;
}


bool I2C_Write_Not_REG(uint8_t Driver_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);      
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if ( Wire.endTransmission(true))
  {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return -1;
  }
  return 0;
}

/**
 * @brief Set a new I2C slave address for the device.
 * 
 * This function allows changing the I2C slave address for the specified device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Addr The new I2C address for the device.
 */
void DEV_I2C_Set_Slave_Addr(uint8_t Addr)
{
    // Configure the new device address
    // i2c_device_config_t i2c_dev_conf = { 
    //     .device_address = Addr,                        // Set new device address
    //     .scl_speed_hz = EXAMPLE_I2C_MASTER_FREQUENCY,  // I2C frequency
    // };
    
    // // Update the device with the new address
    // if (i2c_master_bus_add_device(handle.bus, &i2c_dev_conf, dev_handle) != ESP_OK) {
    //     ESP_LOGE(TAG, "I2C address modification failed");  // Log error if address modification fails
    // }

    Driver_addr = Addr;
}

/**
 * @brief Write a single byte to the I2C device.
 * 
 * This function sends a command byte and a value byte to the I2C device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send.
 * @param value The value byte to send.
 */
void DEV_I2C_Write_Byte( uint8_t Cmd, uint8_t value)
{
    // uint8_t data[2] = {Cmd, value};  // Create an array with command and value
    // ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data, sizeof(data), 100));  // Send the data to the device


    Wire.beginTransmission(Driver_addr);
    Wire.write(Cmd);       
    Wire.write(value);

    if ( Wire.endTransmission(true))
    {
        printf("The I2C transmission fails. - I2C Write\r\n");
    }
}


/**
 * @brief Read a word (2 bytes) from the I2C device.
 * 
 * This function reads two bytes (a word) from the I2C device.
 * The data is received by sending a command byte and receiving the data.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send.
 * @return The word read from the device (combined two bytes).
 */
// uint16_t DEV_I2C_Read_Word(i2c_master_dev_handle_t dev_handle, uint8_t Cmd)
// {
//     uint8_t data[2] = {Cmd};  // Create an array with the command byte
//     ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, data, 1, data, 2, 100));  // Send command and receive two bytes
//     return data[1] << 8 | data[0];  // Combine the two bytes into a word (16-bit)
// }
uint16_t DEV_I2C_Read_Word(uint8_t Cmd)
{
    uint8_t data[2] = {0, 0};
    
    // 1. 发送命令/寄存器地址 (相当于 transmit 部分)
    Wire.beginTransmission(Driver_addr);
    Wire.write(Cmd);
    
    // 注意：如果设备要求重复起始条件（Repeated Start），此处应填 false
    if (Wire.endTransmission(false) != 0) {
        printf("I2C Transmit Error\n");
        return 0;
    }

    // 2. 请求 2 字节数据 (相当于 receive 部分)
    uint8_t bytesReceived = Wire.requestFrom(Driver_addr, (uint8_t)2);
    
    if (bytesReceived == 2) {
        data[0] = Wire.read(); // 先读取的通常是低位或高位，取决于设备
        data[1] = Wire.read();
    }

    // 3. 根据原函数逻辑合并：data[1] 为高位，data[0] 为低位
    return (uint16_t)(data[1] << 8 | data[0]);
}
/**
 * @brief Write multiple bytes to the I2C device.
 * 
 * This function sends a block of data to the I2C device.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param pdata Pointer to the data to send.
 * @param len The number of bytes to send.
 */
void DEV_I2C_Write_Nbyte(uint8_t *pdata, uint8_t len)
{
    // 1. 开始向指定地址的从机进行传输
    Wire.beginTransmission(Driver_addr);
    
    // 2. 将数据块写入 I2C 缓冲区
    // Wire.write 可以直接接收缓冲区指针和长度
    size_t bytesAdded = Wire.write(pdata, len);
    
    // 3. 真正发送缓冲区中的数据到总线
    // 默认发送停止信号 (true)
    uint8_t error = Wire.endTransmission(true);
    
    if (error != 0) {
        printf("I2C Write Nbyte failed! Error code: %u\n", error);
    }
}

/**
 * @brief Read multiple bytes from the I2C device.
 * 
 * This function reads multiple bytes from the I2C device.
 * The function sends a command byte and receives the specified number of bytes.
 * 
 * @param dev_handle The handle to the I2C device.
 * @param Cmd The command byte to send.
 * @param pdata Pointer to the buffer where received data will be stored.
 * @param len The number of bytes to read.
 */
void DEV_I2C_Read_Nbyte(uint8_t Cmd, uint8_t *pdata, uint8_t len)
{
    // 1. 发送命令/寄存器地址 (相当于 transmit 部分)
    Wire.beginTransmission(Driver_addr);
    Wire.write(Cmd);
    
    // 使用 false 表示发送重复起始条件（Repeated Start），不释放总线
    // 模拟 i2c_master_transmit_receive 的不间断操作
    if (Wire.endTransmission(false) != 0) {
        printf("I2C Transmit Error before Read Nbyte\n");
        return;
    }

    // 2. 请求 len 个字节的数据 (相当于 receive 部分)
    uint8_t bytesReceived = Wire.requestFrom(Driver_addr, len);
    
    if (bytesReceived == len) {
        for (uint8_t i = 0; i < len; i++) {
            pdata[i] = Wire.read();
        }
    } else {
        printf("I2C Read Nbyte: Requested %d bytes but received %d\n", len, bytesReceived);
    }
}