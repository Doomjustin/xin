function(add_example name)
    add_executable(xin.example.${name} ${name}.cpp)
    target_link_libraries(xin.example.${name} PRIVATE xin::xin)
    target_compile_features(xin.example.${name} PUBLIC cxx_std_23)
    set_property(TARGET xin.example.${name} PROPERTY CXX_MODULE_STD ON)
endfunction()


add_example(demo)


add_executable(xin.benchmark.asio asio.benchmark.cpp)
target_link_libraries(xin.benchmark.asio PRIVATE asio::asio)
target_compile_definitions(xin.benchmark.asio 
    PRIVATE
        ASIO_HAS_IO_URING
)
target_link_libraries(xin.benchmark.asio 
    PRIVATE 
        asio::asio
        spdlog::spdlog
        Catch2::Catch2WithMain
        PkgConfig::liburing
)

target_compile_features(xin.benchmark.asio PUBLIC cxx_std_23)
set_property(TARGET xin.benchmark.asio PROPERTY CXX_MODULE_STD ON)
