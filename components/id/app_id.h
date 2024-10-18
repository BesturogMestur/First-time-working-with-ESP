#ifndef GUARD_APP_ID_H
#define GUARD_APP_ID_H

#include <stdint.h>

extern int ON;
extern uint8_t FAKE_ID;

void setId_command(char* msg);
uint8_t getId();
void idOnOff();

#endif
