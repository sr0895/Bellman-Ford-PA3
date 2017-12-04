#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_
#include <stdint.h>
#include <sys/select.h>

int control_socket, router_socket, data_socket;
uint16_t periodic_interval;
int my_id; // -1
struct timeval periodic_timer;

fd_set master_list, watch_list;
int head_fd;

void init();

#endif