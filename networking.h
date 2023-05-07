#pragma once

#include <stdbool.h>

#define ROLE_SERVER 0
#define ROLE_CLIENT 1

typedef struct {
	bool connected;
	int role;

	char *server;
	int port;

	int socket;
} network_info;

network_info create_network_info(char *, int);
bool connect_with_network_info(network_info *);
void disconnect_with_network_info(network_info *);
