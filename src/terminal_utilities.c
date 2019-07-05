#define _GNU_SOURCE

#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "terminal_utilities.h"

static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
  tcgetattr(0, &old);     /* grab old terminal i/o settings */
  new = old;              /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */

  new.c_lflag &= ~ISIG;
  new.c_cc[VMIN] = 0;
  new.c_cc[VTIME] = 0;

  if (echo)
  {
    new.c_lflag |= ECHO; /* set echo mode */
  }
  else
  {
    new.c_lflag &= ~ECHO; /* set no echo mode */
  }
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void)
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
  return getch_(1);
}

#include "error.h"
#include "time_utils.h"
#include "comm_consts.h"

void *keyboard_handler(void *args);

/* Let's test it out */
// int main(void)
// {
//   pthread_t control_thread;

//   int errnum;
//   if ((errnum = pthread_create(&control_thread, NULL, keyboard_handler, NULL)))
//   {
//     errorf("\nThread creation failed: %d\n", errnum);
//   }

//   while (1)
//   {
//     timestamp_printf("olá!");
//   }

//   return 0;
// }
#include "thread_utils.h"

static volatile char quit;

void *keyboard_handler(void *args)
{
  timestamp_printf("Starting manual input service!\n");

  pararg *parg = (pararg *)args;
  parscomm *pcomm = parg->pcomm;
  char c, buffer[BUFFER_SIZE];

  printf("\nPress [crtl]+[m] or [enter] to enter manual command.\n\r\033[2A");

  while (!quit)
  {
    c = getch();
    if (c == '\n')
    {
      suspend_timed_output();

      printf("\n\r\033[K");
      timestamp_force_printf("Enter manual command: ");

      scanf("%s", buffer);
      
      //Clear buffer
      while ((c = getchar()) != '\n' && c != EOF)
        ;

      parscomm oldpcomm = *pcomm;
      parse(&oldpcomm, buffer, MIN_VALUE, MAX_VALUE, OK);

      request_ownership(parg, TERMINAL);
      int try = 0;
      do
      {
        timestamp_force_printf("try #%i: command '%s', argument '%s'\n", ++try, oldpcomm.command, oldpcomm.argument);

        strcpy(pcomm->command, oldpcomm.command);
        strcpy(pcomm->argument, oldpcomm.argument);

        wait_for_response(parg, TERMINAL, CLIENT);
      } while (!quit && !is_empty(oldpcomm.command) && (is_empty(pcomm->command) || strstr(oldpcomm.command, pcomm->command) == NULL));

      timestamp_force_printf("Server responded: %s\n", parg->buffer);
      grant_ownership(parg, TERMINAL, CONTROL);

      restore_timed_output();
      printf("\nPress [crtl]+[m] or [enter] to enter manual command.\n\r\033[2A");
    }
  }

  timestamp_printf("Closing manual input service!\n");

  pthread_exit(NULL);
}

void quit_keyboard_handler()
{
  quit = 1;
}