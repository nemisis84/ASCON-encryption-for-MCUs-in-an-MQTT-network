#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>
#include "experiment_settings.h"

void init_prng();
void initialize_masked_key();
void encrypt(const void *data, size_t data_size, uint8_t *output, size_t *output_len,
    uint8_t *nonce, const char *associated_data, uint16_t counter);

int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len, uint16_t *sequence_number);
void generate_nonce(uint8_t *nonce);
void init_primitives();

int get_nonce_size();

#endif // ENCRYPTION_H
