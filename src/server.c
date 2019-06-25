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
#include "simulator.h"

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer_in[256];
  char buffer_out[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2)
    error("ERROR, no port provided");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  printf("\nStarting server!\n");

  /* Create independent thread which will execute simulate */

  parscomm pcomm;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  pararg parg = {&pcomm, SERVER, &mutex, &cond};

  pthread_t simulator_thread;

  int errnum;
  if ((errnum = pthread_create(&simulator_thread, NULL, simulate, &parg)))
  {
    sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
    error(buffer_out);
  }

  while (1)
  {
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");

    bzero(buffer_in, 256);

    n = read(newsockfd, buffer_in, 255);
    if (n < 0)
      error("ERROR reading from socket");

    printf("\nRequest: '%s'\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    printf("command: '%s'\n", pcomm.command);
    printf("argument: '%s'\n", pcomm.argument);

    wait_response(&parg, SERVER);

    sprintf(buffer_out, "%s#%s!", pcomm.command, pcomm.argument);
    n = write(newsockfd, buffer_out, strlen(buffer_out));

    if (n < 0)
      error("ERROR writing to socket");

    if (matches_arg(pcomm.command, pcomm.argument, "Exit", OK))
      break;
  }

  close(newsockfd);
  close(sockfd);

  pthread_join(simulator_thread, NULL);

  printf("\nClosing server!\n");

  return 0;
}
