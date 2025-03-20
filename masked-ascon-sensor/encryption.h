#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

void init_prng();
void initialize_masked_key();
void encrypt(uint16_t data, uint8_t *output, size_t *output_len, uint8_t *nonce, char *associated_data, uint16_t counter);
int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len, uint16_t *sequence_number);
void generate_nonce(uint8_t *nonce);

#endif // ENCRYPTION_H
