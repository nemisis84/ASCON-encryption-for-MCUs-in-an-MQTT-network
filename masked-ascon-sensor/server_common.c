/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include <string.h>
 #include "pico/stdlib.h"
 #include "btstack.h"
 #include "ble/att_server.h"
 #include "hardware/adc.h"
 #include "temp_sensor.h"
 #include "server_common.h"
 #include "experiment_settings.h"
 #include "encryption.h"


#define ENCRYPTION_ASCON_MASKED   1
#define ENCRYPTION_ASCON_UNMASKED 2
#define ENCRYPTION_AES_GCM        3
#define ENCRYPTION_NONE           4

#if !defined(SELECTED_ENCRYPTION_MODE)
#define SELECTED_ENCRYPTION_MODE ENCRYPTION_AES_GCM
#endif

#if SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_MASKED
#define NONCE_SIZE 16
#elif SELECTED_ENCRYPTION_MODE == ENCRYPTION_ASCON_UNMASKED
#define NONCE_SIZE 16
#elif SELECTED_ENCRYPTION_MODE == ENCRYPTION_AES_GCM
#define NONCE_SIZE 12
#else
#define NONCE_SIZE 0
#endif


 #define APP_AD_FLAGS 0x06
 #define MAX_PACKET_SIZE HCI_ACL_PAYLOAD_SIZE // Maximum size of a packet to prevent out-of-memory issues


 static uint8_t adv_data[] = {
     0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
     0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
     0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18, 
 };
 static const uint8_t adv_data_len = sizeof(adv_data);
 
 int le_notification_enabled;
 hci_con_handle_t con_handle;
 temperatures current_temps;
 
 uint8_t sensor_ID[] = "TEMP-1";
 
 
 void pretty_print(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);  // Print each byte in hex
    }
    printf("\n");
}

static uint_fast16_t counter = 0;

data_entry RTT_table[MAX_PACKETS];
data_entry encryption_times[MAX_PACKETS];
data_entry decryption_times[MAX_PACKETS];
data_entry sending_processing_times[MAX_PACKETS];
data_entry receiving_processing_times[MAX_PACKETS];

// Function to log the start time
void log_start_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num < 0) return;

    uint64_t start_time = (uint64_t)time_us_64();  // Get the current timestamp once

    RTT_table[seq_num].seq_num = seq_num;
    RTT_table[seq_num].start_time = start_time;  // Use the same timestamp for RTT_table

    sending_processing_times[seq_num].seq_num = seq_num;
    sending_processing_times[seq_num].start_time = start_time;  // Use the same timestamp for processing_times
}

// Function to log the end time
void log_end_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num <0) return;

    RTT_table[seq_num].end_time = (uint64_t)time_us_64();
}

void log_end_sending_processing_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num < 0) return;

    sending_processing_times[seq_num].end_time = (uint64_t)time_us_64();
}

uint64_t get_start_recieving_processing_time() {
    return (uint64_t)time_us_64();
}

void log_start_recieving_processing_time(uint16_t seq_num, uint64_t start_time) {
    if (seq_num >= MAX_PACKETS || seq_num < 0) return;

    receiving_processing_times[seq_num].seq_num = seq_num;
    receiving_processing_times[seq_num].start_time = start_time;
}

void log_end_recieving_processing_time(uint16_t seq_num) {
    if (seq_num >= MAX_PACKETS || seq_num < 0) return;

    receiving_processing_times[seq_num].end_time = (uint64_t)time_us_64();
}


typedef enum {
    TRANSFER_RTT,
    TRANSFER_ENC,
    TRANSFER_DEC,
    TRANSFER_S_PROC,
    TRANSFER_R_PROC,
} transfer_state_t;

typedef struct {
    void *data;
    size_t data_size;
    size_t bytes_sent;
    size_t chunk_size;
    int total_chunks;
    int current_chunk;
    transfer_state_t transfer_type;
} ble_transfer_t;

static ble_transfer_t active_transfer = {0};  

void print_all_results() {
    printf("\n📊 RTT Results:\n");
    for (int i = 0; i < MAX_PACKETS; i++) {
        printf("RTT %2d → Start: %llu, End: %llu\n", i, RTT_table[i].start_time, RTT_table[i].end_time);
    }

    printf("\n🔐 Encryption Times:\n");
    for (int i = 0; i < MAX_PACKETS; i++) {
        printf("ENC %2d → Start: %llu, End: %llu\n", i, encryption_times[i].start_time, encryption_times[i].end_time);
    }

    printf("\n🔓 Decryption Times:\n");
    for (int i = 0; i < MAX_PACKETS; i++) {
        printf("DEC %2d → Start: %llu, End: %llu\n", i, decryption_times[i].start_time, decryption_times[i].end_time);
    }

    printf("\n📥 Receiving Processing Times:\n");
    for (int i = 0; i < MAX_PACKETS; i++) {
        printf("RECV %2d → Start: %llu, End: %llu\n", i, receiving_processing_times[i].start_time, receiving_processing_times[i].end_time);
    }

    printf("\n📤 Sending Processing Times:\n");
    for (int i = 0; i < MAX_PACKETS; i++) {
        printf("SEND %2d → Start: %llu, End: %llu\n", i, sending_processing_times[i].start_time, sending_processing_times[i].end_time);
    }
}



void send_struct_data(void *data, size_t data_size, const char *data_type, transfer_state_t transfer_type) {
    if (!data || data_size == 0) {
        printf("❌ Error: No data to send for %s.\n", data_type);
        return;
    }

    printf("📤 Sending %s results (%zu bytes)...\n", data_type, data_size);

    int mtu_size = att_server_get_mtu(con_handle) - 3;
    int chunk_size = (mtu_size > 200) ? 200 : ((mtu_size > 20) ? mtu_size : 20);

    active_transfer = (ble_transfer_t){
        .data = data,
        .data_size = data_size,
        .bytes_sent = 0,
        .chunk_size = chunk_size,
        .total_chunks = (data_size + chunk_size - 1) / chunk_size,
        .current_chunk = ((data_size + chunk_size - 1) / chunk_size) - 1,
        .transfer_type = transfer_type
    };

    char associated_data[50];
    snprintf(associated_data, sizeof(associated_data), "|%s|%s", sensor_ID, data_type);
    if (att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE,
                          (const uint8_t *)associated_data, strlen(associated_data)) != 0) {
        printf("❌ BLE notification failed for associated data.\n");
        return;
    }

    sleep_ms(50);
    att_server_request_can_send_now_event(con_handle);
}

void send_next_chunk() {
    if (active_transfer.bytes_sent >= active_transfer.data_size) {
        printf("✅ Completed %s transfer (%zu bytes in %d chunks)\n",
               (active_transfer.transfer_type == TRANSFER_RTT) ? "RTT" :
               (active_transfer.transfer_type == TRANSFER_ENC) ? "ENC" : "DEC",
               active_transfer.data_size, active_transfer.total_chunks);

        // Move to the next struct after RTT -> ENC -> DEC
        if (active_transfer.transfer_type == TRANSFER_RTT) {
            send_struct_data(encryption_times, sizeof(encryption_times), "ENC", TRANSFER_ENC);
        } else if (active_transfer.transfer_type == TRANSFER_ENC) {
            send_struct_data(decryption_times, sizeof(decryption_times), "DEC", TRANSFER_DEC);
        } else if (active_transfer.transfer_type == TRANSFER_DEC) {
            send_struct_data(receiving_processing_times, sizeof(receiving_processing_times), "R_PROC", TRANSFER_R_PROC);
        } else if (active_transfer.transfer_type == TRANSFER_R_PROC) {
            send_struct_data(sending_processing_times, sizeof(sending_processing_times), "S_PROC", TRANSFER_S_PROC);
        } else {
            printf("All BLE transfers complete!\n");
            abort();
        }
        return;
    }

    size_t bytes_to_send = (active_transfer.data_size - active_transfer.bytes_sent > active_transfer.chunk_size)
                               ? active_transfer.chunk_size
                               : (active_transfer.data_size - active_transfer.bytes_sent);

    uint8_t send_buffer[bytes_to_send + 1];
    send_buffer[0] = (uint8_t)active_transfer.current_chunk;
    memcpy(send_buffer + 1, ((uint8_t *)active_transfer.data) + active_transfer.bytes_sent, bytes_to_send);

    int status = att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE,
                                   send_buffer, bytes_to_send + 1);

    if (status == 0) {  // ✅ Success
        active_transfer.bytes_sent += bytes_to_send;
        active_transfer.current_chunk--;
    } else {  // ❌ Failed
        printf("❌ BLE notification failed, retrying...\n");
    }

    att_server_request_can_send_now_event(con_handle);
}

 void send_encrypted_temperature() {
    log_start_time(counter);

    char associated_data[50];
    snprintf(associated_data, sizeof(associated_data), "|%s|%d", sensor_ID, counter);
    size_t ad_len = strlen(associated_data);

    const size_t reserved_meta = NONCE_SIZE + ad_len;
    const size_t max_encrypted_payload_size = MAX_PAYLOAD_SIZE - reserved_meta;

    uint8_t encrypted_payload[max_encrypted_payload_size];
    size_t encrypted_len;
    uint8_t nonce[NONCE_SIZE];

    encrypt(&current_temps, sizeof(current_temps), encrypted_payload, &encrypted_len,
            nonce, associated_data, counter);


    static uint8_t final_message[MAX_PAYLOAD_SIZE] = {0};
    memcpy(final_message, encrypted_payload, encrypted_len);
    memcpy(final_message + encrypted_len, nonce, NONCE_SIZE);
    memcpy(final_message + encrypted_len + NONCE_SIZE, associated_data, ad_len);

    size_t final_message_len = encrypted_len + NONCE_SIZE + ad_len;

    // pretty_print("Sending encrypted temperature\n", final_message, final_message_len);

    int status = att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE,
        final_message, final_message_len);
    
    if (status != 0) {
        printf("❌ BLE notification failed! Status: %d, Seq Num: %d\n", status, counter);
    } else {
        log_end_sending_processing_time(counter);
        counter++;
    }
}

void send_plaintext_temperature() {
    log_start_time(counter);

    char associated_data[50];
    snprintf(associated_data, sizeof(associated_data), "|%s|%d", sensor_ID, counter);
    size_t ad_len = strlen(associated_data);

    const size_t reserved_meta = NONCE_SIZE + ad_len;
    const size_t max_encrypted_payload_size = MAX_PAYLOAD_SIZE - reserved_meta;
    uint8_t plaintext[max_encrypted_payload_size];

    static uint8_t final_message[MAX_PAYLOAD_SIZE] = {0};
    size_t plaintext_len = sizeof(current_temps);

    memcpy(final_message, plaintext, plaintext_len);
    memcpy(final_message + plaintext_len, associated_data, ad_len);

    size_t final_message_len = plaintext_len + ad_len;
    // pretty_print("Sending plaintext temperature\n", final_message, final_message_len);

    int status = att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE,
        final_message, final_message_len);
    
    if (status != 0) {
        printf("❌ BLE notification failed! Status: %d, Seq Num: %d\n", status, counter);
    } else {
        log_end_sending_processing_time(counter);
        counter++;
    }
}


 void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
     UNUSED(size);
     UNUSED(channel);
     if (packet_type != HCI_EVENT_PACKET) return;

     uint8_t event_type = hci_event_packet_get_type(packet);
     switch(event_type){
         case BTSTACK_EVENT_STATE:
             if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
             bd_addr_t local_addr;
             gap_local_bd_addr(local_addr);
            //  printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
 
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
             if (counter < MAX_PACKETS) {
                if (SELECTED_ENCRYPTION_MODE != ENCRYPTION_NONE) {
                    send_encrypted_temperature();
                }
                else {
                    send_plaintext_temperature();
                }
             } else {
                 if (active_transfer.data == NULL) {
                    //  Sleep for a bit to allow the last packet to be sent
                     sleep_ms(5000);
                     print_all_results();
                     send_struct_data(RTT_table, sizeof(RTT_table), "RTT", TRANSFER_RTT);
                 } else {
                    sleep_ms(100);
                    send_next_chunk();
                 }
             }
             break;
         default:
             break;
     }
 }
 

 uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
    uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE) {
    return att_read_callback_handle_blob((const uint8_t *)&current_temps,
                        sizeof(current_temps), offset,
                        buffer, buffer_size);
    }
    return 0;
}

void recieve_encrypted_data(uint8_t *received_data, size_t received_len, uint16_t *sequence_number) {
    uint8_t *decrypted_data = NULL;
    size_t decrypted_len = 0;

    if (received_len > MAX_PACKET_SIZE) {
        printf("Received packet is too large! Rejecting.\n");
        return;
    }
    
    int status = decrypt(received_data, received_len, &decrypted_data, &decrypted_len, sequence_number);

    if (status == 0) {
        if (decrypted_len % sizeof(uint16_t) != 0) {
            printf("❌ Decrypted data not aligned. Length = %zu\n", decrypted_len);
            free(decrypted_data);
            return;
        }
        
        int num_entries = decrypted_len / sizeof(uint16_t);
        uint16_t *temperatures = (uint16_t *)decrypted_data;
        
        // printf("✅ Decrypted Temperatures successfully! Seq Num: %d\n", sequence_number);
        
        free(decrypted_data);
    } else {
        printf("Decryption failed! Invalid or tampered message.\n");
    }
}


int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    uint64_t start_time = get_start_recieving_processing_time();
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    // printf("🔹 ATT Write Callback Triggered! Handle: 0x%X\n", att_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_CLIENT_CONFIGURATION_HANDLE){
        le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
        con_handle = connection_handle;
        if (le_notification_enabled) {
            att_server_request_can_send_now_event(con_handle);
        } else {
            printf("Notifications Disabled!\n");
        }
        return 0;
    }

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE) {
        if (buffer_size > 0) {
            // Decrypt and print data
            uint16_t sequence_number = 0;
            recieve_encrypted_data(buffer, buffer_size, &sequence_number);
            log_end_recieving_processing_time(sequence_number);
            log_start_recieving_processing_time(sequence_number, start_time);
        } else {
            printf("Empty Data Received!\n");
        }
        return 0;
    }

    return 0;
}


void poll_temp(void) {
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    uint32_t raw32 = adc_read();
    const uint32_t bits = 12;
    uint16_t current_temp;

    // Scale raw reading to 16 bit value using a Taylor expansion (for 8 <= bits <= 16)
    uint16_t raw16 = raw32 << (16 - bits) | raw32 >> (2 * bits - 16);

    // ref https://github.com/raspberrypi/pico-micropython-examples/blob/master/adc/temperature.py
    const float conversion_factor = 3.3 / (65535);
    float reading = raw16 * conversion_factor;
    
    // The temperature sensor measures the Vbe voltage of a biased bipolar diode, connected to the fifth ADC channel
    // Typically, Vbe = 0.706V at 27 degrees C, with a slope of -1.721mV (0.001721) per degree. 
    float deg_c = 27 - (reading - 0.706) / 0.001721;
    current_temp = (uint16_t)(deg_c * 100);
    // printf("Write temp %.2f degc\n", deg_c);

    // Shift left to make room for new value
    for (int i = 0; i < PAYLOAD_MULTIPLE - 1; i++) {
        current_temps.values[i] = current_temps.values[i + 1];
    }

    // Store latest value
    current_temps.values[PAYLOAD_MULTIPLE - 1] = current_temp;
}