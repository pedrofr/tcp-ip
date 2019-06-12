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
#include "error.h"
#include "parse.h"
#include "comm_consts.h"

#define h_addr h_addr_list[0] /* for backward compatibility */

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer_in[256];
  char buffer_out[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  parscomm pcomm;

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

    printf("Here is the message: %s\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    printf("command: '%s'\n", pcomm.command);
    printf("argument: '%s'\n", pcomm.argument);

    if (matches_numeric(pcomm.command, pcomm.argument, "OpenValve"))
    {
      double argument = atof(pcomm.argument);
      sprintf(buffer_out, "Open#%i!", (int)argument);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_numeric(pcomm.command, pcomm.argument, "CloseValve"))
    {
      double argument = atof(pcomm.argument);
      sprintf(buffer_out, "Close#%i!", (int)argument);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_numeric(pcomm.command, pcomm.argument, "SetMax"))
    {
      double argument = atof(pcomm.argument);
      sprintf(buffer_out, "Max#%i!", (int)argument);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_no_arg(pcomm.command, pcomm.argument, "GetLevel"))
    {
      sprintf(buffer_out, "Level#%i!", 100);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_no_arg(pcomm.command, pcomm.argument, "CommTest"))
    {
      sprintf(buffer_out, "Comm#%s!", OK);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_no_arg(pcomm.command, pcomm.argument, "Start"))
    {
      sprintf(buffer_out, "Start#%s!", OK);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }
    else if (matches_no_arg(pcomm.command, pcomm.argument, "Exit"))
    {
      sprintf(buffer_out, "Exit#%s!", OK);
      n = write(newsockfd, buffer_out, strlen(buffer_out));
      break;
    }
    else
    {
      sprintf(buffer_out, "NoMessageFound!");
      n = write(newsockfd, buffer_out, strlen(buffer_out));
    }

    if (n < 0)
      error("ERROR writing to socket");
  }

  close(newsockfd);
  close(sockfd);

  return 0;
}
