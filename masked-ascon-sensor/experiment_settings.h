#ifndef EXPERIMENT_SETTINGS_H
#define EXPERIMENT_SETTINGS_H

// Base size and interval definitions
#define INTERVAL_1_MINUTES   60000
#define INTERVAL_10_SECONDS   10000
#define INTERVAL_1_SECOND     1000
#define INTERVAL_500_MS     500

// Define scenario selector (Set this to 1–6)
#define SELECTED_SCENARIO     10



// Scenario switch
 // 2 byte, 10 seconds
#if SELECTED_SCENARIO == 1
    #define MAX_PACKETS           500
    #define PAYLOAD_MULTIPLE 1
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
 // 10 bytes, 10 seconds
#elif SELECTED_SCENARIO == 2
    #define MAX_PACKETS           500    
    #define PAYLOAD_MULTIPLE     5 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
 // 100 bytes, 10 seconds
#elif SELECTED_SCENARIO == 3
    #define MAX_PACKETS           500
    #define PAYLOAD_MULTIPLE     50 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
 // 2 bytes, 1 second
#elif SELECTED_SCENARIO == 4
    #define MAX_PACKETS           500
    #define PAYLOAD_MULTIPLE     100
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_SECOND
 // 10 bytes, 1 second
#elif SELECTED_SCENARIO == 5
    #define MAX_PACKETS           100    
    #define PAYLOAD_MULTIPLE     1 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_10_SECONDS
 // 100 bytes, 1 second
#elif SELECTED_SCENARIO == 6
    #define MAX_PACKETS           100 
    #define PAYLOAD_MULTIPLE     5
    #define TRANSMISSION_INTERVAL_MS INTERVAL_10_SECONDS
#elif SELECTED_SCENARIO == 7
    #define MAX_PACKETS           100 
    #define PAYLOAD_MULTIPLE     50
    #define TRANSMISSION_INTERVAL_MS INTERVAL_10_SECONDS
#elif SELECTED_SCENARIO == 8
    #define MAX_PACKETS           100 
    #define PAYLOAD_MULTIPLE     100
    #define TRANSMISSION_INTERVAL_MS INTERVAL_10_SECONDS
 // 100 bytes, 1 second
 #elif SELECTED_SCENARIO == 9
    #define MAX_PACKETS           100 
    #define PAYLOAD_MULTIPLE     1 
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_MINUTES
#elif SELECTED_SCENARIO == 10
    #define MAX_PACKETS           100
    #define PAYLOAD_MULTIPLE     5
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_MINUTES
#elif SELECTED_SCENARIO == 11
    #define MAX_PACKETS           100
    #define PAYLOAD_MULTIPLE     50
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_MINUTES
#elif SELECTED_SCENARIO == 12
    #define MAX_PACKETS           100
    #define PAYLOAD_MULTIPLE     100
    #define TRANSMISSION_INTERVAL_MS INTERVAL_1_MINUTES
#else
    #error "Invalid SELECTED_SCENARIO (must be 1 to 6)"
#endif

#endif // EXPERIMENT_SETTINGS_H
