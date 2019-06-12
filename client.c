#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "error.h"
#include "parse.h"
#include "comm_consts.h"

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer_in[256];
  char buffer_out[256];

  parscomm pcomm;

  if (argc < 3)
  {
    fprintf(stderr, "usage: %s hostname port\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);

  do
  {
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

    printf("Please enter the message: ");
    bzero(buffer_out, 256);
    fgets(buffer_out, 255, stdin);

    char *pos = strchr(buffer_out, '\n');
    *pos = '\0';

    n = write(sockfd, buffer_out, strlen(buffer_out));
    if (n < 0)
      error("ERROR writing to socket");

    bzero(buffer_in, 256);

    n = read(sockfd, buffer_in, 255);
    if (n < 0)
      error("ERROR reading from socket");

    printf("%s\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    printf("command: '%s'\n", pcomm.command);
    printf("argument: '%s'\n", pcomm.argument);

    close(sockfd);
    
  } while (!matches_arg(pcomm.command, pcomm.argument, "Exit", "OK"));

  printf("Exiting")

  return 0;
}
