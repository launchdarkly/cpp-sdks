#pragma once

#include <set>
#include <string>

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include "data_kind.hpp"

namespace launchdarkly::server_side::data_store {

template <typename Storage>
class TaggedData {
   public:
    TaggedData(DataKind kind) : kind_(kind) {}
    [[nodiscard]] DataKind Kind() const { return kind_; }
    [[nodiscard]] Storage& Data() { return storage_; }

   private:
    DataKind kind_;
    Storage storage_;
};

template <typename Storage>
class ScopedSet {
   public:
    ScopedSet()
        : data_{
              TaggedData<std::set<Storage>>(DataKind::kFlag),
              TaggedData<std::set<Storage>>(DataKind::kSegment),
          } {}
    using DataType = std::array<TaggedData<std::set<Storage>>,
                                static_cast<std::size_t>(DataKind::kKindCount)>;
    void Set(DataKind kind, std::string key) {
        data_[static_cast<std::size_t>(kind)].Data().emplace(std::move(key));
    }

    void Remove(DataKind kind, std::string key) {
        data_[static_cast<std::size_t>(kind)].Data().erase(key);
    }

    [[nodiscard]] bool Contains(DataKind kind, std::string key) {
        return data_[static_cast<std::size_t>(kind)].Data().count(key) != 0;
    }

    /**
     * Return the size of all the data kind sets.
     * @return The combined size of all the data kind sets.
     */
    [[nodiscard]] std::size_t Size() {
        std::size_t size = 0;
        for(auto dk: data_) {
            size += dk.Data().size();
        }
        return size;
    }

    [[nodiscard]] typename DataType::iterator begin() { return data_.begin(); }

    [[nodiscard]] typename DataType::iterator end() { return data_.end(); }

   private:
    DataType data_;
};

template <typename Storage>
class ScopedMap {
   public:
    ScopedMap()
        : data_{
              TaggedData<std::unordered_map<std::string, Storage>>(
                  DataKind::kFlag),
              TaggedData<std::unordered_map<std::string, Storage>>(
                  DataKind::kSegment),
          } {}
    using DataType =
        std::array<TaggedData<std::unordered_map<std::string, Storage>>,
                   static_cast<std::size_t>(DataKind::kKindCount)>;
    void Set(DataKind kind, std::string key, Storage val) {
        data_[static_cast<std::size_t>(kind)].Data().emplace(std::move(key),
                                                             std::move(val));
    }

    [[nodiscard]] std::optional<Storage> Get(DataKind kind, std::string key) {
        auto scope = data_[static_cast<std::size_t>(kind)].Data();
        auto found = scope.find(key);
        if (found != scope.end()) {
            return found->second;
        }
        return std::nullopt;
    }

    void Clear() {
        for (auto& ns : data_) {
            ns.Data().clear();
        }
    }

    [[nodiscard]] typename DataType::iterator begin() { return data_.begin(); }

    [[nodiscard]] typename DataType::iterator end() { return data_.end(); }

   private:
    DataType data_;
};

class DependencyTracker {
   public:
    using FlagDescriptor = data_model::ItemDescriptor<data_model::Flag>;
    using SegmentDescriptor = data_model::ItemDescriptor<data_model::Segment>;

    void updateDependencies(std::string key, FlagDescriptor const& flag) {
        ScopedSet<std::string> dependencies;
        if (flag.item) {
            for (auto const& prereq : flag.item->prerequisites) {
                dependencies.Set(DataKind::kFlag, prereq.key);
            }

            for (auto const& rule : flag.item->rules) {
                auto clauses = rule.clauses;
                CalculateClauseDeps(dependencies, clauses);
            }
        }
        updateDependencies(DataKind::kFlag, key, dependencies);
    }
    void updateDependencies(std::string key, SegmentDescriptor segment) {
        ScopedSet<std::string> dependencies;
        if (segment.item) {
            for (auto const& rule : segment.item->rules) {
                auto clauses = rule.clauses;
                CalculateClauseDeps(dependencies, clauses);
            }
        }
        updateDependencies(DataKind::kSegment, key, dependencies);
    }

    void calculateChanges(DataKind kind, std::string key, ScopedSet<std::string>& dependencySet) {
        if(!dependencySet.Contains(kind, key)) {
            dependencySet.Set(kind, key);
            auto affectedItems = dependenciesTo.Get(kind, key);
            if(affectedItems) {
                for (auto& depNs : *affectedItems) {
                    for(auto& dep: depNs.Data()) {
                        calculateChanges(depNs.Kind(), dep, dependencySet);
                    }
                }
            }
        }
    }

   private:
    void updateDependencies(DataKind kind,
                            std::string key,
                            ScopedSet<std::string> deps) {
        auto currentDeps = dependenciesFrom.Get(kind, key);
        if (currentDeps) {
            for (auto& depNs : *currentDeps) {
                auto depKind = depNs.Kind();
                for (auto& dep : depNs.Data()) {
                    auto depsToThisDep = dependenciesTo.Get(depKind, dep);
                    if (depsToThisDep) {
                        depsToThisDep->Remove(depKind, key);
                    }
                }
            }
        }

        dependenciesFrom.Set(kind, key, deps);
        for (auto& depNs : deps) {
            for (auto& dep : depNs.Data()) {
                auto depsToThisDep = dependenciesTo.Get(depNs.Kind(), dep);
                if (!depsToThisDep) {
                    auto newDepsToThisDep = ScopedSet<std::string>();
                    newDepsToThisDep.Set(kind, key);
                    dependenciesTo.Set(depNs.Kind(), dep, newDepsToThisDep);
                } else {
                    depsToThisDep->Set(kind, key);
                }
            }
        }
    }

    ScopedMap<ScopedSet<std::string>> dependenciesFrom;
    ScopedMap<ScopedSet<std::string>> dependenciesTo;

    void CalculateClauseDeps(ScopedSet<std::string>& dependencies,
                             std::vector<data_model::Clause>& clauses) const {
        for (auto const& clause : clauses) {
            if (clause.op == data_model::Clause::Op::kSegmentMatch) {
                for (auto const& value : clause.values) {
                    dependencies.Set(DataKind::kSegment, value.AsString());
                }
            }
        }
    }
};

}  // namespace launchdarkly::server_side::data_store
