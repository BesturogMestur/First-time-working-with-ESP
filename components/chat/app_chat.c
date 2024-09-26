#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#include "lownet.h"
#include "serial_io.h"

#include "app_chat.h"

//use : revPayload(payload, des, l);
//pre : payload is a 8 bit unsingt pointer and des is
//	a charter pointer and l is intger.
//	payload points to the sorce of the data being
//	resived,
//	des is the destinson of the data,
//	l is the size of the data.
//post: The first l byts of payloand has been copyed into
//	des.
void revPayload(const uint8_t* payload, char* des, int l){
  if(*(payload+l) == '\0'){
    memcpy(des, payload, l);
  }
  else {
    memcpy(des, payload, l);
    des[l+1] = '\0';
  }
}

void chat_receive(const lownet_frame_t* frame) {
  char out[LOWNET_PAYLOAD_SIZE];
  char from[LOWNET_PAYLOAD_SIZE];
  
  if (frame->destination == lownet_get_device_id()) {
    revPayload(frame->payload, out, frame->length);

    sprintf(from, "0x%x tell:", frame->source);
    serial_write_line(from);
    serial_write_line(out);
	  
  }else {
    revPayload(frame->payload, out, frame->length);

    sprintf(from, "0x%x shout:", frame->source);
    serial_write_line(from);
    serial_write_line(out);
  }
}


void chat_shout(const char* message) {
  chat_tell(message, 0xff);
}

void chat_tell(const char* message, uint8_t destination) {
  lownet_frame_t out;

 
  int l = strlen(message)+1;

  out.source = lownet_get_device_id();
  out.destination = destination;
  out.protocol = LOWNET_PROTOCOL_CHAT;
  out.length = l;
  memcpy(out.payload, message, l);
  

  lownet_send(&out);
}
