#ifndef OUTPUT_TYPETAG_INFO
#define OUTPUT_TYPETAG_INFO

#include <iostream>
#include <typeinfo>

template<typename TypeTag>
void outputTypeTagInfo() {
    std::cout << typeid(TypeTag).name() << std::endl;
}
#endif

