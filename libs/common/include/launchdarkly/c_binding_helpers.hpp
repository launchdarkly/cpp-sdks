#include <launchdarkly/c_bindings/status.h>
#include <functional>
#include "error.hpp"
#include "tl/expected.hpp"

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

// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast

/*
 * Given a Builder, calls the Build() method and converts it into an
 * OpaqueResult if successful, or an LDError if unsuccessful.
 *
 * In the case of an error, out_result is set to nullptr.
 *
 * In all cases, the given builder is freed.
 */
template <typename Builder, typename OpaqueBuilder, typename OpaqueResult>
LDStatus ConsumeBuilder(OpaqueBuilder opaque_builder,
                        OpaqueResult* out_result) {
    using ReturnType =
        tl::expected<typename Builder::Result, launchdarkly::Error>;

    static_assert(has_result_type<Builder>::value,
                  "Builder must have an associated type named Result");

    static_assert(
        has_build_method<Builder, ReturnType>::value,
        "Builder must have a Build method that returns "
        "tl::expected<typename Builder::Result, launchdarkly::Error>");

    auto builder = reinterpret_cast<Builder*>(opaque_builder);

    tl::expected<typename Builder::Result, launchdarkly::Error> res =
        builder->Build();

    delete builder;

    if (!res) {
        *out_result = nullptr;
        return reinterpret_cast<LDStatus>(new launchdarkly::Error(res.error()));
    }

    *out_result = reinterpret_cast<OpaqueResult>(
        new typename Builder::Result(std::move(res.value())));

    return LDStatus_Success();
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
