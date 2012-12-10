// file iacamain.cc

// © 2012 Basile Starynkevitch
//   this file iacamain.cc is part of IaCa
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
pthread_mutex_t MemorySegment::_mtx_ = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void* MemorySegment::allocate (size_t alsz, bool exec) throw (std::runtime_error) {
    int logalsz = -1;
    Locker lock(_mtx_);
    for (int lg=5; (1L<<lg) < (long)alsz && logalsz<0; lg++)
        if ((1L<<lg)==(long)alsz)
            logalsz= lg;
    assert (logalsz>0);
    int prot = exec?(PROT_READ|PROT_WRITE):(PROT_READ|PROT_WRITE|PROT_EXEC);
    // first, try to mmap exactly alsz, hoping that the result will be
    // suitably aligned
    void* ad = mmap (nullptr, alsz, prot, MAP_PRIVATE, -1, (off_t)0);
    if (ad != MAP_FAILED && ((intptr_t)ad & ((intptr_t)alsz-1)) == 0)
        return ad;
    if (ad != MAP_FAILED)
        munmap (ad, alsz);
    // then mmap twice the space to extract aligned data
    ad = mmap(nullptr, 2*alsz, prot, MAP_PRIVATE, -1, (off_t)0);
    if (ad == MAP_FAILED)
        throw std::runtime_error(std::string("mmap failed:")+strerror(errno));
    char* endad = (char*)ad + 2*alsz;
    intptr_t start = ((intptr_t) ad + (alsz-1)) & (~(unsigned long)(alsz-1));
    if ((intptr_t) start > (intptr_t) ad)
        munmap (ad, (intptr_t)start - (intptr_t)ad);
    if ((intptr_t) start + (intptr_t)alsz < (intptr_t)endad)
        munmap ((char*)start + alsz, (intptr_t)endad - (intptr_t) (start+alsz));
    return (void*)start;
}

void MemorySegment::deallocate(void*ad, size_t sz) {
    if (!ad) return;
    Locker lock(_mtx_);
    munmap (ad, sz);
}

int main(int argc, char**argv)
{

}
