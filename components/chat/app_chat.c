#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#include "lownet.h"
#include "serial_io.h"
#include "utility.h"
#include "app_id.h"

#include "app_chat.h"

void shout_command(char* args)
{
	chat_shout(args);
}

void tell_command(char* args)
{
	char* dest = strtok(args, " ");
	char* message = strtok(NULL, "\n");

	uint8_t d = (uint8_t) hex_to_dec(dest + 2);
	if (d == 0)
		{
			serial_write_line("Invalid node id\n");
			return;
		}

	if (!message)
		{
			serial_write_line("A message must be provided\n");
			return;
		}

	chat_tell(message, d);
}

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
    memcpy(des, payload, l);
    des[l] = '\0';
}

void chat_receive(const lownet_frame_t* frame) {
  char out[LOWNET_PAYLOAD_SIZE+1];
  char from[LOWNET_PAYLOAD_SIZE];
  
  if (frame->destination == lownet_get_device_id()) {
    revPayload(frame->payload, out, frame->length);

    sprintf(from, "0x%x tell:", frame->source);
    serial_write_line(from);
    serial_write_line(out);
	  
  }
  else if(SNOOP){
    revPayload(frame->payload, out, frame->length);
    
    sprintf(from, "0x%x tells 0x%x:", frame->source, frame->destination);
    serial_write_line(from);
    serial_write_line(out);
  }
  else {
    revPayload(frame->payload, out, frame->length);

    sprintf(from, "0x%x shout:", frame->source);
    serial_write_line(from);
    serial_write_line(out);
  }
}

// Usage: chat_valid_message(MESSAGE)
// Pre:   MESSAGE != NULL
// Value: strlen(MESSAGE) if MESSAGE is a valid message, 0 otherwise
size_t chat_valid_message(const char* message)
{
	size_t i = 0;
	/*
		Loop Invariant:
		0 <= i <= strlen(message)
		forall x | 0 <= x < i : util_printable(message[x])
	 */
	for (; message[i]; i++)
		{
			if (!util_printable(message[i]))
				return 0;
		}

	if (i > LOWNET_PAYLOAD_SIZE)
		return 0;


	return i;
}

void chat_shout(const char* message) {
  chat_tell(message, 0xff);
}

void chat_tell(const char* message, uint8_t destination) {
  size_t l = 0;
  if (!(message && (l = chat_valid_message(message))))
    return;

  lownet_frame_t out;

  out.source = getId();
  out.destination = destination;
  out.protocol = LOWNET_PROTOCOL_CHAT;
  out.length = l;
  memcpy(out.payload, message, l);

  //out.payload[l] = '\0' this maybe needed

  lownet_send(&out);
}

//use : updateSnoop();
//pre : none
//post: SNOOP will be turned on or off
void updateSnoop(){
  if(SNOOP){
    serial_write_line("off");
    SNOOP = 0;
  }
  else{
    serial_write_line("on");
    SNOOP = 1;
  }
}
