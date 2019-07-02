#include "comm_consts.h"

#define is_empty(arg) *(arg) == '\0'
#define empty(arg) *(arg) = '\0'

typedef struct parsed_command
{
  char command[HALF_BUFFER_SIZE];
  char argument[HALF_BUFFER_SIZE];
} parscomm;

char isNumeric(const char *s);
char checkRange(double value, double min_range, double max_range);
char matches_arg(const char *command, const char *argument, const char *desired_command, const char *desired_argument);
char matches_no_arg(const char *command, const char *argument, const char *desired_command);
char matches_numeric(const char *command, const char *argument, const char *desired_command);
void parse(parscomm *pcomm, const char *rawCommand, double min_range, double max_range, const char *exception);