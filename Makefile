## file Iaca/Makefile
## © 2011 Basile Starynkevitch 
##   this file Makefile is part of IaCa 
##   IaCa is free software: you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation, either version 3 of the License, or
##   (at your option) any later version.
## 
##   IaCa is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
## 
##   You should have received a copy of the GNU General Public License
##   along with IaCa.  If not, see <http://www.gnu.org/licenses/>.

## the pkg-config list of required packages
PACKAGES=gtk+-3.0 gmodule-2.0
CC=gcc
INDENT=indent
CFLAGS= -std=gnu99 $(OPTIMFLAGS) $(PREPROFLAGS)
OPTIMFLAGS= -Wall -g -O #-fno-inline
PREPROFLAGS= -D_GNU_SOURCE -I src -I /usr/local/include \
	$(shell pkg-config --cflags $(PACKAGES))
LIBES= -L /usr/local/lib -ljansson $(shell pkg-config --libs $(PACKAGES))  -lgc -lrt -lm
SOURCEFILES=$(wildcard src/*.c)
OBJECTFILES=$(patsubst %.c,%.o,$(SOURCEFILES))
MODULESRCFILES=$(wildcard module/*.c)
MODULES=$(patsubst module/%.c, module/lib%.so,$(MODULESRCFILES))
.PHONY: all clean modules indent

all: iaca modules
	sync

iaca: $(OBJECTFILES)
	$(CC) -rdynamic $(CFLAGS) $^ -o $@ $(LIBES)

$(OBJECTFILES): src/iaca.h

clean:
	$(RM) */*.o */*.so */*~ iaca *~ core*

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

module/lib%.so: module/%.c iaca
	$(CC)  -shared -fPIC $(CFLAGS) -DIACA_MODULE=\"$(basename $(notdir $<))\"  $< -o $@

modules: $(MODULES)

indent:
	for f in */*.h */*.c; do $(INDENT) $$f; mv $$f~ $$f.bak;  $(INDENT) $$f; mv $$f.bak $$f~;  done
