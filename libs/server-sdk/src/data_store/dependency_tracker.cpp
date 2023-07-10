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
    for (auto dk : data_) {
        size += dk.Data().size();
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
    for (auto& ns : data_) {
        ns.Data().clear();
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
                                         DependencySet& dependencySet) {
    if (!dependencySet.Contains(kind, key)) {
        dependencySet.Set(kind, key);
        auto affectedItems = dependenciesTo_.Get(kind, key);
        if (affectedItems) {
            for (auto& depNs : *affectedItems) {
                for (auto& dep : depNs.Data()) {
                    CalculateChanges(depNs.Kind(), dep, dependencySet);
                }
            }
        }
    }
}

// NOLINTEND misc-no-recursion

void DependencyTracker::UpdateDependencies(DataKind kind,
                                           std::string const& key,
                                           DependencySet deps) {
    auto currentDeps = dependenciesFrom_.Get(kind, key);
    if (currentDeps) {
        for (auto& depNs : *currentDeps) {
            auto depKind = depNs.Kind();
            for (auto& dep : depNs.Data()) {
                auto depsToThisDep = dependenciesTo_.Get(depKind, dep);
                if (depsToThisDep) {
                    depsToThisDep->Remove(depKind, key);
                }
            }
        }
    }

    dependenciesFrom_.Set(kind, key, deps);
    for (auto& depNs : deps) {
        for (auto& dep : depNs.Data()) {
            auto depsToThisDep = dependenciesTo_.Get(depNs.Kind(), dep);
            if (!depsToThisDep) {
                auto newDepsToThisDep = DependencySet();
                newDepsToThisDep.Set(kind, key);
                dependenciesTo_.Set(depNs.Kind(), dep, newDepsToThisDep);
            } else {
                depsToThisDep->Set(kind, key);
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
}  // namespace launchdarkly::server_side::data_store
