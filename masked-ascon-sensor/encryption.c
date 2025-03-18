#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ascon/aead-masked.h"
#include "encryption.h"
#include "ascon/random.h"
#include "server_common.h"

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


void encrypt(uint16_t data, uint8_t *output, size_t *output_len, uint8_t *nonce, char *associated_data) {
    uint8_t plaintext[sizeof(data)];
    memcpy(plaintext, &data, sizeof(data));

    size_t ad_len = strlen(associated_data);

    generate_nonce(nonce);

    ascon128a_masked_aead_encrypt(output, output_len, plaintext, sizeof(data),
                          (char *)associated_data, ad_len, nonce, &masked_key);
}

int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len, uint16_t *sequence_number) {

    // 🔹 Ensure packet is long enough
    if (received_len < (ASCON_TAG_SIZE + ASCON_NONCE_SIZE + 5)) {  // 5 = min "|X|Y" length
        printf("❌ Error: Received packet too small\n");
        return -1;
    }

    // 🔹 Locate the start of Associated Data (AD)
    size_t ad_start_index = 0;
    const char *ad_pattern = "|TEMP-";  // Look for AD marker
    for (size_t i = received_len - 1; i >= ASCON_NONCE_SIZE; i--) {
        if (memcmp(received_data + i, ad_pattern, strlen(ad_pattern)) == 0) {
            ad_start_index = i;
            break;
        }
    }

    if (ad_start_index == 0) {
        printf("❌ Error: Associated Data not found in payload.\n");
        return -1;
    }

    // 🔹 Extract Associated Data
    size_t ad_len = received_len - ad_start_index;
    char extracted_ad[ad_len + 1];
    memcpy(extracted_ad, received_data + ad_start_index, ad_len);
    extracted_ad[ad_len] = '\0';  // Null-terminate

    // 🔹 Parse Associated Data: Format "|sensor_id|seq_number"
    char received_sensor_id[20];  // Buffer for sensor ID
    
    if (sscanf(extracted_ad, "|%[^|]|%d", received_sensor_id, sequence_number) != 2) {
        printf("❌ Error: Failed to parse Associated Data.\n");
        return -1;
    }

    // 🔹 Validate Sensor ID
    if (strcmp(received_sensor_id, sensor_ID) != 0) {
        printf("❌ Sensor ID Mismatch! Expected: %s, Received: %s\n", sensor_ID, received_sensor_id);
        return -1;
    }

    // 🔹 Extract Nonce (Before AD, 16 bytes)
    size_t nonce_start_index = ad_start_index - ASCON_NONCE_SIZE;
    uint8_t received_nonce[ASCON_NONCE_SIZE];
    memcpy(received_nonce, received_data + nonce_start_index, ASCON_NONCE_SIZE);

    // 🔹 Extract Ciphertext (Everything before Nonce)
    size_t ciphertext_len = nonce_start_index;
    uint8_t *ciphertext = received_data;  // Ciphertext is at the start of received_data

    // 🔹 Allocate Memory for Decryption
    uint8_t *decrypted_data = (uint8_t *)malloc(ciphertext_len);
    if (!decrypted_data) {
        printf("❌ Memory allocation failed!\n");
        return -1;
    }

    printf("✅ Extracted Associated Data: %s\n", extracted_ad);
    printf("✅ Parsed Sensor ID: %s, Sequence Number: %d\n", received_sensor_id, *sequence_number);

    // 🔹 Perform Decryption
    int status = ascon128a_masked_aead_decrypt(
        decrypted_data, output_len, ciphertext, ciphertext_len,
        (uint8_t *)extracted_ad, ad_len, received_nonce, &masked_key
    );

    if (status >= 0) {
        *output = decrypted_data;
        log_end_time(*sequence_number);
        return 0;
    } else {
        free(decrypted_data);
        return -1;
    }
}



