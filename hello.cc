
#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <locale>

// expr = list | number | symb
// list = ( expr * )

class Expr {
public:
  virtual ~Expr() = default;
};

template <typename T> class ValueHolder {
public:
  ValueHolder(T const &o) : v(o) {}
  ValueHolder(T &&o) : v(std::move(o)) {}
  T const &value() const { return v; }

private:
  T v;
};

class Number : public Expr, public ValueHolder<double> {
  using ValueHolder::ValueHolder;
};

class Symbol : public Expr, public ValueHolder<std::string> {
  using ValueHolder::ValueHolder;
};

class List : public Expr, public ValueHolder<std::list<std::unique_ptr<Expr>>> {
  using ValueHolder::ValueHolder;
};

class ParseResult {
public:
  ParseResult(std::unique_ptr<Expr> expr, char const *pos)
      : expr_(std::move(expr)), pos_(pos) {}
  char const *pos() const { return pos_; }
  bool parsed() const { return expr_ != nullptr; }
  std::unique_ptr<Expr> &result() { return expr_; }

private:
  std::unique_ptr<Expr> expr_;
  char const *pos_;
};

ParseResult parse_expr(char const *lo, char const *hi);

ParseResult parse_symbol(char const *lo, char const *hi) {
  auto &h = std::use_facet<std::ctype<char>>(std::locale::classic());

  char const *f = h.scan_not(std::ctype<char>::alnum, lo, hi);

  std::string s(lo, f - lo);

  std::cout << s << std::endl;

  return ParseResult(std::make_unique<Symbol>(s), f);
}

ParseResult parse_number(char const *lo, char const *hi) {
  auto &h = std::use_facet<std::ctype<char>>(std::locale::classic());

  char const *f = h.scan_not(std::ctype<char>::digit, lo, hi);

  if (f != hi && *f == '.') {
    f = h.scan_not(std::ctype<char>::digit, f + 1, hi);
  }

  if (!h.is(std::ctype<char>::digit, *(f - 1))) {
    return ParseResult(nullptr, lo);
  }

  std::ios::iostate state;
  std::string snum(lo, f - lo);
  double num = std::stod(snum);

  std::cout << num << std::endl;

  return ParseResult(std::make_unique<Number>(num), f);
}

ParseResult parse_list(char const *lo, char const *hi) {
  std::list<std::unique_ptr<Expr>> l;

  std::cout << "(" << std::endl;

  char const *c = lo + 1;
  while (true) {
    ParseResult r = parse_expr(c, hi);

    if (r.parsed()) {
      std::cout << "." << std::endl;

      c = r.pos();
      l.emplace_back(std::move(r.result()));

      continue;
    }

    if (r.pos() == hi || *r.pos() != ')') {
      return ParseResult(nullptr, r.pos());
    }

    std::cout << ")" << std::endl;

    return ParseResult(std::make_unique<List>(std::move(l)), r.pos() + 1);
  }
}

ParseResult parse_expr(char const *lo, char const *hi) {
  auto &h = std::use_facet<std::ctype<char>>(std::locale::classic());
  char const *f = h.scan_not(std::ctype<char>::space, lo, hi);
  if (f == hi) {
    return ParseResult(nullptr, hi);
  } else if (*f == '(') {
    return parse_list(f, hi);
  } else if (h.is(std::ctype<char>::digit, *f)) {
    return parse_number(f, hi);
  } else if (h.is(std::ctype<char>::alpha, *f)) {
    return parse_symbol(f, hi);
  } else {
    return ParseResult(nullptr, f);
  }
}

#include <unordered_map>

class Value {
public:
  virtual ~Value() = default;
};

// value = double | function
// function = arg_binding + var_context + logic

// simple mode create function objects during eval
using FunctionType = std::function<
    std::unique_ptr<Value>(std::list<std::unique_ptr<Value>> const &)>;
class Function : public Value, public ValueHolder<FunctionType> {
  using ValueHolder::ValueHolder;
};

class Double : public Value, public ValueHolder<double> {
  using ValueHolder::ValueHolder;
};

std::unordered_map<std::string, std::shared_ptr<Value>> root;

void setup() {
  root["print"] = std::make_shared<Function>([](auto const &l) {
    for (auto const &v : l) {
      if (Double *d = dynamic_cast<Double *>(v.get())) {
        std::cout << d->value();
      } else {
        std::cout << "[Function]";
      }
    }
    return std::make_unique<Double>(0);
  });
  root["plus"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Double *d = dynamic_cast<Double *>(v.get())) {
        s += d->value();
      }
    }
    return std::make_unique<Double>(s);
  });
}

std::unique_ptr<Value> eval(Expr *e) {
  if (auto n = dynamic_cast<Number *>(e)) {
    return std::make_unique<Double>(n->value());
  } else if (auto l = dynamic_cast<List *>(e)) {
    if (l->value().empty()) {
      return std::make_unique<Double>(0); // empty form
    }
    if (auto s = dynamic_cast<Symbol *>(l->value().front().get())) {
      if (root.count(s->value())) {
        if (Function *f = dynamic_cast<Function *>(root[s->value()].get())) {
          std::list<std::unique_ptr<Value>> args;
          bool first = true;
          for (auto const &v : l->value()) {
            if (first) {
              first = false;
              continue;
            }
            args.emplace_back(eval(v.get()));
          }
          return (f->value())(args);
        }
      }
    }
  }
  return 0;
}

int main(int, char **) {
  setup();
  std::cout << "hello world!" << std::endl;
  std::string x = "(print 1 3 (plus 1 1 1) 7)";
  char const *c = x.c_str();
  ParseResult pr = parse_expr(c, c + x.size());
  if (pr.parsed()) {
    eval(pr.result().get());
  }
  return 0;
}
