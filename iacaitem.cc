// file iacaitem.cc

// © 2016 Basile Starynkevitch
//   this file iacaitem.cc is part of IaCa
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

#include "iaca.hh"

using namespace Iaca;

std::map<QString,std::shared_ptr<StrVal>> ItemVal::_radix_dict_;

bool
ItemVal::valid_radix(const QString&qs) {
    if (qs.isEmpty()) return false;
    int sz = qs.size();
    if (qs[0].unicode()>=128 || !std::isalpha(qs[0].toLatin1()))
        return false;
    for (int ix=1; ix<sz; ix++) {
        if (qs[ix].unicode()>=128) return false;
        char c = qs[ix].toLatin1();
        if (!c) return false;
        if (std::isalnum(c))
            continue;
        if (c=='_') {
            if (ix==sz-1) return false;
            if (qs[ix-1].toLatin1() == '_') return false;
        }
    }
    return true;
}

const StrVal*
ItemVal::register_radix(const QString&qs) {
    if (!valid_radix(qs)) return nullptr;
    auto it = _radix_dict_.find(qs);
    if (it != _radix_dict_.end())
        return it->second.get();
    StrVal* rad = new StrVal(qs);
    _radix_dict_[rad->val()] = std::shared_ptr<StrVal>(rad);
    return rad;
}

const StrVal*
ItemVal::find_radix(const QString&qs) {
    if (!valid_radix(qs)) return nullptr;
    auto it = _radix_dict_.find(qs);
    if (it != _radix_dict_.end())
        return it->second.get();
    return nullptr;
}

