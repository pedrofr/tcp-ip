#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#define TIMEOUT  \
  {              \
    0, 50000000L \
  }

#define BUFFSIZE 255
void Die(char *mess)
{
  perror(mess);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in echoserver;
  struct sockaddr_in echoclient;
  char buffer[BUFFSIZE];
  unsigned int echolen, clientlen;
  int received = 0;
  struct timespec timeout = TIMEOUT;

  if (argc != 4)
  {
    fprintf(stderr, "USAGE: %s <server_ip> <word> <port>\n", argv[0]);
    exit(1);
  }

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    Die("Failed to create socket");
  }
  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));      /* Clear struct */
  echoserver.sin_family = AF_INET;                 /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]); /* IP address */
  echoserver.sin_port = htons(atoi(argv[3]));      /* server port */

  /* Send the word to the server */
  echolen = strlen(argv[2]);

  int n = sendto(sock, argv[2], echolen, 0,
                 (struct sockaddr *)&echoserver,
                 sizeof(echoserver));

  printf("echolen: %i\nn: %i\n", echolen, n);
  if (n != echolen)
  {
    Die("Mismatch in number of sent bytes");
  }

  /* Receive the word back from the server */
  clientlen = sizeof(echoclient);

  struct pollfd fds;
  memset(&fds, 0 , sizeof(fds));
  
  fds.fd = sock;
  fds.events = POLLIN;

  int ready;
  if ((ready = ppoll(&fds, 1, &timeout, NULL)))
  {
    printf("entrou\n");

    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
                             (struct sockaddr *)&echoclient,
                             &clientlen)) != echolen)
    {
      //Die("Mismatch in number of received bytes");
    }

    /* Check that client and server are using same socket */
    if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr)
    {
      Die("Received a packet from an unexpected server");
    }

    buffer[received] = '\0'; /* Assure null terminated string */
    printf("Received: %s\n", buffer);
  }

  printf("saiu\n");

  close(sock);
  exit(0);
}
