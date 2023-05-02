#pragma once

// C++ utility for wrapping iterators.
// Only should be utilized within the implementation of bindings.

#ifdef __cplusplus

template <typename TIterator>
struct IteratorBinding {
    TIterator iter;
    TIterator end;

    bool End() { return iter == end; }

    void Next() {
        iter++;
    }
};

#endif
