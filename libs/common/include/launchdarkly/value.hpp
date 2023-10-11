#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace launchdarkly {

/**
 * Value represents any of the data types supported by JSON, all of which can be
 * used for a LaunchDarkly feature flag variation, or for an attribute in an
 * evaluation context. Value instances are immutable.
 *
 * # Uses of JSON types in LaunchDarkly
 *
 * LaunchDarkly feature flags can have variations of any JSON type other than
 * null. If you want to evaluate a feature flag in a general way that does not
 * have expectations about the variation type, or if the variation value is a
 * complex data structure such as an array or object, you can use the SDK method
 * launchdarkly::client_side::IClient::JsonVariation.
 *
 * Similarly, attributes of an evaluation context (launchdarkly::Context)
 * can have variations of any JSON type other than null. If you want to set a
 * context attribute in a general way that will accept any type, or set the
 * attribute value to a complex data structure such as an array or object, you
 * can use the builder method
 * launchdarkly::AttributesBuilder< BuilderReturn, BuildType >::set.
 *
 * Arrays and objects have special meanings in LaunchDarkly flag evaluation:
 *   - An array of values means "try to match any of these values to the
 * targeting rule."
 *   - An object allows you to match a property within the object to the
 * targeting rule. For instance, in the example above, a targeting rule could
 * reference /objectAttr1/color to match the value "green". Nested property
 * references like /objectAttr1/address/street are allowed if a property
 *     contains another JSON object.
 */
class Value final {
   public:
    /**
     * Array type for values. Provides const iteration and indexing.
     */
    class Array {
       public:
        struct Iterator {
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Value;
            using pointer = value_type const*;
            using reference = value_type const&;

            Iterator(std::vector<Value>::const_iterator iterator);

            reference operator*() const;
            pointer operator->();
            Iterator& operator++();
            Iterator operator++(int);

            Iterator& operator--();
            Iterator operator--(int);

            friend bool operator==(Iterator const& lhs, Iterator const& rhs) {
                return lhs.iterator_ == rhs.iterator_;
            };

            friend bool operator!=(Iterator const& lhs, Iterator const& rhs) {
                return lhs.iterator_ != rhs.iterator_;
            };

           private:
            std::vector<Value>::const_iterator iterator_;
        };

        friend std::ostream& operator<<(std::ostream& out, Array const& arr) {
            out << "[";
            bool first = true;
            for (auto const& item : arr.vec_) {
                if (first) {
                    first = false;
                } else {
                    out << ", ";
                }
                out << item;
            }
            out << "]";
            return out;
        }

        /**
         * Create an array from a vector of Value.
         * @param vec The vector to base the array on.
         */
        Array(std::vector<Value> vec);
        Array(std::initializer_list<Value> values) : vec_(values) {}
        Array() = default;

        Value const& operator[](std::size_t index) const;

        [[nodiscard]] std::size_t Size() const;

        [[nodiscard]] Iterator begin() const;

        [[nodiscard]] Iterator end() const;

       private:
        std::vector<Value> vec_;
    };

    /**
     * Object type for values. Provides const iteration and indexing.
     */
    class Object {
       public:
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

            using value_type = std::pair<std::string const, Value>;
            using pointer = value_type const*;
            using reference = value_type const&;

            Iterator(std::map<std::string, Value>::const_iterator iterator);

            reference operator*() const;
            pointer operator->();
            Iterator& operator++();
            Iterator operator++(int);

            friend bool operator==(Iterator const& lhs, Iterator const& rhs) {
                return rhs.it_ == lhs.it_;
            };
            friend bool operator!=(Iterator const& lhs, Iterator const& rhs) {
                return lhs.it_ != rhs.it_;
            };

           private:
            std::map<std::string, Value>::const_iterator it_;
        };

        friend std::ostream& operator<<(std::ostream& out, Object const& obj) {
            out << "{";
            bool first = true;
            for (auto const& pair : obj.map_) {
                if (first) {
                    first = false;
                } else {
                    out << ", ";
                }
                out << "{" << pair.first << ", " << pair.second << "}";
            }
            out << "}";
            return out;
        }

        /**
         * Create an Object from a map of Values.
         * @param map The map to base the object on.
         */
        Object(std::map<std::string, Value> map) : map_(std::move(map)) {}
        Object() = default;
        Object(std::initializer_list<std::pair<std::string, Value>> values);

        /*
         * Get the Value with the specified key.
         *
         * This operates like `.at` on a map, and accessing out of bounds
         * is invalid.
         */
        Value const& operator[](std::string const& key) const;

        /**
         * The number of items in the Object.
         * @return The number of items in the Object.
         */
        [[nodiscard]] std::size_t Size() const;

        /**
         * Get the number of items with the given key. Will be 1 or 0.
         * @param key The key to get a count for.
         * @return The count of items with the given key.
         */
        [[nodiscard]] std::size_t Count(std::string const& key) const;

        [[nodiscard]] Iterator begin() const;

        [[nodiscard]] Iterator end() const;

        /**
         * Find a Value by key. Operates like `find` on a std::map.
         * @param key The key to find a value for.
         * @return The value, or the end iterator.
         */
        [[nodiscard]] Iterator Find(std::string const& key) const;

       private:
        std::map<std::string, Value> map_;
    };

    /**
     * Create a Value from a string constant.
     * @param str The string constant to base the value on.
     */
    Value(char const* str);

    enum class Type { kNull, kBool, kNumber, kString, kObject, kArray };

    /**
     * Construct a value representing null.
     */
    Value();

    Value(Value const& val) = default;
    Value(Value&&) = default;
    Value& operator=(Value const&) = default;
    Value& operator=(Value&&) = default;

    /**
     * Construct a boolean value.
     * @param boolean
     */
    Value(bool boolean);

    /**
     * Construct a number value from a double.
     * @param num
     */
    Value(double num);

    /**
     * Construct a number value from an integer.
     * @param num
     */
    Value(int num);

    /**
     * Construct a string value.
     * @param str
     */
    Value(std::string str);

    /**
     * Construct an array value from a vector of Value.
     * @param arr
     */
    Value(std::vector<Value> arr);

    Value(Array arr);

    Value(Object obj);

    /**
     * Construct an object value from a map of Value.
     * @param obj
     */
    Value(std::map<std::string, Value> obj);

    /**
     * Create an array type value from the given list.
     *
     * Cannot be used to create object type values.
     * @param values
     */
    Value(std::initializer_list<Value> values);

    /**
     * Create either a value string, or null value, from an optional string.
     * @param opt_string
     */
    Value(std::optional<std::string> opt_string);

    /**
     * Get the type of the attribute.
     */
    [[nodiscard]] Type Type() const;

    /**
     * Returns true if the value is a null.
     *
     * Unlike other variants there is not an as_null(). Instead use the return
     * value from this function as a marker.
     * @return True if the value is null.
     */
    [[nodiscard]] bool IsNull() const;

    /**
     * Returns true if the value is a boolean.
     *
     * @return
     */
    [[nodiscard]] bool IsBool() const;

    /**
     * Returns true if the value is a number.
     *
     * Numbers are always stored as doubles, but can be accessed as either
     * an int or double for convenience.
     * @return True if the value is a number.
     */
    [[nodiscard]] bool IsNumber() const;

    /**
     * Returns true if the value is a string.
     *
     * @return True if the value is a string.
     */
    [[nodiscard]] bool IsString() const;

    /**
     * Returns true if the value is an array.
     *
     * @return True if the value is an array.
     */
    [[nodiscard]] bool IsArray() const;

    /**
     * Returns true if the value is an object.
     *
     * @return True if the value is an object.
     */
    [[nodiscard]] bool IsObject() const;

    /**
     * If the value is a boolean, then return the boolean, otherwise return
     * false.
     *
     * @return The value of the boolean, or false.
     */
    [[nodiscard]] bool AsBool() const;

    /**
     * If the value is a number, then return the internal double value as an
     * integer, otherwise return 0.
     *
     * @return The value as an integer, or 0.
     */
    [[nodiscard]] int AsInt() const;

    [[nodiscard]] double AsDouble() const;

    /**
     * If the value is a string, then return a reference to that string,
     * otherwise return a reference to an empty string.
     *
     * @return The value as a string, or an empty string.
     */
    [[nodiscard]] std::string const& AsString() const;

    /**
     * If the value is an array type, then return a reference to that array as a
     * vector, otherwise return a reference to an empty vector.
     *
     * @return The value as a vector, or an empty vector.
     */
    [[nodiscard]] Array const& AsArray() const;

    /**
     * if the value is an object type, then return a reference to that object
     * as a map, otherwise return a reference to an empty map.
     *
     * @return The value as a map, or an empty map.
     */
    [[nodiscard]] Object const& AsObject() const;

    ~Value() = default;

    /**
     * Get a null value.
     * @return The null value.
     */
    static Value const& Null();

    friend std::ostream& operator<<(std::ostream& out, Value const& value) {
        switch (value.Type()) {
            case Type::kNull:
                out << "null()";
                break;
            case Type::kBool:
                out << "bool("
                    << (std::get<bool>(value.storage_) ? "true" : "false")
                    << ")";
                break;
            case Type::kNumber:
                out << "number(" << std::get<double>(value.storage_) << ")";
                break;
            case Type::kString:
                out << "string(" << std::get<std::string>(value.storage_)
                    << ")";
                break;
            case Type::kObject:
                out << "object(" << std::get<Object>(value.storage_) << ")";
                break;
            case Type::kArray:
                out << "array(" << std::get<Array>(value.storage_) << ")";
                break;
        }
        return out;
    }

    operator bool() const { return AsBool(); }

    operator std::string() const { return AsString(); }

    operator double() const { return AsDouble(); }

    operator int() const { return AsInt(); }

   private:
    struct null_type {};

    std::variant<null_type, bool, double, std::string, Array, Object> storage_;

    // Empty constants used when accessing the wrong type.
    // These are not inline static const because of this bug:
    // https://developercommunity.visualstudio.com/t/inline-static-destructors-are-called-multiple-time/1157794
    static std::string const empty_string_;
    static Array const empty_vector_;
    static Object const empty_map_;
    static Value const null_value_;
};

bool operator==(Value const& lhs, Value const& rhs);
bool operator!=(Value const& lhs, Value const& rhs);

bool operator==(Value::Array const& lhs, Value::Array const& rhs);
bool operator!=(Value::Array const& lhs, Value::Array const& rhs);

bool operator==(Value::Object const& lhs, Value::Object const& rhs);
bool operator!=(Value::Object const& lhs, Value::Object const& rhs);

}  // namespace launchdarkly
