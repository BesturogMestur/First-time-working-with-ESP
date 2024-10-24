#include <stdint.h>
#include <string.h>
#include <stdio.h>

//include for RSA
#include "mbedtls/pk.h"

#include "serial_io.h"
#include "command.h"
#include "lownet.h"
#include "app_ping.h"

cmd_packet_t msg;
cmd_signature_t sig_1;
cmd_signature_t sig_2;

uint64_t sequence = 0;
int sate_msg;
int sate_sig_1;
int sate_sig_2;

lownet_time_t time;


void command_init()
{
  sate_msg = 0;
  sate_sig_1 = 0;
  sate_sig_2 = 0;
  time = lownet_get_time();
  
}

void command_receive(const lownet_frame_t* frame)
{  
  
  uint8_t type = frame->protocol >> 6;
  if(type == 0) goto stop;

  if(lownet_get_time().seconds - time.seconds >= 10){
    command_init();
  }

  if(type == 1){
    cmd_packet_t packet;
    memcpy(&packet, frame->payload, sizeof packet);

    if(packet.sequence <= sequence) goto stop;

    if(msg.type == 0 && msg.type > 2 ){
      printf("Invalied type of command\n");
      goto stop;
    }

    memcpy(&msg, &packet, sizeof packet);
    sate_msg = 1;
  }
  else{
    cmd_signature_t block;
    memcpy(&block, frame->payload, sizeof block);

    uint8_t temp[CMD_HASH_SIZE];
    mbedtls_sha256((unsigned char*) lownet_public_key, LOWNET_KEY_SIZE_RSA, (unsigned char*) temp, 0);
    
    if(strncmp((char*) block.hash_key, (char*) temp, CMD_HASH_SIZE))
      goto stop;

    if(type == 2){
      memcpy(&sig_1, &block, sizeof block);
      sate_sig_1 = 1;
    }
    else if(type == 3){
      memcpy(&sig_2, &block, sizeof block);
      sate_sig_2 = 1;
    }
    else goto stop;
  }

  if(sate_msg && sate_sig_1 && sate_sig_2){
    mbedtls_pk_context pk;
    uint8_t sig_part[CMD_BLOCK_SIZE];
    uint8_t result[MBEDTLS_MPI_MAX_SIZE];

    memcpy(sig_part, sig_1.sig_part, CMD_HASH_SIZE);
    memcpy(sig_part+CMD_HASH_SIZE, sig_2.sig_part, CMD_HASH_SIZE);

    mbedtls_pk_init(&pk);
    mbedtls_pk_parse_public_keyfile(&pk, lownet_public_key);

    if(!mbedtls_rsa_public(mbedtls_pk_rsa(pk), sig_part, result)){
      printf("Invalied signitrue\n");
      goto fullClear;
    }

    
    if(msg.type == 1){
      lownet_time_t newTime;
      memcpy(&newTime, msg.contents, sizeof newTime);
      lownet_set_time(&newTime);
    }
    else{
      uint8_t length = strlen((char*) msg.contents);
      uint8_t payload[180];

      memcpy(payload, msg.contents, 180);

      ping(frame->source, payload, length);
    }

    sequence = msg.sequence;

    fullClear:
      command_init();
      mbedtls_pk_free(&pk);
      goto stop;
  }

  time = lownet_get_time();

  stop:
	 return;
}
