#include "comm_consts.h"

#define is_empty(arg) *arg == '\0'

typedef struct parsed_command
{
  char command[HALF_BUFFER_SIZE];
  char argument[HALF_BUFFER_SIZE];
} parscomm;

int isNumeric(const char *s);
int checkRange(double value, double min_range, double max_range);
int matches_arg(const char *command, const char *argument, const char *desired_command, const char *desired_argument);
int matches_no_arg(const char *command, const char *argument, const char *desired_command);
int matches_numeric(const char *command, const char *argument, const char *desired_command);
parscomm parse(const char *rawCommand, double min_range, double max_range, const char *exception);