CFLAGS = -Wall -Werror -pedantic
LDLIBS = -lm
CC = gcc

SRCDIR = src
OBJDIR = obj
EXEC = JSON

OBJS = $(OBJDIR)/test.o
OBJS += $(OBJDIR)/parser.o
OBJS += $(OBJDIR)/stringify.o
OBJS += $(OBJDIR)/public.o

HEADER = JSON.h

MAIN = test.c
MAIN_O = $(OBJDIR)/test.o

.PHONY : clean all debug

all: $(EXEC)

$(EXEC): build
	$(CC) -o $@ $(OBJS) $(LDLIBS)

build: mkdir $(MAIN_O) $(OBJS)

mkdir:
	-mkdir -p $(OBJDIR)

$(MAIN_O): $(MAIN) $(HEADER)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

debug : $(MAIN) $(SRCDIR)/*.c
	$(CC) -g $(MAIN) $(SRCDIR)/*.c -o debug.out $(CFLAGS)

clean :
	-rm -rf $(EXEC) data.txt $(OBJDIR) debug.out
