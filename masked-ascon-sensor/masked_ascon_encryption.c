#include "ascon/aead-masked.h"
#include "ascon/random.h"
#include <stdint.h>
#include <string.h>

#include "encryption.h"  // for key_128 and shared types

static ascon_masked_key_128_t masked_key;
static ascon_random_state_t prng_state;

void initialize_masked_key(const uint8_t *key) {
    ascon_masked_key_128_init(&masked_key, key);
}

void init_prng() {
    ascon_random_init(&prng_state);
}

void masked_ascon128a_encrypt(uint8_t *output, size_t *output_len,
    const uint8_t *data, size_t data_size,
    const uint8_t *associated_data, size_t ad_len,
    const uint8_t *nonce) {

    ascon128a_masked_aead_encrypt(output, output_len,
        data, data_size,
        associated_data, ad_len,
        nonce, &masked_key);
}

int masked_ascon128a_decrypt(uint8_t *decrypted_data, size_t *output_len,
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *associated_data, size_t ad_len,
    const uint8_t *nonce) {

    return ascon128a_masked_aead_decrypt(decrypted_data, output_len,
        ciphertext, ciphertext_len,
        associated_data, ad_len,
        nonce, &masked_key);
}
