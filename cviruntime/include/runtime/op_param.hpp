/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/

#ifndef CVI_RUNTIME_OP_PARAMETER_H
#define CVI_RUNTIME_OP_PARAMETER_H
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <assert.h>

namespace cvi {

class FieldBase {
public:
  FieldBase() = default;
  virtual ~FieldBase() = default;
};

template <typename T>
class Field: public FieldBase {
public:
  Field(T& val): data(val) {}
  T data;
};

class OpParam {
public:
  template <typename T>
  void put(std::string name, T value) {
    fields[name] = std::make_shared<Field<T>>(value);
  }

  template <typename T>
  T& get(std::string name) {
    auto f = dynamic_cast<Field<T>*>(fields[name].get());
    assert(f);
    return f->data;
  }

  bool has(std::string name) {
    auto it = fields.find(name);
    return it != fields.end();
  }

private:
  std::map<std::string, std::shared_ptr<FieldBase>> fields;
};

}
#endif