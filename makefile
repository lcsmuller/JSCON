CFLAGS = -Wall -Werror -pedantic
LDLIBS =
CC = gcc


SRCDIR = src
OBJDIR = obj
INCLUDE = include
LIB_FILE = libjsonc.a
TEST_EXEC = test_jsonc

OBJS = $(OBJDIR)/public.o
OBJS += $(OBJDIR)/stringify.o
OBJS += $(OBJDIR)/parser.o
OBJS += $(OBJDIR)/hashtable.o
OBJS += $(OBJDIR)/test.o

MAIN = test.c
MAIN_O = $(OBJDIR)/test.o

.PHONY : clean all debug purge test

all: $(LIB_FILE)

test: $(TEST_EXEC)

$(LIB_FILE): build
	-ar rcs $@ $(OBJS)

$(TEST_EXEC): build
	$(CC) -o $@ $(OBJS) $(LDLIBS)

build: mkdir $(MAIN_O) $(OBJS)

mkdir:
	-mkdir -p $(OBJDIR)

$(MAIN_O): $(MAIN)
	$(CC) -I$(INCLUDE) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -I$(INCLUDE) -c $< -o $@ $(CFLAGS)

debug : $(MAIN) $(SRCDIR)/*.c
	$(CC) -g $(MAIN) $(SRCDIR)/*.c -o debug.out $(CFLAGS)

clean :
	-rm -rf $(OBJDIR)

purge : clean
	-rm -rf $(TEST_EXEC) $(LIB_FILE) *.txt debug.out
