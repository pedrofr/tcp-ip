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

int clear_queue(struct pollfd *fds, char *buffer, int bsize, struct sockaddr *addr, unsigned int *addrlen);

int main(int argc, char *argv[])
{
  timestamp_printf("Starting client!\n");

  int sock, ready;
  struct sockaddr_in echoserver;
  struct sockaddr_in echoclient;
  struct hostent *server;
  unsigned int echolen, clientlen;
  int received = 0;
  struct timespec timeout = TIMEOUT;

  parscomm pcomm = {"", ""};
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pararg parg = {&pcomm, ANY, ANY, "", &mutex, &cond};

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
    sendto(sock, "CommTest!", strlen("CommTest!"), 0,
           (struct sockaddr *)&echoserver,
           sizeof(echoserver));

    if ((ready = ppoll(&fds, 1, &timeout, NULL)))
    {
      clientlen = sizeof(echoclient);
      received = recvfrom(sock, parg.buffer, BUFFER_SIZE, 0,
                          (struct sockaddr *)&echoclient,
                          &clientlen);

      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
      {
        timestamp_printf("Starting search for server @%s!\n", inet_ntoa(echoclient.sin_addr));
        errorf("Received a packet from an unexpected server");
      }

      parg.buffer[received] = '\0';

      if (strcmp(parg.buffer, "Comm#OK!") == 0)
      {
        break;
      }
      else
      {
        errorf("Received an unrecognized message from server (%s)", parg.buffer);
      }
    }
  }

  timestamp_printf("Server found!\n");

  int errnum;

  pthread_t control_thread;

  if ((errnum = pthread_create(&control_thread, NULL, control, &parg)))
  {
    errorf("\nThread creation failed: %d\n", errnum);
  }

  while (loading_control())
    ;

  while (strcmp(pcomm.command, "Exit"))
  {
    request_ownership(&parg, CLIENT);

    if (is_empty(pcomm.command))
    {
      empty(pcomm.command);
      empty(pcomm.argument);
      timestamp_printf("granter: %u\n", parg.granter & (CONTROL | TERMINAL));
      timestamp_printf("me: %u\n", parg.holder);
      grant_ownership(&parg, CLIENT, parg.granter & (CONTROL | TERMINAL));
      continue;
    }
    if (is_empty(pcomm.argument))
    {
      sprintf(parg.buffer, "%s!", pcomm.command);
    }
    else
    {
      sprintf(parg.buffer, "%s#%s!", pcomm.command, pcomm.argument);
    }

    /* Send the word to the server */
    echolen = strlen(parg.buffer);

    int n = sendto(sock, parg.buffer, echolen, 0,
                   (struct sockaddr *)&echoserver,
                   sizeof(echoserver));

    // printf("echolen: %i\nn: %i\n", echolen, n);
    if (n != (int)echolen)
    {
      errorf("Mismatch in number of sent bytes");
    }

    empty(parg.buffer);
    unsigned char read = 0;

    // Clean queue
    clear_queue(&fds, parg.buffer, BUFFER_SIZE, (struct sockaddr *)&echoclient, &clientlen);
    
    if ((ppoll(&fds, 1, &timeout, NULL)))
    {
      received = recvfrom(sock, parg.buffer, BUFFER_SIZE, 0,
                          (struct sockaddr *)&echoclient,
                          &clientlen);

      read = 1;
    }

    if (read)
    {
      /* Check that client and server are using same socket */
      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
      {
        errorf("Received a packet from an unexpected server");
      }

      parg.buffer[received] = '\0'; /* Assure null terminated string */

      char oldcomm[HALF_BUFFER_SIZE];
      strcpy(oldcomm, pcomm.command);

      parse(&pcomm, parg.buffer, MIN_VALUE, MAX_VALUE, OK);

      if (!is_empty(pcomm.command) && strstr(oldcomm, pcomm.command) == NULL)
      {
        timestamp_printf("oldcomm: %s | pcomm.command: %s | parg.buffer: %s\n", oldcomm, pcomm.command, parg.buffer);
        empty(pcomm.command);
        empty(pcomm.argument);
      }
    }
    else
    {
      timestamp_printf("pcomm.command: %s | pcomm.argument: %s\n", pcomm.command, pcomm.argument);
      empty(pcomm.command);
      empty(pcomm.argument);
      timestamp_printf("pcomm.command: %s | pcomm.argument: %s\n", pcomm.command, pcomm.argument);
    }

    grant_ownership(&parg, CLIENT, parg.granter & (CONTROL | TERMINAL));
  }

  close(sock);
  pthread_join(control_thread, NULL);

  timestamp_printf("Closing client!\n");

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