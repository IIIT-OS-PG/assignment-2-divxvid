#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TRACKER_IP "127.0.0.1"
#define TRACKER_PORT 33333

std::vector<std::string> client_info_vector ;

struct transfer_unit
{
	int peer_socket ;
	struct sockaddr_in c_addr ;
};

void update_client_info_file()
{
	FILE *fp = fopen("client_info.txt", "w");
	if(fp == 0)
	{
		printf("The Client info file cannot be opened.\n") ;
	} else
	{
		for(const std::string &s : client_info_vector)
		{
			const char* tmp = s.c_str() ;
			fprintf(fp, "%s\n", tmp);
		}
	}
	fclose(fp) ;
}

int connect_peer(const char* IP, const int port)
{
	int active_sock ;
	if( (active_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Couldn't create the active socket.\n") ;
		return -1;
	}

	struct sockaddr_in act_sock ;
	bzero( (char*)&act_sock, sizeof act_sock );
	act_sock.sin_family = AF_INET ;
	act_sock.sin_port = htons(port) ;
	act_sock.sin_addr.s_addr = inet_addr(IP) ;

	//TODO now
	if(connect(active_sock, (struct sockaddr*)&act_sock, sizeof act_sock) < 0)
	{
		printf("cannot connect.\n") ;
		return -1; 
	}

	char msg[100] ;
	recv(active_sock, msg, sizeof msg, 0) ;
	printf("The server side of the client sent me : %s\n", msg) ;

	close(active_sock); 
	return 0 ;
}

void* communicate_peer(void* args)
{
	struct transfer_unit *tf = (transfer_unit*)args ;

	char data[128] ;
	recv(tf->peer_socket, data, sizeof data, 0);
	char IP[32] ;
	int port ;
	sscanf(data, "%s%d", IP, &port);

	printf("I got : %s and %d\n", IP, port) ;

	if(connect_peer(IP, port) == 0)
	{
		std::string temp(data) ;
		client_info_vector.push_back(temp);
		update_client_info_file();
	}

	char buff[32] = "Hello from the Tracker!\n" ;
	send(tf->peer_socket, buff, sizeof buff, 0) ;

	close(tf->peer_socket) ;
	printf("Client seen : %s : %d\n", inet_ntoa(tf->c_addr.sin_addr), tf->c_addr.sin_port);
	pthread_exit(NULL) ;
}

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("Please start the program with tracker_info and tracker number.\n");
		return 0 ;
	}

	const char* tracker_info_file = argv[1] ;
	const int tracker_number = atoi(argv[2]) ;	

	int tracker_socket, peer_socket ;
	if( (tracker_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("Socket cannot be created.\n");
		return 0 ;
	}

	struct sockaddr_in s_addr, c_addr ;

	bzero( (char*)&s_addr, sizeof s_addr );
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(TRACKER_PORT) ;
	s_addr.sin_addr.s_addr = inet_addr(TRACKER_IP) ;

	if(bind(tracker_socket, (struct sockaddr*) &s_addr, sizeof s_addr) < 0)
	{
		printf("Bind failed.\n");
		close(tracker_socket);
		return 0 ;
	}

	listen(tracker_socket, 10) ;
	unsigned int client_sock_size = sizeof c_addr ;

	/*
	peer_socket = accept(tracker_socket, (struct sockaddr*)&c_addr, &client_sock_size);
	if(peer_socket < 0)
	{
		printf("cannot connect to the peer.\n");
		close(tracker_socket) ;
		return 0 ;
	}

	char buff[32] = "Hello from the Tracker!\n" ;
	send(peer_socket, buff, sizeof buff, 0) ;

	close(peer_socket) ;
	*/
	while(1)
	{
		peer_socket = accept(tracker_socket, (struct sockaddr*)&c_addr, &client_sock_size);
		transfer_unit tf = {peer_socket, c_addr} ;
		pthread_t tid ;
		pthread_create(&tid, NULL, communicate_peer, (void*)&tf);
	}

	close(tracker_socket) ;

	return 0 ;
}