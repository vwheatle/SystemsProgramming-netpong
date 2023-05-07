#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <string.h>

#include <sys/types.h> // -> size_t, ssize_t
#include <sys/socket.h>

#include <unistd.h>    // -> write, read
#include <fcntl.h>     // -> open
#include <netdb.h>     // -> getaddrinfo
#include <arpa/inet.h> // -> htons

#include "networking.h"

network_role other_role(network_role role) {
	switch (role) {
	case ROLE_CLIENT: return ROLE_SERVER;
	case ROLE_SERVER: return ROLE_CLIENT;
	default: return ROLE_INVALID;
	}
}

network_info create_network_info(char *server, int port) {
	network_info i;
	i.connected = false;
	i.role = server == NULL ? ROLE_SERVER : ROLE_CLIENT;

	i.server = server;
	i.port = port;

	i.socket = -1;
	return i;
}

bool connect_with_network_info(network_info *i) {
	// adapted from my Computer Communication Networks Lab2 project:
	// https://github.com/vwheatle/ComputerCommunicationNetworks-Lab2
	// and that was adapted from this guide:
	// https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch

	i->connected = false;

	// give getaddrinfo some hints on what we want from the helper object
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;      // don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // must be SOCK_STREAM to be accept()ed
	hints.ai_flags |= AI_NUMERICSERV; // the service will be a port number

	// if you're the server, use your current local address.
	if (i->role == ROLE_SERVER) hints.ai_flags |= AI_PASSIVE;

	// stringify the port, save it into this buffer
	char port_buff[16];
	snprintf(port_buff, sizeof(port_buff) / sizeof(*port_buff), "%d", i->port);

	struct addrinfo *sv_info;
	int gai_status = getaddrinfo(i->server, port_buff, &hints, &sv_info);
	if (gai_status != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(gai_status));
		freeaddrinfo(sv_info);
		return false;
	}

	i->socket =
		socket(sv_info->ai_family, sv_info->ai_socktype, sv_info->ai_protocol);
	if (i->socket == -1) {
		perror("couldn't make socket");
		freeaddrinfo(sv_info);
		return false;
	}
	fprintf(stderr, "made socket\n");

	if (i->role == ROLE_SERVER) {
		fprintf(stderr, "i'm a server\n");

		// you're the server, bind your socket to
		// the IP that getaddrinfo found for you

		// but first, discard any old socket still
		// hanging around and waiting to time out
		// https://stackoverflow.com/a/5730516/
		int flag = 1;
		setsockopt(i->socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

		if (bind(i->socket, sv_info->ai_addr, sv_info->ai_addrlen) == -1) {
			perror("couldn't bind socket");
			close(i->socket);
			i->socket = -1;
			freeaddrinfo(sv_info);
			return false;
		}
		fprintf(stderr, "socket successfully bound\n");

		if (listen(i->socket, 1) == -1) {
			perror("couldn't listen for client");
			close(i->socket);
			i->socket = -1;
			freeaddrinfo(sv_info);
			return false;
		}
		fprintf(stderr, "listening for clients...\n");

		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);

		struct sockaddr *client_addr_p = (struct sockaddr *)&client_addr;

		// if this is valid, we can safely dispose of the first socket we made
		// and use this connection descriptor for the lifetime of the game.
		int connection = accept(i->socket, client_addr_p, &client_len);
		if (connection == -1) {
			perror("couldn't accept client");
			close(i->socket);
			i->socket = -1;
			freeaddrinfo(sv_info);
			return false;
		}

		fprintf(stderr, "got a client!\n");
		close(i->socket);
		i->socket = connection;
		i->connected = true;
	} else if (i->role == ROLE_CLIENT) {
		fprintf(stderr, "i'm a client\n");

		if (connect(i->socket, sv_info->ai_addr, sv_info->ai_addrlen) == -1) {
			perror("couldn't connect to server");
			close(i->socket);
			i->socket = -1;
			freeaddrinfo(sv_info);
			return false;
		}

		fprintf(stderr, "connected to a server!\n");
		i->connected = true;
	} else {
		fprintf(stderr, "i don't know who i am.\n");
	}

	return i->connected;
}

void disconnect_with_network_info(network_info *i) {
	if (i->socket != -1) {
		close(i->socket);
		i->socket = -1;
	}
	i->connected = false;
}
