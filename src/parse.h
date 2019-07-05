#include <stdbool.h>
#include "comm_consts.h"

#define is_empty(arg) *(arg) == '\0'
#define empty(arg)    *(arg) =  '\0'

typedef struct parsed_command
{
  char command[HALF_BUFFER_SIZE];
  char argument[HALF_BUFFER_SIZE];
} parscomm;

bool isNumeric(const char *s);
bool checkRange(double value, double min_range, double max_range);
bool matches_arg(const char *command, const char *argument, const char *desired_command, const char *desired_argument);
bool matches_no_arg(const char *command, const char *argument, const char *desired_command);
bool matches_numeric(const char *command, const char *argument, const char *desired_command);
void parse(parscomm *pcomm, const char *rawCommand, int min_range, int max_range, const char *exception);