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


bool ValuePtr::same(const ValuePtr vp1, const ValuePtr vp2)
{
    const Value*valp1 = vp1.get();
    const Value*valp2 = vp2.get();
    if (valp1 == valp2) return true;
    if (!valp1 || !valp2) return false;
    auto k1 = valp1->kind();
    auto k2 = valp2->kind();
    if (k1 != k2) return false;
    switch (k1) {
    case ValKind::Nil:
        abort();
    case ValKind::Int:
        return IntVal::same(static_cast<const IntVal*>(valp1),
                            static_cast<const IntVal*>(valp2));
    case ValKind::Dbl:
        return DblVal::same(static_cast<const DblVal*>(valp1),
                            static_cast<const DblVal*>(valp2));
    case ValKind::Str:
        return StrVal::same(static_cast<const StrVal*>(valp1),
                            static_cast<const StrVal*>(valp2));
    case ValKind::Tuple:
        return TupleVal::same(static_cast<const TupleVal*>(valp1),
                              static_cast<const TupleVal*>(valp2));
    case ValKind::Set:
        return SetVal::same(static_cast<const SetVal*>(valp1),
                            static_cast<const SetVal*>(valp2));
    case ValKind::Item:
        return ItemVal::same(static_cast<const ItemVal*>(valp1),
                             static_cast<const ItemVal*>(valp2));
    }
    throw std::runtime_error("unexpected kind");
}

bool ValuePtr::less(const ValuePtr vp1, const ValuePtr vp2)
{
    const Value*valp1 = vp1.get();
    const Value*valp2 = vp2.get();
    if (valp1 == valp2) return false;
    if (!valp1) return true;
    if (!valp2) return false;
    auto k1 = valp1->kind();
    auto k2 = valp2->kind();
    if (k1 < k2) return true;
    if (k1 > k2) return false;
    switch (k1) {
    case ValKind::Nil:
        abort();
    case ValKind::Int:
        return IntVal::less(static_cast<const IntVal*>(valp1),
                            static_cast<const IntVal*>(valp2));
    case ValKind::Dbl:
        return DblVal::less(static_cast<const DblVal*>(valp1),
                            static_cast<const DblVal*>(valp2));
    case ValKind::Str:
        return StrVal::less(static_cast<const StrVal*>(valp1),
                            static_cast<const StrVal*>(valp2));
    case ValKind::Tuple:
        return TupleVal::less(static_cast<const TupleVal*>(valp1),
                              static_cast<const TupleVal*>(valp2));
    case ValKind::Set:
        return SetVal::less(static_cast<const SetVal*>(valp1),
                            static_cast<const SetVal*>(valp2));
    case ValKind::Item:
        return ItemVal::less(static_cast<const ItemVal*>(valp1),
                             static_cast<const ItemVal*>(valp2));
    }
    throw std::runtime_error("unexpected kind");
}

const TupleVal*
TupleVal::make_it(std::vector<ItemVal*>vecptr)
{
    assert (std::all_of(vecptr.begin(),vecptr.end(),
    [](ItemVal*vptr) {
        return vptr!=nullptr;
    }));
    return new TupleVal(vecptr.data(),vecptr.size());
}

const TupleVal*
TupleVal::make_it(ItemVal**arr, unsigned siz)
{
    assert (std::all_of(arr,arr+siz,
    [](ItemVal*vptr) {
        return vptr!=nullptr;
    }));
    return new TupleVal(arr,siz);
}

void
TupleVal::add(std::vector<ItemVal*>&vec, ValuePtr val)
{
    switch (val.kind()) {
    case ValKind::Nil:
    case ValKind::Int:
    case ValKind::Dbl:
    case ValKind::Str:
        return;
    case ValKind::Tuple:
    case ValKind::Set:
    {
        auto seq = reinterpret_cast<const SeqItemsVal*>(val.get());
        unsigned sz = seq->size();
        for (unsigned ix=0; ix<sz; ix++) vec.push_back(seq->unsafe_at(ix));
    }
    return;
    case ValKind::Item:
        auto itm = reinterpret_cast<ItemVal*>(val.get());
        vec.push_back(itm);
        return;
    }
}



void TupleVal::add(std::vector<ItemVal*>&vec, ItemPtr val)
{
    ItemVal*ptr = val.get();
    if (ptr)
        vec.push_back(ptr);
}

const TupleVal*
TupleVal::make(std::initializer_list<ItemPtr>il)
{
    std::vector<ItemVal*> vec;
    vec.reserve(il.size());
    for (ItemPtr itp : il) {
        add(vec,itp);
    };
    return make_it(vec);
}

const TupleVal*
TupleVal::make(std::initializer_list<ValuePtr>il)
{
    std::vector<ItemVal*> vec;
    vec.reserve(il.size());
    for (ValuePtr vp : il) {
        add(vec,vp);
    };
    return make_it(vec);
}

const TupleVal*
TupleVal::make(const std::vector<ItemPtr>&ivec)
{
    std::vector<ItemVal*> vec;
    vec.reserve(ivec.size());
    for (ItemPtr itp : ivec) {
        add(vec,itp);
    };
    return make_it(vec);
}

const TupleVal*
TupleVal::make(const std::vector<ValuePtr>&ivec)
{
    std::vector<ItemVal*> vec;
    vec.reserve(ivec.size());
    for (ValuePtr vp : ivec) {
        add(vec,vp);
    };
    return make_it(vec);
}

const TupleVal*
TupleVal::make(const std::list<ItemPtr>&ilis)
{
    std::vector<ItemVal*> vec;
    vec.reserve(ilis.size());
    for (ItemPtr itp : ilis) {
        add(vec,itp);
    };
    return make_it(vec);
}

const TupleVal*
TupleVal::make(const std::list<ValuePtr>&ilis)
{
    std::vector<ItemVal*> vec;
    vec.reserve(ilis.size());
    for (ValuePtr vp : ilis) {
        add(vec,vp);
    };
    return make_it(vec);
}

Json::Value TupleVal::to_json(void) const {
    Json::Value j {Json::objectValue};
    Json::Value t {Json::arrayValue};
    for (unsigned ix=0; ix<_slen; ix++)
        t.append(_sarr[ix]->to_json());
    j["kind"] = "tuple";
    j["comp"] = t;
    return j;
}


Json::Value SetVal::to_json(void) const {
    Json::Value j {Json::objectValue};
    Json::Value t {Json::arrayValue};
    for (unsigned ix=0; ix<_slen; ix++)
        t.append(_sarr[ix]->to_json());
    j["kind"] = "set";
    j["elem"] = t;
    return j;
}
