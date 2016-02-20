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
CC= gcc
CFLAGS= $(OPTIMFLAGS)
# jsoncpp is from https://github.com/open-source-parsers/jsoncpp
# Qt5Gui is from Qt, see http://www.qt.io/download/ & http://doc.qt.io/qt-5/
PACKAGES= jsoncpp Qt5Gui Qt5Widgets
## Qt5 needs -fPIC
OPTIMFLAGS= -Wall -Wextra -g -O -fPIC #-fno-inline
PREPROFLAGS= -D_GNU_SOURCE  $(shell pkg-config --cflags $(PACKAGES))
LIBES=  $(shell pkg-config --libs $(PACKAGES)) -ldl
SOURCES= $(wildcard iaca*.cc)
OBJECTS= $(patsubst %.cc,%.o,$(SOURCES)) iaca.moc.o
QTMOC= moc

.PHONY: all clean modules indent

all: iaca 
	sync

_timestamp.c: Makefile
	@date +'const char iaca_timestamp[]="%c %Z";%nconst long long iaca_timeclock=%sLL;' > _timestamp.tmp
	@(echo -n 'const char iaca_lastgitcommit[]="' ; \
	   env LANG=C git diff --shortstat | awk '{if ($$1>0) printf "/%dF+%d-%d:", $$1, $$4, $$6}' ; \
	   git log --format=oneline --abbrev=12 --abbrev-commit -q  \
	     | head -1 | tr -d '\n\r\f\"' ; \
	   echo '";') >> _timestamp.tmp
	@(echo -n 'const char iaca_lastgittag[]="'; (git describe --abbrev=0 --all || echo '*notag*') | tr -d '\n\r\f\"'; echo '";') >> _timestamp.tmp
	mv _timestamp.tmp _timestamp.c

iaca: $(OBJECTS)
	$(MAKE) _timestamp.o
	$(LINK.cc) -rdynamic $^ _timestamp.o -o $@ $(LIBES)
	rm _timestamp.*

iaca.moc.cc: iaca.hh
	$(QTMOC) $< -o $@

$(OBJECTS): iaca.hh iaca.hh.gch

iaca.hh.gch: iaca.hh
	$(COMPILE.cc) $(PREPROFLAGS) $^ -c -o $@
clean:
	$(RM) *.o *.so *~ iaca *~ core* *.gch *.orig _timestamp.*


indent:
	for f in *.hh *.cc; do $(ASTYLE) $$f;  done
