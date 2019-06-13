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
#include "simulador.h"
#include <pthread.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

parscomm pcomm;
pararg parg;
int mod;

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer_in[256];
  char buffer_out[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

   printf("chegou aqui");

  parg.pcomm = &pcomm;
  parg.modificado = &mod;


   int rc1;
pthread_t thread1, thread2;

   /* Create independent threads each of which will execute functionC */

   if( (rc1=pthread_create( &thread1, NULL, simulate, NULL)) )
   {
      printf("Thread creation failed: %d\n", rc1);
   }



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

    printf("\nRequest: %s\n", buffer_in);

    pcomm = parse(buffer_in, MIN_VALUE, MAX_VALUE, OK);

    printf("command: '%s'\n", pcomm.command);
    printf("argument: '%s'\n", pcomm.argument);
    mod=1;

    while(mod==1);
  
    sprintf(buffer_out, "%s#%s!",pcomm.command, pcomm.argument);
    n = write(newsockfd, buffer_out, strlen(buffer_out));
   
    if (n < 0)
      error("ERROR writing to socket");
   
    if (matches_arg(pcomm.command, pcomm.argument, "Exit",OK))    
      break;
  }

  close(newsockfd);
  close(sockfd);

   pthread_join( thread1, NULL);
   pthread_join( thread2, NULL); 

  printf("\nClosing!\n");

  return 0;
}
