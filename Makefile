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

CC 	= gcc
SRCDIR 	= src
OBJDIR 	= obj
INCLDIR = include

SRC   = $(filter-out src/*_private.c, $(wildcard src/*.c))
_OBJS = $(patsubst src/%.c, %.o, $(SRC))
OBJS  = $(addprefix $(OBJDIR)/, $(_OBJS))

JSCON_DLIB = libjscon.so
JSCON_SLIB = libjscon.a

CFLAGS = -Wall -Werror -pedantic -g -I$(INCLDIR)

LDLIBS =

.PHONY : all clean purge

all: mkdir $(OBJS) $(JSCON_DLIB) $(JSCON_SLIB)

mkdir:
	mkdir -p $(OBJDIR) 

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -fPIC $< -o $@ $(CFLAGS)

$(JSCON_DLIB):
	$(CC) $(OBJS) -shared -o $(JSCON_DLIB) $(LDLIBS)	

$(JSCON_SLIB):
	ar rcs $@ $(OBJS)

install: all
	cp $(JSCON_DLIB) /usr/local/lib && \
	ldconfig

clean :
	-rm -rf $(OBJDIR)

purge : clean
	-rm -rf $(JSCON_DLIB) $(JSCON_SLIB) *.txt
