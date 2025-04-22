/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h" //Manages Bluetooth communication
#include "pico/cyw43_arch.h" // Required for WiFi and Bluetooth
#include "pico/btstack_cyw43.h" 
#include "hardware/adc.h" // Required for temperature sensor
#include "pico/stdlib.h" 
#include "encryption.h"
#include "experiment_settings.h"
#include "server_common.h"
#include "hardware/sync.h"


#define HEARTBEAT_PERIOD_MS transmission_interval_ms


static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
int current_scenario = 1;


static void heartbeat_handler(struct btstack_timer_source *ts) {
    
    poll_temp(); // Poll the temperature sensor

    if (le_notification_enabled) { // If BLE notifications are enabled
        att_server_request_can_send_now_event(con_handle); // Send the temperature value
    }

    // Restart timer
    btstack_run_loop_set_timer(ts, transmission_interval_ms);
    btstack_run_loop_add_timer(ts);

}


int main() {
    stdio_init_all();



    // Initialize the PRNG
    init_primitives();
    configure_scenario(current_scenario);
    allocate_temperature_buffer();
    init_timing_logging();
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Initialise adc for the temp sensor
    adc_init();
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);

    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);
    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, transmission_interval_ms);
    btstack_run_loop_add_timer(&heartbeat);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON); 
    
    // btstack_run_loop_execute is only required when using the 'polling' method (e.g. using pico_cyw43_arch_poll library).
    // This example uses the 'threadsafe background` method, where BT work is handled in a low priority IRQ, so it
    // is fine to call bt_stack_run_loop_execute() but equally you can continue executing user code.

#if 0 // btstack_run_loop_execute() is not required, so lets not use it
    btstack_run_loop_execute();
#else
    // this core is free to do it's own stuff except when using 'polling' method (in which case you should use 
    // btstacK_run_loop_ methods to add work to the run loop.
    
    // this is a forever loop in place of where user code would go.
    while(true) {
        sleep_ms(1000);
        // __wfi(); // wait for interrupt
    }
#endif
    return 0;
}
