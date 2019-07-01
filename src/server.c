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

  // int sockfd, newsockfd, portno;
  // socklen_t clilen;
  // char buffer_in[BUFFER_SIZE];
  // char buffer_out[BUFFER_SIZE];
  // struct sockaddr_in serv_addr, cli_addr;
  // int n;

  // sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // if (sockfd < 0)
  //   errorf("ERROR opening socket");

  // bzero((char *)&serv_addr, sizeof(serv_addr));
  // portno = atoi(argv[1]);
  // serv_addr.sin_family = AF_INET;
  // serv_addr.sin_addr.s_addr = INADDR_ANY;
  // serv_addr.sin_port = htons(portno);

  // if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  //   errorf("ERROR on binding");

  // listen(sockfd, 5);
  // clilen = sizeof(cli_addr);

  // newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  // if (newsockfd < 0)
  //   errorf("ERROR on accept");

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
    // UDP
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

    pcomm = parse(buffer, MIN_VALUE, MAX_VALUE, OK);

    wait_response(&parg, SERVER);

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
    else if (matches_arg(pcomm.command, pcomm.argument, "Comm", OK))
    {
      timestamp_printf("Server reached for 'CommTest' from %s\n", inet_ntoa(echoclient.sin_addr));
    }
    

    // TCP
    // bzero(buffer_in, BUFFER_SIZE);

    // n = read(newsockfd, buffer_in, BUFFER_SIZE);
    // if (n < 0)
    //   errorf("ERROR reading from socket");

    // // timestamp_printf("\nRequest: '%s'\n", buffer_in);

    // pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    // // timestamp_printf("command: '%s'\n", pcomm.command);
    // // timestamp_printf("argument: '%s'\n", pcomm.argument);

    // wait_response(&parg, SERVER);

    // sprintf(buffer_out, "%s#%s!", pcomm.command, pcomm.argument);
    // n = write(newsockfd, buffer_out, strlen(buffer_out));
    // if (n < 0)
    //   errorf("ERROR writing to socket");

    // if (matches_arg(pcomm.command, pcomm.argument, "Exit", OK))
    //   break;
  }

  // close(newsockfd);
  // close(sockfd);

  close(sock);

  pthread_join(simulator_thread, NULL);

  timestamp_printf("Closing server!\n");

  return 0;
}
