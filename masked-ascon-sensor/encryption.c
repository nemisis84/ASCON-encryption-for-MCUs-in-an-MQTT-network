#include <stdio.h>
#include <string.h>
#include "ascon/aead-masked.h"
#include "encryption.h"
#include "ascon/random.h"


#define ASCON_KEY_SIZE 16
#define ASCON_NONCE_SIZE 16
#define ASCON_TAG_SIZE 16


static ascon_random_state_t prng_state;  // 🔹 Persistent PRNG state
uint8_t nonce[ASCON_NONCE_SIZE];

void init_prng() {
    ascon_random_init(&prng_state);
}

void generate_nonce(uint8_t *nonce) {
    ascon_random_fetch(&prng_state, nonce, 16);
}

static unsigned char key_128[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

static ascon_masked_key_128_t masked_key;

void initialize_masked_key() {
    ascon_masked_key_128_init(&masked_key, key_128);
}


void encrypt_temperature_data(uint16_t temperature, uint8_t *output, size_t *output_len, uint8_t *nonce) {
    uint8_t plaintext[sizeof(temperature)];
    memcpy(plaintext, &temperature, sizeof(temperature));

    uint8_t associated_data[] = "BLE-Temp";
    uint64_t ad_len = sizeof(associated_data) - 1;

    generate_nonce(nonce);  // ✅ Generate a fresh nonce per encryption

    ascon128_masked_aead_encrypt(output, output_len, plaintext, sizeof(temperature),
                          associated_data, ad_len, nonce, &masked_key);
}

void decrypt_temperature_data(uint8_t *received_data, size_t received_len, uint16_t *output) {
    if (received_len < (sizeof(uint16_t) + ASCON_TAG_SIZE + ASCON_NONCE_SIZE)) {
        printf("Error: Received packet too small\n");
        return;
    }

    uint8_t decrypted_temp[sizeof(uint16_t)];
    size_t decrypted_len;

    uint8_t received_nonce[ASCON_NONCE_SIZE];
    memcpy(received_nonce, received_data + received_len - ASCON_NONCE_SIZE, ASCON_NONCE_SIZE);

    uint8_t associated_data[] = "BLE-Temp";
    uint64_t ad_len = sizeof(associated_data) - 1;

    int status = ascon128_masked_aead_decrypt(decrypted_temp, &decrypted_len, received_data, received_len - ASCON_NONCE_SIZE,
                                       associated_data, ad_len, received_nonce, &masked_key);

    if (status == 0) {
        memcpy(output, decrypted_temp, sizeof(uint16_t));
        printf("Decrypted Temperature: %.2f°C\n", *output / 100.0);
    } else {
        printf("Decryption failed! Invalid data or tampered message.\n");
    }
}
