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

// value = programmatic value
//       = expr (everything that is code)
//       | function (concrete function value)

class Value {
public:
  virtual ~Value() = default;
};

// expr = list | number | text | symbol
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

class Text : public Expr, public ValueHolder<std::string> {
  using ValueHolder::ValueHolder;
};

class Symbol : public Expr, public ValueHolder<std::string> {
  using ValueHolder::ValueHolder;
};

class List : public Expr,
             public ValueHolder<std::list<std::shared_ptr<Value>>> {
  using ValueHolder::ValueHolder;
};

// todo: shared ptr correct choice?
class ParseResult {
public:
  ParseResult(std::shared_ptr<Expr> expr, std::size_t pos)
      : expr_(expr), pos_(pos) {}
  std::size_t pos() const { return pos_; }
  bool parsed() const { return expr_ != nullptr; }
  std::shared_ptr<Expr> result() const { return expr_; }

private:
  std::shared_ptr<Expr> expr_;
  std::size_t pos_;
};

std::string const whitespace = " \n\r\t";
std::string const digit = "1234567890";

ParseResult parse_expr(std::string const &s, std::size_t pos);

std::size_t len(std::size_t start, std::size_t end) {
  if (end == std::string::npos) {
    return end;
  }
  return end - start;
}

ParseResult parse_symbol(std::string const &s, std::size_t pos) {
  auto f = s.find_first_of(whitespace + "()'", pos);

  std::string sym(s, pos, len(pos, f));

  std::cout << sym << std::endl;

  return ParseResult(sym.length() ? std::make_shared<Symbol>(sym) : nullptr, f);
}

ParseResult parse_string(std::string const &s, std::size_t pos) {
  ++pos;
  std::string ret;
  while (true) {
    auto f = s.find_first_of("'\\", pos);

    ret += std::string(s, pos, len(pos, f));

    if (s[f] == '\\') {
      if (++f < s.size()) { // copy next char as is
        ret += s[f];
      }
      pos = ++f; // skip behind escape
      continue;
    }

    if (s[f] == '\'') {
      std::cout << "'" << ret << "'" << std::endl;
      return ParseResult(std::make_shared<Text>(ret), ++f);
    }

    return ParseResult(nullptr, pos);
  }
}

ParseResult parse_number(std::string const &s, std::size_t pos) {
  auto f = s.find_first_not_of(digit, pos);

  if (f != std::string::npos && s[f] == '.') {
    f = s.find_first_not_of(digit, f + 1);
  }

  if (digit.find(s[f - 1]) == std::string::npos) {
    return ParseResult(nullptr, pos);
  }

  double num = std::stod(std::string(s, pos, len(pos, f)));

  std::cout << num << std::endl;

  return ParseResult(std::make_shared<Number>(num), f);
}

ParseResult parse_list(std::string const &s, std::size_t pos) {
  std::list<std::shared_ptr<Value>> l;

  std::cout << "(" << std::endl;

  ++pos;
  while (true) {
    ParseResult r = parse_expr(s, pos);

    if (r.parsed()) {
      std::cout << "." << std::endl;

      pos = r.pos();
      l.emplace_back(r.result());

      continue;
    }

    if (r.pos() == std::string::npos || s[r.pos()] != ')') {
      return ParseResult(nullptr, r.pos());
    }

    std::cout << ")" << std::endl;

    return ParseResult(std::make_shared<List>(l), r.pos() + 1);
  }
}

ParseResult parse_expr(std::string const &s, std::size_t pos) {
  auto f = s.find_first_not_of(whitespace, pos);
  if (f == std::string::npos) {
    return ParseResult(nullptr, pos);
  } else if (s[f] == '(') {
    return parse_list(s, f);
  } else if (s[f] == '\'') {
    return parse_string(s, f);
  } else if (digit.find(s[f]) != std::string::npos) {
    return parse_number(s, f);
  } else {
    return parse_symbol(s, f);
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

void print(Value const *value) {
  if (auto d = dynamic_cast<Number const *>(value)) {
    std::cout << d->value();
  } else if (auto t = dynamic_cast<Text const *>(value)) {
    std::cout << t->value();
  } else if (auto s = dynamic_cast<Symbol const *>(value)) {
    std::cout << "[Symbol " << s->value() << "]";
  } else if (auto li = dynamic_cast<List const *>(value)) {
    std::cout << "(";
    std::string sep = "";
    for (auto const &vv : li->value()) {
      std::cout << sep;
      sep = " ";
      print(vv.get());
    }
    std::cout << ")";
  } else if (dynamic_cast<Function const *>(value)) {
    std::cout << "[Function]";
  } else {
    std::cout << "[nil]";
  }
}

void setup() {
  root["print"] = std::make_shared<Function>([](auto const &l) {
    for (auto const &v : l) {
      print(v.get());
    }
    return std::make_shared<Number>(0);
  });
  root["symbol-name"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto sym = dynamic_cast<Symbol *>(v.get())) {
            return std::make_shared<Text>(sym->value());
          }
          break;
        }
        return nullptr;
      });
  root["symbol"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto tex = dynamic_cast<Text *>(v.get())) {
            return std::make_shared<Symbol>(tex->value());
          }
          break;
        }
        return nullptr;
      });
  root["cons"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        auto i = std::begin(l);
        auto e = std::end(l);

        if (i == e) {
          return nullptr;
        }

        auto head = *i;

        if (++i == e || *i == nullptr) {
          std::list<std::shared_ptr<Value>> nl;
          nl.emplace_front(head);
          return std::make_shared<List>(nl);
        }

        if (auto ol = dynamic_cast<List *>(i->get())) {
          std::list<std::shared_ptr<Value>> nl(ol->value());
          nl.emplace_front(head);
          return std::make_shared<List>(nl);
        } else {
          return nullptr;
        }
      });
  root["head"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto ll = dynamic_cast<List *>(v.get())) {
            if (ll->value().size()) {
              return ll->value().front();
            }
          }
          break;
        }
        return nullptr;
      });
  root["+"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s += d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["-"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s -= d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["="] = std::make_shared<Function>([](auto const &l) {
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

std::list<std::shared_ptr<Value>> body_list(List const *f) {
  std::list<std::shared_ptr<Value>> ret;

  int i = 0;
  for (auto const &e : f->value()) {
    if (i >= 2) {
      ret.emplace_back(e);
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

std::shared_ptr<Value> eval(Memory &m, std::shared_ptr<Value> e);

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

std::shared_ptr<Value> eval(Memory &m, std::shared_ptr<Value> e) {
  if (auto var = dynamic_cast<Symbol *>(e.get())) {
    return m[var->value()];
  } else if (auto l = dynamic_cast<List *>(e.get())) {
    if (l->value().empty()) {
      return std::make_shared<Number>(0); // empty form
    }
    auto const &list = l->value();
    auto i = std::begin(list);
    auto end = std::end(list);
    if (auto s = dynamic_cast<Symbol *>(i->get())) {
      if ("do" == s->value()) {
        std::shared_ptr<Value> r;
        while (++i != end) {
          r = eval(m, *i);
        }
        return r;
      }
      if ("def" == s->value()) {
        if (++i != end) {
          if (auto t = dynamic_cast<Symbol *>(i->get())) {
            if (++i != end) {
              return (m[t->value()] = eval(m, *i));
            }
          }
        }
        return std::make_shared<Number>(0);
      }
      if ("fn" == s->value()) {
        return std::make_shared<Function>(eval_to_f(m, l));
      }
      if ("if" == s->value()) {
        if (++i != end) {
          auto cond = eval(m, *i);
          if (++i != end) {
            if (cond != nullptr) {
              auto num = dynamic_cast<Number *>(cond.get());
              if (num->value() != 0) {
                return eval(m, *i);
              } else {
                if (++i != end) {
                  return eval(m, *i);
                }
              }
            }
          }
        }
        return std::make_shared<Number>(0);
      }
      if ("quote" == s->value()) {
        if (++i != end) {
          return *i;
        }
        return std::make_shared<Number>(0);
      }
    }
    auto head = eval(m, *i);
    if (Function *f = dynamic_cast<Function *>(head.get())) {
      std::list<std::shared_ptr<Value>> args;
      while (++i != end) {
        args.emplace_back(eval(m, *i));
      }
      return (f->value())(args);
    }
    return nullptr;
  } else {
    return e;
  }
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
  ParseResult pr = parse_expr(x, 0);
  std::list<std::shared_ptr<Expr>> program;
  while (pr.parsed()) {
    program.emplace_back(pr.result());
    pr = parse_expr(x, pr.pos());
  }
  for (auto const &e : program) {
    eval(root, e);
  }
  return 0;
}
