#define _GNU_SOURCE /* See feature_test_macros(7) */

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
#include "time_utils.h"

#include <arpa/inet.h>
#include <time.h>

#include <poll.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

#define TIMEOUT  \
  {              \
    0, 50000000L \
  }

int main(int argc, char *argv[])
{
  // int s = 0;

  timestamp_printf("Starting client!\n");

  int sock, ready;
  struct sockaddr_in echoserver;
  struct sockaddr_in echoclient;
  struct hostent *server;
  char buffer[BUFFER_SIZE];
  unsigned int echolen, clientlen;
  int received = 0;
  struct timespec timeout = TIMEOUT;
  char *commtest = "CommTest!";

  if (argc < 3)
  {
    errorf("Usage: %s <hostname> <port>\n", argv[0]);
  }

  // portno = atoi(argv[2]);

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    errorf("Failed to create socket");
  }

  // sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // if (sockfd < 0)
  //   errorf("ERROR opening socket");

  server = gethostbyname(argv[1]);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  bzero((char *)&echoserver, sizeof(echoserver));
  echoserver.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
        (char *)&echoserver.sin_addr.s_addr,
        server->h_length);
  echoserver.sin_port = htons(atoi(argv[2]));

  /* Receive the word back from the server */

  struct pollfd fds;
  memset(&fds, 0, sizeof(fds));

  fds.fd = sock;
  fds.events = POLLIN;

  timestamp_printf("Starting search for server @%s:%s!\n", inet_ntoa(echoserver.sin_addr), argv[2]);

  while (1)
  {
    sendto(sock, commtest, strlen(commtest), 0,
           (struct sockaddr *)&echoserver,
           sizeof(echoserver));

    if ((ready = ppoll(&fds, 1, &timeout, NULL)))
    {
      clientlen = sizeof(echoclient);
      received = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                          (struct sockaddr *)&echoclient,
                          &clientlen);

      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
      {
        timestamp_printf("Starting search for server @%s!\n", inet_ntoa(echoclient.sin_addr));
        errorf("Received a packet from an unexpected server");
      }

      buffer[received] = '\0';

      if (strcmp(buffer, "Comm#OK!") == 0)
      {
        break;
      }
      else
      {
        errorf("Received an unrecognized message from server (%s)", buffer);
      }
    }
  }

  timestamp_printf("Server found!\n");

  parscomm pcomm;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  pararg parg = {&pcomm, CONTROL, &mutex, &cond};

  int errnum;

  pthread_t control_thread;

  if ((errnum = pthread_create(&control_thread, NULL, control, &parg)))
  {
    errorf("\nThread creation failed: %d\n", errnum);
  }

  do
  {
    wait_request(&parg, CLIENT);

    if (is_empty(pcomm.command))
    {
      release(&parg, CLIENT);
      continue;
    }
    if (is_empty(pcomm.argument))
    {
      sprintf(buffer, "%s!", pcomm.command);
      // printf("%s!", pcomm.command);
    }
    else
    {
      sprintf(buffer, "%s#%s!", pcomm.command, pcomm.argument);
      // printf("%s#%s!", pcomm.command, pcomm.argument);
    }

    // if (s > 1000)
    // {
    //   strcpy(pcomm.command, "Exit");
    //   strcpy(pcomm.argument, "");
    //   sprintf(buffer, "%s!", pcomm.command);
    // }
    // s++;

    /* Send the word to the server */
    echolen = strlen(buffer);

    int n = sendto(sock, buffer, echolen, 0,
                   (struct sockaddr *)&echoserver,
                   sizeof(echoserver));

    // printf("echolen: %i\nn: %i\n", echolen, n);
    if (n != (int)echolen)
    {
      errorf("Mismatch in number of sent bytes");
    }

    if ((ready = ppoll(&fds, 1, &timeout, NULL)))
    {
      received = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                          (struct sockaddr *)&echoclient,
                          &clientlen);

      /* Check that client and server are using same socket */
      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
      {
        errorf("Received a packet from an unexpected server");
      }

      buffer[received] = '\0'; /* Assure null terminated string */
      // printf("Received: %s\n", buffer);
    }

    // timestamp_printf("\nResponse: '%s'\n", buffer_in);

    parse(&pcomm, buffer, MIN_VALUE, MAX_VALUE, OK);

    // timestamp_printf("command: '%s'\n", pcomm.command);
    // timestamp_printf("argument: '%s'\n", pcomm.argument);

    release(&parg, CLIENT);

  } while (!matches_arg(pcomm.command, pcomm.argument, "Exit", "OK"));

  close(sock);
  pthread_join(control_thread, NULL);

  timestamp_printf("Closing client!\n");

  return 0;
}
