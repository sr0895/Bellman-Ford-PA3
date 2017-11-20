/**
 * @control_handler
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
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/control_handler.h"
#include "../include/author.h"
#include "../include/connection_manager.h"

#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&control_conn_list);
    printf("%s, %d\n", "started listing on  ", CONTROL_PORT);

    return sock;
}

int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

int convert_topology_ntoh() {
    for (int i = 0; i < 5; ++i)
    {
       topology[i].router_id = ntohs(topology[i].router_id);
       topology[i].routing_port = ntohs(topology[i].routing_port);
       topology[i].data_port = ntohs(topology[i].data_port);
       topology[i].link_cost = ntohs(topology[i].link_cost);
       topology[i].ip_addr = ntohs(topology[i].ip_addr);
    }
}

int init_response(int sock_index, char* cntrl_payload, uint16_t payload_len) {
    uint16_t num_routers; memcpy(&num_routers, cntrl_payload, sizeof(num_routers));
    num_routers = htons(num_routers);
    assert(num_routers == 5);
    
    memcpy(&periodic_interval, cntrl_payload + sizeof(num_routers), sizeof(periodic_interval));
    periodic_interval = htons(periodic_interval);

    // update routing table

    printf("patload len %d, size of topology %ld\n", payload_len, sizeof(topology[0]) );
    assert(((payload_len - sizeof(num_routers) - sizeof(periodic_interval)) / sizeof(topology[0])) == 5);
    for (int i = 0; i < 5; i++)
    {
        int offset = (i * sizeof(topology[0])) + sizeof(num_routers) + sizeof(periodic_interval);
        
        uint16_t id; memcpy(&id, cntrl_payload + offset, sizeof(id)); offset+= sizeof(id);
        
        topology[id -1].router_id = id;

        memcpy(&topology[id - 1].routing_port, cntrl_payload + offset, sizeof(topology[id-1].routing_port));
        offset += sizeof(topology[id-1].routing_port);
        
        memcpy(&topology[id - 1].data_port, cntrl_payload + offset, sizeof(topology[id-1].data_port));
        offset += sizeof(topology[id-1].data_port);

        memcpy(&topology[id - 1].link_cost, cntrl_payload + offset, sizeof(topology[id-1].link_cost));
        offset += sizeof(topology[id-1].link_cost);

        memcpy(&topology[id - 1].ip_addr, cntrl_payload + offset, sizeof(topology[id-1].ip_addr));
        offset += sizeof(topology[id-1].ip_addr);

        convert_topology_ntoh();
        printf("converted to host\n");

        if (topology[id - 1].link_cost == 0)
        {
            /* me */
            my_id = id - 1;
        }

        // setup listner on routing port

        // setup listner on data port

        // init routing table
        init_routing_table();

        // send ack
        char *cntrl_response_header;
        cntrl_response_header = create_response_header(sock_index, 1, 0, 0);
        sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
        printf("sent control resp header of len %d\n", CNTRL_RESP_HEADER_SIZE);
        free(cntrl_response_header);
    }

}

int init_routing_table() {
    printf("init routing table\n");
    uint16_t my_next_hop = UINT16_MAX;
    uint16_t cost = UINT16_MAX; 
    for (uint16_t i = 0; i < 5; ++i)
    {
        routing_table[i].router_id = htons(i);
        if (i != my_id)
        {
            routing_table[i].next_hop = htons(UINT16_MAX);
            routing_table[i].next_hop = htons(UINT16_MAX);
        }

        if (cost > topology[i].link_cost)
        {
            cost = topology[i].link_cost;
            my_next_hop = i;
        }
    }
    routing_table[my_id].next_hop = htons(my_next_hop);
}

int send_routing_table(int sock_index) {
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = sizeof(routing_table[0]) * 5;
    for (int i = 0; i < 5; ++i)
    {
        memcpy(cntrl_response_payload + i*sizeof(routing_table[0]), &routing_table[i], sizeof(routing_table[0]));
    }

    cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
    /* Copy Payload */
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
}


bool control_recv_hook(int sock_index)
{
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return FALSE;
    }

    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_HEADER) == 8) 
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */
        BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.

        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
    #endif
    #ifndef PACKET_USING_STRUCT
        memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
        memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
        payload_len = ntohs(payload_len);
    #endif

    free(cntrl_header);

    /* Get control payload */
    if(payload_len != 0){
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return FALSE;
        }
    }

    /* Triage on control_code */
    switch(control_code){
        case 0: author_response(sock_index);
                break;

        case 1: init_response(sock_index, cntrl_payload, payload_len);
                break;

        case 2: send_routing_table(sock_index);
                break;
    }

    if(payload_len != 0) free(cntrl_payload);
    return TRUE;
}