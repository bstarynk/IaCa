## file Iaca/Makefile
## © 2016 Basile Starynkevitch 
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
# jsoncpp is from https://github.com/open-source-parsers/jsoncpp
# Qt5Gui is from Qt, see http://www.qt.io/download/ & http://doc.qt.io/qt-5/
PACKAGES= jsoncpp Qt5Gui Qt5Widgets
## Qt5 needs -fPIC
OPTIMFLAGS= -Wall -Wextra -g -O -fPIC #-fno-inline
PREPROFLAGS= -D_GNU_SOURCE -I /usr/local/include $(shell pkg-config --cflags $(PACKAGES))
LIBES= -L /usr/local/lib  $(shell pkg-config --libs $(PACKAGES)) -ldl
SOURCES= $(wildcard iaca*.cc)
OBJECTS= $(patsubst %.cc,%.o,$(SOURCES)) iaca.moc.cc
QTMOC= moc

.PHONY: all clean modules indent

all: iaca 
	sync

iaca: $(OBJECTS)
	$(LINK.cc) -rdynamic $^ -o $@ $(LIBES)

iaca.moc.cc: iaca.hh
	$(QTMOC) $< -o $@

$(OBJECTS): iaca.hh iaca.hh.gch

iaca.hh.gch: iaca.hh
	$(COMPILE.cc) $(PREPROFLAGS) $^ -c -o $@
clean:
	$(RM) *.o *.so *~ iaca *~ core* *.gch *.orig


indent:
	for f in *.hh *.cc; do $(ASTYLE) $$f;  done
