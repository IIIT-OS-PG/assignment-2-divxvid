#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

#define CHUNK_SIZE 524288

struct transfer_unit
{
	const char* my_IP ;
	const int my_port ;
};

struct file_data
{
	char IP[32] ;
	int port ;
	char chunks_available[1536] ;

	file_data() {}
	file_data(char *a, int b, char *c)
	{
		strcpy(IP, a);
		port = b ;
		strcpy(chunks_available, c);
	}
} ;

bool logged_in ;
char *my_IP ;
int my_port ;
char *tracker_info_file ;
char tracker_IP[32] ;
int tracker_port ;

int read_tracker_info(char*, int&, const char*);
void* listen_as_server(void*) ;
void login_and_register(const char*, const char*);
void create_user(const char*, const char*) ;
void logout() ;
void upload_file(const char*, int);
void list_files(int);
void download_file(int, char*, char*);

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

	if(read_tracker_info(tracker_IP, tracker_port, tracker_info_file) < 0)
	{
		/* ADD LOGIC FOR TWO TRACKER SYSTEM */
		printf("Tracker info file seems to be unreadable/empty.\n") ;
		return 0;
	}

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
		} else if(strcmp(cmd, "logout") == 0)
		{
			logout() ;
		} else if(strcmp(cmd, "exit") == 0)
		{
			logout();
			break ;
		} else if (strcmp(cmd, "upload_file") == 0)
		{
			char *f_name = strtok(NULL, " \n");
			char *g_id_s = strtok(NULL, " \n");
			int g_id = atoi(g_id_s);
			upload_file(f_name, g_id) ;
		} else if(strcmp(cmd, "list_files") == 0)
		{
			char *gid_s = strtok(NULL, " \n") ;
			int g_id = (gid_s == NULL ? 0 : atoi(gid_s));
			list_files(g_id);
		} else if(strcmp(cmd, "download_file") == 0)
		{
			char *gid_s = strtok(NULL, " \n") ;
			char *file_name = strtok(NULL, " \n") ;
			char *dest_path = strtok(NULL, " \n") ;
			int gid = atoi(gid_s);
			download_file(gid, file_name, dest_path);
		}
		else printf("Undefined command.\n") ;
	}
	
	return 0 ;
}

void download_file(int g_id, char *fname, char* dest_path)
{
	if(!logged_in)
	{
		printf("Please log in to the system.\n");
		return ;
	}
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

	char cmd[32] = "download_file" ;
	send(tracker_socket, cmd, sizeof cmd, 0) ;

	char file_name[128] ;
	strcpy(file_name, fname);
	send(tracker_socket, file_name, sizeof file_name, 0) ;

	int num_clients ;
	recv(tracker_socket, &num_clients, sizeof num_clients, 0);

	if(num_clients == 0)
	{
		printf("None of the peers have this file.\n");
		close(tracker_socket) ;
		return ; 
	}

	std::vector<file_data> clients_with_file ;
	char IP[32] ;
	int port ;
	char chunks[1536] ;
	for(int i = 0 ; i < num_clients ; ++i)
	{
		char trans[2048] ;
		recv(tracker_socket, trans, sizeof trans, 0);
		sscanf(trans, "%s %d %s", IP, &port, chunks);
		clients_with_file.emplace_back(IP, port, chunks);
	}

	std::vector<std::string> chunks_from_clients ;
	for(const file_data &fd : clients_with_file)
	{
		chunks_from_clients.push_back(fd.chunks_available);
	}

	int num_chunks = chunks_from_clients[0].length() ;
	std::vector<int> chunk_selection(num_chunks, -1) ;

	for(int i = 0 ; i < num_chunks ; ++i)
	{
		int x = i % num_clients ;
		while(x < num_clients && chunks_from_clients[x][i] != '1')
		{
			++x ;
		}
		if(x == num_clients)
		{
			continue ;
		}
		chunk_selection[i] = x ;
	}

	for(const int &x : chunk_selection)
	{
		printf("%d ", x);
	}
	printf("\n") ;

	close(tracker_socket) ;
}

void list_files(int g_id)
{
	if(!logged_in)
	{
		printf("Please log in to the system.\n");
		return ;
	}
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

	char cmd[32] = "list_files" ;
	send(tracker_socket, cmd, sizeof cmd, 0) ;
	printf("Command sent.\n") ;
	int n_files ;
	recv(tracker_socket, &n_files, sizeof n_files, 0);

	printf("The number of files is : %d\n", n_files);
	char f_name_s[128] ;
	for(int i = 0 ; i < n_files ; ++i)
	{
		recv(tracker_socket, f_name_s, sizeof f_name_s, 0);
		printf("%d : %s\n", i+1, f_name_s);
	}

	close(tracker_socket);
}

void upload_file(const char* f_name, int g_id)
{
	if(!logged_in)
	{
		printf("please log in to the system.\n");
		return ;
	}
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

	char chunk_info[1536] ;
	FILE *fp = fopen(f_name, "rb");
	if(fp == 0)
	{
		close(tracker_socket) ;
		printf("File not found.\n");
		return ;
	}
	fseek(fp, 0, SEEK_END) ;
	int file_size = ftell(fp) ;
	fclose(fp) ;

	int num_chunks = file_size / CHUNK_SIZE + (file_size % CHUNK_SIZE == 0 ? 0 : 1) ;
	for(int i = 0 ; i < num_chunks ; ++i)
		chunk_info[i] = '1' ;
	chunk_info[num_chunks] = '\0' ;
	char cmd[32] = "upload_file";
	send(tracker_socket, cmd, sizeof cmd, 0) ;

	char info[2048] ;
	sprintf(info, "%s %s %d %d %d %s", f_name, my_IP, my_port, g_id, file_size, chunk_info);
	printf("INFO is : %s\n", info) ;

	send(tracker_socket, info, sizeof info, 0) ;

	close(tracker_socket) ;
}

void logout()
{
	if(!logged_in) return ;	
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

	char cmd_t[32] = "logout" ;
	send(tracker_socket, cmd_t, sizeof cmd_t, 0) ; 

	char data[128] ;
	sprintf(data, "%s %d", my_IP, my_port);
	send(tracker_socket, data, sizeof data, 0) ;

	close(tracker_socket) ;
	logged_in = false ;
}

void login_and_register(const char* username, const char* passwd)
{
	if(logged_in) return ;
	if(username == NULL || passwd == NULL)
	{
		printf("Empty username or password.\n");
		return ;
	}
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
	pthread_detach(listen_thread) ;
	printf("Listening thread detached.\n");

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

	char cmd_t[32] = "login" ;
	send(tracker_socket, cmd_t, sizeof cmd_t, 0) ; 

	char data[128] ;
	sprintf(data, "%s %d", my_IP, my_port);
	send(tracker_socket, data, sizeof data, 0) ;

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

	close(active_sock);
	close(listen_socket) ;
	pthread_exit(NULL) ;
}