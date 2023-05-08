#include <functional>
#include <tl/expected.hpp>
#include "c_bindings/error.h"
#include "error.hpp"

template <typename T, typename = void>
struct has_result_type : std::false_type {};

template <typename T>
struct has_result_type<T, std::void_t<typename T::Result>> : std::true_type {};

template <typename T, typename ReturnType, typename = void>
struct has_build_method : std::false_type {};

template <typename T, typename ReturnType>
struct has_build_method<T,
                        ReturnType,
                        std::void_t<decltype(std::declval<T>().Build())>>
    : std::integral_constant<
          bool,
          std::is_same_v<decltype(std::declval<T>().Build()), ReturnType>> {};

/*
 * Given a Builder, calls the Build() method and converts it into an
 * OpaqueResult if successful, or an LDError if unsuccessful.
 *
 * In the case of an error, out_result is set to nullptr.
 */
template <typename Builder, typename OpaqueBuilder, typename OpaqueResult>
LDError ConvertError(OpaqueBuilder b, OpaqueResult* out_result) {
    using ReturnType =
        tl::expected<typename Builder::Result, launchdarkly::Error>;

    static_assert(has_result_type<Builder>::value,
                  "Builder must have an associated type named Result");
    static_assert(
        has_build_method<Builder, ReturnType>::value,
        "Builder must have a Build method that returns "
        "tl::expected<typename Builder::Result, launchdarkly::Error>");

    tl::expected<typename Builder::Result, launchdarkly::Error> res =
        reinterpret_cast<Builder*>(b)->Build();
    if (!res) {
        *out_result = nullptr;
        return reinterpret_cast<LDError>(new launchdarkly::Error(res.error()));
    }
    *out_result = reinterpret_cast<OpaqueResult>(
        new typename Builder::Result(std::move(res.value())));
    return nullptr;
}
