#include <stdint.h>

#include "lownet.h"
#include "serial_io.h"
#include "app_id.h"

int ON = 0;
uint8_t FAKE_ID = 0x00;

void setId(uint8_t fakeID){
  FAKE_ID = fakeID;
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
