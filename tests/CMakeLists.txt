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

function(xin_add_benchmark name)
    add_executable(xin.benchmark.${name} ${name}.benchmark.cpp)
    target_link_libraries(xin.benchmark.${name} 
        PRIVATE 
            xin::async
            Catch2::Catch2WithMain    
    )
    target_compile_features(xin.benchmark.${name} PUBLIC cxx_std_23)
    set_property(TARGET xin.benchmark.${name} PROPERTY CXX_MODULE_STD ON)
endfunction()


xin_add_benchmark(async)
xin_add_benchmark(channel)