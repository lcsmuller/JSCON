CFLAGS = -Wall -Werror -pedantic
LDLIBS =
CC = gcc


SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include
LIBDIR = lib
LIB = $(LIBDIR)/libjsonc.a
TEST1 = test_jsonc

OBJS = $(OBJDIR)/public.o
OBJS += $(OBJDIR)/stringify.o
OBJS += $(OBJDIR)/parser.o
OBJS += $(OBJDIR)/hashtable.o
OBJS += $(OBJDIR)/test.o

MAIN = test.c
MAIN_O = $(OBJDIR)/test.o

.PHONY : clean all debug purge test

all: build

test: $(TEST1)

$(TEST1): build
	$(CC) -o $@ $(OBJS) $(LDLIBS)

build: mkdir $(MAIN_O) $(OBJS) $(LIB)

mkdir:
	-mkdir -p $(OBJDIR) $(LIBDIR)

$(MAIN_O): $(MAIN)
	$(CC) -I$(INCLUDEDIR) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -I$(INCLUDEDIR) -c $< -o $@ $(CFLAGS)

$(LIB):
	-ar rcs $@ $(OBJS)

debug : $(MAIN) $(SRCDIR)/*.c
	$(CC) -g $(MAIN) $(SRCDIR)/*.c -o debug.out $(CFLAGS)

clean :
	-rm -rf $(OBJDIR)

purge : clean
	-rm -rf $(TEST1) $(LIBDIR) *.txt debug.out
