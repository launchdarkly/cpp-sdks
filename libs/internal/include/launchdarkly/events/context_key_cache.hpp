#pragma once
#include <list>
#include <string>
#include <unordered_map>
namespace launchdarkly::events {

class ContextKeyCache {
   public:
    explicit ContextKeyCache(std::size_t capacity);
    bool Notice(std::string const& context_key);
    void Clear();

   private:
    using KeyList = std::list<std::string>;
    std::size_t capacity_;
    std::unordered_map<std::string, KeyList::reference> map_;
    KeyList list_;
};

}  // namespace launchdarkly::events
