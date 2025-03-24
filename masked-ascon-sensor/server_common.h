/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SERVER_COMMON_H_
#define SERVER_COMMON_H_

#include "btstack.h"

#define ADC_CHANNEL_TEMPSENSOR 4
#define MAX_PACKETS 15

extern int le_notification_enabled;
extern hci_con_handle_t con_handle;
extern uint16_t current_temp;
extern uint8_t const profile_data[];
extern uint8_t sensor_ID[];
typedef struct {
    uint16_t seq_num;
    uint64_t start_time;
    uint64_t end_time;
} data_entry;

extern data_entry encryption_times[MAX_PACKETS];
extern data_entry decryption_times[MAX_PACKETS];

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
void poll_temp(void);
void log_end_time(uint16_t seq_num);
#endif
