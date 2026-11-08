cmake_minimum_required(VERSION 3.16)

project(Bolt LANGUAGES C CXX)

function(apply_release_optimizations target)
    if(MSVC)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/O2>)
    else()
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-O3>)
    endif()
endfunction()

add_library(frontend STATIC
    debug/errors.cpp
    debug/print.cpp
    frontend/astgen/astgen.cpp
    frontend/checker/structure.cpp
    frontend/checker/variables.cpp
    frontend/lexer/tokens.cpp
)
target_compile_features(frontend PUBLIC cxx_std_17)
apply_release_optimizations(frontend)

add_library(backend STATIC
    backend/codegen/codegen.cpp
)
target_compile_features(backend PUBLIC cxx_std_17)
target_link_libraries(backend PUBLIC frontend)
apply_release_optimizations(backend)

add_executable(bolt
    main.cpp
    runner/arguments.cpp
    runner/runmodes.cpp
    packager/packager.cpp
)
target_compile_features(bolt PRIVATE cxx_std_17)
target_compile_definitions(bolt PRIVATE BOLT_LIBRARY_BUILD)
target_link_libraries(bolt PRIVATE backend)
apply_release_optimizations(bolt)

add_executable(bvm
    vm/errors.c
    vm/executor.c
    vm/main.c
)
target_compile_features(bvm PRIVATE c_std_17)

if(MSVC)
    target_compile_options(bvm PRIVATE $<$<CONFIG:Release>:/Os /GL /GF /Gw>)
    target_link_options(bvm PRIVATE $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>)
else()
    target_compile_options(bvm PRIVATE $<$<CONFIG:Release>:-Os -fdata-sections -ffunction-sections>)
    target_link_options(bvm PRIVATE $<$<CONFIG:Release>:-Wl,--gc-sections -s>)
endif()
