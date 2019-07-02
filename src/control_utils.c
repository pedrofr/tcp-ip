#include "control_utils.h"
#include <stddef.h>

double saturate(double value, double min, double max, char *saturated)
{
    char _saturated = value > max
      ? -1
      : value < min
         ? 1
         : 0;

    if (saturated != NULL) {
        *saturated = _saturated;
    }

    return value > max
        ? max
        : value < min
            ? min
            : value;
}

double dead_zone(double value, double lower_limit, double upper_limit, char *in_dead_zone)
{
    char _in_dead_zone = value > lower_limit && value < upper_limit;

    if (in_dead_zone != NULL)
    {
        *in_dead_zone = _in_dead_zone;
    }

    return _in_dead_zone ? 0 : value;
}

