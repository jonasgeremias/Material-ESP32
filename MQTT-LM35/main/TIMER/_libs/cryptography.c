/*
printf("test_encript\n");
unsigned char output[17] = "";
unsigned char hash_hexstring[35] = "";
char *input = "dado do esp32.  ";
aes_cbc_encrypt((char *) input, (char *) &hash_hexstring);
aes_cbc_decrypt((char *) &hash_hexstring, (unsigned char *) &output);
unsigned char hash_hexstring2[35] = "65d0fc4605b378cf4d20c8e800933679";
aes_cbc_decrypt((char *) &hash_hexstring2, (unsigned char *) &output);
*/

#ifndef __CRYPTOGRAPHY__
   #include "mbedtls/aes.h"
unsigned char aes_key[] = {0x24, 0x54, 0x23, 0x45, 0x24, 0x43, 0x23, 0x4e, 0x24, 0x4e, 0x23, 0x49, 0x24, 0x43, 0x23, 0x24};

static int aes_cbc_encrypt(char *input, char *hash_hexstring_temp) {
   int ret = ESP_OK;
   int len = 0;
   char data[17] = "";
   
   char *p = (char *) data;

   len = strlen(input);

   // printf("Here are first 5 chars only: %.5s\n", mystr);
   // printf("Here are the first 5 characters: %.*s\n", 15, mystr); // 5 here refers to # of characters
   p += sprintf(p, "%.*s", 16, input);
   
   if (len < 16) {
      for (; len < 16;len++) {
         p += sprintf(p, " ");
      }
   }

   // Cria a hashh
   mbedtls_aes_context aes;
   mbedtls_aes_init(&aes);
   mbedtls_aes_setkey_dec(&aes, (const unsigned char *) &aes_key, 128);
   unsigned char encrypt_output[17] = {0};
   char aes_iv[16] = {0};
   p = hash_hexstring_temp;

   ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 16, (unsigned char *) &aes_iv, (const unsigned char *) data, encrypt_output);
   encrypt_output[16] = 0;

   // Convertendo em HEX String
   for (int i = 0; i < 16; i++) {
      p += sprintf(p, "%02x", (uint8_t) encrypt_output[i]);
   }

   mbedtls_aes_free(&aes);

   // Imprime a hash AES
   // printf("AES enc: ret:%d , input:%s , hash:'%s'\n", ret, input, hash_hexstring_temp);

   return ret;
}

static int aes_cbc_decrypt(char *hash_hexstring_temp, unsigned char *output) {
   char encrypt_output[17] = {0};
   char aes_iv[16] = {0};
   char data_hex[3] = {0};
   char *p = hash_hexstring_temp;

   // Convertendo em HEX String
   data_hex[2] = 0;
   for (int i = 0; (((char) *p != 0) || (i < 15)); p += 2, i++) {
      data_hex[0] = *p;
      data_hex[1] = *(p + 1);
      encrypt_output[i] = (char) strtol(data_hex, NULL, 16);
   }

   mbedtls_aes_context aes;
   mbedtls_aes_init(&aes);
   mbedtls_aes_setkey_dec(&aes, (const unsigned char *) &aes_key, 128);

   int ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, (unsigned char *) &aes_iv, (const unsigned char *) encrypt_output, output);
   
   esp_aes_free(&aes);
   
   // Imprime o resultado
   // printf("AES dec: ret:%d , output:%s\n", ret, output);

   return ret;
}

#endif
#define __CRYPTOGRAPHY__