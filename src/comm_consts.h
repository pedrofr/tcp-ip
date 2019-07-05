#define BUFFER_SIZE      32
#define HALF_BUFFER_SIZE 15

#define INITIAL_ANGLE       50
#define INITIAL_LEVEL       40
#define REFERENCE           80
#define LEVEL_RATE          0.00002
#define VALVE_RATE          0.01
#define ACCEPTABLE_ERROR    0.01
#define MAX_VALUE           100
#define MIN_VALUE           0
#define OK                  "OK"

#define KP 10
#define KI 0.001
#define KD 0

#define NONE        0x00
#define SERVER      0x01
#define SIMULATOR   0x02
#define CLIENT      0x04
#define CONTROL     0x08
#define TERMINAL    0x0f
#define ANY         0xff

#define NOWAIT {0, 0}
#define CLIENT_TIMEOUT      {0, 350000000L}

#define PLANT_PERIOD        {0,  10000000L}
#define GRAPHICS_PERIOD     {0,  50000000L}
#define CONTROLLER_PERIOD   {0, 750000000L}

#define EPOCH_DURATION 50