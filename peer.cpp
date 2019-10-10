#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

int read_tracker_info(char*, int&, const char*);

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		printf("Please run the program with IP, port and tracker_info file as arguments.\n");
		return 0 ;
	}

	const char* my_IP = argv[1] ;
	const int my_port = atoi(argv[2]) ;
	const char* tracker_info_file = argv[3] ;

	char tracker_IP[32] ;
	int tracker_port ;

	if(read_tracker_info(tracker_IP, tracker_port, tracker_info_file) < 0)
	{
		printf("Tracker info file seems to be unreadable/empty.\n") ;
		return 0 ;
	}

	printf("I read %s and %d\n", tracker_IP, tracker_port) ;

	int tracker_socket ;
	if( (tracker_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create the socket.\n") ;
		return 0 ;
	}

	struct sockaddr_in tracker_addr ;

	bzero( (char*)&tracker_addr, sizeof tracker_addr ) ;
	tracker_addr.sin_family = AF_INET ;
	tracker_addr.sin_port = tracker_port ;
	tracker_addr.sin_addr.s_addr = inet_addr(tracker_IP) ;

	if( connect(tracker_socket, (struct sockaddr*)&tracker_addr, sizeof tracker_addr ) < 0 )
	{
		printf("cannot connect to the tracker.\n") ;
		close(tracker_socket) ;
		return 0 ;
	}

	char msg[100] ;
	recv(tracker_socket, msg, sizeof msg, 0) ;

	printf("Tracker sent me : %s\n", msg) ;

	close(tracker_socket) ;
	return 0 ;
}

int read_tracker_info(char tracker_IP[32], int &tracker_port, const char* tracker_info_file)
{
	FILE* tracker = fopen(tracker_info_file, "r") ;
	if( fscanf(tracker, "%s%d", tracker_IP, &tracker_port) == EOF )
	{
		fclose(tracker) ;
		return -1 ;
	}
	fclose(tracker) ;
	return 1 ;
}