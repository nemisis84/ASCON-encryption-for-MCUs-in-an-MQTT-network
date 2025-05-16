#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "encryption.h"
#include "server_common.h"
#include "pico/time.h"
#include "pico/rand.h"


#define ENCRYPTION_ASCON_MASKED   1
#define ENCRYPTION_ASCON_UNMASKED 2
#define ENCRYPTION_AES_GCM        3
#define ENCRYPTION_NONE           4


#if !defined(SELECTED_ENCRYPTION_MODE)
#define SELECTED_ENCRYPTION_MODE ENCRYPTION_AES_GCM
#endif

#if SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED
#define NONCE_SIZE 16
#include "masked_ascon_encryption.h"
#elif SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_UNMASKED
#include "crypto_aead.h"
#define NONCE_SIZE 16
#elif SELECTED_ENCRYPTION_MODE == ENCRYPTION_AES_GCM
#include "crypto_aead.h"
#define NONCE_SIZE 12
#else
#define NONCE_SIZE 0
#endif

#define TAG_SIZE 16
#define AD_PATTERN "|TEMP-"
#define AD_PATTERN_LEN 6

static unsigned char key_128[16] = {
    0x9E, 0x88, 0xCD, 0xDB, 0x2D, 0xA9, 0x09, 0x93,
    0x7C, 0xAC, 0xD4, 0xD8, 0x02, 0x3F, 0x0D, 0x88 
};


uint8_t nonce[NONCE_SIZE];

void init_primitives() {
    #if SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED
        init_prng();
        initialize_masked_key(key_128);
    #endif
    }
    

void log_start_decryption_time(uint16_t seq_num) {
    if (seq_num >= max_packets || seq_num <0) return;

    decryption_times[seq_num].seq_num = seq_num;
    decryption_times[seq_num].start_time = (uint64_t)time_us_64();
}

void log_end_decryption_time(uint16_t seq_num) {
    if (seq_num >= max_packets || seq_num <0) return;
    
    decryption_times[seq_num].end_time = (uint64_t)time_us_64();
}

void log_start_encryption_time(uint16_t seq_num) {
    if (seq_num >= max_packets || seq_num <0) return;

    encryption_times[seq_num].seq_num = seq_num;
    encryption_times[seq_num].start_time = (uint64_t)time_us_64();
}

void log_end_encryption_time(uint16_t seq_num) {
    if (seq_num >= max_packets || seq_num <0) return;
    
    encryption_times[seq_num].end_time = (uint64_t)time_us_64();
}


void generate_nonce(uint8_t *nonce) {
    rng_128_t rand128;
    get_rand_128(&rand128);
    memcpy(nonce, &rand128, NONCE_SIZE);
}


void encrypt(const void *data, size_t data_size, uint8_t *output, size_t *output_len,
             uint8_t *nonce, const char *associated_data, uint16_t counter) {
    size_t ad_len = strnlen(associated_data, 50);
    
    if (ad_len >= 50) {
        printf("AD length too long or missing null terminator!\n");
        return;
    }

    generate_nonce(nonce);
    log_start_encryption_time(counter);

    if (SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED) {
        unsigned long long clen = 0;
        masked_ascon128a_encrypt(output, &clen,
            (const uint8_t *)data, data_size,
            (const uint8_t *)associated_data, ad_len,
            nonce);
        *output_len = (size_t)clen;
    } else if (SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_UNMASKED ||
               SELECTED_ENCRYPTION_MODE == ENCRYPTION_AES_GCM) {
        unsigned long long clen = 0;
        crypto_aead_encrypt(output, &clen, // Same function call for ASCON and AES
            (const uint8_t *)data, data_size,
            (const uint8_t *)associated_data, ad_len,
            NULL, nonce, key_128);
        *output_len = (size_t)clen;
    }

    log_end_encryption_time(counter);
}



int parse_unencrypted(uint8_t *received_data, size_t received_len,
    uint8_t **output, size_t *output_len, uint16_t *sequence_number) {
    if (received_len < 5) {
        printf("Error: Received packet too small\n");
        return -1;
    }

    size_t ad_start_index = 0;
    for (ssize_t i = (ssize_t)received_len - 1; i >= 0; i--) {
        if (memcmp(received_data + i, AD_PATTERN, AD_PATTERN_LEN) == 0) {
            ad_start_index = i;
            break;
        }
    }

    if (ad_start_index == 0) {
        printf("Error: Associated Data not found in payload.\n");
        return -1;
    }

    size_t ad_len = received_len - ad_start_index;
    char extracted_ad[ad_len + 1];
    memcpy(extracted_ad, received_data + ad_start_index, ad_len);
    extracted_ad[ad_len] = '\0';

    char received_sensor_id[20];
    int temp_seq_num = 0;
    if (sscanf(extracted_ad, "|%[^|]|%d", received_sensor_id, &temp_seq_num) != 2) return -1;
    if (strcmp(received_sensor_id, sensor_ID) != 0) return -1;

    *sequence_number = (uint16_t)temp_seq_num;
    *output_len = ad_start_index;
    *output = (uint8_t *)malloc(*output_len);
    if (!*output) return -1;

    memcpy(*output, received_data, *output_len);
    log_end_time(*sequence_number);
    return 0;
}


int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len, uint16_t *sequence_number) {


    if (SELECTED_ENCRYPTION_MODE == ENCRYPTION_NONE) {
        return parse_unencrypted(received_data, received_len, output, output_len, sequence_number);
    }

    if (received_len < (TAG_SIZE + NONCE_SIZE + 5) || received_len > MAX_PAYLOAD_SIZE) { 
        return -1;
    }

    //Locate the start of Associated Data (AD)
    size_t ad_start_index = 0;

    for (size_t i = received_len - 1; i >= NONCE_SIZE; i--) {
        if (memcmp(received_data + i, AD_PATTERN, AD_PATTERN_LEN) == 0) {
            ad_start_index = i;
            break;
        }
    }

    if (ad_start_index == 0) {
        printf("Error: Associated Data not found in payload.\n");
        return -1;
    }

    //Extract Associated Data
    size_t ad_len = received_len - ad_start_index;
    char extracted_ad[ad_len + 1];
    memcpy(extracted_ad, received_data + ad_start_index, ad_len);
    extracted_ad[ad_len] = '\0';  // Null-terminate

    //Parse Associated Data: Format "|sensor_id|seq_number"
    char received_sensor_id[20];  // Buffer for sensor ID

    int temp_seq_num = 0;
    if (sscanf(extracted_ad, "|%[^|]|%d", received_sensor_id, &temp_seq_num) != 2) {
        printf("Error: Failed to parse Associated Data.\n");
        return -1;
    }
    *sequence_number = (uint16_t)temp_seq_num;
    
    //Validate Sensor ID
    if (strcmp(received_sensor_id, sensor_ID) != 0) {
        printf("Sensor ID Mismatch! Expected: %s, Received: %s\n", sensor_ID, received_sensor_id);
        return -1;
    }

    //Extract Nonce
    size_t nonce_start_index = ad_start_index - NONCE_SIZE;
    uint8_t received_nonce[NONCE_SIZE];
    memcpy(received_nonce, received_data + nonce_start_index, NONCE_SIZE);

    size_t ciphertext_len = nonce_start_index;
    uint8_t *ciphertext = received_data;

    uint8_t *decrypted_data = (uint8_t *)malloc(ciphertext_len);

    if (!decrypted_data) {
        printf("malloc failed!\n");
        return -1;
    }

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
        case ENCRYPTION_AES_GCM:
            log_start_decryption_time(*sequence_number);
            status = crypto_aead_decrypt(decrypted_data, output_len, // Same function call for ASCON and AES
                NULL, ciphertext, ciphertext_len,
                (uint8_t *)extracted_ad, ad_len,
                received_nonce, key_128);
            break;
        default:
            log_end_decryption_time(-1);
            log_end_time(-1);
            free(decrypted_data);
            return -1;
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



