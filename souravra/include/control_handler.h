#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

int create_control_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);

bool routing_recv_hook();

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
int crash(int sock_index);

int create_router_sock(uint16_t routing_port);

void handle_timer_event();
void send_routing_table_to_peers();
char* get_distace_vector_tosend();

void update_routing_table(char* distance_vector);

#endif