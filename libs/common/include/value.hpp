#pragma once

#include <cstddef>
#include <map>
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
 * [TODO] to get the value and then use Value methods to examine it.
 *
 * Similarly, attributes of an evaluation context ([TODO])
 * can have variations of any JSON type other than null. If you want to set a
 * context attribute in a general way that will accept any type, or set the
 * attribute value to a complex data structure such as an array or object, you
 * can use the builder method [TODO].
 *
 * Arrays and objects have special meanings in LaunchDarkly flag evaluation:
 *   - An array of values means "try to match any of these values to the
 * targeting rule."
 *   - An object allows you to match a property within the object to the
 * targeting rule. For instance, in the example above, a targeting rule could
 * reference /objectAttr1/color to match the value "green". Nested property
 * references like /objectAttr1/address/street are allowed if a property
 *     contains another JSON object.
 *
 * # Constructors and builders
 * [TODO]
 *
 * # Comparisons
 * [TODO]
 */
class Value {
   public:
    /**
     * Array type for values. Provides const iteration and indexing.
     */
    class Array {
       public:
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Value;
            using pointer = value_type const*;
            using reference = value_type const&;

            Iterator(std::vector<Value>::const_iterator it);

            reference operator*() const;
            pointer operator->();
            Iterator& operator++();
            const Iterator operator++(int);

            friend bool operator==(Iterator const& a, Iterator const& b) {
                return a.it_ == b.it_;
            };

            friend bool operator!=(Iterator const& a, Iterator const& b) {
                return a.it_ != b.it_;
            };

           private:
            std::vector<Value>::const_iterator it_;
        };

        /**
         * Create an array from a vector of Value.
         * @param vec The vector to base the array on.
         */
        Array(std::vector<Value> vec);
        Array(std::initializer_list<Value> values) : vec_(values) {}
        Array() = default;

        Value const& operator[](std::size_t i) const;

        std::size_t size() const;

        Iterator begin() const;

        Iterator end() const;

       private:
        std::vector<Value> vec_;
    };

    /**
     * Object type for values. Provides const iteration and indexing.
     */
    class Object {
       public:
        using StdString = const std::string;
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

            using value_type = std::pair<StdString, Value>;
            using pointer = value_type const*;
            using reference = value_type const&;

            Iterator(std::map<std::string, Value>::const_iterator it);

            reference operator*() const;
            pointer operator->();
            Iterator& operator++();
            Iterator operator++(int);

            friend bool operator==(Iterator const& a, Iterator const& b) {
                return a.it_ == b.it_;
            };
            friend bool operator!=(Iterator const& a, Iterator const& b) {
                return a.it_ != b.it_;
            };

           private:
            std::map<std::string, Value>::const_iterator it_;
        };

        /**
         * Create an Object from a map of Values.
         * @param map The map to base the object on.
         */
        Object(std::map<std::string, Value> map) : map_(std::move(map)) {}
        Object() = default;
        Object(std::initializer_list<std::pair<std::string, Value>> values) {
            for (auto pair : values) {
                map_[std::move(pair.first)] = std::move(pair.second);
            }
        }

        /*
         * Get the Value with the specified key.
         *
         * This operates like `.at` on a map, and accessing out of bounds
         * is invalid.
         */
        Value const& operator[](std::string const& key) const {
            return map_.at(key);
        }

        /**
         * The number of items in the Object.
         * @return The number of items in the Object.
         */
        std::size_t size() const;

        /**
         * Get the number of items with the given key. Will be 1 or 0.
         * @param key The key to get a count for.
         * @return The count of items with the given key.
         */
        std::size_t count(std::string const& key) const {
            return map_.count(key);
        }

        Iterator begin() const;

        Iterator end() const;

        /**
         * Find a Value by key. Operates like `find` on a std::map.
         * @param key The key to find a value for.
         * @return The value, or the end iterator.
         */
        Iterator find(std::string const& key) const;

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
     * Construct a string value from a constant string.
     * @param str
     */
    Value(Value&& other);

    /**
     * Construct an array value from a vector of Value.
     * @param arr
     */
    Value(std::vector<Value> arr);

    Value(Array arr) : storage_(std::move(arr)), type_(Type::kArray) {}

    Value(Object obj) : storage_(std::move(obj)), type_(Type::kObject) {}

    /**
     * Construct an object value from a map of Value.
     * @param obj
     */
    Value(std::map<std::string, Value> obj);

    /**
     * Construct a copy of an existing value.
     * @param val
     */
    Value(Value const& val);

    /**
     * Create an array type value from the given list.
     *
     * Cannot be used to create object type values.
     * @param values
     */
    Value(std::initializer_list<Value> values);

    /**
     * Create either a value string, or null value, from an optional string.
     * @param optString
     */
    Value(std::optional<std::string> optString);

    Value& operator=(Value const& other);
    Value& operator=(Value&& other);

    /**
     * Get the type of the attribute.
     */
    Type type() const;

    /**
     * Returns true if the value is a null.
     *
     * Unlike other variants there is not an as_null(). Instead use the return
     * value from this function as a marker.
     * @return True if the value is null.
     */
    bool is_null() const;

    /**
     * Returns true if the value is a boolean.
     *
     * @return
     */
    bool is_bool() const;

    /**
     * Returns true if the value is a number.
     *
     * Numbers are always stored as doubles, but can be accessed as either
     * an int or double for convenience.
     * @return True if the value is a number.
     */
    bool is_number() const;

    /**
     * Returns true if the value is a string.
     *
     * @return True if the value is a string.
     */
    bool is_string() const;

    /**
     * Returns true if the value is an array.
     *
     * @return True if the value is an array.
     */
    bool is_array() const;

    /**
     * Returns true if the value is an object.
     *
     * @return True if the value is an object.
     */
    bool is_object() const;

    /**
     * If the value is a boolean, then return the boolean, otherwise return
     * false.
     *
     * @return The value of the boolean, or false.
     */
    bool as_bool() const;

    /**
     * If the value is a number, then return the internal double value as an
     * integer, otherwise return 0.
     *
     * @return The value as an integer, or 0.
     */
    int as_int() const;

    double as_double() const;

    /**
     * If the value is a string, then return a reference to that string,
     * otherwise return a reference to an empty string.
     *
     * @return The value as a string, or an empty string.
     */
    std::string const& as_string() const;

    /**
     * If the value is an array type, then return a reference to that array as a
     * vector, otherwise return a reference to an empty vector.
     *
     * @return The value as a vector, or an empty vector.
     */
    Array const& as_array() const;

    /**
     * if the value is an object type, then return a reference to that object
     * as a map, otherwise return a reference to an empty map.
     *
     * @return The value as a map, or an empty map.
     */
    Object const& as_object() const;

    ~Value();

    /**
     * Get a null value.
     * @return The null value.
     */
    static Value const& Null();

   private:
    union Storage {
        Storage(bool boolean);
        Storage(double num);
        Storage(int num);
        Storage(std::string str);
        Storage(std::vector<Value> arr);
        Storage(std::map<std::string, Value> obj);
        Storage(Array arr);
        Storage(Object obj);

        bool boolean_;
        double number_;
        std::string string_;
        Array array_;
        Object object_;
        ~Storage(){};
    };

    Storage storage_;
    Type type_;

    // Empty constants used when accessing the wrong type.
    inline static const std::string empty_string_;
    inline static const Array empty_vector_;
    inline static const Object empty_map_;
    static const Value null_value_;

    void move_storage(Value&& other);
    void destruct_storage();
};

}  // namespace launchdarkly
