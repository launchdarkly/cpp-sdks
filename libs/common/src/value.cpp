#include "value.hpp"

#include <cstring>
#include <iostream>
#include <iterator>

using launchdarkly::Value;

const Value Value::null_value_;

Value::Value() : type_(Value::Type::kNull), storage_{0} {}

Value::Value(bool boolean) : type_(Value::Type::kBool), storage_{boolean} {}

Value::Value(double num) : type_(Value::Type::kNumber), storage_{num} {}

Value::Value(int num) : type_(Value::Type::kNumber), storage_{num} {}

Value::Value(std::string str)
    : type_(Value::Type::kString), storage_{std::move(str)} {}

Value::Value(char const* str)
    : type_(Value::Type::kString), storage_{std::string(str)} {}

Value::Value(std::vector<Value> arr)
    : type_(Value::Type::kArray), storage_{std::move(arr)} {}

Value::Value(std::map<std::string, Value> obj)
    : type_(Value::Type::kObject), storage_{std::move(obj)} {}

Value::Value(Value const& other) : type_(other.type_), storage_(0) {
    // The storage_ gets initialized as a number, because we need to inspect
    // the type before we actually set the value.
    switch (type_) {
        case Type::kNull:
            break;
        case Type::kBool:
            storage_.boolean_ = other.storage_.boolean_;
            break;
        case Type::kNumber:
            storage_.number_ = other.storage_.number_;
            break;
        case Type::kString:
            new (&storage_.string_) std::string(other.storage_.string_);
            break;
        case Type::kObject:
            new (&storage_.object_) Object(other.storage_.object_);
            break;
        case Type::kArray:
            auto vec = std::vector<Value>();
            auto const& otherVec = other.storage_.array_;
            for (int index = 0; index < otherVec.size(); index++)
                vec.push_back(otherVec[index]);
            new (&storage_.array_) std::vector(std::move(vec));
            break;
    }
}

Value::Value(Value&& other) : type_(other.type_), storage_{0} {
    // The storage_ gets initialized as a
    // number, because we need to inspect
    // the type before we actually set the value.
    move_storage(std::move(other));
}

void Value::move_storage(Value&& other) {
    switch (type_) {
        case Type::kNull:
            break;
        case Type::kBool:
            storage_.boolean_ = other.storage_.boolean_;
            break;
        case Type::kNumber:
            storage_.number_ = other.storage_.number_;
            break;
        case Type::kString: {
            new (&storage_.string_)
                std::string(std::move(other.storage_.string_));
        } break;
        case Type::kObject:
            new (&storage_.object_) Object(std::move(other.storage_.object_));
            break;
        case Type::kArray:
            new (&storage_.object_) Array(std::move(other.storage_.array_));
            break;
    }
}

Value::~Value() {
    destruct_storage();
}
void Value::destruct_storage() {
    switch (type_) {
        // Nothing to be done for the basic types.
        case Type::kNull:
        case Type::kBool:
        case Type::kNumber:
            break;

        case Type::kString:
            storage_.string_.~basic_string();
            break;
        case Type::kObject:
            storage_.object_.~Object();
            break;
        case Type::kArray:
            storage_.array_.~Array();

            break;
    }
}

Value::Type launchdarkly::Value::type() const {
    return type_;
}

bool launchdarkly::Value::is_null() const {
    return type_ == Type::kNull;
}

bool launchdarkly::Value::is_bool() const {
    return type_ == Type::kBool;
}

bool launchdarkly::Value::is_number() const {
    return type_ == Type::kNumber;
}

bool launchdarkly::Value::is_string() const {
    return type_ == Type::kString;
}

bool launchdarkly::Value::is_array() const {
    return type_ == Type::kArray;
}

bool launchdarkly::Value::is_object() const {
    return type_ == Type::kObject;
}

Value const& launchdarkly::Value::Null() {
    // This still just constructs a value, but it may be more discoverable
    // for people using the API.
    return null_value_;
}

bool launchdarkly::Value::as_bool() const {
    if (type_ == Type::kBool) {
        return storage_.boolean_;
    }
    return false;
}

int launchdarkly::Value::as_int() const {
    if (type_ == Type::kNumber) {
        return storage_.number_;
    }
    return 0;
}

double launchdarkly::Value::as_double() const {
    if (type_ == Type::kNumber) {
        return storage_.number_;
    }
    return 0.0;
}

std::string const& launchdarkly::Value::as_string() const {
    if (type_ == Type::kString) {
        return storage_.string_;
    }
    return empty_string_;
}

Value::Array const& launchdarkly::Value::as_array() const {
    if (type_ == Type::kArray) {
        return storage_.array_;
    }
    return empty_vector_;
}

Value::Object const& launchdarkly::Value::as_object() const {
    if (type_ == Type::kObject) {
        return storage_.object_;
    }
    return empty_map_;
}

Value& launchdarkly::Value::operator=(Value const& other) {
    return *this = Value(other);
}

Value& launchdarkly::Value::operator=(Value&& other) {
    // Destruct storage based on original type.
    destruct_storage();
    // Update the type to the new type.
    type_ = other.type_;
    // Allocate new storage and move teh other value.
    move_storage(std::move(other));

    return *this;
}

launchdarkly::Value::Value(std::optional<std::string> optString) : storage_{0} {
    if (optString.has_value()) {
        type_ = Type::kString;
        new (&storage_.string_) std::string(std::move(optString.value()));
    } else {
        type_ = Type::kNull;
    }
}
launchdarkly::Value::Value(std::initializer_list<Value> values)
    : type_(Type::kArray), storage_(std::vector<Value>(values)) {}

launchdarkly::Value::Storage::Storage(bool boolean) {
    boolean_ = boolean;
}

launchdarkly::Value::Storage::Storage(double num) {
    number_ = num;
}

launchdarkly::Value::Storage::Storage(int num) {
    number_ = num;
}

launchdarkly::Value::Storage::Storage(std::string str) {
    // Use in-place new, otherwise the string member will not be initialized.
    new (&string_) std::string(std::move(str));
}

launchdarkly::Value::Storage::Storage(std::vector<Value> arr) {
    // Use in-place new, otherwise the vector member will not be initialized.
    new (&array_) std::vector(std::move(arr));
}

launchdarkly::Value::Storage::Storage(std::map<std::string, Value> obj) {
    // Use in-place new, otherwise the map member will not be initialized.
    new (&object_) std::map(std::move(obj));
}

launchdarkly::Value::Array::Iterator::Iterator(
    std::vector<Value>::const_iterator it)
    : it_(it) {}

launchdarkly::Value::Array::Iterator::reference
launchdarkly::Value::Array::Iterator::operator*() const {
    return *it_;
}

Value::Array::Iterator::pointer
launchdarkly::Value::Array::Iterator::operator->() {
    return &*it_;
}

Value::Array::Iterator& launchdarkly::Value::Array::Iterator::operator++() {
    it_++;
    return *this;
}

const Value::Array::Iterator launchdarkly::Value::Array::Iterator::operator++(
    int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

Value const& launchdarkly::Value::Array::operator[](size_t i) const {
    return vec_[i];
}

size_t launchdarkly::Value::Array::size() const {
    return vec_.size();
}

Value::Array::Iterator launchdarkly::Value::Array::begin() const {
    return Iterator(vec_.begin());
}

Value::Array::Iterator launchdarkly::Value::Array::end() const {
    return Iterator(vec_.end());
}

launchdarkly::Value::Array::Array(std::vector<Value> vec)
    : vec_(std::move(vec)) {}

launchdarkly::Value::Object::Iterator::Iterator(
    std::map<std::string, Value>::const_iterator it)
    : it_(it) {}

Value::Object::Iterator::reference
launchdarkly::Value::Object::Iterator::operator*() const {
    return *it_;
}

Value::Object::Iterator::pointer
launchdarkly::Value::Object::Iterator::operator->() {
    return &*it_;
}

Value::Object::Iterator& launchdarkly::Value::Object::Iterator::operator++() {
    it_++;
    return *this;
}

Value::Object::Iterator launchdarkly::Value::Object::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

size_t launchdarkly::Value::Object::size() const {
    return map_.size();
}

Value::Object::Iterator launchdarkly::Value::Object::begin() const {
    return Iterator(map_.begin());
}

Value::Object::Iterator launchdarkly::Value::Object::end() const {
    return Iterator(map_.end());
}

Value::Object::Iterator launchdarkly::Value::Object::find(
    std::string const& key) const {
    return Iterator(map_.find(key));
}
