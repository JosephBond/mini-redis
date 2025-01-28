#include <cstddef>
#include <cstring>
#include <iostream>
#include <utility>

class rule_of_three {
    char* cstring;

    public:
        rule_of_three(const char* s, std::size_t n) : cstring(new char[n+1]) {
            std::memcpy(cstring, s, n);
            cstring[n] = '\0';
        }
    
};
