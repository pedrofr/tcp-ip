ODIR = ./obj
BDIR = ./bin
SDIR = ./src
CC = gcc
LIBS = -pthread -lm -lrt -lSDL
OPTS = -g -Wall -Wextra -pedantic

_PROG = server client
_OBJ = error.o parse.o plant.o simulator.o graphics.o control_utils.o controller.o control.o thread_utils.o time_utils.o terminal_utilities.o

DIRS = $(ODIR) $(BDIR)

PROG = $(patsubst %,$(BDIR)/%,$(_PROG))
_POBJ = $(patsubst %,%.o,$(_PROG))
POBJ = $(patsubst %,$(ODIR)/%,$(_POBJ))
OBJECTS = $(patsubst %,$(ODIR)/%,$(_OBJ))

.PHONY: all obj prog clean cleanobj cleanprog cleandir

all: prog

$(shell mkdir -p $(DIRS))

# _DEPS = hellomake.h
# DEPS = $(patsubst %,/%,$(_DEPS))

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
