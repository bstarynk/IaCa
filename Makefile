## file Iaca/Makefile
## © 2012 Basile Starynkevitch 
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

CXX=g++
ASTYLE=astyle
CXXFLAGS= -std=gnu++11 $(OPTIMFLAGS)
# fastcgi++ from http://www.nongnu.org/fastcgipp
PACKAGES= fastcgi++
OPTIMFLAGS= -Wall -g -O #-fno-inline
PREPROFLAGS= -D_GNU_SOURCE -I /usr/local/include $(pkg-config --cflags $(PACKAGES))
LIBES= -L /usr/local/lib  $(pkg-config --libs $(PACKAGES)) -lboost_system -lrt -ldl
SOURCES= $(wildcard iaca*.cc)
OBJECTS= $(patsubst %.cc,%.o,$(SOURCES))

.PHONY: all clean modules indent

all: iaca 
	sync

iaca: $(OBJECTS)
	$(LINK.cc) -rdynamic $^ -o $@ $(LIBES)

$(OBJECTS): iaca.hh iaca.hh.gch

iaca.hh.gch: iaca.hh
	$(COMPILE.cc) $^ -c -o $@
clean:
	$(RM) *.o *.so *~ iaca *~ core* *.gch *.orig


indent:
	for f in *.hh *.cc; do $(ASTYLE) $$f;  done
