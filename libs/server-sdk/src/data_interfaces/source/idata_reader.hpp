#pragma once

#include <launchdarkly/data_model/descriptors.hpp>

#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

/**
 * @brief IDataReader obtains data on-demand. Calls to obtain data may fail, so
 * the getter methods use tl::expected in order to propagate error codes.
 *
 * The IDataReader does not perform caching, so parent components must be
 * careful to avoid repeatedly fetching data (i.e. use a cache.)
 *
 */
class IDataReader {
   public:
    using Error = std::string;

    template <typename T>
    using Single = std::optional<data_model::ItemDescriptor<T>>;

    template <typename T>
    using SingleResult = tl::expected<Single<T>, Error>;

    template <typename T>
    using Collection =
        std::unordered_map<std::string, data_model::ItemDescriptor<T>>;

    template <typename T>
    using CollectionResult = tl::expected<Collection<T>, Error>;

    /**
     * @brief Attempts to get a flag named by key.
     * @param key Key of the flag.
     * @return On success, an optional FlagDescriptor (std::nullopt means the
     * flag doesn't exist.) On failure, an error string.
     */
    [[nodiscard]] virtual SingleResult<data_model::Flag> GetFlag(
        std::string const& key) const = 0;

    /**
     * @brief Attempts to get a segment named by key.
     * @param key Key of the segment.
     * @return On success, an optional SegmentDescriptor (std::nullopt means the
     * segment doesn't exist.) On failure, an error string.
     */
    [[nodiscard]] virtual SingleResult<data_model::Segment> GetSegment(
        std::string const& key) const = 0;

    /**
     * @brief Attempts to get a collection of all flags.
     * @return On success, a collection of FlagDescriptors. On failure, an error
     * string.
     */
    [[nodiscard]] virtual CollectionResult<data_model::Flag> AllFlags()
        const = 0;

    /**
     * @brief Attempts to get a collection of all segments.
     * @return On success, a collection of SegmentDescriptors. On failure, an
     * error string.
     */
    [[nodiscard]] virtual CollectionResult<data_model::Segment> AllSegments()
        const = 0;

    /**
     * @return Identity of the reader. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    /**
     * @return Whether the reader is initialized.
     */
    [[nodiscard]] virtual bool Initialized() const = 0;

    virtual ~IDataReader() = default;
    IDataReader(IDataReader const& item) = delete;
    IDataReader(IDataReader&& item) = delete;
    IDataReader& operator=(IDataReader const&) = delete;
    IDataReader& operator=(IDataReader&&) = delete;

   protected:
    IDataReader() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
