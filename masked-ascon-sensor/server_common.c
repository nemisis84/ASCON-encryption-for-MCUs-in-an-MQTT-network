/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include <string.h>
 #include "btstack.h"
 #include "hardware/adc.h"
 #include "temp_sensor.h"
 #include "server_common.h"
 #include "encryption.h"
 
 #define APP_AD_FLAGS 0x06
 #define ASCON_NONCE_SIZE 16
 #define MAX_PACKET_SIZE 128 // Maximum size of a packet to prevent out-of-memory issues

 static uint8_t adv_data[] = {
     0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
     0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
     0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18, 
 };
 static const uint8_t adv_data_len = sizeof(adv_data);
 
 int le_notification_enabled;
 hci_con_handle_t con_handle;
 uint16_t current_temp;
 
 uint8_t associated_data[] = "|TEMP-1";
 
 
 void pretty_print(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);  // Print each byte in hex
    }
    printf("\n");
}


 void send_encrypted_temperature() {
    uint8_t encrypted_temp[32];  // Buffer for encrypted data
    size_t encrypted_len;
    uint8_t nonce[ASCON_NONCE_SIZE];

    encrypt(current_temp, encrypted_temp, &encrypted_len, nonce);
    size_t ad_len = strlen(associated_data);
    // Create final message: Encrypted data + nonce
    uint8_t final_message[32 + ASCON_NONCE_SIZE];
    memcpy(final_message, encrypted_temp, encrypted_len);
    memcpy(final_message + encrypted_len, nonce, ASCON_NONCE_SIZE);
    memcpy(final_message + encrypted_len + ASCON_NONCE_SIZE, associated_data, ad_len);

    size_t final_message_len = encrypted_len + ASCON_NONCE_SIZE + ad_len;
    pretty_print("Sending encrypted temperature\n", final_message, final_message_len);
    att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, final_message, final_message_len);
 }


 void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
     UNUSED(size);
     UNUSED(channel);
     bd_addr_t local_addr;
     if (packet_type != HCI_EVENT_PACKET) return;
 
     uint8_t event_type = hci_event_packet_get_type(packet);
     switch(event_type){
         case BTSTACK_EVENT_STATE:
             if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
             gap_local_bd_addr(local_addr);
             printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
 
             // Setup BLE advertising
             uint16_t adv_int_min = 800;
             uint16_t adv_int_max = 800;
             uint8_t adv_type = 0;
             bd_addr_t null_addr;
             memset(null_addr, 0, 6);
             gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
             assert(adv_data_len <= 31);
             gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
             gap_advertisements_enable(1);
 
             poll_temp(); // Read temperature initially
 
             break;
         case HCI_EVENT_DISCONNECTION_COMPLETE:
             le_notification_enabled = 0;
             break;
         case ATT_EVENT_CAN_SEND_NOW:
             send_encrypted_temperature();
             break;
         default:
             break;
     }
 }
 

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&current_temp, sizeof(current_temp), offset, buffer, buffer_size);
    }
    return 0;
}

void recieve_encrypted_data(uint8_t *received_data, size_t received_len) {
    uint8_t *decrypted_data = NULL;
    size_t decrypted_len = 0;

    if (received_len > MAX_PACKET_SIZE) {
        printf("Received packet is too large! Rejecting.\n");
        return;
    }

    int status = decrypt(received_data, received_len, &decrypted_data, &decrypted_len);

    if (status == 0) {

        char data[decrypted_len + 1];
        memcpy(data, decrypted_data, decrypted_len);
        data[decrypted_len] = '\0';

        printf("Decrypted data: %s\n", data);

        free(decrypted_data);  // Prevent memory leak
    } else {
        printf("Decryption failed! Invalid or tampered message.\n");
    }
}


int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    printf("🔹 ATT Write Callback Triggered! Handle: 0x%X\n", att_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_CLIENT_CONFIGURATION_HANDLE){
        le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
        con_handle = connection_handle;
        if (le_notification_enabled) {
            printf("Notifications Enabled!\n");
            att_server_request_can_send_now_event(con_handle);
        } else {
            printf("Notifications Disabled!\n");
        }
        return 0;
    }

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE) {
        if (buffer_size > 0) {
            // Decrypt and print data
            recieve_encrypted_data(buffer, buffer_size);
        } else {
            printf("Empty Data Received!\n");
        }
        return 0;
    }
}


void poll_temp(void) {
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    uint32_t raw32 = adc_read();
    const uint32_t bits = 12;

    // Scale raw reading to 16 bit value using a Taylor expansion (for 8 <= bits <= 16)
    uint16_t raw16 = raw32 << (16 - bits) | raw32 >> (2 * bits - 16);

    // ref https://github.com/raspberrypi/pico-micropython-examples/blob/master/adc/temperature.py
    const float conversion_factor = 3.3 / (65535);
    float reading = raw16 * conversion_factor;
    
    // The temperature sensor measures the Vbe voltage of a biased bipolar diode, connected to the fifth ADC channel
    // Typically, Vbe = 0.706V at 27 degrees C, with a slope of -1.721mV (0.001721) per degree. 
    float deg_c = 27 - (reading - 0.706) / 0.001721;
    current_temp = deg_c * 100;
    printf("Write temp %.2f degc\n", deg_c);

    uint8_t encrypted_temp[32];  // Buffer for encrypted data
    size_t encrypted_len;
    uint8_t nonce[ASCON_NONCE_SIZE];
}