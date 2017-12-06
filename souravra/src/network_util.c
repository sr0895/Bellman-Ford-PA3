/**
 * @network_util
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Network I/O utility functions. send/recvALL are simple wrappers for
 * the underlying send() and recv() system calls to ensure nbytes are always
 * sent/received.
 */

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../include/global.h"

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);
    lprint("NET_DEBUG: recvAll initial bytes %d out of %d\n", bytes, nbytes);
    if(bytes < 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes)
{
    struct sockaddr_in addr;
    socklen_t fromlen = sizeof(addr);
    ssize_t bytes = 0;
    bytes = recvfrom(sock_index, buffer, nbytes, 0, (struct sockaddr*) &addr, &fromlen);
    lprint("NET_DEBUG: recvfromALL initial bytes %d out of %d\n", bytes, nbytes);
    if(bytes < 0) return -1;
    while(bytes != nbytes)
        bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0, (struct sockaddr*) &addr, &fromlen);
    lprint("NET_DEBUG: recvfromALL\n");
    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);
    lprint("NET_DEBUG: sendALL initial bytes %d out of %d\n", bytes, nbytes);

    if(bytes < 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);
    lprint("NET_DEBUG: sendALL\n");

    return bytes;
}

ssize_t sendtoALL(char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port)
{
    int sockfd;
    struct sockaddr_in ip4addr;

    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(port);
    ip4addr.sin_addr.s_addr = htonl(ip);
    struct sockaddr* to =  (struct sockaddr*)&ip4addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        ERROR("ERROR opening UDP socket");

    ssize_t bytes = 0;
    bytes = sendto(sockfd, buffer, nbytes, 0, to, sizeof(ip4addr));

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip4addr.sin_addr), str, INET_ADDRSTRLEN);
    lprint("NET_DEBUG: sendtoALL initial send bytes %d, outof %d tp %ld, ip %s\n", bytes, nbytes, port, str);

    if(bytes < 0) return -1;
    while(bytes != nbytes)
        bytes += sendto(sockfd, buffer+bytes, nbytes-bytes, 0, to, sizeof(ip4addr));
    lprint("NET_DEBUG: sendtoALL sent all\n");
    return bytes;
}

bool is_socket_open(int sock_index) {
    int error = 0;
    socklen_t len = sizeof (error);
    int retval = getsockopt (sock_index, SOL_SOCKET, SO_ERROR, &error, &len);

    return !(retval || error) ;
}

