/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
  char command[11];
  double argument; 
} parsed_command;

parsed_command parseCommand(char *rawCommand, char *command, double *argument) {

  char *argument_string;
  char *string, *tofree;

  tofree = string = strdup(rawCommand);

  if (string == NULL)
    err(1, "strcpy");

  command = strdup(strsep(&string, "#!"));

  argument_string = strsep(&string, "!");

  *argument = argument_string[0] == '\0' ? -1 : atof(argument_string);

  //  printf("command: _%s_\n", command);
  //  printf("argument: _%f_\n", *argument);

  parsed_command pcomm;
  strcpy(pcomm.command, command);
  pcomm.argument = *argument;

  free(tofree);
  free(command);

  //  printf("command: _%s_\n", pcomm.command);
  //  printf("argument: _%f_\n", pcomm.argument);

  return pcomm;
}

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  double argument;
  char *command;

  parsed_command pcomm = parseCommand("t#1!", command, &argument);
  
  printf("command: %s\n", pcomm.command);
  printf("argument: %f\n", pcomm.argument);

  pcomm = parseCommand("y#2!", command, &argument);
  
  printf("command: %s\n", pcomm.command);
  printf("argument: %f\n", pcomm.argument);
  
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
	   sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  do {
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
		       (struct sockaddr *) &cli_addr, 
		       &clilen);
    if (newsockfd < 0) 
      error("ERROR on accept");
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);    
    if (n < 0) error("ERROR writing to socket");
  } while (strcmp(buffer, "exit\n")!=0);
  close(newsockfd);
  close(sockfd);

  return 0; 
}
