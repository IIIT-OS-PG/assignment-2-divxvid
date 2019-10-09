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
	int sockfd ;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("SOcket cannot be created.\n");

	struct sockaddr_in s_addr ;
	bzero( (char*)&s_addr, sizeof s_addr );

	s_addr.sin_family = AF_INET ;
	s_addr.sin_port = 42069 ;
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1") ;

	if(connect(sockfd, (struct sockaddr*)&s_addr, sizeof s_addr) < 0)
		printf("Cannot connect to the server.\n");

	int file_size ;
	recv(sockfd, &file_size, sizeof file_size, 0);

	printf("Server sent me file size : %d\n", file_size);
	int num_chunks = file_size / CHUNK_SIZE  + (file_size % CHUNK_SIZE == 0 ? 0 : 1);

	int ack = 1 ;

	FILE* fp ;
	fp = fopen("t_video.mp4", "wb");

	int bytes_recv ;
	int vals[18] = {5, 6, 1, 2, 3, 18, 17, 16, 15, 14, 7, 8, 9, 10, 13, 12, 11, 4} ;
	for(int i = 0 ; i < 18 ; ++i)
	{
		int ack = vals[i] ;
		int size_to_recv = (ack == num_chunks ? file_size - (CHUNK_SIZE * (num_chunks-1)) : CHUNK_SIZE) ;
		fseek(fp, CHUNK_SIZE*(ack-1), SEEK_SET) ;
		send(sockfd, &ack, sizeof ack, 0) ;
		printf("Sent request for : %d\n", ack) ;
		char buf[512] ;
		//int chunk_sent = 0;
		while( size_to_recv > 0 && (bytes_recv = recv(sockfd, buf, sizeof buf, 0)) > 0)
		{
			//printf("%d\n", ++chunk_sent);
			fwrite(buf, sizeof(char), bytes_recv, fp);
			size_to_recv -= bytes_recv ;
			//printf("%d\n", size_to_recv) ;
		}
		printf("Received chunk %d\n", ack) ;
		//file_size -= CHUNK_SIZE ;
	}

	fclose(fp) ;
	close(sockfd) ;
	return 0 ;
}