// file iaca.hh
#ifndef IACA_INCLUDED_
#define IACA_INCLUDED_
// © 2012 Basile Starynkevitch
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
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>
#include <pthread.h>

namespace Iaca {
enum segment_kind {
    seg_none,
    seg_small,
    seg_big
};

class Locker {
    pthread_mutex_t* _mtx;
public:
    Locker (pthread_mutex_t&mtx) : _mtx(&mtx) {
        pthread_mutex_lock(&mtx);
    };
    ~Locker () {
        pthread_mutex_unlock(_mtx);
        _mtx=NULL;
    };
    void* operator new (size_t) = delete;
    void operator delete (void*) = delete;
};				// end class Locker

class MemorySegment {
    static const unsigned magic_number = 1070657425; // 0x3fd0ef91
    static pthread_mutex_t _mtx_;
    unsigned _magic;
    enum segment_kind _kind;
    void* _end;
    void* _client0;
    void* _client1;
    intptr_t _data[];
protected:
    static void* allocate(size_t alsz, bool exec=false) throw (std::runtime_error);
    static void deallocate(void*ptr, size_t sz);
    MemorySegment (enum segment_kind kind, void* end)
        : _magic(magic_number),
          _end(end),
          _client0(0),
          _client1(0)
    {
    };
    ~MemorySegment () {
        assert (_magic == magic_number);
        _magic = 0;
    };
protected:
public:
    static MemorySegment* containing(const void*);
    bool contains (const void*p) const {
        return this && p>=_data && p<_end;
    };
};				// end class MemorySegment;

class SmallMemorySegment : public MemorySegment {
    friend class MemorySegment;
    static std::map<unsigned long, SmallMemorySegment*> _map_;
public:
    static const unsigned long size = 512*1024*sizeof(void*);
    static SmallMemorySegment* make(void);
};				// end class SmallMemorySegment

class LargeMemorySegment  : public MemorySegment {
    friend class MemorySegment;
    static std::map<unsigned long, LargeMemorySegment*> _map_;
public:
    static const unsigned long size = 8*SmallMemorySegment::size;
    static LargeMemorySegment* make(void);
};				// end class LargeMemorySegment

};				// end of namespace Iaca
#endif
