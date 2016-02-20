// file iacaval.cc

// © 2016 Basile Starynkevitch
//   this file iacaval.cc is part of IaCa
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


ItemVal*
IntVal::ikind() const {
}


ItemVal*
DblVal::ikind() const {
}

ItemVal*
StrVal::ikind() const {
}

StrCategory
StrVal::category(const QString&qs)
{
    if (qs.isEmpty()) return StrCategory::None;
    int sz = qs.size();
    int nbalucnu=0;		// number of ASCII alphanumerical & underscore
    int nbletters=0;		// number of Unicode letters
    for (int ix=0; ix<sz; ix++) {
        unsigned uc = qs[ix].unicode();
        if (uc<128 && (std::isalnum((char)uc) || uc=='_')) nbalucnu++;
        if (qs[ix].isLetter()) nbletters++;
    }
    if (nbalucnu == sz
            && (std::isalpha(qs[0].toLatin1()) || qs[0].toLatin1()=='_'))
        return StrCategory::Ident;
    if (nbletters == sz)
        return StrCategory::Word;
    return StrCategory::Plain;
}

StrCategory
StrVal::category(const char*pc)
{
    if (!pc || !pc[0]) return StrCategory::None;
    QString qs {pc};
    return category(qs);
}

uint
SeqItemsVal::hash_itemsarr( ItemVal* arr[], unsigned siz, unsigned seed)
{
    uint h1 = seed, h2 = 0;
    for (unsigned ix=0; ix<siz; ix++) {
        auto curit = arr[ix];
        if (!curit) throw std::runtime_error("nil item in sequence");
        if (ix % 2 == 0)
            h1 = (1193*h1 + ix) ^ (curit->hash()*2087);
        else
            h2 = (1151*h2 - ix) ^ (curit->hash()*2099);
    }
    uint h = (11*h1) ^ (31*h2);
    if (!h)
        h = (((13*h1) ^ (h2 / 17))&0xfffff) + (siz&0xff) +11;
    return h;
}


bool SeqItemsVal::less(const SeqItemsVal*sq1, const SeqItemsVal*sq2) {
    if (sq1 == sq2) return false;
    if (!sq1) return true;
    if (!sq2) return false;
    if (sq1->_shash == sq2->_shash && same(sq1,sq2)) return false;
    return std::lexicographical_compare
           (sq1->_sarr,sq1->_sarr+sq1->_slen,
            sq2->_sarr,sq2->_sarr+sq2->_slen,
            ItemVal::less);
}
