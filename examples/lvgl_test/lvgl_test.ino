
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl.h"
#include "lv_demo_benchmark.h"
#include "lv_demo_stress.h"


/* Uncomment and select one to start testing */
#define DEMO_BENCHMARK_TEST_EN 1
// #define DEMO_STRESS_TEST_EN 1

void setup(void)
{
    Serial.begin(115200);
    int start_delay = 2;
    while (start_delay) {
        Serial.print(start_delay);
        delay(1000);
        start_delay--;
    }

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* Select one to run */ 
#if (DEMO_BENCHMARK_TEST_EN == 1)
    lv_demo_benchmark();
#endif

#if (DEMO_STRESS_TEST_EN == 1)
    lv_demo_stress();
#endif
}

void loop(void)
{
    lv_timer_handler();
}

