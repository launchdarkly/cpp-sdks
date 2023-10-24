// This file is used to include boost url/json when building a shared library on
// linux/mac. Windows links static libs in this case and does not include these
// src files, as there are issues compiling the value.ipp file from JSON with
// MSVC.
#include <boost/json/src.hpp>
#include <boost/url/src.hpp>
