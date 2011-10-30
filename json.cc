// Copyright (c) 2007 Simon Kroeger (simonkroeger@gmx.de)
// see json.h for license details

#include "json.h"

#include <sstream>
#include <iomanip>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/utility/lists.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>

namespace tbd {
namespace json_details {

//////////////////////////// Parser ////////////////////////////

using namespace boost::spirit;

struct json_stack {
  json_stack(json& root): root_(&root) {}

  void add_to_name(const char c) { name_ += c; }
  void add_to_str(const char c) { str_ += c; }
  void push(const json& value) { stack_.push_back(add(value)); }
  void pop() { stack_.pop_back();  }

  void add_str() { 
    add(str_.erase(str_.size() - 1)); 
    str_.erase();
  }

  json* add(const json& value) {
    if (stack_.empty()) return &(*root_ = value);

    if (stack_.back()->type() == ary_type) {
      return &stack_.back()->add(value);
    } else {
      name_.erase(name_.size() - 1);
      json* p = &stack_.back()->add(name_, value);
      name_.erase();
      return p;
    }
  }
private:
  friend struct json_parser;
  json_stack(): root_(0) {}

  json* root_;
  std::string name_;
  std::string str_;
  std::vector<json*> stack_;
};

struct json_grammar : public grammar<json_grammar> {

  json_grammar(json_stack& stack): stack_(&stack) {}

  template<typename ScannerT> struct definition {
    typedef typename ScannerT::iterator_t iterator;

    definition(const json_grammar& self) {
    typedef boost::function<void(iterator, iterator)> str_action;
    typedef boost::function<void(char)              > char_action;
    typedef boost::function<void(const json&)       > value_action;

    value_action add(boost::bind(&json_stack::add, self.stack_, _1));
    value_action push(boost::bind(&json_stack::push, self.stack_, _1));
    char_action pop(boost::bind(&json_stack::pop, self.stack_));
    char_action add_to_name(boost::bind(&json_stack::add_to_name, self.stack_, _1));
    char_action add_to_str(boost::bind(&json_stack::add_to_str, self.stack_, _1));
    str_action add_str(boost::bind(&json_stack::add_str, self.stack_));

    json_ = (object_ | array_) >> end_p;

    object_ 
        = confix_p
          (
            ch_p('{')[ boost::bind(push, json_obj()) ], 
            !list_p(pair_, ','),
            ch_p('}')[ pop ] 
         )
        ;

    array_
        = confix_p
          (
            ch_p('[')[ boost::bind(push, json_ary()) ],
            !list_p(value_, ','),
            ch_p(']') [ pop ]
          )
        ;

    pair_ = name_ >> ':' >> value_;

    value_
        = object_ 
        | array_ 
        | strict_real_p [ add ] 
        | int_p         [ add ]
        | string_       [ add_str ]
        | str_p("true") [ boost::bind(add, true) ] 
        | str_p("false")[ boost::bind(add, false) ] 
        | str_p("null") [ boost::bind(add, json::null) ]
        ;
  
    string_
        = lexeme_d
          [
              confix_p('"', *(lex_escape_ch_p[ add_to_str ]), '"') 
          ]
        ;

    name_
        = lexeme_d
          [
              confix_p('"', *(lex_escape_ch_p[ add_to_name ]), '"') 
          ]
        ;
    }

    rule< ScannerT > json_, object_, pair_, array_, 
      value_, string_, name_, number_;

    const rule< ScannerT >& start() const { return json_; }
  };

private:
    json_stack* stack_;
};

struct json_parser {
  json_parser(): stack_(), grammar_(stack_) {}

  bool read(const std::string& s, json& value) {
    stack_.root_ = &value;
    return parse(s.c_str(), grammar_, space_p).full;
  }

  bool read(std::istream& is, json& value) {
    is.unsetf(std::ios::skipws);
    stack_.root_ = &value;

    typedef multi_pass< std::istream_iterator<char> > iterator_t;
    iterator_t begin(make_multi_pass(std::istream_iterator<char>(is)));
    iterator_t end(make_multi_pass(std::istream_iterator<char>()));

    return parse(begin, end, grammar_, space_p).hit;
  }
private:
  json_parser(const json_parser&);
  json_stack stack_;
  json_grammar grammar_;
};

//////////////////////////// Writer ////////////////////////////

struct json_writer {
  json_writer(std::ostream& os, bool pretty = false): os_(os), indent_(), pretty_(pretty) {
    os_ << std::setfill('0') << std::showpoint;
    os_.precision(12);
  }

  void operator() (const json_obj& obj) {
    os_ << '{';
    output_container(obj);
    os_ << '}';
  }

  void operator() (const json_ary& arr) {
    os_ << '[';
    output_container(arr);
    os_ << ']';
  }

  void operator() (const null_t&) { os_ << "null"; }
  void operator() (const std::string& s) { output(s); }
  void operator() (bool b) { os_ << (b ? "true" : "false"); }
  void operator() (int i) { os_ << std::dec << i; }
  void operator() (double d) { os_ << d; }
private:
  void output(const json& value) {
    value.apply(*this);
  }

  void output(const json_obj::value_type& pair) {
    output(pair.first);
    os_ << (pretty_ ? " : " : ":"); 
    output(pair.second);
  }

  void output(const std::string& s) {
    const std::string::size_type len(s.length());

    os_ << '"';
    for(std::string::size_type i = 0; i < len; ++i) {
      switch(s[i]) {
        case '"' : os_ << "\\\"";break;
        case '/' : os_ << "\\/"; break;
        case '\\': os_ << "\\\\";break;
        case '\b': os_ << "\\b"; break;
        case '\f': os_ << "\\f"; break;
        case '\n': os_ << "\\n"; break;
        case '\r': os_ << "\\r"; break;
        case '\t': os_ << "\\t"; break;
        default: {
          unsigned char c = s[i];
          if (!std::isprint(c)) {
            os_ << "\\x" << std::setw(2) << std::hex << (int)c;
          } else os_ << c;
        }
      }
    }
    os_ << '"';
  }

  template<class T> void output_container(const T& t) {
    if (pretty_) indent_ += "  ";
        
    for (typename T::const_iterator i = t.begin(); i != t.end();) {
      if (pretty_) os_ << std::endl << indent_; 
      output(*i);
      if(++i != t.end()) os_ << ',';
    }

    if (pretty_) {
      indent_.resize(indent_.size() - 2);
      os_ << std::endl << indent_; 
    }
  }

  std::ostream& os_;
  std::string indent_;
  bool pretty_;
};

//////////////////////////// json ////////////////////////////

json_exception::json_exception(const char* what): std::exception(what) {};

const json json::null = json();

json::json(): type_(null_type) {}
json::json(const json&        value): type_(null_type){ *this = value; }
json::json(const char*        value): type_(str_type) { value_.s = new std::string(value); }
json::json(const std::string& value): type_(str_type) { value_.s = new std::string(value); }
json::json(const json_obj&    value): type_(obj_type) { value_.o = new json_obj(value); }
json::json(const json_ary&    value): type_(ary_type) { value_.a = new json_ary(value); }
json::json(bool               value): type_(bool_type){ value_.b = value; }
json::json(int                value): type_(int_type) { value_.i = value; }
json::json(double             value): type_(real_type){ value_.r = value; }

json::~json() { destroy(); }

void json::destroy() {
  switch(type()) {
    case obj_type: delete value_.o; break;
    case ary_type: delete value_.a; break;
    case str_type: delete value_.s; break;
  }
  type_ = null_type;
}

json& json::operator= (const json& other) {
  destroy();

  switch(other.type()) {
    case obj_type : value_.o = new json_obj(other.as_obj()); break;
    case ary_type : value_.a = new json_ary(other.as_ary()); break;
    case str_type : value_.s = new std::string(other.as_str()); break;
    case bool_type: value_.b = other.as_bool(); break;
    case int_type : value_.i = other.as_int(); break;
    case real_type: value_.r = other.as_real(); break;
    case null_type: break;
    default: assert(false);
  }
  type_ = other.type();

  return *this;
}

bool json::operator== (const json& other) const {
  if (this == &other) return true;
  if (type() != other.type()) return false;

  switch (type()) {
    case str_type:  return as_str()  == other.as_str();
    case obj_type:  return as_obj()  == other.as_obj();
    case ary_type:  return as_ary()  == other.as_ary();
    case bool_type: return as_bool() == other.as_bool();
    case int_type:  return as_int()  == other.as_int();
    case real_type: return fabs(as_real() - other.as_real()) < 1E-10;
    case null_type: return true;
  default: return false;
  };
}

inline json_value_type json::type() const { return type_; }

inline bool json::is_obj() const { return type() == obj_type; }
inline bool json::is_ary() const { return type() == ary_type; }
inline bool json::is_str() const { return type() == str_type; }
inline bool json::is_bool() const { return type() == bool_type; }
inline bool json::is_int() const { return type() == int_type; }
inline bool json::is_real() const { return type() == real_type; }
inline bool json::is_null() const { return type() == null_type; }

inline const json_obj& json::as_obj() const {
  if (!is_obj()) throw json_exception("not an object");
  return *value_.o;
}
inline json_obj& json::as_obj() {
  if (!is_obj()) throw json_exception("not an object");
  return *value_.o;
}

inline const json_ary& json::as_ary() const {
  if (!is_ary()) throw json_exception("not an array");
  return *value_.a;
}
inline json_ary& json::as_ary() {
  if (!is_ary()) throw json_exception("not an array");
  return *value_.a;
}

inline const std::string& json::as_str() const {
  if (!is_str()) throw json_exception("not a string");
  return *value_.s;
}
inline std::string& json::as_str() {
  if (!is_str()) throw json_exception("not a string");
  return *value_.s;
}

inline bool json::as_bool() const {
  if (!is_bool()) throw json_exception("not a bool");
  return value_.b;
}

inline int json::as_int() const {
  if (!is_int()) throw json_exception("not an integer");
  return value_.i;
}

inline double json::as_real() const {
  if (!is_real()) throw json_exception("not a real");
  return value_.r;
}

void json::from_str(const std::string& str) {
  json_parser parser;
  parser.read(str, *this);
}

std::string json::to_str(bool pretty /* = false */) const {
  std::ostringstream os;
  json_writer g(os, pretty);
  this->apply(g);
    return os.str();
}

double json::to_real() const {
  switch(type()) {
    case str_type : return atof(value_.s->c_str()); 
    case int_type : return value_.i;
    case real_type: return value_.r;
    default: return 0.0;
  }
}
bool json::to_bool() const {
  switch(type()) {
    case obj_type : return !value_.o->empty();
    case ary_type : return !value_.a->empty();
    case str_type : return !value_.s->empty();
    case bool_type: return value_.b;
    case int_type : return value_.i != 0;
    case real_type: return value_.r != 0.0;
    default: return false;
  }
}
int json::to_int() const {
  switch(type()) {
    case str_type : return atoi(value_.s->c_str()); 
    case bool_type: return value_.b;
    case int_type : return value_.i;
    case real_type: return (int)value_.r;
    default: return 0;
  }
}

json& json::add(const json& value) {
  if (!is_ary()) throw json_exception("not an array");
  value_.a->push_back(value);
  return value_.a->back();
}

json& json::add(const std::string& name, const json& value) {
  if (!is_obj()) throw json_exception("not an object");
  return (*value_.o)[name] = value;
}

json& json::operator[] (const std::string& key) {
  if (!is_obj()) {
    if (!is_null()) throw json_exception("not an object");
    *this = json_obj();
  }
  return (*value_.o)[key];
}

json& json::operator[] (const unsigned int index) {
  if (!is_ary()) {
    if (!is_null()) throw json_exception("not an array");
    *this = json_ary();
  }
  if (index >= as_ary().size()) as_ary().resize(index + 1);
  return (*value_.a)[index];
}

std::ostream &operator<<(std::ostream &os, const json& value) {
  json_writer g(os);
  value.apply(g);
    return os;
}

std::istream &operator>>(std::istream &is, json& value) {
  json_parser parser;
  parser.read(is, value);
  return is;
}

} // namespace json_details
} // namespace tbd
