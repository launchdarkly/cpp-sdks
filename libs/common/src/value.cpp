#include <launchdarkly/value.hpp>

#include <cstring>
#include <iostream>

namespace launchdarkly {

std::string const Value::empty_string_;
Value::Array const Value::empty_vector_;
Value::Object const Value::empty_map_;
Value const Value::null_value_;

Value::Value() : storage_{null_type{}} {}

Value::Value(bool boolean) : storage_{boolean} {}

Value::Value(double num) : storage_{num} {}

Value::Value(int num) : storage_{(double)num} {}

Value::Value(std::string str) : storage_{std::move(str)} {}

Value::Value(char const* str) : storage_{std::string(str)} {}

Value::Value(std::vector<Value> arr) : storage_{std::move(arr)} {}

Value::Value(std::map<std::string, Value> obj) : storage_{std::move(obj)} {}

template <class>
inline constexpr bool always_false_v = false;

enum Value::Type Value::Type() const {
    return std::visit(
        [](auto const& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, null_type>) {
                return Type::kNull;
            } else if constexpr (std::is_same_v<T, bool>) {
                return Type::kBool;
            } else if constexpr (std::is_same_v<T, double>) {
                return Type::kNumber;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return Type::kString;
            } else if constexpr (std::is_same_v<T, Value::Array>) {
                return Type::kArray;
            } else if constexpr (std::is_same_v<T, Value::Object>) {
                return Type::kObject;
            } else {
                static_assert(always_false_v<T>,
                              "all value types must be visited");
            }
        },
        storage_);
}

bool Value::IsNull() const {
    return std::holds_alternative<null_type>(storage_);
}

bool Value::IsBool() const {
    return std::holds_alternative<bool>(storage_);
}

bool Value::IsNumber() const {
    return std::holds_alternative<double>(storage_);
}

bool Value::IsString() const {
    return std::holds_alternative<std::string>(storage_);
}

bool Value::IsArray() const {
    return std::holds_alternative<Value::Array>(storage_);
}

bool Value::IsObject() const {
    return std::holds_alternative<Value::Object>(storage_);
}

Value const& Value::Null() {
    // This still just constructs a value, but it may be more discoverable
    // for people using the API.
    return null_value_;
}

bool Value::AsBool() const {
    if (IsBool()) {
        return std::get<bool>(storage_);
    }
    return false;
}

int Value::AsInt() const {
    if (IsNumber()) {
        return static_cast<int>(std::get<double>(storage_));
    }
    return 0;
}

double Value::AsDouble() const {
    if (IsNumber()) {
        return std::get<double>(storage_);
    }
    return 0.0;
}

std::string const& Value::AsString() const {
    if (IsString()) {
        return std::get<std::string>(storage_);
    }
    return empty_string_;
}

Value::Array const& Value::AsArray() const {
    if (IsArray()) {
        return std::get<Array>(storage_);
    }
    return empty_vector_;
}

Value::Object const& Value::AsObject() const {
    if (IsObject()) {
        return std::get<Object>(storage_);
    }
    return empty_map_;
}

Value::Value(std::optional<std::string> opt_str) : storage_{0.0} {
    if (opt_str.has_value()) {
        storage_ = opt_str.value();
    } else {
        storage_ = null_type{};
    }
}
Value::Value(std::initializer_list<Value> values)
    : storage_(std::vector<Value>(values)) {}

Value::Value(Value::Array arr) : storage_(std::move(arr)) {}

Value::Value(Value::Object obj) : storage_(std::move(obj)) {}

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
