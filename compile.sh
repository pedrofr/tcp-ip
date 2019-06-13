#!/bin/bash
gcc server.c parse.c simulator.c error.c -o server -pthread
gcc client.c parse.c error.c -o client