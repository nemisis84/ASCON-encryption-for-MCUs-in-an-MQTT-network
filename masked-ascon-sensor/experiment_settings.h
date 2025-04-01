#ifndef EXPERIMENT_SETTINGS_H
#define EXPERIMENT_SETTINGS_H

// Base size and interval definitions
#define INTERVAL_10_SECONDS   10000
#define INTERVAL_1_SECOND     1000
#define INTERVAL_500_MS     500

// Define scenario selector (Set this to 1–6)
#define SELECTED_SCENARIO     1

// Set MAX_PACKETS globally
#define MAX_PACKETS           100

// Scenario switch
 // 2 byte, 10 seconds
#if SELECTED_SCENARIO == 1
    #define PAYLOAD_MULTIPLE 1
    #define TRANSMISSION_INTERVAL_MS INTERVAL_500_MS
 // 10 bytes, 10 seconds
#elif SELECTED_SCENARIO == 2
    #define PAYLOAD_MULTIPLE     5 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_500_MS
 // 100 bytes, 10 seconds
#elif SELECTED_SCENARIO == 3
    #define PAYLOAD_MULTIPLE     50 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_500_MS
 // 2 bytes, 1 second
#elif SELECTED_SCENARIO == 4
    #define PAYLOAD_MULTIPLE     100
    #define TRANSMISSION_INTERVAL_MS INTERVAL_500_MS
 // 10 bytes, 1 second
#elif SELECTED_SCENARIO == 5
    #define PAYLOAD_MULTIPLE     1 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
 // 100 bytes, 1 second
#elif SELECTED_SCENARIO == 6
    #define PAYLOAD_MULTIPLE     5
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
#elif SELECTED_SCENARIO == 7
    #define PAYLOAD_MULTIPLE     50
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
#elif SELECTED_SCENARIO == 8
    #define PAYLOAD_MULTIPLE     100
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND

#else
    #error "Invalid SELECTED_SCENARIO (must be 1 to 6)"
#endif

#endif // EXPERIMENT_SETTINGS_H
