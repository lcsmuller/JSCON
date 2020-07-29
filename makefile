CFLAGS = -Wall -Werror -pedantic
LDLIBS = -lm
CC = gcc

SRCDIR = src
OBJDIR = obj

OBJS = $(OBJDIR)/main.o
OBJS += $(OBJDIR)/parser.o

HEADER = json_parser.h

MAIN = main.c
MAIN_O = $(OBJDIR)/main.o

.PHONY : clean all debug

all: json_parser

json_parser: build
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
	-rm -rf json_parser test.txt $(OBJDIR) debug.out
