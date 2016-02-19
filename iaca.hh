// file iaca.hh
#ifndef IACA_INCLUDED_
#define IACA_INCLUDED_
// © 2016 Basile Starynkevitch
//   this file iaca.hh is part of IaCa
//   IaCa is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   IaCa is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with IaCa.  If not, see <http://www.gnu.org/licenses/>.
#define _GNU_SOURCE 1

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <QApplication>
#include <QCommandLineParser>

namespace Iaca {
  constexpr const char* version = "0.0";
  // forward declarations
  class Value;
  class Payload;
  class ItemVal;

  typedef const std::shared_ptr<ItemVal> ItemPtr;
  class Value {
  public:
    virtual ItemVal* ikind(void) const =0;
    virtual ~Value();
  };
  class IntVal : public Value {
    const intptr_t ival;
    virtual ItemVal* ikind(void) const;
    IntVal(intptr_t i=0): ival(i) {};
  };

  class DblVal : public Value {
    const double dval;
    virtual ItemVal* ikind(void) const;
    DblVal(double d=0): dval(d) {};
  };

  class StrVal : public Value {
    const QString sval;
    virtual ItemVal* ikind(void) const;
    StrVal(const QString&qs) : sval(qs) {};
    StrVal(const std::string& s) : sval(s.c_str()) {};
  };
  class ItemVal : public Value {
  };
};				// end of namespace Iaca
#endif
