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
CFLAGS= $(OPTIMFLAGS) $(PREPROFLAGS)
OPTIMFLAGS= -Wall -O -g -std=gnu99
PREPROFLAGS= -D_GNU_SOURCE -I src -I /usr/local/include \
	$(shell pkg-config --cflags $(PACKAGES))
LIBES= -L /usr/local/lib -ljansson $(shell pkg-config --libs $(PACKAGES))  -lgc -lrt -lm
SOURCEFILES=$(wildcard src/*.c)
OBJECTFILES=$(patsubst %.c,%.o,$(SOURCEFILES))
.PHONY: all clean indent

all: iaca

iaca: $(OBJECTFILES)
	$(CC) -rdynamic $(CFLAGS) $^ -o $@ $(LIBES)

$(OBJECTFILES): src/iaca.h

clean:
	$(RM) src/*.o src/*.so src/*~ iaca *~

src/%.o: src/%.c
	$(CC) $(CFLAGS) -DIACA_MODULE=\"$(basename $(notdir $<))\" -c $< -o $@
indent:
	for f in src/*.h src/*.c; do $(INDENT) $$f; done
