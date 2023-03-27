#include "value.hpp"

#include <iterator>

using launchdarkly::Value;

Value::Value() : type_(Value::Type::kNull), storage_{0} {}

Value::Value(bool boolean) : type_(Value::Type::kBool), storage_{boolean} {}

Value::Value(double num) : type_(Value::Type::kNumber), storage_{num} {}

Value::Value(int num) : type_(Value::Type::kNumber), storage_{num} {}

Value::Value(std::string str)
    : type_(Value::Type::kString), storage_{std::move(str)} {}

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
            storage_.string_ = other.storage_.string_;
            break;
        case Type::kObject:
            storage_.object_ = other.storage_.object_;
            break;
        case Type::kArray:
            auto vec = std::vector<Value>();
            auto const& otherVec = other.storage_.array_;
            for (int index = 0; index < otherVec.size(); index++)
                vec.push_back(otherVec[index]);
            storage_.array_ = std::move(vec);
            break;
    }
}

Value::~Value() {
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
            storage_.array_.~vector();
            break;
        case Type::kArray:
            storage_.object_.~map();
            break;
    }
}

Value::Type launchdarkly::Value::type() {
    return type_;
}

bool launchdarkly::Value::is_null() {
    return type_ == Type::kNull;
}

bool launchdarkly::Value::is_bool() {
    return type_ == Type::kBool;
}

bool launchdarkly::Value::is_number() {
    return type_ == Type::kNumber;
}

bool launchdarkly::Value::is_string() {
    return type_ == Type::kString;
}

bool launchdarkly::Value::is_array() {
    return type_ == Type::kArray;
}

bool launchdarkly::Value::is_object() {
    return type_ == Type::kObject;
}

Value launchdarkly::Value::Null() {
    // This still just constructs a value, but it may be more discoverable
    // for people using the API.
    return Value();
}

bool launchdarkly::Value::as_bool() {
    if (type_ == Type::kBool) {
        return storage_.boolean_;
    }
    return false;
}

int launchdarkly::Value::as_int() {
    if (type_ == Type::kNumber) {
        return storage_.number_;
    }
    return 0;
}

double launchdarkly::Value::as_double() {
    if (type_ == Type::kNumber) {
        return storage_.number_;
    }
    return 0.0;
}

std::string const& launchdarkly::Value::as_string() {
    if (type_ == Type::kString) {
        return storage_.string_;
    }
    return empty_string_;
}

std::vector<Value> const& launchdarkly::Value::as_array() {
    if (type_ == Type::kArray) {
        return storage_.array_;
    }
    return empty_vector_;
}

std::map<std::string, Value> const& launchdarkly::Value::as_object() {
    if (type_ == Type::kObject) {
        return storage_.object_;
    }
    return empty_map_;
}

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
    string_ = str;
}

launchdarkly::Value::Storage::Storage(std::vector<Value> arr) {
    array_ = std::move(arr);
}

launchdarkly::Value::Storage::Storage(std::map<std::string, Value> obj) {
    object_ = std::move(obj);
}
