#include "Touch_CST816.h"

static uint16_t CST816_Read_cfg(void);
static void CST816_AutoSleep(bool Sleep_State);

// I2C
static bool I2C_Read_Touch(uint16_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr); 
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    return true;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return true;
}

static bool I2C_Write_Touch(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);       
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if(Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Write\r\n");
    return false;
  }
  return true;
}
uint8_t cst816_touch_init(void) 
{
  uint16_t Verification = CST816_Read_cfg();
  CST816_AutoSleep(true);
  return true;
}

static uint16_t CST816_Read_cfg(void) {

  uint8_t buf[3];
  I2C_Read_Touch(CST816_ADDR, CST816_REG_Version,buf, 1);
  printf("TouchPad_Version:0x%02x\r\n", buf[0]);
  I2C_Read_Touch(CST816_ADDR, CST816_REG_ChipID, buf, 3);
  printf("ChipID:0x%02x   ProjID:0x%02x   FwVersion:0x%02x \r\n",buf[0], buf[1], buf[2]);

  return true;
}
/*!
    @brief  Fall asleep automatically
*/
static void CST816_AutoSleep(bool Sleep_State) 
{
  uint8_t Sleep_State_Set = (uint8_t)(!Sleep_State);
  Sleep_State_Set = 10;
  I2C_Write_Touch(CST816_ADDR, CST816_REG_DisAutoSleep, &Sleep_State_Set, 1);
}


// reads sensor and touches
// updates Touch Points
uint8_t CST816_touch_read_data(struct Touch_Data *touch_data) 
{
    uint8_t buf[6];
    I2C_Read_Touch(CST816_ADDR, CST816_REG_GestureID, buf, 6);
    
    // 触摸手势处理（与原代码完全一致）
    if (buf[0] != 0x00) 
        touch_data->gesture = (GESTURE)buf[0];
    
    // 触摸点处理（与原代码完全一致） 
    if (buf[1] != 0x00) {        
        noInterrupts(); 
        // 触摸点数量 
        touch_data->points = (uint8_t)buf[1];
        if(touch_data->points > TOUCH_MAX_POINTS)
            touch_data->points = TOUCH_MAX_POINTS;
        // 坐标填充 
        touch_data->coords[0].x = ((buf[2] & 0x0F) << 8) + buf[3];               
        touch_data->coords[0].y = ((buf[4] & 0x0F) << 8) + buf[5];
        
        interrupts(); 
    }
    
    return true;
}


