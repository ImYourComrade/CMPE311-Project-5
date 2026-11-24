#ifndef FAN_H
#define FAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



void init_myfan(void);
void init_myfanbutton(void);
uint8_t fanButtonPress(void);
uint8_t fanButtonPressRTOS(void);
void advanceFanCycle(void);
void setFanSpeed(uint8_t);

#ifdef __cplusplus
}
#endif

#endif