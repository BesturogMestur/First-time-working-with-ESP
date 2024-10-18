#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_ping.h"

#include "serial_io.h"
#include "utility.h"


void ping_command(char* args)
{
  if (!args)
    {
      serial_write_line("A node id must be provided\n");
      return;
    }

  uint8_t dest = (uint8_t) hex_to_dec(args + 2);
  if (dest == 0)
    {
      serial_write_line("Invalid node id\n");
      return;
    }
  ping(dest, NULL, 0);
}

void ping(uint8_t node, const uint8_t* payload, uint8_t length)
{ 
  lownet_frame_t frame;
  frame.source = lownet_get_device_id();
  frame.destination = node;
  frame.protocol = LOWNET_PROTOCOL_PING;

  ping_packet_t packet;
  packet.timestamp_out = lownet_get_time();
  packet.origin = lownet_get_device_id();

  memcpy(&frame.payload, &packet, sizeof packet);


  if (payload)
    {
      memcpy(frame.payload + frame.length,
	     payload,
	     min(LOWNET_PAYLOAD_SIZE - sizeof(ping_packet_t), length));
      frame.length += min(LOWNET_PAYLOAD_SIZE - sizeof(ping_packet_t), length);
    }
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
    serial_write_line("p4");
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
