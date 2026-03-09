#include "touch_drive.h"
#include "io_extension.h"
#include "Touch_CST816.h"


/* Reset controller */
static uint8_t Touch_Reset(void)
{
  IO_EXTENSION_Output(IO_EXTENSION_IO_0, 0);
  vTaskDelay(pdMS_TO_TICKS(50));
  IO_EXTENSION_Output(IO_EXTENSION_IO_0, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}

void touch_driver_init(void)
{
  Touch_Reset();

  cst816_touch_init();

}


void touch_read_data(struct Touch_Data *touch_data)
{
  CST816_touch_read_data(touch_data);
}

/*!
    @brief  get the gesture event name
*/
String Touch_GestureName(struct Touch_Data *touch_msg) 
{
  switch (touch_msg->gesture) 
  {
    case NONE:
      return "NONE";
      break;
    case SWIPE_DOWN:
      return "SWIPE DOWN";
      break;
    case SWIPE_UP:
      return "SWIPE UP";
      break;
    case SWIPE_LEFT:
      return "SWIPE LEFT";
      break;
    case SWIPE_RIGHT:
      return "SWIPE RIGHT";
      break;
    case SINGLE_CLICK:
      return "SINGLE CLICK";
      break;
    case DOUBLE_CLICK:
      return "DOUBLE CLICK";
      break;
    case LONG_PRESS:
      return "LONG PRESS";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}