#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 524288

int main()
{
	int server_sock, client_sock ;
	if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Could not create a socket.\n");

	struct sockaddr_in s_addr, c_addr ;
	bzero((char*)&s_addr, sizeof s_addr);

	s_addr.sin_family = AF_INET ;
	s_addr.sin_port = 42069 ;
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1") ;

	if(bind(server_sock, (struct sockaddr*)&s_addr, sizeof s_addr) < 0)
		printf("Bind Failed.\n");

	listen(server_sock, 3) ;
	unsigned int sz = sizeof c_addr ;

	client_sock = accept(server_sock, (struct sockaddr*) &c_addr, &sz) ;
	if(client_sock < 0) printf("Could not accept the connection from the client.\n");

	FILE* fp ;
	fp = fopen("video.mp4", "rb");

	int file_size ;
	fseek(fp, 0, SEEK_END) ;
	file_size = ftell(fp) ;
	rewind(fp);

	int num_chunks = file_size / CHUNK_SIZE  + (file_size % CHUNK_SIZE == 0 ? 0 : 1);

	send(client_sock, &file_size, sizeof file_size, 0) ;
	int ack ;

	/*if(ack == 1)
	{
		printf("Now sending the file.\n"); 
		char buff[512] ;
		int bytes_read ;
		while( (bytes_read = fread(buff, sizeof(char), sizeof buff, fp)) > 0 && file_size > 0)
		{
			send(client_sock, buff, bytes_read, 0);
			file_size -= bytes_read ;
		}
	}*/

	int bytes_read ;
	for(int i = 1 ; i <= num_chunks ; ++i)
	{
		recv(client_sock, &ack, sizeof ack, 0) ;
		printf("Client asked for : %d\n", ack) ;
		char buff[512] ;
		int bytes_to_send = (ack == num_chunks ? file_size - (CHUNK_SIZE * (num_chunks-1)) : CHUNK_SIZE) ;
		fseek(fp, CHUNK_SIZE*(ack-1), SEEK_SET);
		while( bytes_to_send > 0 && (bytes_read = fread(buff, sizeof(char), sizeof buff, fp)) > 0 )
		{
			send(client_sock, buff, bytes_read, 0);
			bytes_to_send -= bytes_read ;
		}
		printf("Sent the chunk %d\n", ack);
	}

	fclose(fp) ;
	close(client_sock) ;
	close(server_sock) ;
	return 0;
}