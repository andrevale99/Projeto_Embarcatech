#ifndef PERSONAL_DEFS_H
#define PERSONAL_DEFS_H

#include "hardware/clocks.h"
#include "hardware/timer.h"

// Macro para o delay do TIMER sem travar a CPU
#define DELAY_MS(ms, call_back, flag)            \
    add_alarm_in_ms(ms, call_back, NULL, false); \
    while (!flag)                                \
        tight_loop_contents();                   \
    flag = false;

struct DataHouse_t
{
    uint16_t TempSensor;
    uint16_t UmidadeSolo;

    uint16_t Portao;
} data;

#endif