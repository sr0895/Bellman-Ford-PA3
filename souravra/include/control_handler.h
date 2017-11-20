#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

int create_control_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);

/* Routing Table*/
struct TOPOLOGY
{
    uint16_t router_id;
    uint16_t routing_port;
    uint16_t data_port;
    uint16_t link_cost;
    uint32_t ip_addr;
} topology[5];
#ifdef PACKET_USING_STRUCT
	struct __attribute__((__packed__)) ROUTING_TABLE
	{
	    uint16_t router_id;
	    uint16_t zero;
	    uint16_t next_hop;
	    uint16_t path_cost;
	} routing_table[5];
#endif

/* ops */
int init_response(int sock_index, char* cntrl_payload, uint16_t payload_len);
int init_routing_table();
int send_routing_table(int sock_index);

#endif