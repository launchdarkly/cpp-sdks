## Contribution Guide

### Formatting

Slightly modified "Chromium" `clang-tidy` configuration.

### Style

See https://google.github.io/styleguide/cppguide.html. 

Quick start:

**Class member variables**
```c++
class Thing {
    std::string foo_;
    std::string bar_baz_;
};
```
**Struct member variables**
```c++
struct Thing {
    std::string foo;
    std::string bar_baz;
};
```
**Class names**
```c++
class Foo;
class BarBaz;
```
**Functions**
```c++
DoTheFoo();
Bar();
```
**Constants**
```c++
const int kFooBarBaz = 1;
```
**Enums**
```C++
enum class Foo {
    kUnknown = 0,
    kBar,
};
```

**Divergence: file names**

In C:
```
file_name.c
file_name.h
```
In C++:
```
file_name.cpp
file_name.hpp
```
### C++ Standard

C++17.

### C Standard

C99. 

### Testing

Use `googletest`.

### Exporting

Symbols should be hidden by default. Only export what is part of the public API.


### Include Guards

Use `#pragma once`.
