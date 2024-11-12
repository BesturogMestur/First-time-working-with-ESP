#include <stdint.h>
#include <string.h>
#include <stdio.h>

//include for RSA
#include "mbedtls/pk.h"

#include "serial_io.h"
#include "command.h"
#include "lownet.h"
#include "app_ping.h"

#define TAG "COMMAND"

cmd_packet_t msg;
cmd_signature_t sig_1;
cmd_signature_t sig_2;

uint64_t sequence = 0;
int sate_msg;
int sate_sig_1;
int sate_sig_2;
uint8_t hash[CMD_HASH_SIZE];
uint8_t length;

lownet_time_t time;


void command_init()
{
  sate_msg = 0;
  sate_sig_1 = 0;
  sate_sig_2 = 0;
  time = lownet_get_time();
  
}
//use : proessMsg(frame);
//pre : frame is constan poiner that points to a
//	lownet_frame_t struct.
//post: If frame is valed the paylod of the frame is
//	set into msg.
int proessMsg(const lownet_frame_t* frame){
  cmd_packet_t packet;
  memcpy(&packet, frame->payload, sizeof packet);

  if(packet.sequence <= sequence) return 1;

  if(packet.type <= 0 && packet.type > 2 ){
    printf("Invalied type of command\n");
    return 1;
  }

  memcpy(&msg, &packet, sizeof packet);
    
  mbedtls_sha256((unsigned char*) frame, LOWNET_FRAME_SIZE, (unsigned char*) hash, 0);
  sate_msg = 1;
  length = frame->length;
  return 0;
}

//use : prossesSig(frame, type);
//pre : frame is constan poiner that points to a
//	lownet_frame_t struct.
//	type is an uint8_t number, 0 < type <= 2.
//post: If frame is valid then sig_x is set where
//	x = type.
int proessSig(const lownet_frame_t* frame, uint8_t type){
  cmd_signature_t block;
  memcpy(&block, frame->payload, sizeof block);

  uint8_t temp[CMD_HASH_SIZE];
  mbedtls_sha256((unsigned char*) lownet_public_key, strlen(lownet_public_key)
		 , (unsigned char*) temp, 0
		);
  
  if(
     !sate_msg
     ||
     memcmp(block.hash_key, temp, CMD_HASH_SIZE)
     ||
     memcmp(hash, block.hash_msg, CMD_HASH_SIZE)
     ){
    return 1;
  }

  if(type == 2){
    memcpy(&sig_1, &block, sizeof block);
    sate_sig_1 = 1;
  }
  else if(type == 3){
    memcpy(&sig_2, &block, sizeof block);
    sate_sig_2 = 1;
  }
  else return 1;

  return 0;
}

//use : sigVailid();
//pre : none
//post: returns 0 if the RSA signature is valid
int sigVailid(){
  mbedtls_pk_context pk;
  uint8_t sig_part[CMD_BLOCK_SIZE];
  uint8_t result[CMD_BLOCK_SIZE];

  memcpy(sig_part, sig_1.sig_part, CMD_BLOCK_SIZE/2);
  memcpy(sig_part+CMD_BLOCK_SIZE/2, sig_2.sig_part, CMD_BLOCK_SIZE/2);

  mbedtls_pk_init(&pk);
  mbedtls_pk_parse_public_key(&pk,
			      (unsigned char*)lownet_public_key,
			      strlen(lownet_public_key) + 1
			     );
  
  if(mbedtls_rsa_public(mbedtls_pk_rsa(pk), sig_part, result)){
    printf("Invalied signitrue\n");
    mbedtls_pk_free(&pk);
    return 1;
  }

  mbedtls_pk_free(&pk);

  uint8_t zero[220];
  uint8_t one[4];

  memset(zero, 0, 220);
  memset(one, 1, 4);

  if(memcmp(result, zero, 220)
     ||
     memcmp(result+220, one, 4)
     ||
     memcmp(result+224, hash, CMD_HASH_SIZE)
    ) return 1;

  return 0;
}

//use : commadProsses(dec)
//pre : dec is a uint8_t number
//post: will do the commad if is a vailed type
//	of commad and well send to the node with
//	id = dec if nessesary.
int commadProsses(uint8_t dec){
  if(msg.type == 1){
    lownet_time_t newTime;
    memcpy(&newTime, msg.contents, sizeof newTime);
    lownet_set_time(&newTime);
  }
  else if(msg.type == 2){
      
    uint8_t payload[LOWNET_PAYLOAD_SIZE];
    memcpy(payload, msg.contents, length-12);

    ping(dec, payload, length-12);
  }
  else return 1;

  return 0;
}

void command_receive(const lownet_frame_t* frame)
{  
 
  uint8_t type = frame->protocol >> 6;
  if(type == 0) goto stop;
  
  if(lownet_get_time().seconds - time.seconds >= 10){
    command_init();
  }

  if(type == 1){
    if(proessMsg(frame)) goto stop;
  }
  else{	   
    if(proessSig(frame, type)) goto stop;
  }

  if(sate_msg && sate_sig_1 && sate_sig_2){
    if(sigVailid()) goto fullClear;
    
    if(commadProsses(frame->source)) goto fullClear;

    sequence = msg.sequence;

    fullClear:
    command_init();
    goto stop;
  }

  time = lownet_get_time();

  stop:
  return;
}
