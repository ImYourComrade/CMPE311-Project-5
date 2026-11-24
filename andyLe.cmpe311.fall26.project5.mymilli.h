#ifndef MYMILLI_H
#define MYMILLI_H

#include <stdint.h>

#ifdef __cplusplus // wraps around C 
extern "C" {
#endif

// intializes registers within mymilli during setup 
void init_mymilli(void);

// function to get elapsed milliseconds
unsigned long mymilli(void);

#ifdef __cplusplus
}
#endif

#endif 