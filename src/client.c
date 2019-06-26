#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include "error.h"
#include "comm_consts.h"
#include "control.h"

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer_in[BUFFER_SIZE];
  char buffer_out[BUFFER_SIZE];

  if (argc < 3)
  {
    fprintf(stderr, "usage: %s hostname port\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);

  printf("\nStarting client!\n");

  parscomm pcomm;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  pararg parg = {&pcomm, CONTROL, &mutex, &cond};

  pthread_t control_thread;

  int errnum;
  if ((errnum = pthread_create(&control_thread, NULL, control, &parg)))
  {
    sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
    error(buffer_out);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  server = gethostbyname(argv[1]);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");

  do
  {
    wait_request(&parg, CLIENT);

    // printf("\nPlease enter the message: ");
    // bzero(buffer_out, BUFFER_SIZE);
    // fgets(buffer_out, BUFFER_SIZE, stdin);

    // char *pos = strchr(buffer_out, '\n');
    // *pos = '\0';
    
    bzero(buffer_out, BUFFER_SIZE);
    if (is_empty(pcomm.argument))
    {
		  sprintf(buffer_out, "%s!", pcomm.command);
    }
    else
    {
		  sprintf(buffer_out, "%s#%s!", pcomm.command, pcomm.argument);
    }

    // printf("\n1 Request: '%s'\n", buffer_out);
    n = write(sockfd, buffer_out, strlen(buffer_out));
    if (n < 0)
      error("ERROR writing to socket");
    // printf("\n2 Request: '%s'\n", buffer_out);

    bzero(buffer_in, BUFFER_SIZE);
    n = read(sockfd, buffer_in, BUFFER_SIZE); 
    if (n < 0)
      error("ERROR reading from socket");

    // printf("\nResponse: '%s'\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    // printf("command: '%s'\n", pcomm.command);
    // printf("argument: '%s'\n", pcomm.argument);

    release(&parg, CLIENT);

  } while (!matches_arg(pcomm.command, pcomm.argument, "Exit", "OK"));

  close(sockfd);

  pthread_join(control_thread, NULL);

  printf("\nClosing client!\n");

  return 0;
}
