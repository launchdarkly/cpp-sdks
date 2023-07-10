#include "dependency_tracker.hpp"

namespace launchdarkly::server_side::data_store {

DependencySet::DependencySet()
    : data_{
          TaggedData<std::set<std::string>>(DataKind::kFlag),
          TaggedData<std::set<std::string>>(DataKind::kSegment),
      } {}

void DependencySet::Set(DataKind kind, std::string key) {
    data_[static_cast<std::size_t>(kind)].Data().emplace(std::move(key));
}

void DependencySet::Remove(DataKind kind, std::string const& key) {
    data_[static_cast<std::size_t>(kind)].Data().erase(key);
}

bool DependencySet::Contains(DataKind kind, std::string const& key) {
    return data_[static_cast<std::size_t>(kind)].Data().count(key) != 0;
}

std::size_t DependencySet::Size() {
    std::size_t size = 0;
    for (auto data_kind : data_) {
        size += data_kind.Data().size();
    }
    return size;
}

std::array<TaggedData<std::set<std::string>>, 2>::iterator
DependencySet::begin() {
    return data_.begin();
}

std::array<TaggedData<std::set<std::string>>, 2>::iterator
DependencySet::end() {
    return data_.end();
}

std::set<std::string> DependencySet::SetForKind(DataKind kind) {
    return {data_[static_cast<std::size_t>(kind)].Data()};
}

DependencyMap::DependencyMap()
    : data_{
          TaggedData<std::unordered_map<std::string, DependencySet>>(
              DataKind::kFlag),
          TaggedData<std::unordered_map<std::string, DependencySet>>(
              DataKind::kSegment),
      } {}

void DependencyMap::Set(DataKind kind, std::string key, DependencySet val) {
    data_[static_cast<std::size_t>(kind)].Data().emplace(std::move(key),
                                                         std::move(val));
}

std::optional<DependencySet> DependencyMap::Get(DataKind kind,
                                                std::string const& key) {
    auto scope = data_[static_cast<std::size_t>(kind)].Data();
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
           2>::iterator
DependencyMap::begin() {
    return data_.begin();
}

std::array<TaggedData<std::unordered_map<std::string, DependencySet>>,
           2>::iterator
DependencyMap::end() {
    return data_.end();
}

void DependencyTracker::UpdateDependencies(
    std::string const& key,
    DependencyTracker::FlagDescriptor const& flag) {
    DependencySet dependencies;
    if (flag.item) {
        for (auto const& prereq : flag.item->prerequisites) {
            dependencies.Set(DataKind::kFlag, prereq.key);
        }

        for (auto const& rule : flag.item->rules) {
            auto clauses = rule.clauses;
            CalculateClauseDeps(dependencies, clauses);
        }
    }
    UpdateDependencies(DataKind::kFlag, key, dependencies);
}

void DependencyTracker::UpdateDependencies(
    std::string const& key,
    DependencyTracker::SegmentDescriptor const& segment) {
    DependencySet dependencies;
    if (segment.item) {
        for (auto const& rule : segment.item->rules) {
            auto clauses = rule.clauses;
            CalculateClauseDeps(dependencies, clauses);
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
            for (auto& depNs : *affected_items) {
                for (auto& dep : depNs.Data()) {
                    CalculateChanges(depNs.Kind(), dep, dependency_set);
                }
            }
        }
    }
}

// NOLINTEND misc-no-recursion

void DependencyTracker::UpdateDependencies(DataKind kind,
                                           std::string const& key,
                                           DependencySet deps) {
    auto current_deps = dependencies_from_.Get(kind, key);
    if (current_deps) {
        for (auto& dep_kind : *current_deps) {
            auto depKind = dep_kind.Kind();
            for (auto& dep : dep_kind.Data()) {
                auto deps_to_this_dep = dependencies_to_.Get(depKind, dep);
                if (deps_to_this_dep) {
                    deps_to_this_dep->Remove(depKind, key);
                }
            }
        }
    }

    dependencies_from_.Set(kind, key, deps);
    for (auto& dep_kind : deps) {
        for (auto& dep : dep_kind.Data()) {
            auto deps_to_this_dep = dependencies_to_.Get(dep_kind.Kind(), dep);
            if (!deps_to_this_dep) {
                auto new_deps_to_this_dep = DependencySet();
                new_deps_to_this_dep.Set(kind, key);
                dependencies_to_.Set(dep_kind.Kind(), dep,
                                     new_deps_to_this_dep);
            } else {
                deps_to_this_dep->Set(kind, key);
            }
        }
    }
}

void DependencyTracker::CalculateClauseDeps(
    DependencySet& dependencies,
    std::vector<data_model::Clause>& clauses) {
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

}  // namespace launchdarkly::server_side::data_store
