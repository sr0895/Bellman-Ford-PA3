#ifndef NETWORK_UTIL_H_
#define NETWORK_UTIL_H_

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes);
ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes);

ssize_t sendtoALL(char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port);
ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes);

bool is_socket_open(int sock_index);

#endif