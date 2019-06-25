ODIR = obj
BDIR = bin
SDIR = src
CC = gcc
LIBS = -pthread -lm -lrt -lSDL
OPTS = -g -Wall -Wextra

# _DEPS = hellomake.h
# DEPS = $(patsubst %,/%,$(_DEPS))

_PROG = server client
PROG = $(patsubst %,$(BDIR)/%,$(_PROG))

_POBJ = $(patsubst %,%.o,$(_PROG))
POBJ = $(patsubst %,$(ODIR)/%,$(_POBJ))

_OBJ = error.o parse.o plant.o simulator.o graph.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: prog
obj: $(OBJ) $(POBJ)
prog: $(PROG)

$(OBJ) $(POBJ): $(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(OPTS) -c -o $@ $<

$(PROG): $(BDIR)/%: $(ODIR)/%.o $(OBJ)
	$(CC) $(OPTS) -o $@ $^ $(LIBS)
     
clean:
	rm -f $(OBJ) $(POBJ) $(PROG)