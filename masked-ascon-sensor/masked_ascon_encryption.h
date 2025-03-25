#ifndef MASKED_ASCON_ENCRYPTION_H
#define MASKED_ASCON_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

void initialize_masked_key(const uint8_t *key);
void init_prng();
void masked_ascon128a_encrypt(uint8_t *output, size_t *output_len,
    const uint8_t *data, size_t data_size,
    const uint8_t *associated_data, size_t ad_len,
    const uint8_t *nonce);

int masked_ascon128a_decrypt(uint8_t *decrypted_data, size_t *output_len,
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *associated_data, size_t ad_len,
    const uint8_t *nonce);

#endif
