#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define TRACKER_IP "127.0.0.1"
#define TRACKER_PORT 33333

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
	s_addr.sin_port = TRACKER_PORT ;
	s_addr.sin_addr.s_addr = inet_addr(TRACKER_IP) ;

	if(bind(tracker_socket, (struct sockaddr*) &s_addr, sizeof s_addr) < 0)
	{
		printf("Bind failed.\n");
		close(tracker_socket);
		return 0 ;
	}

	listen(tracker_socket, 10) ;
	unsigned int client_sock_size = sizeof c_addr ;

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
	close(tracker_socket) ;

	return 0 ;
}