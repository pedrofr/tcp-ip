#!/bin/bash
gcc server.c parse.c simulator.c error.c plant.c -o server -pthread -lm -Wall -Wextra
gcc client.c parse.c error.c -o client -Wall -Wextra