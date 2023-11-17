#include "dependency_tracker.hpp"
#include "tagged_data.hpp"

#include <type_traits>

namespace launchdarkly::server_side::data_components {

DependencySet::DependencySet()
    : data_{
          TaggedData<std::set<std::string>>(DataKind::kFlag),
          TaggedData<std::set<std::string>>(DataKind::kSegment),
      } {}

void DependencySet::Set(DataKind const kind, std::string key) {
    Data(kind).emplace(std::move(key));
}

void DependencySet::Remove(DataKind const kind, std::string const& key) {
    Data(kind).erase(key);
}

bool DependencySet::Contains(DataKind const kind,
                             std::string const& key) const {
    return Data(kind).count(key) != 0;
}

std::size_t DependencySet::Size() const {
    std::size_t size = 0;
    for (auto data_kind : data_) {
        size += data_kind.Data().size();
    }
    return size;
}

std::array<TaggedData<std::set<std::string>>, 2>::const_iterator
DependencySet::begin() const {
    return data_.begin();
}

std::array<TaggedData<std::set<std::string>>, 2>::const_iterator
DependencySet::end() const {
    return data_.end();
}

std::set<std::string> const& DependencySet::SetForKind(DataKind kind) {
    return Data(kind);
}

std::set<std::string> const& DependencySet::Data(DataKind kind) const {
    return data_[static_cast<std::underlying_type_t<DataKind>>(kind)].Data();
}

std::set<std::string>& DependencySet::Data(DataKind kind) {
    return data_[static_cast<std::underlying_type_t<DataKind>>(kind)].Data();
}

DependencyMap::DependencyMap()
    : data_{
          TaggedData<std::unordered_map<std::string, DependencySet>>(
              DataKind::kFlag),
          TaggedData<std::unordered_map<std::string, DependencySet>>(
              DataKind::kSegment),
      } {}

void DependencyMap::Set(DataKind kind, std::string key, DependencySet val) {
    data_[static_cast<std::underlying_type_t<DataKind>>(kind)].Data().emplace(
        std::move(key), std::move(val));
}

std::optional<DependencySet> DependencyMap::Get(DataKind kind,
                                                std::string const& key) const {
    auto const& scope =
        data_[static_cast<std::underlying_type_t<DataKind>>(kind)].Data();
    auto found = scope.find(key);
    if (found != scope.end()) {
        return found->second;
    }
    return std::nullopt;
}

void DependencyMap::Clear() {
    for (auto& data_kind : data_) {
        data_kind.Data().clear();
    }
}

std::array<TaggedData<std::unordered_map<std::string, DependencySet>>,
           2>::const_iterator
DependencyMap::begin() const {
    return data_.begin();
}

std::array<TaggedData<std::unordered_map<std::string, DependencySet>>,
           2>::const_iterator
DependencyMap::end() const {
    return data_.end();
}

void DependencyTracker::UpdateDependencies(
    std::string const& key,
    data_model::FlagDescriptor const& flag) {
    DependencySet dependencies;
    if (flag.item) {
        for (auto const& prereq : flag.item->prerequisites) {
            dependencies.Set(DataKind::kFlag, prereq.key);
        }

        for (auto const& rule : flag.item->rules) {
            CalculateClauseDeps(dependencies, rule.clauses);
        }
    }
    UpdateDependencies(DataKind::kFlag, key, dependencies);
}

void DependencyTracker::UpdateDependencies(
    std::string const& key,
    data_model::SegmentDescriptor const& segment) {
    DependencySet dependencies;
    if (segment.item) {
        for (auto const& rule : segment.item->rules) {
            CalculateClauseDeps(dependencies, rule.clauses);
        }
    }
    UpdateDependencies(DataKind::kSegment, key, dependencies);
}

// Function intentionally uses recursion.
// NOLINTBEGIN misc-no-recursion

void DependencyTracker::CalculateChanges(DataKind kind,
                                         std::string const& key,
                                         DependencySet& dependency_set) {
    if (!dependency_set.Contains(kind, key)) {
        dependency_set.Set(kind, key);
        auto affected_items = dependencies_to_.Get(kind, key);
        if (affected_items) {
            for (auto& deps_by_kind : *affected_items) {
                for (auto& dep : deps_by_kind.Data()) {
                    CalculateChanges(deps_by_kind.Kind(), dep, dependency_set);
                }
            }
        }
    }
}

// NOLINTEND misc-no-recursion

void DependencyTracker::UpdateDependencies(DataKind kind,
                                           std::string const& key,
                                           DependencySet const& deps) {
    auto current_deps = dependencies_from_.Get(kind, key);
    if (current_deps) {
        for (auto const& deps_by_kind : *current_deps) {
            auto kind_of_dep = deps_by_kind.Kind();
            for (auto const& dep : deps_by_kind.Data()) {
                auto deps_to_this_dep = dependencies_to_.Get(kind_of_dep, dep);
                if (deps_to_this_dep) {
                    deps_to_this_dep->Remove(kind_of_dep, key);
                }
            }
        }
    }

    dependencies_from_.Set(kind, key, deps);
    for (auto const& deps_by_kind : deps) {
        for (auto const& dep : deps_by_kind.Data()) {
            auto deps_to_this_dep =
                dependencies_to_.Get(deps_by_kind.Kind(), dep);
            if (!deps_to_this_dep) {
                auto new_deps_to_this_dep = DependencySet();
                new_deps_to_this_dep.Set(kind, key);
                dependencies_to_.Set(deps_by_kind.Kind(), dep,
                                     new_deps_to_this_dep);
            } else {
                deps_to_this_dep->Set(kind, key);
            }
        }
    }
}

void DependencyTracker::CalculateClauseDeps(
    DependencySet& dependencies,
    std::vector<data_model::Clause> const& clauses) {
    for (auto const& clause : clauses) {
        if (clause.op == data_model::Clause::Op::kSegmentMatch) {
            for (auto const& value : clause.values) {
                dependencies.Set(DataKind::kSegment, value.AsString());
            }
        }
    }
}

void DependencyTracker::Clear() {
    dependencies_to_.Clear();
    dependencies_from_.Clear();
}

}  // namespace launchdarkly::server_side::data_components
