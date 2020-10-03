#
# Copyright (c) 2020 Lucas Müller
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

CFLAGS = -Wall -Werror -pedantic
LDLIBS =
CC = gcc


SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include
LIBDIR = lib
LIB = $(LIBDIR)/libjscon.a
TEST1 = test_jscon

OBJS = $(OBJDIR)/public.o \
       $(OBJDIR)/stringify.o \
       $(OBJDIR)/parser.o \
       $(OBJDIR)/hashtable.o \
       $(OBJDIR)/test2.o

MAIN = test2.c
MAIN_O = $(OBJDIR)/test2.o

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
	$(CC) -g -I$(INCLUDEDIR) $(MAIN) $(SRCDIR)/*.c -o debug.out $(CFLAGS)

clean :
	-rm -rf $(OBJDIR)

purge : clean
	-rm -rf $(TEST1) $(LIBDIR) *.txt debug.out
