#pragma once

#include <stdbool.h>

typedef enum { ROLE_SERVER = 0, ROLE_CLIENT = 1, ROLE_INVALID } network_role;

network_role other_role(network_role);

typedef struct {
	bool connected;
	network_role role;

	char *server;
	int port;

	int socket;
} network_info;

network_info create_network_info(char *, int);
bool connect_with_network_info(network_info *);
void disconnect_with_network_info(network_info *);
