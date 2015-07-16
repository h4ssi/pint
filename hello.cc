
#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <locale>

// expr = list | number | symb
// list = ( expr * )

class Expr {

};

template<typename T>
class ValueHolder {
    public:
        ValueHolder(T o) : v(o) {}
        T const & value() const { return value; }
    private:
        T v;
};

class Number : public Expr, public ValueHolder<double> {
    using ValueHolder::ValueHolder;
};

class Symbol : public Expr, public ValueHolder<std::string> {
    using ValueHolder::ValueHolder;
};

class List : public Expr {
    public:
        List(std::list<std::unique_ptr<Expr>> exprs): exprs_(std::move(exprs)) {}

    private:
        std::list<std::unique_ptr<Expr>> exprs_;
};

class ParseResult {
    public:
        ParseResult(std::unique_ptr<Expr> expr, char const *pos) : expr_(std::move(expr)), pos_(pos) {}
        char const *pos() const { return pos_; }
        bool parsed() const { return expr_ != nullptr; }
        std::unique_ptr<Expr> grab() { return std::move(expr_); }
    private:
        std::unique_ptr<Expr> expr_;
        char const *pos_;
};

ParseResult parse_expr (char const * lo, char const * hi);

ParseResult parse_symbol (char const * lo, char const * hi) {
    auto & h = std::use_facet<std::ctype<char>>(std::locale::classic());

    char const *f = h.scan_not( std::ctype<char>::alnum, lo, hi);

    std::string s(lo, f - lo);

    std::cout << s << std::endl;

    return ParseResult(std::make_unique<Symbol>(s), f);
}

ParseResult parse_number(char const * lo, char const * hi) {
    auto & h = std::use_facet<std::ctype<char>>(std::locale::classic());

    char const * f = h.scan_not( std::ctype<char>::digit, lo, hi );

    if(f != hi && *f == '.') {
        f = h.scan_not( std::ctype<char>::digit, f + 1, hi );
    }

    if(! h.is( std::ctype<char>::digit, *(f-1) )) {
        return ParseResult(nullptr, lo);
    }

    std::ios::iostate state;
    std::string snum(lo, f - lo);
    double num = std::stod(snum);

    std::cout << num << std::endl;

    return ParseResult(std::make_unique<Number>(num), f);
}

ParseResult parse_list(char const * lo, char const * hi) {
    std::list<std::unique_ptr<Expr>> l;

    std::cout << "(" << std::endl;

    char const *c = lo + 1;
    while(true) {
        ParseResult r = parse_expr(c, hi);


        if(r.parsed()) {
            std::cout << "." << std::endl;

            c = r.pos();
            l.emplace_back(r.grab());

            continue;
        } 
        
        if( r.pos() == hi || *r.pos() != ')') {
            return ParseResult(nullptr, r.pos());
        }
        
        std::cout << ")" << std::endl;

        return ParseResult(std::make_unique<List>(std::move(l)), r.pos() + 1);
    }
}

ParseResult parse_expr (char const * lo, char const * hi) {
    auto & h = std::use_facet<std::ctype<char>>(std::locale::classic());
    char const *f = h.scan_not( std::ctype<char>::space, lo, hi);
    if( f == hi ) {
        return ParseResult(nullptr, hi);
    } else if(*f == '(') {
        return parse_list(f, hi);
    } else if( h.is( std::ctype<char>::digit, *f ) ) {
        return parse_number(f, hi);
    } else if( h.is( std::ctype<char>::alpha, *f ) ) {
        return parse_symbol(f, hi);
    } else {
        return ParseResult(nullptr, f);
    }
}

#include <unordered_map>

std::unordered_map<std::string, std::function<double(std::list<double>)>> fs;

void setup() {
    fs["print"] = [](auto l) {
        for(auto v : l) {
            std::cout << v;
        }
        return 0;
    };
}

int main (int, char**) {
    setup();
    std::cout << "hello world!" << std::endl;
    std::string x = "(print (plus 1 1))";
    char const * c = x.c_str();
    parse_expr(c, c + x.size());
    fs["print"](std::list<double>{1337});
    return 0;
}
