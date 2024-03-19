#include <iostream>
#include <typeinfo>

template<typename TypeTag>
void outputTypeTagInfo() {
    std::cout << typeid(TypeTag).name() << std::endl;
}

