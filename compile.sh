#!/bin/bash

echo "error.o:"
gcc -g -c -Wall -Wextra error.c
echo "parse.o:"
gcc -g -c -Wall -Wextra parse.c
echo "plant.c:"
gcc -g -c -Wall -Wextra plant.c -pthread -lm -lrt
echo "simulator.c:"
gcc -g -c -Wall -Wextra simulator.c -pthread

echo "client.o:"
gcc -g -c -Wall -Wextra client.c
echo "server.c:"
gcc -g -c -Wall -Wextra server.c -pthread

echo "client:"
gcc -g -Wall -Wextra -o client client.o error.o parse.o
echo "server:"
gcc -g -Wall -Wextra -o server server.o error.o parse.o plant.o simulator.o -pthread -lm -lrt
