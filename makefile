ODIR = ./obj
BDIR = ./bin
SDIR = ./src
CC = gcc
LIBS = -pthread -lm -lrt -lSDL
OPTS = -g -Wall -Wextra

DIRS = $(ODIR) $(BDIR)

.PHONY: all obj prog clean cleanobj cleanprog cleandir

all: prog

$(shell mkdir -p $(DIRS))

# _DEPS = hellomake.h
# DEPS = $(patsubst %,/%,$(_DEPS))

_PROG = server client
PROG = $(patsubst %,$(BDIR)/%,$(_PROG))

_POBJ = $(patsubst %,%.o,$(_PROG))
POBJ = $(patsubst %,$(ODIR)/%,$(_POBJ))

_OBJ = error.o parse.o plant.o simulator.o graph.o
OBJECTS = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(OBJECTS) $(POBJ): $(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(OPTS) -c -o $@ $<

$(PROG): $(BDIR)/%: $(ODIR)/%.o $(OBJECTS)
	$(CC) $(OPTS) -o $@ $^ $(LIBS)

obj: $(OBJECTS) $(POBJ)
prog: $(PROG)
     
clean: cleanobj cleanprog
cleandir:
	rm -rf $(ODIR) $(BDIR)
cleanprog:
	rm -f $(PROG)
cleanobj:
	rm -f $(OBJECTS) $(POBJ)