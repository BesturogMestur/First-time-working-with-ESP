#include "crypt.h"

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <aes/esp_aes.h>

#include "serial_io.h"
#include "lownet.h"

#include <stdio.h>
void crypt_decrypt(const lownet_secure_frame_t* cipher,
		   lownet_secure_frame_t* plain)
{
  unsigned char iv[16];
  memcpy(iv, &cipher->ivt, sizeof iv);
  memcpy(plain, cipher, LOWNET_UNENCRYPTED_SIZE + LOWNET_IVT_SIZE);

  const uint8_t* aes_key = lownet_get_key()->bytes;
  esp_aes_context ctx;
  esp_aes_init(&ctx);
  esp_aes_setkey(&ctx, aes_key, 256);
  esp_aes_crypt_cbc(&ctx,
		    ESP_AES_DECRYPT,
		    LOWNET_ENCRYPTED_SIZE,
		    iv,
		    (const unsigned char*) &cipher->protocol,
		    (unsigned char*) &plain->protocol
		    );
  esp_aes_free(&ctx);
}

void crypt_encrypt(const lownet_secure_frame_t* plain,
		   lownet_secure_frame_t* cipher)
{
  unsigned char iv[16];
  memcpy(iv, &plain->ivt, sizeof iv);

  memcpy(cipher, plain, LOWNET_UNENCRYPTED_SIZE + LOWNET_IVT_SIZE);
  const uint8_t* aes_key = lownet_get_key()->bytes;
  esp_aes_context ctx;

  esp_aes_init(&ctx);
  esp_aes_setkey(&ctx, aes_key, 256);
  esp_aes_crypt_cbc(
		    &ctx,
		    ESP_AES_ENCRYPT,
		    LOWNET_ENCRYPTED_SIZE,
		    iv,
		    (const unsigned char*) &plain->protocol,
		    (unsigned char*) &cipher->protocol
		    );
  esp_aes_free(&ctx);
}

// Usage: crypt_setkey_command(KEY)
// Pre:   KEY is a valid AES key or NULL
// Post:  If key == NULL encryption has been disabled
//        Else KEY has been set as the encryption key to use for
//        lownet communication.
void crypt_setkey_command(char* args)
{
  //þarf að skija hvering liglar virkar, spurja þegar þú getur.
  if(!args) goto off;

  int l = strlen(args);
  lownet_key_t key;

  if((*args == '0' || *args == '1') && l == 1){
    puts("Key 0 or 1 has been set");
    key = lownet_keystore_read(*args - '0');
  }
  else if(l == LOWNET_KEY_SIZE_AES){
    /* puts("costom key has been set"); */
    /* key.size = LOWNET_KEY_SIZE_AES; */
    /* memcpy(key.bytes, args, LOWNET_KEY_SIZE_AES); */
  }
  else{
    goto off;
  }
  
  lownet_set_key(&key);
  return;

 off:
  lownet_set_key(NULL);
  puts("Encreson off");
}


void crypt_test_command(char* str)
{
	if (!str)
		return;
	if (!lownet_get_key())
		{
			serial_write_line("No encryption key set!");
			return;
		}

	// Encrypts and then decrypts a string, can be used to sanity check your
	// implementation.
	lownet_secure_frame_t plain;
	lownet_secure_frame_t cipher;
	lownet_secure_frame_t back;

	memset(&plain, 0, sizeof(lownet_secure_frame_t));
	memset(&cipher, 0, sizeof(lownet_secure_frame_t));
	memset(&back, 0, sizeof(lownet_secure_frame_t));

	*((uint32_t*) plain.ivt) = 123456789;
	strcpy((char*) plain.payload, str);

	crypt_encrypt(&plain, &cipher);
	crypt_decrypt(&cipher, &back);

	if (strlen((char*) back.payload) != strlen(str))
		ESP_LOGE("APP", "Length violation");
	else
		serial_write_line((char*) back.payload);
}
