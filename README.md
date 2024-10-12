# xin
Code space

# requests
1. C++ >= 23
2. CMake >= 3.29
3. MSVC >= 18.4

# modules
1. fmt : 11.0.1
2. spdlog: v1.14.1
3. Catch2: v3.6.0
4. pybind11: stable

# how to add a C++20 module

not support C++23 _import std_ yet

```cmake
add_library(hello STATIC)
# Add sources.
target_sources(hello
  PUBLIC
    FILE_SET CXX_MODULES 
    FILES
        hello.cpp
)
```
