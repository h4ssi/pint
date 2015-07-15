
#include <iostream>
#include <string>
#include <list>

// expr = list | number | symb
// list = ( expr * )

class Expr {

};

template<typename T>
class ValueHolder {
    public:
        ValueHolder(T o) : v(o) {}
        T const & value();
    private:
        T v;
};

class Number : public Expr, public ValueHolder<double> {

};

class Symbol : public Expr, public ValueHolder<std::string> {

};

class List : public Expr, public ValueHolder<std::list<Expr>> {

};

int main (int, char**) {
    std::cout << "hello world!" << std::endl;
    return 0;
}
