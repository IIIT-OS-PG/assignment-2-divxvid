#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

struct transfer_unit
{
	const char* my_IP ;
	const int my_port ;
};

bool logged_in ;
char *my_IP ;
int my_port ;
char *tracker_info_file ;

int read_tracker_info(char*, int&, const char*);
void* listen_as_server(void*) ;
void login_and_register(const char*, const char*);
void create_user(const char*, const char*) ;

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		printf("Please run the program with IP, port and tracker_info file as arguments.\n");
		return 0 ;
	}

	logged_in = false ;
	my_IP = argv[1] ;
	my_port = atoi(argv[2]) ;
	tracker_info_file = argv[3] ;

	char command[1024] ;
	while(1)
	{
		printf(">> ");
		fgets(command, sizeof command, stdin) ;
		char *cmd = strtok((char*)command, " \n");
		if(strcmp(cmd, "login") == 0)
		{
			char *uname = strtok(NULL, " \n") ;
			char *pwd = strtok(NULL, " \n") ;
			login_and_register(uname, pwd) ;
			if(!logged_in) continue ;	
		} else if (strcmp(cmd, "create_user") == 0)
		{
			char *uname = strtok(NULL, " \n") ;
			char *pwd = strtok(NULL, " \n") ;
			create_user(uname, pwd) ;
		} else printf("Undefined command.\n") ;
	}
	
	return 0 ;
}

void login_and_register(const char* username, const char* passwd)
{
	if(logged_in) return ;
	FILE *fp ;
	fp = fopen("authentication.txt", "r");
	if(fp == 0)
	{
		printf("login failed as no file found.\n");
		return ;	
	}
	char uname[32] ;
	char pwd[32] ;
	bool valid = false ;
	while(fscanf(fp, "%s%s", uname, pwd) != EOF)
	{
		if(strcmp(uname, username) == 0)
		{
			if(strcmp(passwd, pwd) == 0)
			{
				valid = true ;
				break ;
			} else printf("Invalid Password.\n") ;
		}
	}
	fclose(fp) ;
	if(!valid)
	{
		printf("Login failed.\n") ;
		return ;
	}

	logged_in = true ;

	struct transfer_unit tu = {my_IP, my_port};

	pthread_t listen_thread ;
	pthread_create(&listen_thread, NULL, listen_as_server, (void*)&tu);
	printf("Listening thread detached.\n");

	char tracker_IP[32] ;
	int tracker_port ;

	if(read_tracker_info(tracker_IP, tracker_port, tracker_info_file) < 0)
	{
		/* ADD LOGIC FOR TWO TRACKER SYSTEM */
		printf("Tracker info file seems to be unreadable/empty.\n") ;
		return ;
	}

	printf("I read %s and %d\n", tracker_IP, tracker_port) ;

	int tracker_socket ;
	if( (tracker_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create the socket.\n") ;
		return;
	}

	struct sockaddr_in tracker_addr ;

	bzero( (char*)&tracker_addr, sizeof tracker_addr ) ;
	tracker_addr.sin_family = AF_INET ;
	tracker_addr.sin_port = htons(tracker_port) ;
	tracker_addr.sin_addr.s_addr = inet_addr(tracker_IP) ;

	if( connect(tracker_socket, (struct sockaddr*)&tracker_addr, sizeof tracker_addr ) < 0 )
	{
		printf("cannot connect to the tracker.\n") ;
		close(tracker_socket) ;
		return ;
	}

	char data[128] ;
	sprintf(data, "%s %d", my_IP, my_port);
	send(tracker_socket, data, sizeof data, 0) ;

	char msg[100] ;
	recv(tracker_socket, msg, sizeof msg, 0) ;

	printf("Tracker sent me : %s\n", msg) ;

	close(tracker_socket) ;
}

void create_user(const char* uname, const char* pwd)
{
	if(uname == NULL || pwd == NULL) return ;
	FILE *fp = fopen("authentication.txt", "a") ;
	if(fp == 0) 
	{
		printf("cannot create a user.\n") ;
		return ;
	}
	fprintf(fp, "%s %s", uname, pwd);
	fclose(fp) ;
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

void* listen_as_server(void* args)
{

	struct transfer_unit *tu = (struct transfer_unit*)args ;

	int listen_socket ;
	if( (listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Couldn't create the listening socket.\n") ;
		return NULL ;
	}

	struct sockaddr_in list_sock ;
	bzero( (char*)&list_sock, sizeof list_sock );
	list_sock.sin_family = AF_INET ;
	list_sock.sin_port = htons(tu->my_port) ;
	list_sock.sin_addr.s_addr = inet_addr(tu->my_IP) ;

	if(bind(listen_socket, (struct sockaddr*)&list_sock, sizeof list_sock) < 0)
	{
		printf("bind failed.\n");
		close(listen_socket) ;
		return NULL ;
	}

	listen(listen_socket, 10) ;

	int sz = sizeof list_sock ;
	int active_sock = accept(listen_socket, (struct sockaddr*)&list_sock, (socklen_t*)&sz) ;

	char msg[100] = "hahaha this client is a server as well lol.\n" ;
	send(active_sock, msg, sizeof msg, 0) ;

	close(active_sock);
	close(listen_socket) ;
	pthread_exit(NULL) ;
}