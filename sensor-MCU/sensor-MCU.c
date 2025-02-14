#include <stdio.h>
#include "pico/stdlib.h"
#include "ascon/aead-masked.h"  // Masked ASCON AEAD version

#define USE_ASCON_MASKED_AEAD // Uses masked ASCON AEAD for encryption/decryption

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    stdio_init_all();
    sleep_ms(3000);  // Wait for serial connection

    printf("Starting ASCON Masked AEAD encryption test...\n");

    uint8_t plaintext[] = "Hello, ASCON!";
    uint64_t plaintext_len = sizeof(plaintext) - 1;

    uint8_t nonce[16] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                           0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };

    uint8_t associated_data[] = "MQTT-Test";
    uint64_t ad_len = sizeof(associated_data) - 1;

    uint8_t ciphertext[plaintext_len + 16]; // Ciphertext + tag
    size_t ciphertext_len;

    ascon_masked_key_128_t masked_key;
    ascon128_masked_key_init(&masked_key, (uint8_t[]){ 0x00, 0x01, 0x02, 0x03,
                                                       0x04, 0x05, 0x06, 0x07,
                                                       0x08, 0x09, 0x0A, 0x0B,
                                                       0x0C, 0x0D, 0x0E, 0x0F });

#ifdef USE_ASCON_MASKED_AEAD
    printf("Using ascon128_masked_aead_encrypt()...\n");

    // Encrypt
    ascon128_masked_aead_encrypt(ciphertext, &ciphertext_len, plaintext, plaintext_len, 
                                 associated_data, ad_len, nonce, &masked_key);

    print_hex("Ciphertext", ciphertext, plaintext_len);
    print_hex("Tag", ciphertext + plaintext_len, 16);

    // Decrypt
    uint8_t decrypted[plaintext_len];
    size_t decrypted_len;

    printf("Decrypting...\n");
    int decrypt_result = ascon128_masked_aead_decrypt(decrypted, &decrypted_len, 
                                                      ciphertext, ciphertext_len, 
                                                      associated_data, ad_len, 
                                                      nonce, &masked_key);

    if (decrypt_result != 0) {
        printf("Decryption failed!\n");
        return 1;
    }

    printf("Decryption successful!\n");
    printf("Decrypted message: %s\n", decrypted);
#endif

    return 0;
}
