#ifndef GUARD_APP_ID_H
#define GUARD_APP_ID_H

#include <stdint.h>

extern int ON;
extern uint8_t FAKE_ID;

void setId(uint8_t fakeID);
uint8_t getId();
void idOnOff();

#endif
