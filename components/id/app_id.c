#include <stdint.h>
#include <string.h>

#include "lownet.h"
#include "serial_io.h"
#include "app_id.h"

int ON = 0;
uint8_t FAKE_ID = 0x00;

void setId_command(char* msg){
  FAKE_ID = strtol(msg+2, NULL, 16);
}

uint8_t getId(){
  if(ON){
    return FAKE_ID;
  }
  else{
    return lownet_get_device_id();
  }
}

void idOnOff(){
  if(ON){
    ON = 0;
    serial_write_line("fake id off");
  }
  else{
    ON = 1;
    serial_write_line("fake id on");
  }
}
