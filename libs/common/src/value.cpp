#pragma clang diagnostic push

#include "value.hpp"

#include <cstring>
#include <iostream>
#include <iterator>

namespace launchdarkly {

const Value Value::null_value_;

Value::Value() : type_(Value::Type::kNull), storage_{0.0} {}

Value::Value(bool boolean) : type_(Value::Type::kBool), storage_{boolean} {}

Value::Value(double num) : type_(Value::Type::kNumber), storage_{num} {}

Value::Value(int num) : type_(Value::Type::kNumber), storage_{(double)num} {}

Value::Value(std::string str)
    : type_(Value::Type::kString), storage_{std::move(str)} {}

Value::Value(char const* str)
    : type_(Value::Type::kString), storage_{std::string(str)} {}

Value::Value(std::vector<Value> arr)
    : type_(Value::Type::kArray), storage_{std::move(arr)} {}

Value::Value(std::map<std::string, Value> obj)
    : type_(Value::Type::kObject), storage_{std::move(obj)} {}

Value::Type Value::type() const {
    return type_;
}

bool Value::is_null() const {
    return type_ == Type::kNull;
}

bool Value::is_bool() const {
    return type_ == Type::kBool;
}

bool Value::is_number() const {
    return type_ == Type::kNumber;
}

bool Value::is_string() const {
    return type_ == Type::kString;
}

bool Value::is_array() const {
    return type_ == Type::kArray;
}

bool Value::is_object() const {
    return type_ == Type::kObject;
}

Value const& Value::Null() {
    // This still just constructs a value, but it may be more discoverable
    // for people using the API.
    return null_value_;
}

bool Value::as_bool() const {
    if (type_ == Type::kBool) {
        return boost::get<bool>(storage_);
    }
    return false;
}

int Value::as_int() const {
    if (type_ == Type::kNumber) {
        return boost::get<double>(storage_);
    }
    return 0;
}

double Value::as_double() const {
    if (type_ == Type::kNumber) {
        return boost::get<double>(storage_);
    }
    return 0.0;
}

std::string const& Value::as_string() const {
    if (type_ == Type::kString) {
        return boost::get<std::string>(storage_);
    }
    return empty_string_;
}

Value::Array const& Value::as_array() const {
    if (type_ == Type::kArray) {
        return boost::get<Array>(storage_);
    }
    return empty_vector_;
}

Value::Object const& Value::as_object() const {
    if (type_ == Type::kObject) {
        return boost::get<Object>(storage_);
    }
    return empty_map_;
}

Value::Value(std::optional<std::string> opt_str) : storage_{0.0} {
    if (opt_str.has_value()) {
        type_ = Type::kString;
        storage_ = opt_str.value();
    } else {
        type_ = Type::kNull;
    }
}
Value::Value(std::initializer_list<Value> values)
    : type_(Type::kArray), storage_(std::vector<Value>(values)) {}

Value::Array::Iterator::Iterator(std::vector<Value>::const_iterator it)
    : it_(it) {}

Value::Array::Iterator::reference Value::Array::Iterator::operator*() const {
    return *it_;
}

Value::Array::Iterator::pointer Value::Array::Iterator::operator->() {
    return &*it_;
}

Value::Array::Iterator& Value::Array::Iterator::operator++() {
    it_++;
    return *this;
}

const Value::Array::Iterator Value::Array::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

Value const& Value::Array::operator[](std::size_t i) const {
    return vec_[i];
}

std::size_t Value::Array::size() const {
    return vec_.size();
}

Value::Array::Iterator Value::Array::begin() const {
    return Iterator(vec_.begin());
}

Value::Array::Iterator Value::Array::end() const {
    return Iterator(vec_.end());
}

Value::Array::Array(std::vector<Value> vec) : vec_(std::move(vec)) {}

Value::Object::Iterator::Iterator(
    std::map<std::string, Value>::const_iterator it)
    : it_(it) {}

Value::Object::Iterator::reference Value::Object::Iterator::operator*() const {
    return *it_;
}

Value::Object::Iterator::pointer Value::Object::Iterator::operator->() {
    return &*it_;
}

Value::Object::Iterator& Value::Object::Iterator::operator++() {
    it_++;
    return *this;
}

Value::Object::Iterator Value::Object::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

std::size_t Value::Object::size() const {
    return map_.size();
}

Value::Object::Iterator Value::Object::begin() const {
    return Iterator(map_.begin());
}

Value::Object::Iterator Value::Object::end() const {
    return Iterator(map_.end());
}

Value::Object::Iterator Value::Object::find(std::string const& key) const {
    return Iterator(map_.find(key));
}
}  // namespace launchdarkly
