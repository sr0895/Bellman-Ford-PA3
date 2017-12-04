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
    lprint("%s, %d\n", "started listing on  ", CONTROL_PORT);

    return sock;
}

int create_router_sock(uint16_t routing_port) {
    int sock;
    struct sockaddr_in rountng_addr;
    socklen_t addrlen = sizeof(rountng_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&rountng_addr, sizeof(rountng_addr));

    rountng_addr.sin_family = AF_INET;
    rountng_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rountng_addr.sin_port = htons(routing_port);

    if(bind(sock, (struct sockaddr *)&rountng_addr, sizeof(rountng_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&control_conn_list);
    lprint("%s, %d\n", "started listing on  ", routing_port);

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
void print_topo() {
    for (int i = 0; i < 5; ++i)
    {
        lprint("ith topology %d, id %d, rp %d, dp %d, lc %d, ip %d\n", i,
                topology[i].router_id,
                topology[i].routing_port,
                topology[i].data_port,
                topology[i].link_cost,
                topology[i].ip_addr);
    }
}

void print_routing_table() {
    for (int i = 0; i < 5; ++i)
    {
        lprint("ith router %d, id %d, 0 %d, nh %d, c %d\n", i,
                ntohs(routing_table[i].router_id),
                ntohs(routing_table[i].zero),
                ntohs(routing_table[i].next_hop),
                ntohs(routing_table[i].path_cost));
    }
}

int init_routing_table() {
    lprint("init routing table, my_id %d\n", my_id);
    print_topo();
    uint16_t my_next_hop = UINT16_MAX;
    uint16_t cost = UINT16_MAX; 
    for (uint16_t i = 0; i < 5; ++i)
    {
        routing_table[i].router_id = htons(topology[i].router_id);
        routing_table[i].next_hop = topology[i].link_cost < UINT16_MAX ?
                                    htons(topology[i].router_id) :
                                    UINT16_MAX;
        routing_table[i].path_cost = htons(topology[i].link_cost);
    }
    print_routing_table();
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

int send_rep_header(int sock_index, int control_code) {
    // send ack
    char *cntrl_response_header;
    cntrl_response_header = create_response_header(sock_index, control_code, 0, 0);
    sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    lprint("sent control resp header of len %d\n", CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
    return 0;
}

int init_response(int sock_index, char* cntrl_payload, uint16_t payload_len) {
    uint16_t num_routers; memcpy(&num_routers, cntrl_payload, sizeof(num_routers));
    num_routers = htons(num_routers);
    assert(num_routers == 5);
    
    memcpy(&periodic_interval, cntrl_payload + sizeof(num_routers), sizeof(periodic_interval));
    periodic_interval = htons(periodic_interval);
    lprint("periodic_interval %ld\n", periodic_interval)

    // set periodic timer
    periodic_timer.tv_sec = periodic_interval;

    // update routing table

    lprint("payload len %d, size of topology %ld\n", payload_len, sizeof(topology[0]) );
    assert(((payload_len - sizeof(num_routers) - sizeof(periodic_interval)) / sizeof(topology[0])) == 5);
    for (uint16_t i = 0; i < 5; i++)
    {
        lprint("server %d\n", i);
        int offset = (i * sizeof(topology[0])) + sizeof(num_routers) + sizeof(periodic_interval);
        
        uint16_t nid; memcpy(&nid, cntrl_payload + offset, sizeof(nid)); offset+= sizeof(nid);
        lprint("id memcpy done, id = %d, nid = %d\n", i, nid);

        topology[i].router_id = nid;
        lprint("id assign done\n");

        memcpy(&topology[i].routing_port, cntrl_payload + offset, sizeof(topology[i].routing_port));
        offset += sizeof(topology[i].routing_port);
        lprint("rport memcpy done\n");

        memcpy(&topology[i].data_port, cntrl_payload + offset, sizeof(topology[i].data_port));
        offset += sizeof(topology[i].data_port);
        lprint("dport memcpy done\n");

        memcpy(&topology[i].link_cost, cntrl_payload + offset, sizeof(topology[i].link_cost));
        offset += sizeof(topology[i].link_cost);
        lprint("cost memcpy done\n");

        memcpy(&topology[i].ip_addr, cntrl_payload + offset, sizeof(topology[i].ip_addr));
        offset += sizeof(topology[i].ip_addr);
        lprint("all memcpy done\n");

        if (topology[i].link_cost == 0)
        {
            /* me */
            my_id = i;
        }
    }

    convert_topology_ntoh();
    lprint("converted to host\n");
    // setup listner on routing port
    router_socket = create_router_sock();

    // setup listner on data port

    // init routing table
    init_routing_table();

    send_rep_header(sock_index, 1);
}

int send_routing_table(int sock_index) {
    lprint("send_routing_table\n");
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = sizeof(routing_table[0]) * 5;
    cntrl_response_payload = (char*)malloc(payload_len);

    memcpy(cntrl_response_payload, routing_table, payload_len);

    lprint("created payload buffer\n");
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
    lprint("sent routing table, of size %d\n", response_len);
    free(cntrl_response);
}

int crash(int sock_index) {
    lprint("crashing.............");
    running_app = FALSE;
    send_rep_header(sock_index, 4);

    remove_control_conn(sock_index);

    // remove all server sockets i.e control, router, data
    remove_control_conn(control_socket);
    //remove_control_conn(router_socket);
    //remove_control_conn(data_socket);
    return 0;
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

        case 4: crash(sock_index);
                break;
    }

    if(payload_len != 0) free(cntrl_payload);
    return TRUE;
}

char* get_distace_vector_tosend() {
    char* distance_vector = (char* )malloc(DISTANCE_VECTOR_SIZE);
    int offset = 0;
    uint16_t num_router = 5;

    memcpy(distance_vector + offset, &htons(num_router), sizeof(num_router));
    offset += sizeof(num_router);

    memcpy(distance_vector + offset, &htons(topology[my_id].routing_port), sizeof(topology[my_id].routing_port));
    offset += sizeof(topology[my_id].routing_port);

    memcpy(distance_vector + offset, &htons(topology[my_id].ip_addr), sizeof(topology[my_id].ip_addr));
    offset += sizeof(topology[my_id].ip_addr);

    for (int i = 0; i < 5; i++) {
        memcpy(distance_vector + offset, &htons(topology[ntohs(routing_table[i].router_id)].ip_addr), sizeof(topology[i].ip_addr));
        offset += sizeof(topology[i].ip_addr);

        memcpy(distance_vector + offset, &htons(topology[ntohs(routing_table[i].router_id)].routing_port), sizeof(topology[i].routing_port));
        offset += sizeof(topology[i].routing_port);

        memcpy(distance_vector + offset, &0, sizeof(num_router));
        offset += sizeof(num_router);

        memcpy(distance_vector + offset, &routing_table[i].router_id, sizeof(routing_table[i].router_id)); // routing table is in ntoh
        offset += sizeof(routing_table[i].router_id);

        memcpy(distance_vector + offset, &routing_table[i].path_cost, sizeof(routing_table[i].path_cost));
        offset += sizeof(routing_table[i].path_cost);
    }

    return distance_vector;
}

void handle_timer_event() {
    if (!periodic_interval) return;
    send_routing_table_to_peers();
}

void send_routing_table_to_peers() {
    char* distance_vector = get_distace_vector_tosend();
    for (int i = 0; i < 5; i++) {
        if(topology[i].link_cost < UINT16_MAX) {
            lprint("router %ls is peer, sending distance vevtor to it\n", topology[i].router_id);
            assert(sendtoAll(distance_vector, DISTANCE_VECTOR_SIZE, topology[i].ip_addr, topology[i].routing_port) == DISTANCE_VECTOR_SIZE);
        }
    }
}

bool routing_recv_hook() {
    char* distance_vector = (char* )malloc(DISTANCE_VECTOR_SIZE);
    assert(recvfromALL(routing_port, distance_vector, DISTANCE_VECTOR_SIZE) == DISTANCE_VECTOR_SIZE);
    update_routing_table(distance_vector);
}

void update_routing_table(char* distance_vector) {
    //sanity check
    uint16_t num_routers; memcpy(&num_routers, distance_vector, sizeof(num_routers));
    num_routers = htons(num_routers);
    assert(num_routers == 5);

    uint16_t sender_port = memcpy(&sender_port, distance_vector + sizeof(num_routers), sizeof(sender_port));
    sender_port = ntohs(sender_port);
    
    // find sender id
    uint16_t sender_id = 0;
    for (sender_id; i < 5; sender_id++) {
        if(sender_port == topology[sender_port].routing_port) {
            sender_id = topology[sender_port].router_id;
            lprint("get distace distance_vector from %ld on port %ld\n", sender_id, sender_port);
            break;
        }
    }

    // find my cost to sender
    uint16_t cost_to_sender;
    for (int i = 0; i < 5; i++) {
        if(routing_table[i].router_id == htons(sender_id)) {
            cost_to_sender = ntohs(routing_table[i].path_cost);
        }
    }


    int offset = 8; // 8 bytes into buffer
    for (int i = 0; i < 5; i++) {
        offset +  = 8; // only inetersted in id and cost
        uint16_t hop_id;memcpy(&hop_id, distance_vector + offset, sizeof(hop_id)); //routing table has is in network format
        offset += sizeof(hop_id);

        uint16_t hop_cost;memcpy(&hop_cost, distance_vector + offset, sizeof(hop_cost)); hop_cost = ntohs(hop_cost);
        offset += sizeof(hop_cost);

        // need to find my cost to hop_id
        int j = 0;
        for (j; j < 5; j++) {
            if(routing_table[j].router_id == hop_id) break;
        }

        uint16_t my_cost_to_hop = ntohs(routing_table[j].path_cost);

        //bellman ford
        uint16_t path_cost_via_peer = cost_to_sender + hop_cost;

        if(path_cost_via_peer < my_cost_to_hop) {
            lprint("found new path to %ld via %ld at cost %ld\n", ntohs(hop_id), sender_id, path_cost_via_peer);
            // update routing table
            routing_table[j].next_hop = htons(sender_id); // keep network format
            routing_table[j].path_cost = htons(path_cost_via_peer);
        }
    }    
}
