#include <stdio.h>
#include "pico/stdlib.h"
#include "ascon.h"
#include "crypto_aead.h"


// Uncomment the desired encryption method
// #define USE_ASCON_AEAD // Uses ascon_aead_encrypt / ascon_aead_decrypt
#define USE_CRYPTO_AEAD // Uses crypto_aead_encrypt / crypto_aead_decrypt
// #define USE_PROTECTED_BI32_ARMV6_LEVELED


void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    stdio_init_all();
    sleep_ms(3000);  // Wait for serial connection

    printf("Starting ASCON encryption test...\n");

    uint8_t key[CRYPTO_KEYBYTES] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                     0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

    uint8_t nonce[CRYPTO_NPUBBYTES] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                                        0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };

    uint8_t plaintext[] = "Hello, ASCON!";
    uint64_t plaintext_len = sizeof(plaintext) - 1;

    uint8_t ciphertext[plaintext_len + CRYPTO_ABYTES];  // Space for ciphertext + tag
    uint64_t ciphertext_len;

    uint8_t tag[CRYPTO_ABYTES];  // Authentication tag
    uint8_t associated_data[] = "MQTT-Test";
    uint64_t ad_len = sizeof(associated_data) - 1;

#ifdef USE_ASCON_AEAD
    printf("Using ascon_aead_encrypt()...\n");

    // Encrypt
    ascon_aead_encrypt(tag, ciphertext, plaintext, plaintext_len, 
                       associated_data, ad_len, nonce, key);

    print_hex("Ciphertext", ciphertext, plaintext_len);
    print_hex("Tag", tag, CRYPTO_ABYTES);

    // Decrypt
    uint8_t decrypted[plaintext_len];
    int decrypt_result = ascon_aead_decrypt(decrypted, tag, ciphertext, 
                                            plaintext_len, associated_data, 
                                            ad_len, nonce, key);

    if (decrypt_result != 0) {
        printf("Decryption failed!\n");
        return 1;
    }

    printf("Decrypted message: %s\n", decrypted);

#endif

#ifdef USE_CRYPTO_AEAD
    printf("Using crypto_aead_encrypt()...\n");

    // Encrypt
    crypto_aead_encrypt(ciphertext, &ciphertext_len, plaintext, plaintext_len, 
                        associated_data, ad_len, NULL, nonce, key);

    print_hex("Ciphertext", ciphertext, plaintext_len);
    print_hex("Tag", ciphertext + plaintext_len, CRYPTO_ABYTES);

    // Decrypt
    uint8_t decrypted[plaintext_len];
    uint64_t decrypted_len;

    int decrypt_result = crypto_aead_decrypt(decrypted, &decrypted_len, NULL, 
                                             ciphertext, ciphertext_len, 
                                             associated_data, ad_len, nonce, key);

    if (decrypt_result != 0) {
        printf("Decryption failed!\n");
        return 1;
    }

    printf("Decrypted message: %s\n", decrypted);
#endif

// #ifdef USE_PROTECTED_BI32_ARMV6_LEVELED
//     printf("Using protected_bi32_armv6_leveled()...\n");

//     ascon_state_t state;
//     mask_key_uint32_t masked_key;
//     mask_npub_uint32_t masked_nonce;
//     mask_m_uint32_t masked_plaintext;
//     mask_c_uint32_t masked_ciphertext;

//     generate_shares_encrypt(plaintext, &masked_plaintext, plaintext_len,
//                             associated_data, NULL, ad_len,
//                             nonce, &masked_nonce, key, &masked_key);

//     ascon_initaead(&state, &masked_key, &masked_nonce);
//     ascon_adata(&state, NULL, ad_len);
//     ascon_encrypt(&state, &masked_ciphertext, &masked_plaintext, plaintext_len);
//     ascon_final(&state, &masked_key);
//     ascon_settag(&state, (mask_c_uint32_t*)tag);

//     combine_shares_encrypt(&masked_ciphertext, ciphertext, plaintext_len + CRYPTO_ABYTES);

//     print_hex("Ciphertext", ciphertext, plaintext_len);
//     print_hex("Tag", tag, CRYPTO_ABYTES);

//     // Decryption
//     printf("Decrypting...\n");
//     mask_m_uint32_t masked_decrypted;
//     ascon_state_t state_dec;

//     generate_shares_decrypt(ciphertext, &masked_ciphertext, ciphertext_len,
//                             associated_data, NULL, ad_len,
//                             nonce, &masked_nonce, key, &masked_key);

//     ascon_initaead(&state_dec, &masked_key, &masked_nonce);
//     ascon_adata(&state_dec, NULL, ad_len);
//     ascon_decrypt(&state_dec, &masked_decrypted, &masked_ciphertext, plaintext_len);
//     ascon_final(&state_dec, &masked_key);
//     ascon_xortag(&state_dec, (mask_c_uint32_t*)tag);

//     int auth_success = ascon_iszero(&state_dec);
//     uint8_t decrypted[plaintext_len];
//     combine_shares_decrypt(&masked_decrypted, decrypted, plaintext_len);

//     if (auth_success == 0) {
//         printf("Decryption successful!\n");
//         printf("Decrypted message: %s\n", decrypted);
//     } else {
//         printf("Decryption failed: Authentication mismatch!\n");
//         return 1;
//     }

// #endif

//     return 0;
}
