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
#include <ctype.h>

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

int isNumeric(const char *s){
  if(s == NULL || *s == '\0' || isspace(*s))
    return 0;
  char * p;
  strtod (s, &p);
  return *p == '\0';
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

  int length;
  length = strlen(string);
  if(string[length-1] != '!')
    string = "";

  command = strsep(&string, "#!");

  if (string == NULL || *command == '\0')
    {
      //error
      strcpy(pcomm.command, "");
      pcomm.argument = -1.;
      
      free(tofree);

      return pcomm;
    }

  int diff = string - command;

  if (rawCommand[diff-1] == '!')
    {
      strcpy(pcomm.command, command);
      pcomm.argument = -1.;
    }
  if (rawCommand[diff-1] == '#')
    {
      argument_string = strsep(&string, "!");

      if (isNumeric(argument_string))
	{
	  strcpy(pcomm.command, command);
	  pcomm.argument = atof(argument_string);
	}
      else if (strcmp(argument_string, "OK") == 0)
	{
	  strcpy(pcomm.command, command);
	  pcomm.argument = -1.;
	}
      else
	{
	  //error
	  strcpy(pcomm.command, "");
	  pcomm.argument = -1.;
	}
    }

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

  while (1)
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

      pcomm = parse(buffer);

      printf("command: '%s'\n", pcomm.command);
      printf("argument: '%.0f'\n", pcomm.argument);
      char *response;
      if(strcmp(pcomm.command,"OpenValve") ==0)
	{
	  sprintf(response, "Open#%i!", pcomm.argument);
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command,"CloseValve") == 0)
	{
	  sprintf(response, "Close#%i!", pcomm.argument);
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command,"GetLevel") == 0)
	{
	  sprintf(response, "Level#%i!", pcomm.argument);
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command,"CommTest") == 0)
	{
	  sprintf(response, "Comm#%s!", "OK");
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command,"SetMax") == 0)
	{
	  sprintf(response, "Max#%i!", pcomm.argument);
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command,"Start") == 0)
	{
	  sprintf(response, "Start#%s!", "OK");
	  n=write(newsockfd,response,strlen(response));
	}
      else if(strcmp(pcomm.command, "Exit") == 0)
	{
	  break;
	}
      else 
	{
	  sprintf(response, "NoMessageFound!", 10.2);
	  n=write(newsockfd,response,strlen(response));
	}

      if (n < 0)
	error("ERROR writing to socket");
    } 

  close(newsockfd);
  close(sockfd);

  return 0;
}
