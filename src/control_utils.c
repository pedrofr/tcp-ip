#include "control_utils.h"
#include <stddef.h>

double saturate(double value, double min, double max, int *saturated)
{
    int _saturated = value > max || value < min;

    if (saturated != NULL) {
        *saturated = _saturated;
    }

    return value > max
        ? max
        : value < min
            ? min
            : value;
}

double dead_zone(double value, double lower_limit, double upper_limit, int *in_dead_zone)
{
    int _in_dead_zone = value > lower_limit && value < upper_limit;

    if (in_dead_zone != NULL)
    {
        *in_dead_zone = _in_dead_zone;
    }

    return _in_dead_zone ? 0 : value;
}

