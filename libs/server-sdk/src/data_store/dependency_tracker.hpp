#pragma once

#include <set>
#include <string>

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include "data_kind.hpp"

namespace launchdarkly::server_side::data_store {

/**
 * Class which can be used to tag a collection with the DataKind that collection
 * is for. This is primarily to decrease the complexity of iterating collections
 * allowing for a kvp style iteration, but with an array storage container.
 * @tparam Storage
 */
template <typename Storage>
class TaggedData {
   public:
    explicit TaggedData(DataKind kind) : kind_(kind) {}
    [[nodiscard]] DataKind Kind() const { return kind_; }
    [[nodiscard]] Storage& Data() { return storage_; }

   private:
    DataKind kind_;
    Storage storage_;
};

/**
 * Class used to maintain a set of dependencies. Each dependency may be either
 * a flag or segment.
 * For instance, if we have a flagA, which has a prerequisite of flagB, and
 * a segmentMatch targeting segmentA, then its dependency set would be
 * ```
 * [{DataKind::kFlag, "flagB"}, {DataKind::kSegment, "segmentA"}]
 * ```
 */
class DependencySet {
   public:
    DependencySet();
    using DataType = std::array<TaggedData<std::set<std::string>>,
                                static_cast<std::size_t>(DataKind::kKindCount)>;
    void Set(DataKind kind, std::string key);

    void Remove(DataKind kind, std::string const& key);

    [[nodiscard]] bool Contains(DataKind kind, const std::string& key);

    /**
     * Return the size of all the data kind sets.
     * @return The combined size of all the data kind sets.
     */
    [[nodiscard]] std::size_t Size();

    [[nodiscard]] typename DataType::iterator begin();

    [[nodiscard]] typename DataType::iterator end();

   private:
    DataType data_;
};

/**
 * Class used to map flag/segments to their set of dependencies.
 * For instance, if we have a flagA, which has a prerequisite of flagB, and
 * a segmentMatch targeting segmentA, then a dependency map, containing
 * this set, would be:
 * ```
 * {{DataKind::kFlag, "flagA"}, [{DataKind::kFlag, "flagB"},
 *  {DataKind::kSegment, "segmentA"}]}
 * ```
 */
class DependencyMap {
   public:
    DependencyMap();
    using DataType =
        std::array<TaggedData<std::unordered_map<std::string, DependencySet>>,
                   static_cast<std::size_t>(DataKind::kKindCount)>;
    void Set(DataKind kind, std::string key, DependencySet val);

    [[nodiscard]] std::optional<DependencySet> Get(DataKind kind,
                                                   const std::string& key);

    void Clear();

    [[nodiscard]] typename DataType::iterator begin();

    [[nodiscard]] typename DataType::iterator end();

   private:
    DataType data_;
};

/**
 * This class implements a mechanism of tracking dependencies of flags and
 * segments. Both the forward dependencies (flag A depends on flag B) but also
 * the reverse (flag B is depended on by flagA).
 */
class DependencyTracker {
   public:
    using FlagDescriptor = data_model::ItemDescriptor<data_model::Flag>;
    using SegmentDescriptor = data_model::ItemDescriptor<data_model::Segment>;

    /**
     * Update the dependency tracker with a new or updated flag.
     *
     * @param key The key for the flag.
     * @param flag A descriptor for the flag.
     */
    void UpdateDependencies(const std::string& key, FlagDescriptor const& flag);

    /**
     * Update the dependency tracker with a new or updated segment.
     *
     * @param key The key for the segment.
     * @param flag A descriptor for the segment.
     */
    void UpdateDependencies(const std::string& key, SegmentDescriptor const& segment);

    /**
     * Given the current dependencies, determine what flags or segments may be
     * impacted by a change to the given flag/segment.
     *
     * @param kind The kind of data.
     * @param key The key for the data.
     * @param dependencySet A dependency set, which dependencies are
     * accumulated in.
     */
    void CalculateChanges(DataKind kind,
                          std::string const& key,
                          DependencySet& dependencySet);

   private:
    /**
     * Common logic for dependency updates used for both flags and segments.
     */
    void UpdateDependencies(DataKind kind,
                            std::string const& key,
                            DependencySet deps);

    DependencyMap dependenciesFrom_;
    DependencyMap dependenciesTo_;

    /**
     * Determine dependencies for a set of clauses.
     * @param dependencies A set of dependencies to extend.
     * @param clauses The clauses to determine dependencies for.
     */
    static void CalculateClauseDeps(DependencySet& dependencies,
                                    std::vector<data_model::Clause>& clauses);
};

}  // namespace launchdarkly::server_side::data_store
