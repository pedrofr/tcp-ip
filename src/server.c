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
// #include "comm_utils.h"
#include "simulator.h"
#include "time_utils.h"

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer_in[BUFFER_SIZE];
  char buffer_out[BUFFER_SIZE];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2)
    errorf("ERROR, no port provided");

  timestamp_printf("Starting server!");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    errorf("ERROR opening socket");

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    errorf("ERROR on binding");

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0)
    errorf("ERROR on accept");

  /* Create independent thread which will execute simulate */

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

    bzero(buffer_in, BUFFER_SIZE);

    n = read(newsockfd, buffer_in, BUFFER_SIZE);
    if (n < 0)
      errorf("ERROR reading from socket");

    // printf("\nRequest: '%s'\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    // printf("command: '%s'\n", pcomm.command);
    // printf("argument: '%s'\n", pcomm.argument);

    wait_response(&parg, SERVER);

    sprintf(buffer_out, "%s#%s!", pcomm.command, pcomm.argument);
    // printf("\n1 Response: '%s'\n", buffer_out);
    n = write(newsockfd, buffer_out, strlen(buffer_out));
    if (n < 0)
      errorf("ERROR writing to socket");
    // printf("\n2 Response: '%s'\n", buffer_out);

    if (matches_arg(pcomm.command, pcomm.argument, "Exit", OK))
      break;
  }

  close(newsockfd);
  close(sockfd);

  pthread_join(simulator_thread, NULL);

  timestamp_printf("Closing server!");

  return 0;
}
