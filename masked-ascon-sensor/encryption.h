#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

void encrypt_temperature_data(uint16_t temperature, uint8_t *output, size_t *output_len, uint8_t *nonce);
void decrypt_temperature_data(uint8_t *received_data, size_t received_len, uint16_t *output);
void generate_nonce(uint8_t *nonce);

#endif // ENCRYPTION_H
