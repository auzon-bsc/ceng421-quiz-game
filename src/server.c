/* server for quiz game
gcc server.c -o Server
./Server
*/
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// additional includes
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#define ANSWER_COUNT 6
#define QUESTION_COUNT 5

int indexOfAnswer(char answers[ANSWER_COUNT][30], char* givenAnswer);

typedef struct Quiz
{
	char question[100];
	char answers[ANSWER_COUNT][30];
} Quiz;

/** Generate random number in range [min, max]
 * @param min lower bound
 * @param max upper bound
 * @return randomly generated integer
*/
int randomNumber(int min, int max)
{
  int random_number = (rand() % (max - min + 1)) + min;
  return random_number;
}

int main(int argc, char *argv[])
{
	
	/* initializing quiz array */
	Quiz quiz_arr[QUESTION_COUNT];

	/* file reading*/
	FILE* fptr;
	if ((fptr = fopen("quiz.txt", "r")) == NULL)
	{
		printf("Error! File cannot be opened.");
		// Program exits if the file pointer returns NULL.
		exit(1);
	}

	char read_holder[100];		// temp question	
	int q_ind = 0;
	int a_ind = 0;
	int pos = 0;
	int isQuestion = 1;
	char ch;
	while((ch = fgetc(fptr)) != EOF)
	{
		if(ch == ',')
		{
			read_holder[pos] = '\0';
			pos = 0;
			if(isQuestion)
			{
				strcpy(quiz_arr[q_ind].question, read_holder);				
				a_ind = 0;
				isQuestion = 0;
			}
			else
			{
				strcpy(quiz_arr[q_ind].answers[a_ind], read_holder);
				a_ind++;
			}
			// it is a part
		}
		else if(ch == '\n')
		{
			read_holder[pos] = '\0';
			strcpy(quiz_arr[q_ind].answers[a_ind], read_holder);
			q_ind++;
			a_ind = 0;
			isQuestion = 1;
			pos = 0;
		}
		else
		{
			read_holder[pos] = ch;
			pos++;
		}
		
	}

	/*generating random number for question to send*/
	srand(time(NULL)); // random seed
	int rnd_num = randomNumber(0, QUESTION_COUNT - 1);

	/* initialize socket variables */
	int listenSocket = 0;
	int port = 1972;
	int returnStatus = 0;
	struct sockaddr_in server;

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenSocket == -1)
	{
		fprintf(stderr, "Could not create a socket!\n");
		exit(1);
	}
	else
	{
		fprintf(stderr, "Socket created!\n");
	}

	/* set up the address structure */
	/* use INADDR_ANY to bind to all local addresses */
	/* note use of htonl() and htons() */
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	server.sin_port = htons(port);
	/* bind to the address and port with our socket */
	returnStatus = bind(listenSocket, (struct sockaddr *)&server,
	sizeof(server));
	if (returnStatus == 0)
	{
		fprintf(stderr, "Bind completed!\n");
	}
	else
	{
		fprintf(stderr, "Could not bind to address!\n");
		close(listenSocket);
		exit(1);
	}

	/* tell the socket we are ready to accept connections */
	returnStatus = listen(listenSocket, 5);
	if (returnStatus == -1)
	{
		fprintf(stderr, "Cannot listen on socket!\n");
		close(listenSocket);
		exit(1);
	}

	/* loop for accepting sockets and game operations */
	while (1)
	{
		/* set up variables to handle client connections */
		struct sockaddr_in clientName = { 0 };
		int playerSocketArr[2] = {0, 0};
		int clientNameLength = sizeof(clientName);

		/* wait for players to connect*/
		for (int i = 0; i < 2; i++)
		{
			/* block on accept function call */
			playerSocketArr[i] = accept(listenSocket, (struct sockaddr *)&clientName,
					&clientNameLength);
			
			if (playerSocketArr[i] == -1)
			{
				fprintf(stderr, "Cannot accept connections!\n");
				close(listenSocket);
				exit(1);
			}
		}

		char out[100];					// buffer for sending data
		char incoming[100];			// buffer for receiving data
		int nread;							// characters read so far

		/* send question to players */
		for (int i = 0; i < 2; i++)
		{
			strcpy(out, quiz_arr[rnd_num].question);
			send(playerSocketArr[i], out, strlen(out), 0);
			recv(playerSocketArr[i], incoming, 100, 0);				// wait until client receives and sends "OK" to server
		}

		/* initialize point table */
		int point_arr[2] = {0, 0};

		/* recevie answers */
		for (int i = 0; i < 3; i++)
		{
			/* receive answers in order. first player1 then player2 */
			for (int j = 0; j < 2; j++)
			{
				send(playerSocketArr[j], "YT", 100, 0);						// send "YT" (your turn) signal
				recv(playerSocketArr[j], incoming, 100, 0);
				send(playerSocketArr[1 - j], "OT", 100, 0);				// send "OT" (opponent's turn) signal
				recv(playerSocketArr[1 - j], incoming, 100, 0);

				/* receive answer from the player which has turn */
				nread = recv(playerSocketArr[j], incoming, 100, 0);
				incoming[nread] = '\0';

				/* check if answer is correct */
				int ioa = indexOfAnswer(quiz_arr[rnd_num].answers, incoming);		// -1 if not found
				if(ioa != -1)			// found
				{
					snprintf(out, 150, "%s is correct. 10pts to player%d", incoming, j + 1);
					strcpy(quiz_arr[rnd_num].answers[ioa], "removed");
					point_arr[j] += 10;
				}
				else			// not found
				{
					snprintf(out, 200, "%s is not correct or the answer already given. -5pts to player%d", incoming, j + 1);
					point_arr[j] -= 5;
				}
				/* inform clients about the correctness of the answer */
				for(int k = 0; k < 2; k++)
				{
					send(playerSocketArr[k], out, strlen(out), 0);
					recv(playerSocketArr[k], incoming, 100, 0);
				}
				
			}
		}
		
		/* announce the points table then close sockets*/
		int winner_no = point_arr[0] > point_arr[1] ? 0 : 1;
		snprintf(out, 100, "Points Table\nPlayer 1: %i points\nPlayer 2:  %i points\n", 
				point_arr[0], point_arr[1]);
		
		for(int i = 0; i < 2; i++)
		{
			send(playerSocketArr[i], out, strlen(out), 0);
			recv(playerSocketArr[i], incoming, 100, 0);
			close(playerSocketArr[i]);
		}
		
	}

	/* close listening socket */
	close(listenSocket);
	exit(0);
}

/** Find the index of the answer
 * @param answers
 * @param givenAnswer 
 * @return index of the answer, if not found return -1
*/
int indexOfAnswer(char answers[ANSWER_COUNT][30], char* givenAnswer)
{
	int result = -1;
	for(int i = 0; i < ANSWER_COUNT; i++)
	{
		int isRemoved = strcmp("removed", givenAnswer);
		int isFound = strcmp(answers[i], givenAnswer);
		if(isRemoved != 0 && isFound == 0)
		{
			result = i;
			break;
		}
	}
	return result;

}
