#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_ping.h"

#include "serial_io.h"

typedef struct __attribute__((__packed__))
{
	lownet_time_t timestamp_out;
	lownet_time_t timestamp_back;
	uint8_t origin;
} ping_packet_t;


void ping(uint8_t node) {
  lownet_frame_t frame;
  ping_packet_t p;
  uint8_t id = lownet_get_device_id();

  p.origin = id;
  p.timestamp_out = lownet_get_time();

  frame.source = id;
  frame.destination = node;
  frame.protocol = LOWNET_PROTOCOL_PING;
  frame.length = sizeof(p);
  memcpy(frame.payload, &p, sizeof(p));

  lownet_send(&frame);
}


void ping_receive(const lownet_frame_t* frame) {
  if (frame->length < sizeof(ping_packet_t)) {
    // Malformed frame.  Discard.
    return;
  }

  uint8_t id = lownet_get_device_id();
  ping_packet_t p;
  memcpy(&p, frame->payload, sizeof(p));

  if(p.origin == id){
    //gott an awser
    char out[LOWNET_PAYLOAD_SIZE];
    sprintf(out, "Pinged %x at time %ld"".%x got packed back at %ld"".%d",
	    frame->source, p.timestamp_out.seconds, p.timestamp_out.parts,
	    p.timestamp_back.seconds, p.timestamp_back.parts);
    serial_write_line(out);
	  
  }
  else{
    //Got pinged
    lownet_frame_t sendBack;
    lownet_time_t time = lownet_get_time();

    lownet_get_time(time);
	  
    p.timestamp_back = time;
    memcpy(sendBack.payload, &p, sizeof(p));

    sendBack.source = id;
    sendBack.destination = p.origin;
    sendBack.protocol = LOWNET_PROTOCOL_PING;
    sendBack.length = sizeof(p);

    char c[LOWNET_PAYLOAD_SIZE];
    sprintf(c, "Got pinged at %ld"".%d by %x",
	    time.seconds, time.parts, p.origin
	    );
    serial_write_line(c);
    
    lownet_send(&sendBack);
  }

	
}
