#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "encryption.h"
#include "server_common.h"
#include "pico/time.h"
#include "pico/rand.h"


#define ENCRYPTION_ASCON_MASKED   1
#define ENCRYPTION_ASCON_UNMASKED 2
#define ENCRYPTION_AES_MASKED     3
#define ENCRYPTION_NONE           4


#if !defined(SELECTED_ENCRYPTION_MODE)
#define SELECTED_ENCRYPTION_MODE ENCRYPTION_ASCON_UNMASKED
#endif

#if SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED
#include "masked_ascon_encryption.h"
#elif SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_UNMASKED
#include "crypto_aead.h"
#endif


static unsigned char key_128[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };


#define ASCON_KEY_SIZE 16
#define ASCON_NONCE_SIZE 16
#define ASCON_TAG_SIZE 16


#define AD_PATTERN "|TEMP-"
#define AD_PATTERN_LEN 6

uint8_t nonce[ASCON_NONCE_SIZE];


void init_primitives() {
    #if SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED
        init_prng();
        initialize_masked_key(key_128);
    #endif
    }
    

void log_start_decryption_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num <0) return;

    decryption_times[seq_num].seq_num = seq_num;
    decryption_times[seq_num].start_time = (uint64_t)time_us_64();  // Example with seconds (use microseconds for precision)
}

void log_end_decryption_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num <0) return;
    
    decryption_times[seq_num].end_time = (uint64_t)time_us_64();  // Example with seconds (use microseconds for precision)
}

void log_start_encryption_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num <0) return;

    encryption_times[seq_num].seq_num = seq_num;
    encryption_times[seq_num].start_time = (uint64_t)time_us_64();  // Example with seconds (use microseconds for precision)
}

void log_end_encryption_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num <0) return;
    
    encryption_times[seq_num].end_time = (uint64_t)time_us_64();  // Example with seconds (use microseconds for precision)
}


void generate_nonce(uint8_t *nonce) {
    // ascon_random_fetch(&prng_state, nonce, 16);
    rng_128_t rand128;
    get_rand_128(&rand128);
    memcpy(nonce, &rand128, sizeof(rng_128_t));
}


void encrypt(const void *data, size_t data_size, uint8_t *output, size_t *output_len,
    uint8_t *nonce, const char *associated_data, uint16_t counter) {

    size_t ad_len = strlen(associated_data);

    generate_nonce(nonce);

    log_start_encryption_time(counter);
    switch (SELECTED_ENCRYPTION_MODE) {
        case ENCRYPTION_ASCON_MASKED:
            masked_ascon128a_encrypt(output, output_len,
                (const uint8_t *)data, data_size,
                (const uint8_t *)associated_data, ad_len,
                nonce);
            break;
        case ENCRYPTION_ASCON_UNMASKED:
            unsigned long long clen = 0;
            crypto_aead_encrypt(output, &clen,
                (const uint8_t *)data, data_size,
                (const uint8_t *)associated_data, ad_len,
                NULL, nonce, key_128);
            *output_len = (size_t)clen;
            printf("Encrypted data: ");
            for (size_t i = 0; i < *output_len; i++) {
                printf("%02X ", output[i]);
            }
            printf("\n");
            printf("Nonce: ");
            for (int i = 0; i < ASCON_NONCE_SIZE; i++) {
                printf("%02X ", nonce[i]);
            }
            printf("\n");
            printf("Associated Data: %s\n", associated_data);

        break;
    }
    log_end_encryption_time(counter);
}

int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len, uint16_t *sequence_number) {

    // 🔹 Ensure packet is long enough
    if (received_len < (ASCON_TAG_SIZE + ASCON_NONCE_SIZE + 5)) {  // 5 = min "|X|Y" length
        printf("❌ Error: Received packet too small\n");
        return -1;
    }

    // 🔹 Locate the start of Associated Data (AD)
    size_t ad_start_index = 0;

    for (size_t i = received_len - 1; i >= ASCON_NONCE_SIZE; i--) {
        if (memcmp(received_data + i, AD_PATTERN, AD_PATTERN_LEN) == 0) {
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

    int temp_seq_num = 0;
    if (sscanf(extracted_ad, "|%[^|]|%d", received_sensor_id, &temp_seq_num) != 2) {
        printf("❌ Error: Failed to parse Associated Data.\n");
        return -1;
    }
    *sequence_number = (uint16_t)temp_seq_num;
    

    // 🔹 Validate Sensor ID
    if (strcmp(received_sensor_id, sensor_ID) != 0) {
        printf("❌ Sensor ID Mismatch! Expected: %s, Received: %s\n", sensor_ID, received_sensor_id);
        return -1;
    }

    // 🔹 Extract Nonce (Before AD, 16 bytes)
    size_t nonce_start_index = ad_start_index - ASCON_NONCE_SIZE;
    uint8_t received_nonce[ASCON_NONCE_SIZE];
    memcpy(received_nonce, received_data + nonce_start_index, ASCON_NONCE_SIZE);

    size_t ciphertext_len = nonce_start_index;
    uint8_t *ciphertext = received_data;

    uint8_t decrypted_data = (uint8_t *)malloc(ciphertext_len);
    if (!decrypted_data) return -1;

    int status = -1;


    switch (SELECTED_ENCRYPTION_MODE) {
        case ENCRYPTION_ASCON_MASKED:
            log_start_decryption_time(*sequence_number);
            status = masked_ascon128a_decrypt(
                decrypted_data, output_len,
                ciphertext, ciphertext_len,
                (uint8_t *)extracted_ad, ad_len,
                received_nonce);
            break;

        case ENCRYPTION_ASCON_UNMASKED:
            log_start_decryption_time(*sequence_number);
            status = crypto_aead_decrypt(decrypted_data, output_len,
                NULL, ciphertext, ciphertext_len,
                (uint8_t *)extracted_ad, ad_len,
                received_nonce, key_128);
            break;
        default:
            break;
    }

    if (status >= 0) {
        *output = decrypted_data;
        log_end_decryption_time(*sequence_number);
        log_end_time(*sequence_number);
        return 0;
    } else {
        log_end_decryption_time(-1);
        log_end_time(-1);
        free(decrypted_data);
        return -1;
    }
}



