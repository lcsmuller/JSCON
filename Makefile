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

CC			?= gcc
SRCDIR	:= src
OBJDIR	:= obj
INCLDIR	:= include
LIBDIR	:= lib

SRC		:= $(wildcard src/*.c)
_OBJS	:= $(patsubst src/%.c, %.o, $(SRC))
OBJS	:= $(addprefix $(OBJDIR)/, $(_OBJS))

JSCON_DLIB	:= $(LIBDIR)/libjscon.so
JSCON_SLIB	:= $(LIBDIR)/libjscon.a

LIBJSCON_CFLAGS	:= -I$(INCLDIR)

LIBS_CFLAGS	:= $(LIBJSCON_CFLAGS)

CFLAGS	:= -Wall -Wextra -pedantic \
	-fPIC -std=c11 -O0 -g -D_XOPEN_SOURCE=700

.PHONY : all clean purge

all : mkdir $(OBJS) $(JSCON_DLIB) $(JSCON_SLIB)

mkdir :
	mkdir -p $(OBJDIR) $(LIBDIR)

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(LIBS_CFLAGS) \
		-c $< -o $@

$(JSCON_DLIB) :
	$(CC) $(LIBS_CFLAGS) \
	      $(OBJS) -shared -o $(JSCON_DLIB)

$(JSCON_SLIB) :
	$(AR) -cvq $@ $(OBJS)

install : all
	cp $(INCLDIR)/* /usr/local/include
	cp $(JSCON_DLIB) /usr/local/lib && \
	ldconfig

clean :
	rm -rf $(OBJDIR)

purge : clean
	$(MAKE) -C test clean
	$(MAKE) -C examples clean
	rm -rf $(LIBDIR) *.txt
