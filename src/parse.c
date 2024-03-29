#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "error.h"
#include "parse.h"

bool isNumeric(const char *s)
{
    if (s == NULL || is_empty(s) || isspace(*s))
        return 0;
    char *p;
    strtod(s, &p);
    return is_empty(p);
}

bool checkRange(double value, double min_range, double max_range)
{
    if (min_range > max_range)
        errorf("checkRange limits");

    return (min_range <= value) & (value <= max_range);
}

bool matches_arg(const char *command, const char *argument, const char *desired_command, const char *desired_argument)
{
    return strcmp(command, desired_command) == 0 && strcmp(argument, desired_argument) == 0;
}

bool matches_no_arg(const char *command, const char *argument, const char *desired_command)
{
    return strcmp(command, desired_command) == 0 && is_empty(argument);
}

bool matches_numeric(const char *command, const char *argument, const char *desired_command)
{
    return strcmp(command, desired_command) == 0 && isNumeric(argument);
}

void parse(parscomm *pcomm, const char *rawCommand, int min_range, int max_range, const char *exception)
{
    char *command;
    char *argument;
    char *string, *tofree;

    //default means error
    empty(pcomm->command);
    empty(pcomm->argument);

    if (min_range > max_range)
        errorf("parse limits");

    tofree = string = strdup(rawCommand);

    if (string == NULL)
        errorf("strcpy");

    if (string[strlen(string) - 1] != '!')
    {
        //error
        free(tofree);

        return;
    }

    command = strsep(&string, "#!");

    if (is_empty(command))
    {
        //error
        free(tofree);

        return;
    }

    int diff = string - command;

    int length = strlen(string);

    if (!length && rawCommand[diff - 1] == '!')
    {
        //only command
        strcpy(pcomm->command, command);
    }
    else if (length && rawCommand[diff - 1] == '#')
    {
        argument = strsep(&string, "!");

        if (strlen(string))
        {
            //error
        }
        else if (isNumeric(argument))
        {
            double argument_d = atof(argument);

            if (checkRange(argument_d, min_range, max_range))
            {
                //command and numeric argument
                strcpy(pcomm->command, command);
                strcpy(pcomm->argument, argument);
            }
        }
        else if (!strcmp(argument, exception))
        {
            //command and exception argument
            strcpy(pcomm->command, command);
            strcpy(pcomm->argument, argument);
        }
        //else error
        
    }
    //else error

    free(tofree);
}
