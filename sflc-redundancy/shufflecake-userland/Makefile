EXEC=shufflecake
SRCDIR=src
DEPDIR=$(SRCDIR)/include
BUILDDIR=build
OBJDIR=$(BUILDDIR)/obj

_OBJ=main.o sflc.o dmtask.o utils.o crypto.o disk.o
OBJ=$(patsubst %,$(OBJDIR)/%,$(_OBJ))

_DEP=sflc.h dmtask.h utils.h
DEP=$(patsubst %,$(DEPDIR)/%,$(_DEP))

CC=gcc
CFLAGS= -I$(DEPDIR) -Wall -Wno-pointer-sign -Wno-format -g
LIBS=-ldevmapper -lsodium

all: dirs $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $^ -o $(BUILDDIR)/$@ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEP)
	$(CC) $(CFLAGS) -c $< -o $@

dirs:
	mkdir -p $(BUILDDIR) $(OBJDIR)

debug: all

install: all
	cp $(BUILDDIR)/$(EXEC) /usr/bin

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/$(EXEC)
	rm -df $(OBJDIR) $(BUILDDIR)
