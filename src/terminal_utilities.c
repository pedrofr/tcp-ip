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
  // char c, 
  char buffer[BUFFER_SIZE];

  //https://stackoverflow.com/questions/29989516/deep-copying-structs-with-char-arrays-in-c-how-to-copy-the-arrays?noredirect=1&lq=1
  parscomm old_pcomm = *pcomm;

  //printf("\nPress [crtl]+[m] or [enter] to enter manual command.\n\r\033[2A");

  while (!quit && strcmp(old_pcomm.command, "Exit"))
  {
    // c = getch();
    // if (c == '\n')
    // {

      suspend_timed_output();

      printf("\n\r\033[K");
      timestamp_force_printf("Enter manual command: ");

      scanf("%s", buffer);
      
      //Clear buffer
      // while ((c = getchar()) != '\n' && c != EOF)
      //   ;

      old_pcomm = *pcomm;
      parse(&old_pcomm, buffer, MIN_VALUE, MAX_VALUE, OK);

      request_ownership(parg, TERMINAL);
      int try = 0;
      do
      {
        *pcomm = old_pcomm;

        timestamp_force_printf("try #%i: command '%s', argument '%s'\n", ++try, pcomm->command, pcomm->argument);

        wait_for_response(parg, TERMINAL, CLIENT);

      } while (!quit && !is_empty(old_pcomm.command) && (is_empty(pcomm->command) || strstr(old_pcomm.command, pcomm->command) == NULL));

      old_pcomm = *pcomm;

      timestamp_force_printf("Server responded: %s\n", parg->buffer);
      grant_ownership(parg, TERMINAL, CONTROL);

      restore_timed_output();
      //printf("\nPress [crtl]+[m] or [enter] to enter manual command.\n\r\033[2A");
    // }
  }

  timestamp_printf("Closing manual input service!\n");

  pthread_exit(NULL);
}

void quit_keyboard_handler()
{
  quit = 1;
}