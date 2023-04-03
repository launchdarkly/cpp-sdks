#pragma clang diagnostic push

#include "value.hpp"

#include <cstring>
#include <iostream>

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

Value const& Value::null() {
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
        return static_cast<int>(boost::get<double>(storage_));
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

Value const& Value::Array::operator[](std::size_t index) const {
    return vec_[index];
}

std::size_t Value::Array::size() const {
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

std::size_t Value::Object::size() const {
    return map_.size();
}

Value::Object::Iterator Value::Object::begin() const {
    return {map_.begin()};
}

Value::Object::Iterator Value::Object::end() const {
    return {map_.end()};
}

Value::Object::Iterator Value::Object::find(std::string const& key) const {
    return {map_.find(key)};
}

Value tag_invoke(boost::json::value_to_tag<launchdarkly::Value> const& unused,
                 boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    // The name of the function needs to be tag_invoke for boost::json.

    // The conditions in these switches explicitly use the constructors, because
    // otherwise it is an init list, which is an array.
    switch (json_value.kind()) {
        case boost::json::kind::null:
            return Value();
        case boost::json::kind::bool_:
            return Value(json_value.as_bool());
        case boost::json::kind::int64:
            return Value((double)json_value.as_int64());
        case boost::json::kind::uint64:
            return Value((double)json_value.as_uint64());
        case boost::json::kind::double_:
            return Value(json_value.as_double());
        case boost::json::kind::string:
            return Value(std::string(json_value.as_string()));
        case boost::json::kind::array: {
            auto vec = json_value.as_array();
            std::vector<Value> values;
            for (auto const& item : vec) {
                values.push_back(boost::json::value_to<Value>(item));
            }
            return Value(values);
        }
        case boost::json::kind::object: {
            auto map = json_value.as_object();
            std::map<std::string, Value> values;
            for (auto const& pair : map) {
                auto value = boost::json::value_to<Value>(pair.value());
                values.emplace(pair.key().data(), std::move(value));
            }
            return Value(std::move(values));
        }
    }
    // Cannot happen.
    return Value();
}

}  // namespace launchdarkly
