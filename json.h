#ifndef __json_h__
#define __json_h__

/* The MIT License

Copyright (c) 2007 Simon Kroeger (simonkroeger@gmx.de)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

#include <map>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <cassert>

namespace tbd {
namespace json_details {

enum json_value_type{obj_type, ary_type, str_type, 
  bool_type, int_type, real_type, null_type};

struct json;
typedef std::map<std::string, json> json_obj;
typedef std::vector<json> json_ary;

struct null_t {};

struct json_exception: public std::exception {
  json_exception(const char* what);
};

struct json {
  json();
  json(const json& value);
  json(const char* value);
  json(const std::string& value);
  json(const json_obj& value);
  json(const json_ary& value);
  json(double value);
  json(bool value);
  json(int value);

  ~json();

  json& operator= (const json& other);

  bool operator== (const json& other) const;

  json_value_type type() const;

  bool is_obj() const;
  bool is_ary() const;
  bool is_str() const;
  bool is_bool() const;
  bool is_int() const;
  bool is_real() const;
  bool is_null() const;

  const json_obj& as_obj() const;
  const json_ary& as_ary() const;
  const std::string& as_str() const;
  bool as_bool() const;
  int as_int() const;
  double as_real() const;

  json_obj& as_obj();
  json_ary& as_ary();
  std::string& as_str();

  void from_str(const std::string& str);

  std::string to_str(bool pretty = false) const;
  double to_real() const;
  bool to_bool() const;
  int to_int() const;

  json& operator[] (const std::string& key);
  json& operator[] (const unsigned int index);

  static const json null;

  json& add(const json& value);
  json& add(const std::string& name, const json& value);

  template<class V> void apply(V& visitor) {
    switch(type()) {
      case obj_type : visitor(as_obj()); break;
      case ary_type : visitor(as_ary()); break;
      case str_type : visitor(as_str()); break;
      case bool_type: visitor(as_bool()); break;
      case int_type : visitor(as_int()); break;
      case real_type: visitor(as_real()); break;
      case null_type: visitor(null_t()); break;
      default: assert(false);
    }
  }

  template<class V> void apply(V& visitor) const {
    switch(type()) {
      case obj_type : visitor(as_obj()); break;
      case ary_type : visitor(as_ary()); break;
      case str_type : visitor(as_str()); break;
      case bool_type: visitor(as_bool()); break;
      case int_type : visitor(as_int()); break;
      case real_type: visitor(as_real()); break;
      case null_type: visitor(null_t()); break;
      default: assert(false);
    }
  }

private:
  void destroy();

  union {
    double r;
    int i;
    bool b;
    std::string* s;
    json_obj* o;
    json_ary* a;
  } value_;

  json_value_type type_;
};

std::ostream &operator<<(std::ostream &stream, const json& value);
std::istream &operator>>(std::istream &stream, json& value);

} // namespace json_details

using json_details::json;
using json_details::json_ary;
using json_details::json_obj;
using json_details::json_value_type;

} // namespace tbd

#endif // __json_h__

