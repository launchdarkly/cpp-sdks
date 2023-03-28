
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
    Value(Value && other);

    /**
     * Construct an array value from a vector of Value.
     * @param arr
     */
    Value(std::vector<Value> arr);

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
    Value(std::initializer_list<Value> values): type_(Type::kArray), storage_(std::vector<Value>(values)) {
    }

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
    std::vector<Value> const& as_array() const;

    /**
     * if the value is an object type, then return a reference to that object
     * as a map, otherwise return a reference to an empty map.
     *
     * @return The value as a map, or an empty map.
     */
    std::map<std::string, Value> const& as_object() const;

    ~Value();

    /**
     * Get a null value.
     * @return The null value.
     */
    static Value Null();

   private:
    union Storage {
        Storage(bool boolean);
        Storage(double num);
        Storage(int num);
        Storage(std::string str);
        Storage(std::vector<Value> arr);
        Storage(std::map<std::string, Value> obj);

        bool boolean_;
        double number_;
        std::string string_;
        std::vector<Value> array_;
        std::map<std::string, Value> object_;
        ~Storage(){};
    };

    Storage storage_;
    Type type_;

    // Empty constants used when accessing the wrong type.
    inline static const std::string empty_string_;
    inline static const std::vector<Value> empty_vector_;
    inline static const std::map<std::string, Value> empty_map_;

    void move_storage(Value && other);
    void destruct_storage();
};

}  // namespace launchdarkly
