#pragma once

#include "freertos/event_groups.h"

// Signals that we are connected to the network
#define WIFI_EVENT_CONNECTED_BIT BIT0
// Signals connection to the network failed
#define WIFI_EVENT_FAIL_BIT BIT1
// Signals that all sockets should be closed / recreated
#define WIFI_EVENT_RECREATE_SOCKETS_BIT BIT2

// This event group is not initiated until after the wifi setup is complete.
EventGroupHandle_t wifi_event_group;