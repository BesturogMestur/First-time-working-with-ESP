#include "crane.h"

#include <string.h>

#include <esp_log.h>

#include <lownet.h>
#include <utility.h>
#include <serial_io.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define CRANE_PROTO 0x05

#define TAG "crane"

void crane_connect(uint8_t id);
void crane_disconnect();
int  crane_action(uint8_t action); // returns zero if ACK is received
void crane_test(uint8_t id);
void crane_receive(const lownet_frame_t* frame);
void crane_send(uint8_t destination, const crane_packet_t* packet);

conn_t bitChallenge(conn_t conn);

// state of a single flow
static struct
{
	uint16_t seq;
	uint8_t crane;
	QueueHandle_t acks;
	enum
		{
			ST_DISCONNECTED,
			ST_HANDSHAKE,
			ST_CONNECTED,
		} state;
} state;

int crane_init(void)
{
	if (lownet_register_protocol(CRANE_PROTO, crane_receive) != 0)
		{
			ESP_LOGE(TAG, "Failed to register crane protocol");
			return 1;
		}

	state.seq = 0;
	state.crane = 0;
	state.state = ST_DISCONNECTED;
	state.acks = xQueueCreate(8, sizeof(uint16_t));
	return 0;
}

void crane_command(char* args)
{
	if (!args)
		{
			serial_write_line("Missing argument COMMAND");
			return;
		}
	char* saveptr;

	char* command = strtok_r(args, " ", &saveptr);
	if (!command)
		{
			serial_write_line("Missing argument COMMAND");
			return;
		}

	if (strcmp(command, "open") == 0)
		{
			char* id = strtok_r(NULL, " ", &saveptr);
			if (!id)
				{
					serial_write_line("Missing argument ID");
					return;
				}
			uint8_t dest = hex_to_dec(id + 2);
			crane_connect(dest);
		}
	else if (strcmp(command, "close") == 0)
		{
			crane_disconnect();
		}
	else if (strcmp(command, "test") == 0)
		{
			char* id = strtok_r(NULL, " ", &saveptr);
			if (!id)
				{
					serial_write_line("Missing argument ID");
					return;
				}
			uint8_t dest = hex_to_dec(id + 2);
			crane_test(dest);
		}
	else
		{
		  //set action as 0 for now so I can test milestone I
			uint8_t action = 0;
			switch (command[0])
				{
					// ------------------------------------------------
					// Milestone II, Task 2: implement commands to CLI
				case 'f':  // FORWARD, call crane_action with appropriate action!
					// your code goes here!
					break;
				// repeat the above for all commands!
				// ------------------------------------------------
				default:
					ESP_LOGI(TAG, "Invalid crane command");
					return;
				}
			crane_action(action);
		}
}

void crane_recv_connect(const crane_packet_t* packet)
{
  ESP_LOGI(TAG, "Received CONNECT packet");
  if (state.state != ST_HANDSHAKE)
    return;

  ESP_LOGI(TAG, "packet flags: %02x", packet->flags);

  crane_packet_t outpkt;

  if(
     (packet->flags & CRANE_NAK)
     |
     ((packet->flags & (CRANE_SYN | CRANE_ACK)) != 0b11)
    )
    return;

  outpkt.type = CRANE_CONNECT;
  outpkt.flags = CRANE_ACK;
  outpkt.seq = 0;
  outpkt.d.conn = bitChallenge(packet->d.conn);
    
  crane_send(state.crane, &outpkt);
}

void crane_recv_close(const crane_packet_t* packet)
{
	ESP_LOGI(TAG, "Closing connection");
	state.seq = 0;
	state.state = ST_DISCONNECTED;
	state.crane = 0;
}

void crane_recv_status(const crane_packet_t* packet)
{
	char buffer[200];

	if ( packet->flags & CRANE_NAK )	// crane missed some packet
		{
			// Not in use yet -- anywhere, so you can ignore
			ESP_LOGI(TAG, "Received status packet with NAK -- not in use yet" );
			return;
		}
	else // push the cumulative ack to ACK-queue
		xQueueSend( state.acks, &packet->seq, 0 );

	snprintf(buffer, sizeof buffer,
					 "backlog: %d\n"
					 "time: %d\n"
					 "light: %s\n",
					 packet->d.status.backlog,
					 packet->d.status.time_left,
					 packet->d.status.light ? "on" : "off");
	serial_write_line(buffer);
}

void crane_receive(const lownet_frame_t* frame)
{
	crane_packet_t packet;
	memcpy(&packet, frame->payload, sizeof packet);
	ESP_LOGI(TAG, "Received packet frame from %02x, type: %d", frame->source, packet.type);
	switch (packet.type)
		{
		case CRANE_CONNECT:
			crane_recv_connect(&packet);
			break;
		case CRANE_STATUS:
			crane_recv_status(&packet);
			break;
		case CRANE_ACTION:
			break;
		case CRANE_CLOSE:
			crane_recv_close(&packet);
		}
}

/*
 * This function starts the connection establishment
 * procedure by sending a SYN packet to the given node.
 */
void crane_connect(uint8_t id)
{
	if (state.state != ST_DISCONNECTED)
		return;

	crane_packet_t packet;
	packet.type = CRANE_CONNECT;
	packet.flags = CRANE_SYN;
	packet.seq = state.seq;

	state.crane = id;
	state.state = ST_HANDSHAKE;
	++state.seq;

	crane_send(id, &packet);
}

void crane_disconnect(void)
{
  crane_packet_t packet;

  if(state.state == ST_DISCONNECTED)
    return;
  
  packet.type = CRANE_CLOSE;
  packet.flags = CRANE_ACK;
  packet.d.close = 0;
    
  crane_send(state.crane, &packet);
  
  state.state = ST_DISCONNECTED;
  state.seq = 0;
}


/*
 *	Subroutine for crane_action: read ACKs from crane, blocks for some time
 */
uint16_t read_acks(void)
{
	uint16_t seq, x;

	// Wait for an ack up to 5 seconds
	if ( xQueueReceive(state.acks, &seq, 5000/portTICK_PERIOD_MS) != pdTRUE )
		seq = 0;
	// read any other acks if in the queue
	while ( xQueueReceive(state.acks, &x, 0) == pdTRUE )
		seq = seq >= x ? seq : x;
	return seq;
}

/*
 *	This can block for a while if no immediate ACK
 */
int crane_action(uint8_t action)
{
	crane_packet_t packet;

	packet.type = CRANE_ACTION;
	packet.seq = state.seq;
	packet.d.action.cmd = action;

	crane_send(state.crane, &packet);

	// ------------------------------------------------
	// Milestone II, Task 1: run the following up to five times
	// Your code goes here
	{
		uint16_t seq = read_acks( );	 // seq is the cumulative ack from crane, or zero if none

		// If seq > state.seq, report an error and close the connection, return -2
		// - Else if seq == state.seq, we are good, increment state.seq by one (why?!) and return 0
		// - Otherwise retransmit
	}
	// ------------------------------------------------

	// No ack received, disconnect
	ESP_LOGI(TAG, "Received no ack from node=0x%02x", state.crane );
	crane_disconnect();
	return -1;
}


// ------------------------------------------------
//
// Milestone III: run the test pattern
//
// 1. establish connection with TEST flag
// 2. run the test pattern according to the specs
// 3. close the connection
//
// Hint: you can work it all out here slowly, or be a wizard
//       and launch a separate task for this!
//
void crane_test(uint8_t id)
{
}
// ------------------------------------------------


void crane_send(uint8_t id, const crane_packet_t* packet)
{
	lownet_frame_t frame;
	frame.destination = id;
	frame.protocol = CRANE_PROTO;
	frame.length = sizeof *packet;
	memcpy(frame.payload, packet, sizeof *packet);

	lownet_send(&frame);
}

//use : bitChallenge(conn);
//pre : conn is an initiated instan of conn_t struct,
//	conn can not be NULL,
//post: Return a new instan of conn after the bit
//	Challenge has been performd.
conn_t bitChallenge(const conn_t conn){
  conn_t out;
  out.challenge = ~(conn.challenge);

  return out;
}
