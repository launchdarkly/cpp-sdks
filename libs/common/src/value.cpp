#pragma clang diagnostic push

#include <launchdarkly/value.hpp>

#include <cstring>
#include <iostream>

namespace launchdarkly {

const std::string Value::empty_string_;
const Value::Array Value::empty_vector_;
const Value::Object Value::empty_map_;
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

enum Value::Type Value::Type() const {
    return type_;
}

bool Value::IsNull() const {
    return type_ == Type::kNull;
}

bool Value::IsBool() const {
    return type_ == Type::kBool;
}

bool Value::IsNumber() const {
    return type_ == Type::kNumber;
}

bool Value::IsString() const {
    return type_ == Type::kString;
}

bool Value::IsArray() const {
    return type_ == Type::kArray;
}

bool Value::IsObject() const {
    return type_ == Type::kObject;
}

Value const& Value::Null() {
    // This still just constructs a value, but it may be more discoverable
    // for people using the API.
    return null_value_;
}

bool Value::AsBool() const {
    if (type_ == Type::kBool) {
        return std::get<bool>(storage_);
    }
    return false;
}

int Value::AsInt() const {
    if (type_ == Type::kNumber) {
        return static_cast<int>(std::get<double>(storage_));
    }
    return 0;
}

double Value::AsDouble() const {
    if (type_ == Type::kNumber) {
        return std::get<double>(storage_);
    }
    return 0.0;
}

std::string const& Value::AsString() const {
    if (type_ == Type::kString) {
        return std::get<std::string>(storage_);
    }
    return empty_string_;
}

Value::Array const& Value::AsArray() const {
    if (type_ == Type::kArray) {
        return std::get<Array>(storage_);
    }
    return empty_vector_;
}

Value::Object const& Value::AsObject() const {
    if (type_ == Type::kObject) {
        return std::get<Object>(storage_);
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

Value::Value(Value::Array arr)
    : storage_(std::move(arr)), type_(Type::kArray) {}
Value::Value(Value::Object obj)
    : storage_(std::move(obj)), type_(Type::kObject) {}

Value::Array::Iterator::Iterator(std::vector<Value>::const_iterator iterator)
    : iterator_(iterator) {}

Value::Array::Iterator::reference Value::Array::Iterator::operator*() const {
    return *iterator_;
}

Value::Array::Iterator::pointer Value::Array::Iterator::operator->() {
    return &*iterator_;
}

Value::Array::Iterator& Value::Array::Iterator::operator++() {
    iterator_++;
    return *this;
}

Value::Array::Iterator Value::Array::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

Value::Array::Iterator& Value::Array::Iterator::operator--() {
    iterator_--;
    return *this;
}

Value::Array::Iterator Value::Array::Iterator::operator--(int) {
    Iterator tmp = *this;
    --(*this);
    return tmp;
}

Value const& Value::Array::operator[](std::size_t index) const {
    return vec_[index];
}

std::size_t Value::Array::Size() const {
    return vec_.size();
}

Value::Array::Iterator Value::Array::begin() const {
    return {vec_.begin()};
}

Value::Array::Iterator Value::Array::end() const {
    return {vec_.end()};
}

Value::Array::Array(std::vector<Value> vec) : vec_(std::move(vec)) {}

Value::Object::Iterator::Iterator(
    std::map<std::string, Value>::const_iterator iterator)
    : it_(iterator) {}

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

std::size_t Value::Object::Size() const {
    return map_.size();
}

Value::Object::Iterator Value::Object::begin() const {
    return {map_.begin()};
}

Value::Object::Iterator Value::Object::end() const {
    return {map_.end()};
}

Value::Object::Iterator Value::Object::Find(std::string const& key) const {
    return {map_.find(key)};
}

std::size_t Value::Object::Count(std::string const& key) const {
    return map_.count(key);
}
Value::Object::Object(
    std::initializer_list<std::pair<std::string, Value>> values) {
    map_.insert(std::make_move_iterator(values.begin()),
                std::make_move_iterator(values.end()));
}

Value const& Value::Object::operator[](std::string const& key) const {
    return map_.at(key);
}

bool operator==(Value const& lhs, Value const& rhs) {
    if (lhs.Type() == rhs.Type()) {
        switch (lhs.Type()) {
            case Value::Type::kNull:
                return true;
            case Value::Type::kBool:
                return lhs.AsBool() == rhs.AsBool();
            case Value::Type::kNumber:
                return lhs.AsDouble() == rhs.AsDouble();
            case Value::Type::kString:
                return lhs.AsString() == rhs.AsString();
            case Value::Type::kObject:
                return lhs.AsObject() == rhs.AsObject();
            case Value::Type::kArray:
                return lhs.AsArray() == rhs.AsArray();
        }
    }
    return false;
}

bool operator!=(Value const& lhs, Value const& rhs) {
    return !(lhs == rhs);
}

bool operator==(Value::Array const& lhs, Value::Array const& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(Value::Array const& lhs, Value::Array const& rhs) {
    return !(lhs == rhs);
}

bool operator==(Value::Object const& lhs, Value::Object const& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(Value::Object const& lhs, Value::Object const& rhs) {
    return !(lhs == rhs);
}

}  // namespace launchdarkly
