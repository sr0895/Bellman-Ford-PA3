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

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes)
{
    struct sockaddr_in addr;
    socklen_t fromlen = sizeof(addr);
    ssize_t bytes = 0;
    bytes = recvfrom(sock_index, buffer, nbytes, 0, &addr, &fromlen);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0, &addr, &fromlen);

    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

ssize_t sendtoALL(char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port)
{

    struct sockaddr_in ip4addr;

    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(port);
    ip4addr.sin_addr.sin_addr = htons(ip);
    struct sockaddr* to =  (struct sockaddr*)&ip4addr;

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("failed to creater UDP socket");
        exit(1);
    }

    ssize_t bytes = 0;
    bytes = sendto(sockfd, buffer, nbytes, 0, to, sizeof(to));

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += sendto(sockfd, buffer+bytes, nbytes-bytes, 0, to, sizeof(to));

    return bytes;
}
