#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_
#include <stdint.h>

int control_socket, router_socket, data_socket;
uint16_t periodic_interval;
int my_id; // -1
struct timeval periodic_timer = {0, 0};

void init();

#endif