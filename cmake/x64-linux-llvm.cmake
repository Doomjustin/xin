set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_C_COMPILER "/home/doom/llvm-project/build/bin/clang")
set(VCPKG_CXX_COMPILER "/home/doom/llvm-project/build/bin/clang++")

set(VCPKG_C_FLAGS "-Wall")
set(VCPKG_CXX_FLAGS "-stdlib=libc++")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++ -L/home/doom/llvm-project/build/lib/x86_64-unknown-linux-gnu -Wl,-rpath,/home/doom/llvm-project/build/lib/x86_64-unknown-linux-gnu")