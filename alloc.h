#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <iostream>

namespace TD {

template <class Tp>
class Alloc {
public:
    typedef Tp value_type;

    Alloc() = default;
    template <class T> Alloc(const Alloc<T>&) {}

    value_type* allocate(std::size_t n) {
        n *= sizeof(value_type);
        std::cout<<"Allocating "<<n<<" bytes"<<std::endl;
        return static_cast<value_type*>(::operator new(n));
    }

    void deallocate(value_type* p, std::size_t n) {
        std::cout<<"Deallocating "<<n*sizeof(value_type)<<" bytes"<<std::endl;
        ::operator delete(p);
    }
};

template <class T, class U>
bool operator==(const Alloc<T>&, const Alloc<U>&) { return true; }

template <class T, class U>
bool operator!=(const Alloc<T>&, const Alloc<U>&) { return false; }

}

#endif // _ALLOC_H_
