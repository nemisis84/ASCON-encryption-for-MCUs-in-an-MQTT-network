#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

void init_prng();
void initialize_masked_key();
void encrypt(uint16_t temperature, uint8_t *output, size_t *output_len, uint8_t *nonce);
int decrypt(uint8_t *received_data, size_t received_len, uint8_t **output, size_t *output_len);
void generate_nonce(uint8_t *nonce);

#endif // ENCRYPTION_H
