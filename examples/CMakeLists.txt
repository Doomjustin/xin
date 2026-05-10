function(add_example name)
    add_executable(xin.example.${name} ${name}.cpp)
    target_link_libraries(xin.example.${name} PRIVATE xin::xin)
    target_compile_features(xin.example.${name} PUBLIC cxx_std_23)
    set_property(TARGET xin.example.${name} PROPERTY CXX_MODULE_STD ON)
endfunction()


add_example(demo)