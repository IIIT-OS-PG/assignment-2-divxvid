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
#include <map>
#include <set>

#define TRACKER_IP "127.0.0.1"
#define TRACKER_PORT 33333

std::vector<std::string> client_info_vector ;

struct transfer_unit
{
	int peer_socket ;
	struct sockaddr_in c_addr ;
};

struct file_data
{
	char file_name[128];
	char IP[32] ;
	int port ;
	int group_id ;
	int file_size ;
	char chunks_available[1536] ;
	file_data() {}
	file_data(char* a, char *b, int c, int d, int e, char *f)
	{
		strcpy(file_name, a);
		strcpy(IP, b) ;
		port = c ;
		group_id = d ;
		file_size = e; 
		strcpy(chunks_available, f); 
	} 
};

std::vector<file_data> files_info_vector ;

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

	close(active_sock); 
	return 0 ;
}

void login_stub(struct transfer_unit *tf)
{
	char data[128] ;
	recv(tf->peer_socket, data, sizeof data, 0);
	std::string temp(data) ;
	client_info_vector.push_back(temp);
	update_client_info_file();
}

void logout_stub(struct transfer_unit* tf)
{
	char data[128] ;
	recv(tf->peer_socket, data, sizeof data, 0);
	std::string temp(data) ;
	int idx = -1 ;
	for(int i = 0 ; i < client_info_vector.size() ; ++i)
	{
		if(client_info_vector[i] == data)
		{
			idx = i ;
			break ;
		}
	}
	if(idx != -1)
	{
		client_info_vector.erase(client_info_vector.begin()+idx);
		update_client_info_file() ;	
	}
}

void upload_file(struct transfer_unit* tf)
{
	char info[2048] ;
	recv(tf->peer_socket, info, sizeof info, 0);
	char f_name[128] ;
	char chunk_info[1536] ;
	char peer_IP[32] ;
	int peer_port, file_size, peer_g_id ;
	sscanf(info, "%s %s %d %d %d %s", f_name, peer_IP, &peer_port, &peer_g_id, &file_size, chunk_info);
	files_info_vector.emplace_back(f_name, peer_IP, peer_port, peer_g_id, file_size, chunk_info);
}

void list_files(struct transfer_unit* tf)
{
	/*std::map<std::string, std::vector<std::string> > files_in_tracker ;
	for(const struct file_data &fd : files_info_vector)
	{
		char addx[64] ;
		sprintf(addx, "%s %d", fd.IP, fd.port);
		std::string temp(addx) ;
		files_in_tracker[fd.file_name].push_back(temp);
	}*/

	std::set<std::string> files_in_tracker ;
	for(const struct file_data &fd : files_info_vector)
	{
		std::string temp(fd.file_name) ;
		files_in_tracker.insert(temp);
	}

	int num_files = files_in_tracker.size() ;
	send(tf->peer_socket, &num_files, sizeof num_files, 0);

	char f_name_s[128] ;
	for(const std::string &s : files_in_tracker)
	{
		strcpy(f_name_s, s.c_str());
		send(tf->peer_socket, f_name_s, sizeof f_name_s, 0);
	}
}

void download_file(struct transfer_unit* tf)
{
	char file_name[128] ;
	recv(tf->peer_socket, file_name, sizeof file_name, 0);

	std::map<std::string, std::vector<std::string> > files_in_tracker ;
	for(const struct file_data &fd : files_info_vector)
	{
		char addx[2048] ;
		sprintf(addx, "%s %d %d %s", fd.IP, fd.port, fd.file_size, fd.chunks_available);
		std::string temp(addx) ;
		files_in_tracker[fd.file_name].push_back(temp);
	}

	int transmission_size ;
	std::string f_name(file_name) ;
	if(files_in_tracker.find(f_name) == files_in_tracker.end())
	{
		transmission_size = 0 ;
	} else 
	{
		transmission_size = files_in_tracker[f_name].size() ;
	}

	send(tf->peer_socket, &transmission_size, sizeof transmission_size, 0) ;
	if(transmission_size == 0) return ;
	for(const std::string &s : files_in_tracker[f_name])
	{
		char trans[2048] ;
		strcpy(trans, s.c_str());
		send(tf->peer_socket, trans, sizeof trans, 0) ;
	}
}

void update_file_data(struct transfer_unit* tu)
{
	char new_info[2048] ;
	recv(tu->peer_socket, new_info, sizeof new_info, 0);

	char IP[32] ;
	int port ;
	int chunk_num, num_chunks, file_size ;
	char fname[128] ;

	sscanf(new_info, "%s %d %s %d %d %d", IP, &port, fname, &chunk_num, &num_chunks, &file_size);

	int idx = -1 ;
	for(int i = 0 ; i < files_info_vector.size() ; ++i)
	{
		file_data x = files_info_vector[i] ;
		if(strcmp(IP, x.IP) == 0 && strcmp(fname, x.file_name) == 0 && port == x.port)
		{
			idx = i ;
			break ;
		}
	}

	if(idx == -1)
	{
		//new entry.
		char chunks[1536] ;
		for(int i = 0 ; i < num_chunks ; ++i)
		{
			chunks[i] = (i == chunk_num ? '1' : '0') ;
		}
		chunks[num_chunks] = '\0' ;
		files_info_vector.emplace_back(fname, IP, port, 0, file_size, chunks);
	} else {
		files_info_vector[idx].chunks_available[chunk_num] = '1' ;
	}

}

void* communicate_peer(void* args)
{
	struct transfer_unit *tf = (transfer_unit*)args ;

	char cmd[32] ;
	recv(tf->peer_socket, cmd, sizeof cmd, 0) ;

	if(strcmp(cmd, "login") == 0)
	{
		login_stub(tf) ;
	} else if (strcmp(cmd, "logout") == 0)
	{
		logout_stub(tf);
	} else if(strcmp(cmd, "upload_file") == 0)
	{
		upload_file(tf) ;
	} else if(strcmp(cmd, "list_files") == 0)
	{
		list_files(tf);
	} else if(strcmp(cmd, "download_file") == 0)
	{
		download_file(tf) ;
	} else if(strcmp(cmd, "update_file_data") == 0)
	{
		update_file_data(tf);
	}

	close(tf->peer_socket) ;
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

	while(1)
	{
		peer_socket = accept(tracker_socket, (struct sockaddr*)&c_addr, &client_sock_size);
		transfer_unit tf = {peer_socket, c_addr} ;
		pthread_t tid ;
		pthread_create(&tid, NULL, communicate_peer, (void*)&tf);
		pthread_detach(tid);
	}

	close(tracker_socket) ;

	return 0 ;
}