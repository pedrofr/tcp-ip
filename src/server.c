/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#include <pthread.h>
#include "error.h"
#include "comm_consts.h"
// #include "thread_utils.h"
#include "simulator.h"
#include "time_utils.h"

// UDP
#include <arpa/inet.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  timestamp_printf("Starting server!\n");

  int sock;
  struct sockaddr_in echoserver, echoclient;
  char buffer[BUFFER_SIZE];
  unsigned int clientlen, serverlen; //, echolen;
  int received = 0;

  if (argc != 2)
    errorf("Usage: %s <port>\n", argv[0]);

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    errorf("Failed to create socket");

  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));     /* Clear struct */
  echoserver.sin_family = AF_INET;                /* Internet/IP */
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY); /* Any IP address */
  echoserver.sin_port = htons(atoi(argv[1]));     /* server port */

  /* Bind the socket */
  serverlen = sizeof(echoserver);
  if (bind(sock, (struct sockaddr *)&echoserver, serverlen) < 0)
    errorf("Failed to bind server socket");

  parscomm pcomm;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  pararg parg = {&pcomm, SERVER, &mutex, &cond};

  pthread_t simulator_thread;

  int errnum;
  if ((errnum = pthread_create(&simulator_thread, NULL, simulate, &parg)))
  {
    errorf("\nThread creation failed: %d\n", errnum);
  }

  while (1)
  {
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&echoclient,
                             &clientlen)) < 0)
    {
      errorf("Failed to receive message");
    }

    buffer[received] = '\0';
    // printf("Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
    // printf("Message: %s\n", buffer);

    parse(&pcomm, buffer, MIN_VALUE, MAX_VALUE, OK);

		if (matches_no_arg(pcomm.command, pcomm.argument, "CommTest"))
		{
			strcpy(pcomm.command, "Comm");
			strcpy(pcomm.argument, OK);
      timestamp_printf("Server reached for 'CommTest' from %s!\n", inet_ntoa(echoclient.sin_addr));
		}
    else
    {
      wait_response(&parg, SERVER);
    }
    
    
    sprintf(buffer, "%s#%s!", pcomm.command, pcomm.argument);

    int bufferlen = strlen(buffer);
    /* Send the message back to client */
    if (sendto(sock, buffer, bufferlen, 0,
               (struct sockaddr *)&echoclient,
               sizeof(echoclient)) != bufferlen)
    {
      errorf("Mismatch in number of bytes sent");
    }

    if (matches_arg(pcomm.command, pcomm.argument, "Exit", OK))
    {
      break;
    }
  }

  close(sock);

  pthread_join(simulator_thread, NULL);

  timestamp_printf("Closing server!\n");

  return 0;
}
