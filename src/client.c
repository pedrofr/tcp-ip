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
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include <fcntl.h>

#include "error.h"
#include "comm_consts.h"
#include "control.h"
#include "time_utils.h"

#define h_addr h_addr_list[0] /* for backward compatibility */
  
static volatile sig_atomic_t quit;

void sigint_handler(int sig_num);

int main(int argc, char *argv[])
{
  timestamp_printf("Starting client!\n");

  signal(SIGINT, sigint_handler);

  int sock, ready;
  struct sockaddr_in echoserver;
  struct hostent *server;
  ssize_t sent_len, received_len;
  struct timespec timeout = CLIENT_TIMEOUT;

  parscomm pcomm = {"", ""};
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pararg parg = {&pcomm, ANY, ANY, "", &mutex, &cond};

  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s <hostname> <port>", argv[0]);
    exit(0);
  }

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) < 0)
  {
    fprintf(stderr, "Failed to create socket");
    exit(0);
  }

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

  struct pollfd fd_out;
  struct pollfd fd_in;

  memset(&fd_out, 0, sizeof(fd_out));
  memset(&fd_in, 0, sizeof(fd_in));

  fd_in.fd = fd_out.fd = sock;

  fd_out.events = POLLOUT;
  fd_in.events = POLLIN;

  timestamp_printf("Starting search for server @%s:%s!\n", inet_ntoa(echoserver.sin_addr), argv[2]);

  int try = 0;
  sent_len = strlen("CommTest!");
  while (!quit && strcmp(parg.buffer, "Comm#OK!"))
  {
    timestamp_force_printf("'CommTest!' try #%i", ++try);

    if (sendto(sock, "CommTest!", sent_len, 0,
               (struct sockaddr *)&echoserver,
               sizeof(echoserver)) != sent_len)
    {
      errorf("Mismatch in number of sent bytes");
    }

    if ((ready = ppoll(&fd_in, 1, &timeout, NULL)))
    {
      if ((received_len = recv(sock, parg.buffer, BUFFER_SIZE, 0)) < 0)
      {
        errorf("recv");
      }
      
      parg.buffer[received_len] = '\0';
    }
  }

  timestamp_printf("Server found!\n");

  int errnum;

  pthread_t control_thread;

  if ((errnum = pthread_create(&control_thread, NULL, control, &parg)))
  {
    errorf("Thread creation failed: %d", errnum);
  }

  while (!quit && loading_control())
    ;

  while (!quit && strcmp(pcomm.command, "Exit"))
  {
    request_ownership(&parg, CLIENT);

    if (is_empty(pcomm.command))
    {
      empty(pcomm.argument);
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
    sent_len = strlen(parg.buffer);
    if (sendto(sock, parg.buffer, sent_len, 0,
               (struct sockaddr *)&echoserver,
               sizeof(echoserver)) != sent_len)
    {
      errorf("Mismatch in number of sent bytes");
    }

    empty(parg.buffer);

    if (ppoll(&fd_in, 1, &timeout, NULL))
    {
      if ((received_len = recv(sock, parg.buffer, BUFFER_SIZE, 0)) < 0)
      {
        errorf("recv");
      }
      else if (received_len)
      {
        parg.buffer[received_len] = '\0'; /* Assure null terminated string */

        parse(&pcomm, parg.buffer, MIN_VALUE, MAX_VALUE, OK);
      }
      else
      {
        empty(pcomm.command);
        empty(pcomm.argument);
      }
    }
    else
    {
      empty(pcomm.command);
      empty(pcomm.argument);
    }

    grant_ownership(&parg, CLIENT, parg.granter & (CONTROL | TERMINAL));
  }
  
  quit_control();
  
  request_ownership(&parg, CLIENT);
  grant_ownership(&parg, CLIENT, CONTROL);

  close(sock);
  pthread_join(control_thread, NULL);

  timestamp_printf("Closing client!\n");

  return 0;
}

void sigint_handler(__attribute__((unused)) int sig_num)
{
  quit = 1;
}