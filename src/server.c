#define _GNU_SOURCE /* See feature_test_macros(7) */

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
#include <poll.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

int clear_queue(struct pollfd *fds, char *buffer, int bsize, struct sockaddr *addr, unsigned int *addrlen);

int main(int argc, char *argv[])
{
  timestamp_printf("Starting server!\n");

  int sock;
  struct sockaddr_in echoserver, echoclient;
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

  struct pollfd fds;
  memset(&fds, 0, sizeof(fds));

  fds.fd = sock;
  fds.events = POLLIN;

  parscomm pcomm = {"", ""};
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  pararg parg = {&pcomm, SERVER, ANY, "", &mutex, &cond};

  pthread_t simulator_thread;

  int errnum;
  if ((errnum = pthread_create(&simulator_thread, NULL, simulate, &parg)))
  {
    errorf("\nThread creation failed: %d\n", errnum);
  }

  while (strcmp(pcomm.command, "Exit"))
  {
    clear_queue(&fds, parg.buffer, BUFFER_SIZE, (struct sockaddr *)&echoclient, &clientlen);

    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, parg.buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&echoclient,
                             &clientlen)) < 0)
    {
      errorf("Failed to receive message");
    }

    parg.buffer[received] = '\0';
    // printf("Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
    // printf("Message: %s\n", parg.buffer);

    parse(&pcomm, parg.buffer, MIN_VALUE, MAX_VALUE, OK);

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
    
    sprintf(parg.buffer, "%s#%s!", pcomm.command, pcomm.argument);

    int bufferlen = strlen(parg.buffer);
    /* Send the message back to client */
    if (sendto(sock, parg.buffer, bufferlen, 0,
               (struct sockaddr *)&echoclient,
               sizeof(echoclient)) != bufferlen)
    {
      errorf("Mismatch in number of bytes sent");
    }
  }

  close(sock);

  pthread_join(simulator_thread, NULL);

  timestamp_printf("Closing server!\n");

  return 0;
}

int clear_queue(struct pollfd *fds, char *buffer, int bsize, struct sockaddr *addr, unsigned int *addrlen)
{
  int read = 0, received = 0;
  struct timespec nowait = NOWAIT;

  if ((ppoll(fds, 1, &nowait, NULL)))
  {
    do
    {
      received = recvfrom(fds->fd, buffer, bsize, 0, addr, addrlen);

      read = 1;
    } while ((ppoll(fds, 1, &nowait, NULL)));
  }

  if (read)
    buffer[received] = '\0'; /* Assure null terminated string */

  return read;
}