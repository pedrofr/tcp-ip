#include <stdio.h>
#include <string.h>
#include "error.h"
#include "parse.h"
#include "simulador.h"
#include "comm_consts.h"

void *simulate(void *args)
{
  pararg *parg = (pararg *)args;
  parscomm * pcomm = parg->pcomm;
  int*  mod = parg->modificado; 
  char command[256]; 
  char argument[256];

  double delta = 0;

  double Max = 100;

  double dt = 0.05;

  while(1)
    {
      while((*mod)==0);
      
      if (matches_numeric(pcomm->command, pcomm->argument, "OpenValve"))
	{
	  double value = (int)atof(pcomm->argument);
	    delta += value;
	  sprintf(argument, "%i", value);
	  strcpy(pcomm->command,"Open");
	  strcpy(pcomm->argument,argument);
	}
      else if (matches_numeric(pcomm->command, pcomm->argument, "CloseValve"))
	{
	  double value = (int)atof(pcomm->argument);
	    delta -= value;
	  sprintf(argument, "%i", value);
	  strcpy(pcomm->command,"Close");
	  strcpy(pcomm->argument,argument);
	}
      else if (matches_numeric(pcomm->command, pcomm->argument, "SetMax"))
	{
	  Max = (int)atof(pcomm->argument);
	  sprintf(argument, "%i", value);
	  strcpy(pcomm->command,"Max");
	  strcpy(pcomm->argument,argument);
	}
      else if (matches_no_arg(pcomm->command, pcomm->argument, "GetLevel"))
	{
	  sprintf(argument, "%i!", (int)delta);
	  strcpy(pcomm->command,"Level");
	  strcpy(pcomm->argument,argument);
	}
      else if (matches_no_arg(pcomm->command, pcomm->argument, "CommTest"))
	{
	  strcpy(pcomm->command,"Comm");
	  strcpy(pcomm->argument,OK);
	}
      else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
	{
	  strcpy(pcomm->command,"Start");
	  strcpy(pcomm->argument,OK);
	}
      else if (matches_no_arg(pcomm->command, pcomm->argument, "Exit"))
	{
	  strcpy(pcomm->command,"Exit");
	  strcpy(pcomm->argument,OK);
	  break;
	}else
	{
	  strcpy(pcomm->command, "NoMessageFound!");
	  strcpy(pcomm->argument,"");
	}

          (*mod)=0;
    }

  printf("\nSaindo do simulador\n");
}
