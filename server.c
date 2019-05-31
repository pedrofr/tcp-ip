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

typedef struct parsed_command
{
  char command[11];
  double argument;
} parscomm;

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

parscomm parse(char *rawCommand)
{
  char *command;
  char *argument_string;
  char *string, *tofree;
  parscomm pcomm;

  tofree = string = strdup(rawCommand);

  if (string == NULL)
    error("strcpy");

  command = strsep(&string, "#!");

  if (string == NULL || *command == '\0')
  {
    strcpy(pcomm.command, "");
    pcomm.argument = -1.;

    return pcomm;
  }

  argument_string = strsep(&string, "!");

  strcpy(pcomm.command, command);
  pcomm.argument = *argument_string == '\0' ? -1. : atof(argument_string);

  free(tofree);

  return pcomm;
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
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

  do
  {
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");

    bzero(buffer, 256);

    n = read(newsockfd, buffer, 255);
    if (n < 0)
      error("ERROR reading from socket");

    printf("Here is the message: %s\n", buffer);

    n = write(newsockfd, "I got your message", 18);
    if (n < 0)
      error("ERROR writing to socket");

    pcomm = parse(buffer);

    printf("command: '%s'\n", pcomm.command);
    printf("argument: '%.0f'\n", pcomm.argument);

  } while (strcmp(pcomm.command, "exit") != 0);

  close(newsockfd);
  close(sockfd);

  return 0;
}
