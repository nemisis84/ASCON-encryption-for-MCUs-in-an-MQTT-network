#ifndef EXPERIMENT_SETTINGS_H
#define EXPERIMENT_SETTINGS_H

// Timing constants
#define INTERVAL_1_MINUTES    60000
#define INTERVAL_10_SECONDS   10000
#define INTERVAL_1_SECOND     1000
#define INTERVAL_500_MS       500

// Runtime configuration variables
extern int max_packets;
extern int payload_multiple;
extern int transmission_interval_ms;

// Scenario configuration function
void configure_scenario(int scenario);

#endif
