#include "peripheral.h"



void battery_task(void *param)
{
    vTaskSuspend(battery_handle);
    while (1)
    {
        delay(1000);
    }
}

