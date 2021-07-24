/* testClient.c */
/* 
gcc testClient.c testClient
./testClient 5
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
// additional includes
#include <stdlib.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>

void main(int argc, char *argv[])
{
	
	int sock;
	struct sockaddr_in sAddr;
	char buffer[100];

	memset((void *) &sAddr, 0, sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = INADDR_ANY;
	sAddr.sin_port = 0;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(sock, (const struct sockaddr *) &sAddr, sizeof(sAddr));

	sAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sAddr.sin_port = htons(1972);
	if (connect(sock, (const struct sockaddr *) &sAddr, sizeof(sAddr)) != 0) 
	{
		perror("client");
		return;
	}

	int nread;

	/* receive question */
	nread = recv(sock, buffer, 100, 0);
	send(sock, "OK", 100, 0);
	buffer[nread] = '\0';
	printf("\n%s\n", buffer);

	/* send answers */
	for(int i = 0; i < 6; i++)
	{
		/*read turn info */
		nread = recv(sock, buffer, 100, 0);
		send(sock, "OK", 100, 0);
		buffer[nread] = '\0';
		printf("\n%s\n", buffer);
		
		/* if turn is yours*/
		if(strcmp(buffer, "YT") == 0)
		{
			scanf("%s" ,buffer);
			send(sock, buffer, strlen(buffer), 0);
		}
		
		/* receive answers (your or oppenent's) from server */
		nread = recv(sock, buffer, 100, 0);
		send(sock, "OK", 100, 0);
		buffer[nread] = '\0';
		printf("\n%s\n", buffer);

	}

	/*receive point table*/
	nread = recv(sock, buffer, 100, 0);
	send(sock, "OK", 100, 0);
	buffer[nread] = '\0';
	printf("\n%s\n", buffer);
	
	close(sock);
}