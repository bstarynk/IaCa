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
#include <cmath>
#include <map>
#include <memory>
#include <random>
#include <exception>


#include <QApplication>
#include <QCommandLineParser>
#include <QHash>


// generated by the Makefile in file _timestamp.c
extern "C" const char iaca_timestamp[]; /// human readable timestamp
extern "C" long long iaca_timeclock; // time from Unix Epoch (Jan 1, 1970)
extern "C" const char iaca_lastgitcommit[];
extern "C" const char iaca_lastgittag[];

namespace Iaca {
  // forward declarations
  class Value;
  class Payload;
  class ItemVal;

  extern bool batch;
  
  typedef const std::shared_ptr<ItemVal> ItemPtr;
    typedef const std::shared_ptr<Value> ValuePtr;
  class Value {
  public:
    // every value has these
    virtual ItemVal* ikind(void) const =0;
    virtual uint hash(void) const =0;
    virtual ~Value();
    // a subtype T of Value also has
    /// static bool same(const T*v1, const T*v2);
    template<typename T> static bool same_val(const T*v1, const T*v2);
    /// static bool less(const T*v1, const T*v2);
    template<typename T> static bool less_val (const T*v1, const T*v2); 
  };
  
  class IntVal : public Value {
    const intptr_t _ival;
  public:
    intptr_t val() const { return _ival; };
    virtual ItemVal* ikind(void) const;
    virtual uint hash(void) const {
      uint h = qHash(_ival);
      if (!h) h = ((_ival&0xffff)+3);
      return h;
    }
    static bool same(const IntVal*i1, const IntVal*i2) {
      if (i1==i2) return true;
      if (!i1 || !i2) return false;
      return i1->_ival == i2->_ival;
    }
    static bool less(const IntVal*i1, const IntVal*i2) {
      if (i1==i2) return false;
      if (!i1) return true;
      if (!i2) return false;
      return i1->_ival < i2->_ival;
    };
    IntVal(intptr_t i=0): _ival(i) {};
    virtual ~IntVal() {  };
  };				// end class IntVal
  template<>
  bool Value::same_val<IntVal> (const IntVal*i1, const IntVal*i2)
  { return IntVal::same(i1,i2); };
  template<>
  bool Value::less_val<IntVal>(const IntVal*i1, const IntVal*i2)
  { return IntVal::less(i1,i2); };

  class DblVal : public Value {
    const double _dval;
  public:
    virtual ItemVal* ikind(void) const;
    DblVal(double d=0): _dval(d) {};
    virtual ~DblVal() {  };
    virtual uint hash(void) const {
      uint h = qHash(_dval);
      if (!h) h = 317;
      return h;
    };
    static bool same(const DblVal*d1, const DblVal*d2) {
      if (d1==d2) return true;
      if (!d1 || !d2) return false;
      // for doubles, NAN != NAN, so x != x is true when x is NAN
      return d1->_dval == d2->_dval
	|| (std::isnan(d1->_dval)&&std::isnan(d2->_dval));
    }
    // special care for NAN
    static bool less(const DblVal*d1, const DblVal*d2) {
      if (d1==d2) return false;
      if (!d1) return true;
      if (!d2) return false;
      double x1 = d1->_dval;
      double x2 = d2->_dval;
      if (x1 <= x2) return true;
      if (x1 > x2) return false;
      if (std::isnan(x1) && std::isnan(x2)) return false;
      if (std::isnan(x1)) return true;
      if (std::isnan(x2)) return false;
      return x1 < x2;
    };
  };
  template<>
  bool Value::same_val<DblVal> (const DblVal*d1, const DblVal*d2)
  { return DblVal::same(d1,d2); };
  template<>
  bool Value::less_val<DblVal>(const DblVal*d1, const DblVal*d2)
  { return DblVal::less(d1,d2); };

  enum class StrCategory :uint8_t {
    None,
      Word,			// has only letters
      Ident,			// C-ident like
      Plain			// other
      };
  StrCategory category(const QString&qs);
  StrCategory category(const char*pc);
  class StrVal : public Value {
    static uint hash_qstring (const QString&qs) {
      uint h = qHash(qs);
      if (!h) h = 2+((3*qs.size()+11)&0xfffff);
      return h;
    }
    const QString _sval;
    const StrCategory _scat;
    const uint _shash;
    StrVal(const QString&qs)
      : _sval(qs),
	_scat(category(qs)),
	_shash(hash_qstring(qs)) {};
    StrVal(const std::string& s)
      : _sval(s.c_str()),
	_scat(category(s.c_str())),
	_shash(hash_qstring(_sval)) {};
  public:
    const QString& val(void) const { return _sval; };
    virtual uint hash(void) const { return _shash; };    
    virtual ItemVal* ikind(void) const;
    static StrVal* make(const QString&q) {
      if (q.isEmpty()) return nullptr;
      return new StrVal(q); };
    static StrVal* make(const std::string&s) {
      if (s.empty()) return nullptr;
      return new StrVal(s); };
    static bool same(const StrVal*s1, const StrVal*s2) {
      if (s1==s2) return true;
      if (!s1 || !s2) return false;
      return s1->_sval == s2->_sval;
    }
    static bool less(const StrVal*s1, const StrVal*s2) {
      if (s1==s2) return false;
      if (!s1) return true;
      if (!s2) return false;
      return s1->_sval < s2->_sval;
    }
  };
  template<>
  bool Value::same_val<StrVal> (const StrVal*s1, const StrVal*s2)
  { return StrVal::same(s1,s2); };
  template<>
  bool Value::less_val<StrVal>(const StrVal*s1, const StrVal*s2)
  { return StrVal::less(s1,s2); };


  class Payload {
    ItemVal* _owneritem;
  public:
    virtual ~Payload() { _owneritem = nullptr; };
  };
  
  class ItemVal : public Value {
    const std::shared_ptr<const StrVal> _iradix;
    const uint64_t _irank;
    uint _ihash;
    std::unique_ptr<Payload> _ipayload;
    std::map<ItemPtr,ValuePtr> _iattrmap;
    static uint hash_str_rank(const StrVal*pradix, uint64_t rk)
    {
      if (!pradix)  throw std::runtime_error("nil radix for item");
      uint hr = pradix->hash();
      uint h = (313*hr) ^ (rk*2039);
      if (!h) h = (hr&0xfffff) + 3*(rk&0xffff) + 17;
      return h;
    }
    ItemVal(const StrVal*pradix,uint64_t rk) 
      : _iradix(pradix),_irank(rk), _ihash(hash_str_rank(pradix,rk)),
	_ipayload(),
	_iattrmap() {
      if (!pradix) throw std::runtime_error("nil radix for item");
    };
public:
    virtual uint hash(void) const { return _ihash; };
    static bool same(const ItemVal*it1, const ItemVal*it2) {
      return it1 == it2;
    };
    static bool less(const ItemVal*it1, const ItemVal*it2) {
      if (it1 == it2) return false;
      if (!it1) return true;
      if (!it2) return false;
      if (it1->_iradix == it2->_iradix)
	return it1->_irank < it2->_irank;
      return (it1->_iradix->val() < it2->_iradix->val());
    }
  };				// end class ItemVal
  template<>
  bool Value::same_val<ItemVal> (const ItemVal*it1, const ItemVal*it2)
  { return ItemVal::same(it1,it2); };
  template<>
  bool Value::less_val<ItemVal>(const ItemVal*it1, const ItemVal*it2)
  { return ItemVal::less(it1,it2); };

};				// end of namespace Iaca
#endif
