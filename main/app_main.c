// CSTDLIB includes.
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// LowNet includes.
#include "lownet.h"

#include "serial_io.h"
#include "utility.h"

#include "app_chat.h"
#include "app_ping.h"

const char* ERROR_OVERRUN = "ERROR // INPUT OVERRUN";
const char* ERROR_UNKNOWN = "ERROR // PROCESSING FAILURE";

const char* ERROR_COMMAND = "Command error";
const char* ERROR_ARGUMENT = "Argument error";

void app_frame_dispatch(const lownet_frame_t* frame) {
  switch(frame->protocol) {
  case LOWNET_PROTOCOL_RESERVE:
    // Invalid protocol, ignore.
    break;
  case LOWNET_PROTOCOL_TIME:
    // Not handled here.	Time protocol is special case
    break;
  case LOWNET_PROTOCOL_CHAT:
    chat_receive(frame);
    break;

  case LOWNET_PROTOCOL_PING:
    ping_receive(frame);
    break;
  }
}

void time(char* out){
  lownet_time_t time = lownet_get_time();

  if(time.seconds != 0){
    sprintf(out, "%ld"".%d sec since the course started.",
	    time.seconds, time.parts
	    );
      }
  else{
    strncpy(out, "Network time is not available.", 31);
  }
  serial_write_line(out);
}

void app_main(void)
{
  char msg_in[MSG_BUFFER_LENGTH];
  char msg_out[MSG_BUFFER_LENGTH];

  // Initialize the serial services.
  init_serial_service();

  // Initialize the LowNet services.
  lownet_init(app_frame_dispatch);

  while (true) {
    memset(msg_in, 0, MSG_BUFFER_LENGTH);
    memset(msg_out, 0, MSG_BUFFER_LENGTH);

    if (!serial_read_line(msg_in)) {
      if (msg_in[0] == '/') {
	char* p = msg_in+1;
	if(!strncmp(p, "ping", 4))
	  {
	    ping((uint8_t)strtol(p+7, NULL, 16));
	  }
	else if(!strncmp(p, "date", 4)){
	  time(msg_out);
	}
	else {
	  serial_write_line("Command error");
	}
	    
      } else if (msg_in[0] == '@') {
	char* p = msg_in+3;
	chat_tell(p+2, (uint8_t)strtol(strtok(p," "), NULL, 16));
      } else {
	chat_shout(msg_in);
      }
    }
  }
}
