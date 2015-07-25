/* Copyright (C) 2015  Florian Hassanen
 *
 * This file is part of pint.
 *
 * pint is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pint is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <locale>

// value = programmatic value
//       = expr (everything that is code)
//       | function (concrete function value)

class Value {
public:
  virtual ~Value() = default;
};

// expr = list | number | symbol
// list = ( expr * ) // todo should be "value *"

class Expr : public Value {};

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

// function = arg_binding + var_context + logic

// simple mode create function objects during eval
using FunctionType = std::function<std::shared_ptr<Value>(
    std::list<std::shared_ptr<Value>> const &)>;
class Function : public Value, public ValueHolder<FunctionType> {
  using ValueHolder::ValueHolder;
};

using Memory = std::unordered_map<std::string, std::shared_ptr<Value>>;
Memory root;

void setup() {
  root["print"] = std::make_shared<Function>([](auto const &l) {
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        std::cout << d->value();
      } else if (Symbol *s = dynamic_cast<Symbol *>(v.get())) {
        std::cout << "[Symbol " << s->value() << "]";
      } else {
        std::cout << "[Function]";
      }
    }
    return std::make_shared<Number>(0);
  });
  root["plus"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s += d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["minus"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s -= d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["eq"] = std::make_shared<Function>([](auto const &l) {
    bool first = true;
    double p;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        double c = d->value();
        if (!first && p != c) {
          return std::make_shared<Number>(0);
        }
        first = false;
        p = c;
      }
    }
    return std::make_shared<Number>(1);
  });
}

#include <unordered_set>

std::unordered_set<std::string> free_vars(List *l) {
  std::unordered_set<std::string> vars;

  Symbol *s = nullptr;
  List *args = nullptr;
  int i = 0;
  bool is_fn = false;
  for (auto const &e : l->value()) {
    if (i == 0 && (s = dynamic_cast<Symbol *>(e.get())) && "fn" == s->value()) {
      is_fn = true;
    } else if (is_fn && i == 1 && (args = dynamic_cast<List *>(e.get()))) {
    } else if ((s = dynamic_cast<Symbol *>(e.get()))) {
      vars.emplace(s->value());
    } else if (List *il = dynamic_cast<List *>(e.get())) {
      for (auto const &v : free_vars(il)) {
        vars.emplace(v);
      }
    }
    ++i;
  }
  if (args != nullptr) {
    for (auto const &v : free_vars(args)) {
      vars.erase(v);
    }
  }

  return vars;
}

Memory copy_vars(Memory &m, std::unordered_set<std::string> free_vars) {
  Memory ret;

  for (auto const &v : free_vars) {
    ret[v] = m[v];
  }

  return ret;
}

std::list<std::string> arg_list(List const *f) {
  std::list<std::string> ret;

  auto i = std::begin(f->value());

  ++i;

  for (auto const &a : dynamic_cast<List *>(i->get())->value()) {
    if (Symbol *s = dynamic_cast<Symbol *>(a.get())) {
      ret.emplace_back(s->value());
    }
  }

  return ret;
}

std::list<Expr *> body_list(List const *f) {
  std::list<Expr *> ret;

  int i = 0;
  for (auto const &e : f->value()) {
    if (i >= 2) {
      ret.emplace_back(e.get());
    }

    ++i;
  }

  return ret;
}

Memory populate(Memory m, std::list<std::string> args,
                std::list<std::shared_ptr<Value>> const &vals) {
  auto i = std::begin(vals);
  for (auto const &s : args) {
    m[s] = *i++;
  }
  return m;
}

std::shared_ptr<Value> eval(Memory &m, Expr *e);

FunctionType eval_to_f(Memory &pm, List *f) {
  return [
    m(copy_vars(pm, free_vars(f))),
    args(arg_list(f)),
    body(body_list(f))
  ](auto const &arg_vals) {
    auto mm = populate(m, args, arg_vals);

    std::shared_ptr<Value> r = nullptr;

    for (auto const &e : body) {
      r = eval(mm, e);
    }

    return r;
  };
}

std::shared_ptr<Value> eval(Memory &m, Expr *e) {
  if (auto n = dynamic_cast<Number *>(e)) {
    return std::make_shared<Number>(*n);
  } else if (auto var = dynamic_cast<Symbol *>(e)) {
    return m[var->value()];
  } else if (auto l = dynamic_cast<List *>(e)) {
    if (l->value().empty()) {
      return std::make_shared<Number>(0); // empty form
    }
    if (auto s = dynamic_cast<Symbol *>(l->value().front().get())) {
      if ("do" == s->value()) {
        bool first = true;
        std::shared_ptr<Value> r;
        for (auto const &e : l->value()) {
          if (first) {
            first = false;
            continue;
          }
          r = eval(m, e.get());
        }
        return r;
      }
      if ("def" == s->value()) {
        auto i = std::begin(l->value());
        if (i != std::end(l->value())) {
          ++i;
          if (i != std::end(l->value())) {
            if (auto t = dynamic_cast<Symbol *>(i->get())) {
              ++i;
              if (i != std::end(l->value())) {
                return (m[t->value()] = eval(m, i->get()));
              }
            }
          }
        }
        return std::make_shared<Number>(0);
      }
      if ("fn" == s->value()) {
        return std::make_shared<Function>(eval_to_f(m, l));
      }
      if ("if" == s->value()) {
        auto i = std::begin(l->value());
        auto end = std::end(l->value());
        if (++i != end) {
          auto cond = eval(m, i++->get());
          if (i != end) {
            if (cond != nullptr) {
              auto dbl = std::dynamic_pointer_cast<Number>(cond);
              if (dbl->value() != 0) {
                return eval(m, i->get());
              } else {
                if (++i != end) {
                  return eval(m, i->get());
                }
              }
            }
          }
        }
        return std::make_shared<Number>(0);
      }
      if ("quote" == s->value()) {
        auto i = std::begin(l->value());
        if (++i != std::end(l->value())) {
          if (Symbol *sym = dynamic_cast<Symbol *>(i->get())) {
            return std::make_shared<Symbol>(*sym);
          }
        }
        return std::make_shared<Number>(0);
      }
    }
    if (std::shared_ptr<Function> f = std::dynamic_pointer_cast<Function>(
            eval(m, l->value().front().get()))) {
      std::list<std::shared_ptr<Value>> args;
      bool first = true;
      for (auto const &v : l->value()) {
        if (first) {
          first = false;
          continue;
        }
        args.emplace_back(eval(m, v.get()));
      }
      return (f->value())(args);
    }
  }
  return 0;
}

#include <sstream>
#include <fstream>

int main(int argc, char **argv) {
  setup();
  std::cout << "hello world!" << std::endl;

  std::stringstream str;

  if (argc == 2) {
    str << std::ifstream(argv[1]).rdbuf();
  } else {
    str << std::cin.rdbuf();
  }

  std::string x = str.str();
  char const *c = x.c_str();
  char const *e = c + x.size();
  ParseResult pr = parse_expr(c, e);
  std::list<std::unique_ptr<Expr>> program;
  while (pr.parsed()) {
    program.emplace_back(std::move(pr.result()));
    pr = parse_expr(pr.pos(), e);
  }
  for (auto const &e : program) {
    eval(root, e.get());
  }
  return 0;
}
