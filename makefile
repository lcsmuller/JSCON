CFLAGS = -Wall -Werror -pedantic
LDLIBS =
CC = gcc

SRCDIR = src
OBJDIR = obj
EXEC = JSONc

OBJS = $(OBJDIR)/public.o
OBJS += $(OBJDIR)/stringify.o
OBJS += $(OBJDIR)/parser.o
OBJS += $(OBJDIR)/hashtable.o
OBJS += $(OBJDIR)/test.o

MAIN = test.c
MAIN_O = $(OBJDIR)/test.o

.PHONY : clean all debug purge

all: $(EXEC)

$(EXEC): build
	$(CC) -o $@ $(OBJS) $(LDLIBS)

build: mkdir $(MAIN_O) $(OBJS)

mkdir:
	-mkdir -p $(OBJDIR)

$(MAIN_O): $(MAIN)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

debug : $(MAIN) $(SRCDIR)/*.c
	$(CC) -g $(MAIN) $(SRCDIR)/*.c -o debug.out $(CFLAGS)

clean :
	-rm -rf $(OBJDIR)

purge : clean
	-rm -rf $(EXEC) *.txt debug.out
